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
//to convert char* std::string std::vector<std::string> to c-style str;
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
//output to shared memory
template<typename T>
void outToSHM(const T &a, void *shm) {
    auto get = getIPCString(a);
    char *dest = (char *)shm + 1;
    while(*dest != 0) dest++;
//    *(char *)shm = 0;
    strcpy(dest, get.c_str());
//    *(char *)shm = 1;
//    std::cout << "out============\n" << dest << "\n==============\n";
}

void outToSHM(void *src, void *shm, int n) {
    shm = (char *)shm + 1;
    memcpy(shm, src, n);
}
//to indicate that the shared memory is ready to read
void allout(void *shm) {
    *(char *)shm = 1;
}
//to indicate that the shared memory is about be to wrote
void preparing(void *shm) {
    *(char *)shm = 0;
}
//to read str from shared memory
bool inFromSHM(void *dest, void *shm, int n) {
    using namespace std::literals;
    char *src = (char *)shm + 1;
    while(*(char *)shm == 0);//if the memory is ready, reading from it
    //        std::this_thread::sleep_for(0.5s);
    memcpy(dest, src, n);
    //    memcpy(dest, src, n);
    //    std::cout << "in============\n" << dest << "\n==============\n";
    memset(shm, 0, (1<<20));
    return true;
}
//to read str from shared memory
bool inFromSHM(char *dest, void *shm) {
    using namespace std::literals;
    char *src = (char *)shm + 1;
    while(*(char *)shm == 0);
//        std::this_thread::sleep_for(0.5s);
    strcpy(dest, src);
//    std::cout << "in============\n" << dest << "\n==============\n";
    memset(shm, 0, (1<<20));
    return true;
}
//to read str from shared memory
bool inFromSHM(std::string& dest, void *shm) {
    using namespace std::literals;
    char *src = (char *)shm + 1;
    while(*(char *)shm == 0);
//        std::this_thread::sleep_for(0.5s);
    dest = std::string((char *)src);
//    std::cout << "in============\n" << dest << "\n==============\n";
//    *(char *)shm = 0;
    memset(shm, 0, (1<<20));
    return true;
}
#endif//SIMDISK_IPC_H
