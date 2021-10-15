#include <iostream>
#include <fstream>
#include <string>
#include "string.h"
#include <string_view>
#include "inode.h"
#include "path.h"
#include <fmt/core.h>
using namespace std;

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
    cout << inodes << endl;
//    cout << inodes[0]

    cout << inodes[0].i_zone << endl;
    cout << &inodes[0].i_zone[0] << endl;
    cout << inodes[0].i_zone[1] << endl;
//    cout << "simdisk " << currDir.dir() << ">" ;
    fmt::print("simdisk {}>", currDir.dir());
    while (getline(cin, cmdline)) {
        auto cmds = split(cmdline);
        if (cmds.empty()) {
            fmt::print("simdisk {}>", currDir.dir());
            continue;
        }
        if (cmds[0] == "cd") {
            if (cmds.size() != 2){
                cerr << "Usage: cd <dir>\n";
                fmt::print("simdisk {}>", currDir.dir());
                continue;
            }
            auto dirs = split(cmds[1], std::regex("[^\\/]+"));

            path destDir;
            if (cmds[1].front() != '/')
                destDir = currDir;

            bool reachDestDir = true;
            for (auto &p : dirs) {
                if (destDir.find(p))
                    destDir = destDir.findDir(p);
                else{
                    cerr << "no dir named \"" << p << "\"\n";
                    reachDestDir = false;
                    break;
                }
            }
            if (reachDestDir)
                currDir = destDir;
        }

        if (cmds[0] == "ls") {
            auto dirs = currDir.list();
            for (auto &p : dirs) {
                cout << p->name << endl;
            }
        }

        if (cmds[0] == "mkdir") {
            currDir.create_dir(cmds[1]);
        }
//        printf("simdisk %s>", currDir.dir());

        if (cmds[0] == "rm") {
            if (cmds.size() != 2) {
                cerr << "Usage: rm <dir>\n";
                fmt::print("simdisk {}>", currDir.dir());
                continue;
            }
            currDir.remove_dir(cmds[1]);

        }

        if (cmds[0] == "exit") {
            break;
        }

        fmt::print("simdisk {}>", currDir.dir());
    }
    DISK.seekp(0);
    DISK.write(startPos, 100<<20);
//    cout << DISK.tellp() << endl;

    return 0;
}
