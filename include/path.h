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

    std::vector<std::pair<uint, std::string>> list() {
        std::vector<std::pair<uint, std::string>> all;
        int pos = 0;
        uint destInode;
        DISK.seekg(inodes[inode_n].i_zone[pos++]);
        char temp[28];
        for (int i = 0; i < inodes[inode_n].i_size; ++i) {
            DISK.read((char *)&destInode, sizeof(int));
            DISK.read(temp, 32 - 4);
            all.emplace_back(destInode, std::string(temp));
            if ((i+1) * sizeof(dir_entry) % BLOCKSIZE == 0)
                DISK.seekg(inodes[inode_n].i_zone[pos++]);
        }
        return all;
    }

    bool find(const std::string& dest) {
        auto all = list();
        for (auto &[_, p] : all)
            if (p == dest)
                return true;
        return false;
    }

    path findDir(const std::string &dest) {
        auto all = list();
        for (auto &[_, p] : all)
            if (p == dest){
                if (p == ".")
                    return *this;
                else if (p == ".."){
                    if (name == "/")
                        return *this;
                    else {
                        std::string temp(dir());
                        temp.erase(temp.rfind('/'));
                        if (temp.empty())
                            temp.append("/");
                        return {_, temp};
                    }
                }
                else
                    return {_, name + dest + "/"};
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

        DISK.seekp(thisInode.i_zone[pos] + off);
        DISK.write((char *)&newDir, sizeof(newDir));
        thisInode.i_size ++;

        {
            auto tempBlockNo = getNewEmptyBlockNo();
            if (tempBlockNo == 0){
                std::cerr << "no block available\n";
                return false;
            } else
                tempBlockNo -= 1;
            inodes[newInode].i_size = 2;
            inodes[newInode].i_zone[0] = tempBlockNo * BLOCKSIZE;
            DISK.seekp(tempBlockNo * BLOCKSIZE);

            newDir.inode_id = newInode;
            strcpy(newDir.name, ".");
            DISK.write((char *)&newDir, sizeof(dir_entry));
            newDir.inode_id = inode_n;
            strcpy(newDir.name, "..");
            DISK.write((char *)&newDir, sizeof(dir_entry));
            std::cout << newInode << "  " << tempBlockNo << std::endl;
        }


        return true;
    }
public:
    uint inode_n;
    std::string name;
};

#endif //SIMDISK_PATH_H
