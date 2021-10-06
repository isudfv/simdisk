#include <iostream>
#include <fstream>
#include <string>
#include "string.h"
#include <string_view>
#include "inode.h"
#include "path.h"
using namespace std;

void startover( ){
    for (int i = 0; i < 16; ++i) {
        bitmap[i] = UINT64_MAX;
    }
    bitmap[16] = 0x7FFF;
    DISK.seekp(FREEBLOCKS);
    DISK.write((char *)bitmap, sizeof(bitmap));
    /*inodes[0].i_size = 2;
    inodes[0].i_zone[0] = ROOTDIR;
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
    DISK.seekp(32 - 4 - 3, ios::cur);*/
}

int main() {
//    startover();
//    return 0;

    cout << sizeof(inode) << endl;

    DISK.seekg(INDEXNODE);
    for (int i = 0; i < INODENUM; ++i) {
        DISK.read((char *)&inodes[i], sizeof(inode));
    }

    DISK.seekg(FREEBLOCKS);
    DISK.read((char *)bitmap, sizeof(bitmap));

//    cout << getNewEmptyBlockNo();
//    return 0;



    path currDir;
    string cmdline;
    cout << "simdisk " << currDir.dir() << ">" ;
    while (getline(cin, cmdline)) {
        auto cmds = split(cmdline);
        if (cmds.empty()) {
            cout << "simdisk " << currDir.dir() << ">" ;
            continue;
        }
        if (cmds[0] == "cd") {
            if (cmds.size() != 2){
                cerr << "Usage: cd <dir>\n";
                cout << "simdisk " << currDir.dir() << ">" ;
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
            for (auto &[_, p] : dirs) {
                cout << p << endl;
            }
        }

        if (cmds[0] == "mkdir") {
            currDir.create_dir(cmds[1]);
        }
//        printf("simdisk %s>", currDir.dir());

        DISK.seekp(FREEBLOCKS);
        DISK.write((char *)bitmap, sizeof(bitmap));
        DISK.seekp(INDEXNODE);
        for (int i = 0; i < INODENUM; ++i)
            DISK.write((char *)&inodes[i], sizeof(inode));


        cout << "simdisk " << currDir.dir() << ">" ;

    }

    return 0;
}
