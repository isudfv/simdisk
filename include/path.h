//
// Created by yieatn on 2021/09/29.
//

#ifndef SIMDISK_PATH_H
#define SIMDISK_PATH_H
#include <iostream>
#include <fstream>
#include <string_view>
#include "inode.h"
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

    std::string filename() {

    }


    /*std::vector<std::string> list(){
        std::vector<std::string> all;
        int pos = 0;
        int destInode;
        DISK.seekg(inodes[inode_n].i_zone[pos++]);
        char temp[28];
        for (int i = 0; i < inodes[inode_n].i_size; ++i) {
            DISK.read((char *)&destInode, sizeof(int));
            DISK.read(temp, 32 - 4);
            all.emplace_back(temp);
            if ((i+1) * sizeof(dir_entry) % BLOCKSIZE == 0)
                DISK.seekg(inodes[inode_n].i_zone[pos++]);
        }
        return all;
    }*/

    std::vector<dir_entry*> list() {
        std::vector<dir_entry*> all;
//        std::vector<std::pair<uint, std::string>> all;
        int pos = 0;
        uint destInode;
//        DISK.seekg(inodes[inode_n].i_zone[pos++]);
//        std::cout << inode_n << " " << &inodes[inode_n].i_zone[pos++] << std::endl;
        auto *seek = (dir_entry *)(inodes[inode_n].i_zone[pos++] + startPos);
//        std::cout << (void *)seek << std::endl;
        char temp[28];
        for (int i = 0; i < inodes[inode_n].i_size; ++i) {
//            DISK.read((char *)&destInode, sizeof(int));
//            DISK.read(dest, 32 - 4);
//            std::cout << *(int *)seek << std::endl;
//            std::string(seek + 4, seek + 32);
//            dir_entry dest{};
//            memcpy(&dest, seek, sizeof(dir_entry));
            all.push_back(seek++);
//            all.emplace_back(*(int *)seek, std::string((char *)seek + 4, (char *)seek + 32));
//            seek ++;
//            std::cout << all.back().first << " " << all.back().second << std::endl;
            if ((i+1) * sizeof(dir_entry) % BLOCKSIZE == 0)
                seek = (dir_entry *)(inodes[inode_n].i_zone[pos++] + startPos);
        }
        return all;
    }

    bool find(const std::string& dest) {
        auto all = list();
        for (auto &p : all)
            if (p->name == dest)
                return true;
        return false;
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
                        if (temp.empty())
                            temp.append("/");
                        return {p->inode_id, temp};
                    }
                }
                else
                    return {p->inode_id, name + dest + "/"};
            }
    }

    //0-success, -1-error, 1-permission denied, 2-dir already exists
    bool create_dir(const std::string& dest) {
        if (find(dest)){
            std::cerr << "dir already exists\n";
            return false;
        }
        inode &thisInode = inodes[inode_n];
        uint pos = thisInode.i_size * sizeof(dir_entry) / BLOCKSIZE;
        uint off = thisInode.i_size * sizeof(dir_entry) % BLOCKSIZE;

        //get new empty block.
        if (off == 0){
            auto temp = getNewEmptyBlockNo();
            if (temp == 0){
                std::cerr << "no block available\n";
                return false;
            } else
                temp -= 1;
            thisInode.i_zone[pos] = temp;
        }

        //get new inode
        auto newInode = getNewInode();
        if (newInode == -1) {
            std::cerr << "no inode available\n";
            return false;
        }

        dir_entry newDir{};
        newDir.inode_id = newInode;
        strcpy(newDir.name, dest.data());

//        std::cout << "name :" << newDir.name << std::endl ;
//        std::cout << "id :" << newDir.inode_id << std::endl ;

        auto seek = (dir_entry *)(startPos + thisInode.i_zone[pos] + off);
        *seek = newDir;
        thisInode.i_size ++;

        {
            auto tempBlockNo = getNewEmptyBlockNo();
            if (tempBlockNo == 0){
                std::cerr << "no block available\n";
                return false;
            } else
                tempBlockNo -= 1;
            inodes[newInode].i_size = 2;
            inodes[newInode].i_zone[0] = tempBlockNo ;
//            DISK.seekp(tempBlockNo * BLOCKSIZE);
            seek = (dir_entry *)(startPos + tempBlockNo);
            newDir.inode_id = newInode;
            strcpy(newDir.name, ".");
            *(seek++) = newDir;

            newDir.inode_id = inode_n;
            strcpy(newDir.name, "..");
            *(seek++) = newDir;
//            std::cout << newInode << "  " << tempBlockNo << std::endl;
        }


        return true;
    }

    bool remove_dir (std::string& dest) {
        if (!find(dest)) {
            std::cerr << "dir not found\n";
            return 0;
        }

        auto p = find_dir_entry(dest);
        memset(&inodes[p->inode_id], 0, sizeof(inode));
        inodes[inode_n].i_size --;
        auto s = find_last_dir_entry();
        std::swap(*p, *s);
    }

private:
    dir_entry* find_dir_entry(std::string dest) {
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
        auto seek = (dir_entry *)(startPos + inodes[inode_n].i_zone[pos] + off);
        return seek;
    }

public:
    uint inode_n;
    std::string name;
};

#endif //SIMDISK_PATH_H
