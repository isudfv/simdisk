//
// Created by yieatn on 2021/09/30.
//

#ifndef SIMDISK_USERS_H
#define SIMDISK_USERS_H
#include <iostream>
#include "inode.h"
struct user{
    uint16_t uid;
    uint64_t password;
    char name[24];

    /*user(uint16_t _uid, uint64_t _password, const char *_name) : uid(_uid), password(_password){
        strcpy(name, _name);
    }*/
};

//user * const users = (user *)(startPos + USERS);

const uint16_t USERNUM = 4;

user users[USERNUM];


#endif //SIMDISK_USERS_H
