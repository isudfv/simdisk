//
// Created by yieatn on 2021/09/29.
//

#ifndef SIMDISK_PATH_H
#define SIMDISK_PATH_H
#include "inode.h"
#include "ipc.h"
#include "users.h"
#include <algorithm>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>
class path {
public:
    //default contructr constructs root directory{0,/}
    path() : name("/"), inode_n(0) {
    }
    //construct a path object from inode and the path ({1, /home}for example)
    path(uint _inode, std::string _name) : name(std::move(_name)), inode_n(_inode) {
        if (name.empty())
            name.append("/");
    }
    /*    std::string_view dir() {
        std::string_view temp(name);
        if (temp != "/" && temp.ends_with("/"))
            temp.remove_suffix(1);
        return temp;
    }*/
    //to judge if a path is a directory or file
    bool is_directory() const {
        return inodes[inode_n].i_mode & IS_DIRECTORY;
    }

    bool is_file() const {
        return inodes[inode_n].i_mode & IS_FILE;
    }

    //to get the filename of a path object, for example filename of /home/yieatn/test is test
    std::string filename() const {
        return name.substr(name.rfind('/') + 1);
    }
    //to get a list of all the file(dir_entry) of a path(directory file)
    std::vector<dir_entry> list() const {
        std::vector<dir_entry> all;
        //        std::vector<std::pair<uint, std::string>> all;
        int                    pos = 0;
        uint                   destInode;
        //        DISK.seekg(inodes[inode_n].i_zone[pos++]);
        //        outToSHM(inode_n << " " << &inodes[inode_n].i_zone[pos++] << std::endl, shm_out);
        auto                   seek = (inodes[inode_n].i_zone[pos++]);
        //        outToSHM((void *)seek << std::endl, shm_out);
        char                   temp[28];
        DISK.seekg(seek);
        for (int i = 0; i < inodes[inode_n].i_size; ++i) {
            dir_entry src{};
            DISK.tellg();
            //read each dir_entry from simdisk to memory
            DISK.read((char *) &src, sizeof(dir_entry));
            all.push_back(src);
            //if read all dir_entries of a block, then switch to next block of the inode
            if ((i + 1) * sizeof(dir_entry) % BLOCKSIZE == 0)
                seek = (inodes[inode_n].i_zone[pos++]);
        }
        //        outToSHM("END \n", shm_out);
        return all;
    }
    //if there're parameters ,return formatted strings rather than dir_entry objects
    std::vector<std::string> list(std::string_view params) {
        std::vector<std::string> all;
        auto                     entries = list();
        for (auto &item : entries) {
            if (item.name[0] == '.' && params.find('a') == -1)
                continue;
            if (params.find('l') == -1) {
                all.emplace_back(item.name);
            } else {
                //format a dir_entry, for example [drwxr-xr-x  1 root   root       2 Thu 21 18:07 home]
                all.push_back(fmt::format("{}{} {:>2} {:<6} {:<6} {:>5} {} {}",
                                          file_type(inodes[item.inode_n].i_mode),
                                          rwx(inodes[item.inode_n].i_mode),
                                          1,
                                          users[inodes[item.inode_n].i_uid].name,
                                          users[inodes[item.inode_n].i_uid].name,
                                          inodes[item.inode_n].i_size,
                                          time_format(inodes[item.inode_n].i_time),
                                          item.name));
            }
        }
        return all;
    }
    //to find dest in current path and return its inode number
    inode_t find(const std::string &dest) const {
        auto all = list();
        for (const auto &item : all) {
            if (item.name == dest)
                return item.inode_n;
        }
        return -1;
    }

    /*    path findDir(const std::string &dest) {
        auto all = list();
        for (auto &p : all)
            if (p->name == dest){
                if (strcmp(p->name, ".") == 0)
                    return *this;
                else if (strcmp(p->name, "..") == 0){
                    if (name == "/")
                        return *this;
                    else {
                        std::string temp(dir());
                        temp.erase(temp.rfind('/'));
//                        if (temp.empty())
                            temp.append("/");
                        return {p->inode_n, temp};
                    }
                }
                else
                    return {p->inode_n, name + dest + "/"};
            }
    }*/
    //to find dest (both relative and absolute) and return the path object of it
    path find_dest_dir(const std::string &dest) {
        path curr = *this;
        //if the dest starts with /, then look for the path from root directory
        if (dest.starts_with("/"))
            curr = path{};
        //to split a direcotry, for example (/home/yieatn/test) to (home yieatn test)
        auto dirs = split(dest, std::regex("[^\\/]+"));
        for (const auto &p : dirs) {
            if (!curr.is_directory()) {
                outToSHM(fmt::format("{} not a directory\n", curr.name), shm_out);
                break;
            }
            auto seek = curr.find_dir_entry(p);
            if (seek.inode_n == -1) {
                return path(-1, p);
            }
            //both . and .. of root directory point to itself
            if (curr.name == "/") {
                if (p.starts_with("."))
                    curr = *this;
                else
                    curr = {seek.inode_n, curr.name + seek.name};
            } else {
                if (p == ".")
                    curr = *this;
                else if (p == "..") {
                    curr = {seek.inode_n, curr.name.substr(0, name.rfind('/'))};
                } else
                    curr = {seek.inode_n, curr.name + "/" + seek.name};
            }
        }
        return curr;
    }


    path operator/(const std::string &dest) {
        return find_dest_dir(dest);
    }
    //to create a dir_entry in current dirctory; dest_inode means to which this dir_etry binds, -1 stands for a new inode
    inode_t create_dir_entry(const std::string &dest, inode_t dest_inode = -1, inode_mode_t mode = IS_DIRECTORY) {
        if (find(dest) != -1) {
            outToSHM("dir already exists\n", shm_out);
            return -1;
        }
        inode &thisInode = inodes[inode_n];
        //to locate the position to put this new dir_entry
        uint   pos       = thisInode.i_size * sizeof(dir_entry) / BLOCKSIZE;
        uint   off       = thisInode.i_size * sizeof(dir_entry) % BLOCKSIZE;

        //get new empty block.
        if (off == 0) {
            auto temp = getNewEmptyBlockNo();
            if (temp == -1) {
                outToSHM("no block available\n", shm_out);
                return -1;
            }
            thisInode.i_zone[pos] = temp * BLOCKSIZE;
        }

        //get new inode
        dest_inode = dest_inode == -1 ? getNewInode() : dest_inode;
        if (dest_inode == -1) {
            outToSHM("no inode available\n", shm_out);
            return -1;
        }

        dir_entry newDir{dest_inode, dest.data()};

        auto      seek = (thisInode.i_zone[pos] + off);
        //        outToSHM(seek << std::endl, shm_out);
        //        *seek = newDir;
        DISK.seekp(seek);
        DISK.write((char *) &newDir, sizeof(newDir));

        inodes[dest_inode].i_mode |= mode;
        inodes[dest_inode].i_uid  = curr_uid;
        inodes[dest_inode].i_time = std::time(nullptr);
        if (mode == IS_DIRECTORY) {
            // |= 0755
            inodes[dest_inode].i_mode |= (7 << 6);
            inodes[dest_inode].i_mode |= (5 << 3);
            inodes[dest_inode].i_mode |= (5);
        } else if (mode == IS_FILE) {
            // |= 0644
            inodes[dest_inode].i_mode |= (6 << 6);
            inodes[dest_inode].i_mode |= (4 << 3);
            inodes[dest_inode].i_mode |= (4);
        }
        thisInode.i_size++;

        return dest_inode;
    }
    //to create a sub_directory in current path
    bool create_dir(const std::string &dest) {
        auto dest_inode = create_dir_entry(dest);
        if (dest_inode == -1) {
            return false;
        }
        path sub = *this / dest;
        //        outToSHM("Here: " << inode_n << " " << "Sub: " << sub.inode_n << std::endl, shm_out);
        sub.create_dir_entry(".", dest_inode);
        sub.create_dir_entry("..", inode_n);
        return true;
    }
    //to create a file with giving content in current path
    bool create_file(const std::string &dest, const std::string_view content) {
        auto dest_inode = create_dir_entry(dest, -1, IS_FILE);

        if (dest_inode == -1) {
            return false;
        }

        uint                          current_block;
        uint                          pos = 0;
        //to split content into chunks of 1024
        std::vector<std::string_view> chunks;
        for (int i = 0; i < content.size(); i += 1024)
            chunks.push_back(content.substr(i, 1024));

        for (const auto &p : chunks) {
            current_block = getNewEmptyBlockNo();
            if (current_block == -1) {
                outToSHM("no block available\n", shm_out);
                return false;
            }
            inodes[dest_inode].i_zone[pos] = current_block * BLOCKSIZE;
            //            auto seek = current_block;
            //            memcpy(seek, p.data(), p.size());
            DISK.seekp(inodes[dest_inode].i_zone[pos++]);
            DISK.write(p.data(), p.size());
            inodes[dest_inode].i_size += p.size();
        }
        return true;
    }

    /*    bool copy_file(const std::string &dest, inode_t src_inode) {
        auto dest_inode = create_dir_entry(dest, -1, IS_FILE);
        for (uint i = 0, pos = 0; i < inodes[src_inode].i_size; i += BLOCKSIZE) {
            auto current_block = getNewEmptyBlockNo();
            if (current_block == -1) {
                outToSHM("no block available\n", shm_out);
                return false;
            }
            inodes[dest_inode].i_zone[pos] = current_block * BLOCKSIZE;
            auto seek = current_block;
            auto seek_src = startPos + inodes[src_inode].i_zone[pos++] * BLOCKSIZE;
            auto size = std::min(inodes[src_inode].i_size - i * BLOCKSIZE, 1024u);
            memcpy(seek, seek_src, size);
            inodes[dest_inode].i_size += size;
        }
        return true;
    }*/

    /*    std::string get_content(const std::string &dest) {
        inode_t dest_inode = find(dest);
        if (dest_inode == -1) {
            outToSHM("File not found\n", shm_out);
            return {};
        }
        std::string content;
        content.reserve(inodes[dest_inode].i_size);
//        auto seek = startPos +
        for (uint i = 0, pos = 0; i < inodes[dest_inode].i_size; i += BLOCKSIZE) {
//            auto seek = inodes[dest_inode].i_zone[pos++];
//            memcpy(temp.content(), seek, std::min(1024u, inodes[dest_inode].i_size - i));
            char data[BLOCKSIZE];
            DISK.seekg(inodes[dest_inode].i_zone[pos++]);
            DISK.read(data, BLOCKSIZE);
            content.append(data, std::min(1024u, inodes[dest_inode].i_size - i));
        }
        return content;
    }*/
    //to get content of a file
    std::string get_content() {
        if (!is_file())
            return {};
        if (inodes[inode_n].i_mode & IS_WRITTING)
            return "The File Is Being Used";
        std::string content;
        content.reserve(inodes[inode_n].i_size);
        //        auto seek = startPos +
        for (uint i = 0, pos = 0; i < inodes[inode_n].i_size; i += BLOCKSIZE) {
            //            auto seek = inodes[inode_n].i_zone[pos++];
            char data[BLOCKSIZE];
            DISK.seekg(inodes[inode_n].i_zone[pos++]);
            DISK.read(data, BLOCKSIZE);
            //            memcpy(temp.content(), seek, std::min(1024u, inodes[dest_inode].i_size - i));
            content.append(data, std::min(1024u, inodes[inode_n].i_size - i));
        }
        return content;
    }
    //to append a string to a file
    bool apppend(std::string_view content) {
        if (!is_file())
            return false;
        std::vector<std::string_view> chunks;

        uint                          pos = inodes[inode_n].i_size / BLOCKSIZE;
        uint                          off = inodes[inode_n].i_size % BLOCKSIZE;

        uint                          i   = BLOCKSIZE - off;
        //continue with previous content
        chunks.push_back(content.substr(0, i));
        //        content.remove_prefix(std::min(BLOCKSIZE - off, (uint)content.size()));

        //        std::cout << content.size() << std::endl;
        for (; i < content.size(); i += 1024)
            chunks.push_back(content.substr(i, 1024));

        for (const auto &item : chunks) {
            if (off == 0) {
                auto temp = getNewEmptyBlockNo();
                if (temp == -1) {
                    outToSHM("no block available\n", shm_out);
                    //                    std::cout << "no block available\n";
                    return -1;
                }
                inodes[inode_n].i_zone[pos] = temp * BLOCKSIZE;
            }
            auto seek = inodes[inode_n].i_zone[pos] + off;
            DISK.seekp(seek);
            DISK.write(item.data(), item.size());
            inodes[inode_n].i_size += item.size();
            pos++, off = 0;// locate next block
        }
        return true;
    }
    //to remove a dir_entry of a path, just wrap out the dir_entry and take back inode without clearing the block
    bool remove_dir(const std::string &dest) {
        auto p = find_dir_entry(dest);

        if (p.inode_n == -1) {
            outToSHM("dir not found\n", shm_out);
            return false;
        }

        if (inodes[p.inode_n].i_uid != curr_uid && curr_uid != 0 &&
            (inodes[p.inode_n].i_mode & (2)) == 0) {
            outToSHM("Permission denied\n", shm_out);
            return false;
        }

        if (inodes[inode_n].i_mode & IS_WRITTING) {
            outToSHM("The File Is Being Used\n", shm_out);
            return false;
        }
        //if the directory is not empty, additional confirm is required
        if (inodes[p.inode_n].i_mode & IS_DIRECTORY && inodes[p.inode_n].i_size > 2) {
            outToSHM("Directory not empty, please type yes: \n", shm_out);
            allout(shm_out);
            std::string get;
            inFromSHM(get, shm_in);
            //            std::cin >> get;
            //            getchar();
            if (get != "yes")
                return false;
        }
        memset(&inodes[p.inode_n], 0, sizeof(inode));
        inodes[inode_n].i_size--;
        auto s              = find_last_dir_entry();
        //find a dir_entry to cover the gap
        auto getDirentryAdd = [this, &dest]() {
            int  pos  = 0;
            auto seek = (inodes[inode_n].i_zone[pos++]);
            DISK.seekg(seek);
            for (int i = 0; i < inodes[inode_n].i_size; ++i) {
                dir_entry src{};
                auto      curr_pos = DISK.tellg();
                DISK.read((char *) &src, sizeof(dir_entry));
                if (src.name == dest)
                    return curr_pos;
                if ((i + 1) * sizeof(dir_entry) % BLOCKSIZE == 0)
                    seek = (inodes[inode_n].i_zone[pos++]);
            }
        };

        DISK.seekp(getDirentryAdd());
        DISK.write((char *) &s, sizeof(s));


        return true;
    }

private:
    //find a dir_entry
    dir_entry find_dir_entry(const std::string &dest) {
        auto all = list();
        for (auto &p : all) {
            if (p.name == dest)
                return p;
        }
        return {UINT32_MAX, ""};
    }

    dir_entry find_last_dir_entry() {
        uint pos = inodes[inode_n].i_size * sizeof(dir_entry) / BLOCKSIZE;
        uint off = inodes[inode_n].i_size * sizeof(dir_entry) % BLOCKSIZE;
        if (off == 0 && pos != 0)
            pos -= 1, off = BLOCKSIZE - sizeof(dir_entry);
        auto      seek = (inodes[inode_n].i_zone[pos] + off);
        dir_entry src{};
        DISK.seekg(seek);
        DISK.read((char *) &src, sizeof(dir_entry));
        return src;
    }

public:
    inode_t     inode_n;
    std::string name;
};

#endif//SIMDISK_PATH_H
//
// Created by yieatn on 2021/09/29.
