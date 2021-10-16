#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include "inode.h"
#include "path.h"
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <chrono>
#include <thread>
using namespace std;

std::mutex locker;

void startOver( ){
    for (int i = 0; i < 16; ++i) {
        bitmap[i] = UINT64_MAX;
    }
    bitmap[16] = 0x7FFF;
    DISK.seekp(FREEBLOCKS);
    DISK.write((char *)bitmap, sizeof(bitmap[0]) * BITMAPNUM);
    for (int i = 0; i < INODENUM; ++i) {
        memset(&inodes[i], 0, sizeof(inodes[i]));
    }
    DISK.seekp(INDEXNODE);
    DISK.write((char *)inodes, sizeof(inodes[0]) * 1600);
    inodes[0].i_size = 2;
    inodes[0].i_zone[0] = ROOTDIR / 1024;
    inodes[0].i_mode |= IS_DIRECTORY;
    DISK.seekp(INDEXNODE);
    DISK.write((char *)&inodes[0], sizeof(inode));

    DISK.seekp(ROOTDIR);
    int temp = 0;
    DISK.write((char *)&temp, sizeof(int));
    DISK.write(".", 2);
    DISK.seekp(32 - 4 - 2, ios::cur);
    DISK.write((char *)&temp, sizeof(int));
    DISK.write("..", 3);
    DISK.seekp(32 - 4 - 3, ios::cur);
}

int main() {
//    startOver();
//    return 0;
    memset(startPos, 0, 100<<20);
    DISK.seekg(0);
    cout << (void *)startPos << endl;
    DISK.read(startPos, 100<<20);


    /*DISK.seekg(INDEXNODE);
    for (auto & i : inodes) {
        DISK.read((char *)&i, sizeof(inode));
    }

    DISK.seekg(FREEBLOCKS);
    DISK.read((char *)bitmap, sizeof(bitmap));

    cout << getNewEmptyBlockNo();
    return 0;*/



    path currDir;
    string cmdline;

    std::thread writeIntoDisk([](){
        locker.lock();
        while (true){
            locker.lock();
            DISK.seekp(0);
            DISK.write(startPos, 100<<20);
        }
    });
    writeIntoDisk.detach();
//    cout << "simdisk " << currDir.dir() << ">" ;
    fmt::print("simdisk {}>", currDir.dir());
    while (getline(cin, cmdline)) {
        auto cmds = split(cmdline);
        if (cmds.empty()) {
            goto out;
            continue;
        }
        if (cmds[0] == "cd") {
            if (cmds.size() != 2){
                cerr << "Usage: cd <dir>\n";
                goto out;
                continue;
            }
            auto dirs = split(cmds[1], std::regex("[^\\/]+"));

            path destDir;
            if (cmds[1].front() != '/')
                destDir = currDir;

            bool reachDestDir = true;
            for (auto &p : dirs) {
                if (destDir.find(p) != -1)
                    destDir = destDir.findDir(p);
                else{
                    cerr << "no dir named \"" << p << "\"\n";
                    goto out;
                }
            }
            currDir = destDir;
        }

        else if (cmds[0] == "ls") {
            auto dirs = currDir.list();
            for (auto &p : dirs) {
                cout << p->name << endl;
            }
        }

        else if (cmds[0] == "mkdir") {
            currDir.create_dir(cmds[1]);
        }
//        printf("simdisk %s>", currDir.dir());

        else if (cmds[0] == "newfile") {
            if (cmds.size() == 2)
                cmds.emplace_back();
            if (cmds.size() != 3) {
                cerr << "Usage newfile <file> \"content\"\n";
                goto out;
            }
            currDir.create_file(cmds[1], cmds[2]);
        }

        else if (cmds[0] == "cat") {
            if (cmds.size() != 2) {
                cerr << "Usage cat <file>\n";
                goto out;
            }
            currDir.show_content(cmds[1]);
        }

        else if (cmds[0] == "rm") {
            if (cmds.size() != 2) {
                cerr << "Usage: rm <dir>\n";
                goto out;
                continue;
            }
            currDir.remove_dir(cmds[1]);

        }

        else if (cmds[0] == "exit") {
            break;
        }

        else
            fmt::print("Unknown command \"{}\"\n", cmds[0]);

        out:
        locker.unlock();
        fmt::print("simdisk {}>", currDir.dir());
    }
//    auto startTime = chrono::high_resolution_clock::now();

//    auto endTime = chrono::high_resolution_clock::now();
//    fmt::print("writing total time {}", chrono::duration_cast<chrono::milliseconds>(endTime - startTime));
//    cout << DISK.tellp() << endl;

    return 0;
}
