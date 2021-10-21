//
// Created by yieatn on 2021/9/15.
//

#ifndef SIMDISK_INODE_H
#define SIMDISK_INODE_H

#include <regex>
#include <cstdint>
#include <fstream>
#include <filesystem>

//char * const startPos = (char *)malloc(100<<20);

const uint SUPERBLOCK = 0;

    const uint USERS = SUPERBLOCK + 0;

//    const uint16_t USERNUM = USERS + 128;
//
//    const uint32_t USEDINODE = USERNUM + 4;
//
//    const uint32_t USEDBLOCK = USEDINODE + 4;


const uint BLOCKSIZE = 1024;

const uint SIMDISKSIZE = 100 << 20;

const uint BLOCKNUM = SIMDISKSIZE / BLOCKSIZE;

const uint FREEBLOCKS = SUPERBLOCK + 1024;
//bit map uses 12.5 blocks`
const uint INDEXNODE = FREEBLOCKS + (13 << 10);
//inode used 1 MiB
const uint ROOTDIR = INDEXNODE + (1 << 20);

using inode_mode_t = uint16_t;
const inode_mode_t IS_DIRECTORY = 0x0200;
const inode_mode_t IS_FILE = 0x0400;
const inode_mode_t IS_WRITTING = 0x800;

struct inode{
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    time_t   i_time;
    uint32_t i_zone[12];
};

/*struct inode{
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_time;
    uint32_t i_zone[13];
};*/

using inode_t = uint32_t;

const uint INODENUM = (1<<20) / (sizeof(inode));

inline inode inodes[INODENUM];
//inode * const inodes = (inode *)(startPos + INDEXNODE);

struct dir_entry{
    inode_t inode_n;
    //only support file name for 28 characters
    char name[32-sizeof(int)];

    dir_entry() = default;

    dir_entry(inode_t n, const char *dest) : inode_n(n) {
        if (dest)
            strcpy(name, dest);
    }
};

const uint BITMAPNUM = 1600;

uint64_t bitmap[BITMAPNUM];

//uint64_t * const bitmap = (uint64_t *)(startPos + FREEBLOCKS);

inline std::fstream DISK("./disk", std::ios::in | std::ios::out | std::ios::binary);

inline uint16_t curr_uid;

inline char curr_username[24];
//to get an unused inode
inode_t getNewInode() {
    for (uint i = 0; i < INODENUM; ++i) {
        if (inodes[i].i_size == 0)
            return i;
    }
    return -1;
}
//to count used inodes
inode_t inodeNum() {
    inode_t tot = 0;
    for (auto &item : inodes)
        tot += (item.i_size != 0);
    return tot;
}
//to get an unused block
uint32_t getNewEmptyBlockNo() {
    uint i;
    for (i = 0; i < BITMAPNUM && 0xFFFFFFFFFFFFFFFF == bitmap[i]; ++i);
    const int bitInWord = ffsll(~bitmap[i]);
    if (bitInWord != 0)
        bitmap[i] |= (1 << (bitInWord - 1));
    const uint32_t firstZeroBit = bitInWord ? i * sizeof(*bitmap) * 8 + bitInWord - 1 : -1;
    return firstZeroBit ;
}
//to count used blocks
uint32_t blockNum() {
    uint32_t tot = 0;
    for (auto n : bitmap) {
        while (n != 0) {
            n = n & (n-1);
            tot ++;
        }
    }
    return tot;
}
//to split a string with regex
std::vector<std::string> split(const std::string& s,
                               const std::regex& re = std::regex("[^\\s \"]+|\"([^\"]*)\"")) {
    std::vector<std::string> cmds;
    for_each(std::sregex_iterator(s.begin(), s.end(), re),
             std::sregex_iterator(),
             [&](const auto& match) {
                 int p = 0;
                 while (match[p].length()) p++;
                 cmds.push_back(match[p - 1]);
             });
    return cmds;
}
//to distinguish a file from directory to regular file
char file_type(inode_mode_t p) {
    if (p & IS_DIRECTORY) return 'd';
    else if (p & IS_FILE) return 'f';
    else return '-';
}
//to output the permissions of a file
std::string rwx(inode_mode_t p) {
    using std::filesystem::perms;
    auto check = [p](perms bit, char c) {
        return (p & (inode_mode_t)bit) ? c : '-';
    };
    return {check(perms::owner_read  , 'r'),
            check(perms::owner_write , 'w'),
            check(perms::owner_exec  , 'x'),
            check(perms::group_read  , 'r'),
            check(perms::group_write , 'w'),
            check(perms::group_exec  , 'x'),
            check(perms::others_read , 'r'),
            check(perms::others_write, 'w'),
            check(perms::others_exec , 'x')};
}
//to format time, for example [Thu 21 18:07]
std::string time_format(time_t t) {
    std::stringstream fmtedTime;
    fmtedTime << std::put_time(localtime(&t), "%a %d %H:%M");
    return fmtedTime.str();
}

#endif //SIMDISK_INODE_H
