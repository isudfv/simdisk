//
// Created by yieatn on 2021/10/20.
//

#ifndef SIMDISK_IPC_H
#define SIMDISK_IPC_H
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

int shmid_out, shmid_in;
key_t key_out = 41045, key_in = 41046;
void *shm_out, *shm_in;

std::string getIPCString (const std::vector<std::string> &strs) {
    std::string str;
    for (const auto &item : strs) {
        str.append(item);
        str.append("\n");
//        if (&item != &strs.back())
//            str.append("\n");
    }
    return str;
}

std::string getIPCString (const char* str) {
    return str;
}

std::string getIPCString (const std::string &str) {
    return str;
}

template<typename T>
void outToSHM(const T &a, void *shm) {
    auto get = getIPCString(a);
    char *src = (char *)shm;
    while(*(char *)src != 0) src++;
    strcpy((char *)src, get.c_str());
    std::cout << "out============\n" << src << "\n==============\n";
}

bool inFromSHM(char *dest, void *shm) {
    using namespace std::literals;
    while(*(char *)shm == 0)
        std::this_thread::sleep_for(0.5s);
    std::cout << "in============\n" << dest << "\n==============\n";
    strcpy(dest, (char *)shm);
    *(char *)shm = 0;
    return true;
}

bool inFromSHM(std::string& dest, void *shm) {
    using namespace std::literals;
    while(*(char *)shm == 0)
        std::this_thread::sleep_for(0.5s);
    dest = std::string((char *)shm);
    std::cout << "in============\n" << dest << "\n==============\n";
    *(char *)shm = 0;
    return true;
}
#endif//SIMDISK_IPC_H
