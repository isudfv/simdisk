//
// Created by yieatn on 2021/09/29.
//

#ifndef SIMDISK_PATH_H
#define SIMDISK_PATH_H
#include "inode.h"
#include <fstream>
#include <iostream>
#include <string_view>
#include <algorithm>
#include <utility>
#include <vector>
#include <fmt/core.h>
#include "users.h"
class path{
public:
    path(): name("/"), inode_n(0) {

    }

        path(uint _inode, std::string  _name): name(std::move(_name)), inode_n(_inode) {
        if (name.empty())
            name.append("/");
    }
/*    std::string_view dir() {
        std::string_view temp(name);
        if (temp != "/" && temp.ends_with("/"))
            temp.remove_suffix(1);
        return temp;
    }*/

    bool is_directory() const {
        return inodes[inode_n].i_mode & IS_DIRECTORY;
    }

    bool is_file() const {
        return inodes[inode_n].i_mode & IS_FILE;
    }

    std::string filename() const {
        return name.substr(name.rfind('/') + 1);
    }

    std::vector<dir_entry> list() const {
        std::vector<dir_entry> all;
//        std::vector<std::pair<uint, std::string>> all;
        int pos = 0;
        uint destInode;
//        DISK.seekg(inodes[inode_n].i_zone[pos++]);
//        std::cout << inode_n << " " << &inodes[inode_n].i_zone[pos++] << std::endl;
        auto seek = (inodes[inode_n].i_zone[pos++]);
//        std::cout << (void *)seek << std::endl;
        char temp[28];
        DISK.seekg(seek);
        for (int i = 0; i < inodes[inode_n].i_size; ++i) {
            dir_entry src{};
            DISK.read((char *)&src, sizeof(dir_entry));
//            DISK.read((char *)&destInode, sizeof(int));
//            DISK.read(dest, 32 - 4);
//            std::cout << *(int *)seek << std::endl;
//            std::string(seek + 4, seek + 32);
//            dir_entry dest{};
//            memcpy(&dest, seek, sizeof(dir_entry));
//            std::cout << "HereHere: " << seek << std::endl;
            all.push_back(src);
//            all.emplace_back(*(int *)seek, std::string((char *)seek + 4, (char *)seek + 32));
//            seek ++;
//            std::cout << all.back().first << " " << all.back().second << std::endl;
            if ((i+1) * sizeof(dir_entry) % BLOCKSIZE == 0)
                seek = (inodes[inode_n].i_zone[pos++]);
        }
//        std::cout << "END \n";
        return all;
    }

    std::vector<std::string> list(std::string_view params) {
        std::vector<std::string> all;
        auto entries = list();
        for (auto &item : entries) {
            if (item.name[0] == '.' && params.find('a') == -1)
                continue;
            if (params.find('l') == -1) {
                all.emplace_back(item.name);
            } else {
                all.push_back(fmt::format("{}{} {:>2} {:<6} {:<6} {:>5} {} {}",
                                          file_type(inodes[item.inode_n].i_mode),
                                          rwx(inodes[item.inode_n].i_mode),
                                          1,
                                          users[inodes[item.inode_n].i_uid].name,
                                          users[inodes[item.inode_n].i_uid].name,
                                          inodes[item.inode_n].i_size,
                                          time_format(inodes[item.inode_n].i_time),
                                          item.name
                                          ));
            }
        }
        return all;
    }

    inode_t find(const std::string& dest) const {
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

    path find_dest_dir(const std::string &dest) {
        path curr = *this;
        if (dest.starts_with("/"))
            curr = path{};
        auto dirs = split(dest, std::regex("[^\\/]+"));
        for (const auto &p : dirs) {
            if (!curr.is_directory()) {
                std::cout << curr.name << " not a directory\n";
                break;
            }
            auto seek = curr.find_dir_entry(p);
            if (seek.inode_n == -1) {
                return path(-1, p);
            }
            if (curr.name == "/") {
                if (p.starts_with("."))
                    curr =  *this;
                else
                    curr = {seek.inode_n, curr.name + seek.name};
            } else {
                if (p == ".")
                    curr = *this;
                else if (p == ".."){
                    curr = {seek.inode_n, curr.name.substr(0, name.rfind('/'))};
                }
                else
                    curr = {seek.inode_n, curr.name + "/" + seek.name};
            }
        }
        return curr;
    }


    path operator / (const std::string &dest) {
        return find_dest_dir(dest);
    }

    inode_t create_dir_entry(const std::string& dest, inode_t dest_inode = -1, inode_mode_t mode = IS_DIRECTORY) {
        if (find(dest) != -1){
            std::cout << "dir already exists\n";
            return -1;
        }
        inode &thisInode = inodes[inode_n];
        uint pos = thisInode.i_size * sizeof(dir_entry) / BLOCKSIZE;
        uint off = thisInode.i_size * sizeof(dir_entry) % BLOCKSIZE;

        //get new empty block.
        if (off == 0){
            auto temp = getNewEmptyBlockNo();
            if (temp == -1){
                std::cout << "no block available\n";
                return -1;
            }
            thisInode.i_zone[pos] = temp * BLOCKSIZE;
        }

        //get new inode
        dest_inode = dest_inode == -1 ? getNewInode() : dest_inode;
        if (dest_inode == -1) {
            std::cout << "no inode available\n";
            return -1;
        }

        dir_entry newDir{dest_inode, dest.data()};

        auto seek = (thisInode.i_zone[pos] + off);
//        std::cout << seek << std::endl;
//        *seek = newDir;
        DISK.seekp(seek);
        DISK.write((char *)&newDir, sizeof(newDir));

        inodes[dest_inode].i_mode |= mode;
        inodes[dest_inode].i_uid = curr_uid;
        if (mode == IS_DIRECTORY){
            inodes[dest_inode].i_mode |= (7 << 6);
            inodes[dest_inode].i_mode |= (5 << 3);
            inodes[dest_inode].i_mode |= (5     );
        } else if (mode == IS_FILE) {
            inodes[dest_inode].i_mode |= (6 << 6);
            inodes[dest_inode].i_mode |= (4 << 3);
            inodes[dest_inode].i_mode |= (4     );
        }
        thisInode.i_size ++;

        return dest_inode;
    }

    bool create_dir(const std::string& dest) {
        auto dest_inode = create_dir_entry(dest);
        if (dest_inode == -1) {
            return false;
        }
        path sub = *this / dest;
//        std::cout << "Here: " << inode_n << " " << "Sub: " << sub.inode_n << std::endl;
        sub.create_dir_entry(".", dest_inode);
        sub.create_dir_entry("..", inode_n);
        return true;
    }

    bool create_file(const std::string &dest, const std::string_view content) {
        auto dest_inode = create_dir_entry(dest, -1, IS_FILE);

        if (dest_inode == -1) {
            return false;
        }

        uint current_block;
        uint pos = 0;
        std::vector<std::string_view> chunks;
        for (int i = 0; i < content.size(); i += 1024)
            chunks.push_back(content.substr(i, 1024));

        for (const auto &p : chunks) {
            current_block = getNewEmptyBlockNo();
            if (current_block == -1) {
                std::cout << "no block available\n";
                return false;
            }
            inodes[dest_inode].i_zone[pos] = current_block * BLOCKSIZE;
//            auto seek = current_block;
//            memcpy(seek, p.data(), p.size());
            DISK.seekp(inodes[dest_inode].i_zone[pos]);
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
                std::cout << "no block available\n";
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

    std::string get_content(const std::string &dest) {
        inode_t dest_inode = find(dest);
        if (dest_inode == -1) {
            std::cout << "File not found\n";
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
    }

    std::string get_content() {
        if (!is_file())
            return {};
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

    bool remove_dir (const std::string& dest) {
        auto p = find_dir_entry(dest);

        if (p.inode_n == -1) {
            std::cout << "dir not found\n";
            return false;
        }

        if (inodes[p.inode_n].i_uid != curr_uid && curr_uid != 0 &&
            (inodes[p.inode_n].i_mode & (2)) == 0) {
            std::cout << "Permission denied\n";
        }

        if (inodes[p.inode_n].i_mode & IS_DIRECTORY && inodes[p.inode_n].i_size > 2){
            std::cout << "Directory not empty, please type yes: \n";
            std::string get;
            std::cin >> get;
            getchar();
            if (get != "yes")
                return false;
        }
        memset(&inodes[p.inode_n], 0, sizeof(inode));
        inodes[inode_n].i_size --;
        auto s = find_last_dir_entry();

        auto getDirentryAdd = [this, &dest]() {
            int pos = 0;
            auto seek = (inodes[inode_n].i_zone[pos++]);
            DISK.seekg(seek);
            for (int i = 0; i < inodes[inode_n].i_size; ++i) {
                dir_entry src{};
                auto curr_pos = DISK.tellg();
                DISK.read((char *)&src, sizeof(dir_entry));
                if (src.name == dest)
                    return curr_pos;
                if ((i+1) * sizeof(dir_entry) % BLOCKSIZE == 0)
                    seek = (inodes[inode_n].i_zone[pos++]);
            }
        };

        DISK.seekp(getDirentryAdd());
        DISK.write((char *)&s, sizeof(s));


        return true;
    }

private:
    dir_entry find_dir_entry(const std::string& dest) {
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
        auto seek = (inodes[inode_n].i_zone[pos] + off);
        dir_entry src{};
        DISK.seekg(seek);
        DISK.read((char *)&src, sizeof(dir_entry));
        return src;
    }

public:
    inode_t inode_n;
    std::string name;
};

#endif //SIMDISK_PATH_H
