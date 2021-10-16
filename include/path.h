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
#include <vector>
class path{
public:
    path(): name("/"), inode_n(0) {

    }

    path(uint _inode, const std::string& _name): name(_name), inode_n(_inode) {
    }
    std::string_view dir() {
        std::string_view temp(name);
        if (temp != "/" && temp.ends_with("/"))
            temp.remove_suffix(1);
        return temp;
    }

    std::vector<dir_entry*> list() const {
        std::vector<dir_entry*> all;
//        std::vector<std::pair<uint, std::string>> all;
        int pos = 0;
        uint destInode;
//        DISK.seekg(inodes[inode_n].i_zone[pos++]);
//        std::cout << inode_n << " " << &inodes[inode_n].i_zone[pos++] << std::endl;
        auto *seek = (dir_entry *)(inodes[inode_n].i_zone[pos++] * BLOCKSIZE + startPos);
//        std::cout << (void *)seek << std::endl;
        char temp[28];
        for (int i = 0; i < inodes[inode_n].i_size; ++i) {
//            DISK.read((char *)&destInode, sizeof(int));
//            DISK.read(dest, 32 - 4);
//            std::cout << *(int *)seek << std::endl;
//            std::string(seek + 4, seek + 32);
//            dir_entry dest{};
//            memcpy(&dest, seek, sizeof(dir_entry));
//            std::cout << "HereHere: " << seek << std::endl;
            all.push_back(seek++);
//            all.emplace_back(*(int *)seek, std::string((char *)seek + 4, (char *)seek + 32));
//            seek ++;
//            std::cout << all.back().first << " " << all.back().second << std::endl;
            if ((i+1) * sizeof(dir_entry) % BLOCKSIZE == 0)
                seek = (dir_entry *)(inodes[inode_n].i_zone[pos++] * BLOCKSIZE + startPos);
        }
//        std::cout << "END \n";
        return all;
    }

    inode_t find(const std::string& dest) const {
        auto all = list();
        for (const auto &item : all) {
            if (item->name == dest)
                return item->inode_n;
        }
        return -1;
    }

    path findDir(const std::string &dest) {
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
    }



    path operator / (const std::string &dest) {
        return findDir(dest);
    }

    inode_t create_dir_entry(const std::string& dest, inode_t dest_inode = -1) {
        if (find(dest) != -1){
            std::cerr << "dir already exists\n";
            return -1;
        }
        inode &thisInode = inodes[inode_n];
        uint pos = thisInode.i_size * sizeof(dir_entry) / BLOCKSIZE;
        uint off = thisInode.i_size * sizeof(dir_entry) % BLOCKSIZE;

        //get new empty block.
        if (off == 0){
            auto temp = getNewEmptyBlockNo();
            if (temp == -1){
                std::cerr << "no block available\n";
                return -1;
            }
            thisInode.i_zone[pos] = temp;
        }

        //get new inode
        dest_inode = dest_inode == -1 ? getNewInode() : dest_inode;
        if (dest_inode == -1) {
            std::cerr << "no inode available\n";
            return -1;
        }

        dir_entry newDir{dest_inode, dest.data()};

        auto seek = (dir_entry *)(startPos + thisInode.i_zone[pos] * BLOCKSIZE + off);
//        std::cout << seek << std::endl;
        *seek = newDir;
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
        auto dest_inode = create_dir_entry(dest);
        uint current_block;
        uint pos = 0;
        std::vector<std::string_view> chunks;
        for (int i = 0; i < content.size(); i += 1024)
            chunks.push_back(content.substr(i, 1024));

        for (const auto &p : chunks) {
            current_block = getNewEmptyBlockNo();
            if (current_block == -1) {
                std::cerr << "no block available\n";
                return false;
            }
            inodes[dest_inode].i_zone[pos++] = current_block;
            auto seek = startPos + current_block * BLOCKSIZE;
            memcpy(seek, p.data(), p.size());
            inodes[dest_inode].i_size += p.size();
        }
        return true;
    }

    bool show_content(const std::string &dest) {
        inode_t dest_inode = find(dest);
        if (dest_inode == -1) {
            std::cerr << "File not found\n";
            return false;
        }
        std::string data;
        data.reserve(inodes[dest_inode].i_size);
//        auto seek = startPos +
        for (uint i = 0, pos = 0; i < inodes[dest_inode].i_size; i += BLOCKSIZE) {
            auto seek = startPos + inodes[dest_inode].i_zone[pos] * BLOCKSIZE;
//            memcpy(temp.data(), seek, std::min(1024u, inodes[dest_inode].i_size - i));
            data.append(seek, std::min(1024u, inodes[dest_inode].i_size - i));
        }
        std::cout << data << std::endl;
        return true;
    }

    bool remove_dir (const std::string& dest) {
        if (find(dest) == -1) {
            std::cerr << "dir not found\n";
            return false;
        }

        auto p = find_dir_entry(dest);
        memset(&inodes[p->inode_n], 0, sizeof(inode));
        inodes[inode_n].i_size --;
        auto s = find_last_dir_entry();
        std::swap(*p, *s);
        return true;
    }

private:
    dir_entry* find_dir_entry(const std::string& dest) {
        auto all = list();
        for (auto &p : all) {
            if (p->name == dest)
                return p;
        }
        return nullptr;
    }

    dir_entry* find_last_dir_entry() {
        uint pos = inodes[inode_n].i_size * sizeof(dir_entry) / BLOCKSIZE;
        uint off = inodes[inode_n].i_size * sizeof(dir_entry) % BLOCKSIZE;
        if (off == 0 && pos != 0)
            pos -= 1, off = BLOCKSIZE - sizeof(dir_entry);
        auto seek = (dir_entry *)(startPos + inodes[inode_n].i_zone[pos] * BLOCKSIZE + off);
        return seek;
    }

public:
    inode_t inode_n;
    std::string name;
};

#endif //SIMDISK_PATH_H
