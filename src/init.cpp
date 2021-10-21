//
// Created by yieatn on 2021/10/20.
//
#include <iostream>
#include <fstream>
#include "inode.h"
#include "path.h"
#include "users.h"
using namespace std;

void init( ){
    fstream DISK("./disk", ios::binary | ios::out | ios::in);
    //initialize bitmap
    for (int i = 0; i < 16; ++i) {
        bitmap[i] = UINT64_MAX;
    }
    bitmap[16] = 0x7FFF;
    DISK.seekp(FREEBLOCKS);
    DISK.write((char *)bitmap, sizeof(bitmap[0]) * BITMAPNUM);
    //initialize inodes
    for (auto & inode : inodes) {
        memset(&inode, 0, sizeof(inode));
    }
    DISK.seekp(INDEXNODE);
    DISK.write((char *)inodes, sizeof(inodes[0]) * 1600);
    inodes[0].i_size = 2;
    inodes[0].i_zone[0] = ROOTDIR;
    inodes[0].i_mode |= IS_DIRECTORY;
    inodes[0].i_mode |= 0755;
    inodes[0].i_uid = 0;
    inodes[0].i_time = std::time(nullptr);
    DISK.seekp(INDEXNODE);
    DISK.write((char *)&inodes[0], sizeof(inode));
    //create root directory
    DISK.seekp(ROOTDIR);
    int temp = 0;
    DISK.write((char *)&temp, sizeof(int));
    DISK.write(".", 2);
    DISK.seekp(32 - 4 - 2, ios::cur);
    DISK.write((char *)&temp, sizeof(int));
    DISK.write("..", 3);
    DISK.seekp(32 - 4 - 3, ios::cur);
    //create root user
    char rootname[24] = "root";
    uint64_t roothash = hash<string>{}("123");
    uint16_t rootuid = 0;
    user rootuser{rootuid, roothash, "root"};
    DISK.seekp(USERS);
    DISK.write((char *)&rootuser, sizeof(rootuser));
    uint16_t usernum = 1;
    DISK.write((char *)&usernum, sizeof(usernum));
}

int main() {
    ofstream out("./disk", ios::binary);
    //create a file whose size is 100 MiB
    uint64_t block = 0;
    for (int i = 0; i < (100 << 20) / (sizeof(uint64_t)); ++i) {
        out.write((char *)&block, sizeof(uint64_t));
    }

    init();
    out.close();

//    dir_entry s;
//    ifstream in("../disk", ios::binary);
//    in.seekg(ROOTDIR);
//    in.read((char *)&s, sizeof(s));
//    cout << s.name << " " << s.inode_n << endl;
//    in.read((char *)&s, sizeof(s));
//    cout << s.name << " " << s.inode_n << endl;
//
//    user rootuser;
//    in.seekg(USERS);
//    in.read((char *)&rootuser, sizeof(user));
//    cout << rootuser.name << " " << rootuser.password << " " << rootuser.uid << endl;

    return 0;
}