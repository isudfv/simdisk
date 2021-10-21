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
//to judge if there is enough place for new users
auto userNotFull () {
    return std::any_of(std::begin(users), std::end(users), [](auto &p){
        return p.password == 0;
    });
};
//to judge if a user exits
auto userExists (const std::string &name) {
    return std::any_of(std::begin(users), std::end(users), [&name](auto &p){
        return p.name == name;
    });
};
//to count all the users
uint16_t userNum () {
    uint16_t tot = 0;
    for (auto &item : users) {
        tot += (item.password != 0);
    }
    return tot;
}
#endif //SIMDISK_USERS_H
