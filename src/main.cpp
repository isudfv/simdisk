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
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;

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
    startOver();
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
    fmt::print("simdisk {}>", currDir.name);
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
            auto dest = currDir.find_dest_dir(cmds[1]);
            if (dest.inode_n == -1){
                fmt::print("No directory named {}\n", dest.name);
            }
            else
                currDir = dest;
        }

        else if (cmds[0] == "ls") {
            auto dirs = currDir.list();
            for (auto &p : dirs) {
                cout << p->name << endl;
            }
        }

        else if (cmds[0] == "mkdir") {
            auto pos = cmds[1].rfind('/') + 1;
            auto filename = cmds[1].substr(pos);
            cmds[1].erase(pos);
            auto dest = currDir.find_dest_dir(cmds[1]);
            if (dest.inode_n == -1) {
                fmt::print("No directory named {}\n", dest.name);
            } else {
                dest.create_dir(filename);
            }
        }
//        printf("simdisk %s>", currDir.dir());

        else if (cmds[0] == "newfile") {
            if (cmds.size() == 2)
                cmds.emplace_back();
            if (cmds.size() != 3) {
                cerr << "Usage: newfile <file> \"content\"\n";
                goto out;
            }
            auto pos = cmds[1].rfind('/') + 1;
            auto filename = cmds[1].substr(pos);
            cmds[1].erase(pos);
            auto dest = currDir.find_dest_dir(cmds[1]);
            if (dest.inode_n == -1) {
                fmt::print("No directory named {}\n", dest.name);
            } else {
                dest.create_file(filename, cmds[2]);
            }
        }

        else if (cmds[0] == "cat") {
            if (cmds.size() != 2) {
                cerr << "Usage: cat <file>\n";
                goto out;
            }
            cout << currDir.get_content(cmds[1]) << endl;
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

        else if (cmds[0] == "cp") {
            if (cmds.size() != 3) {
                cerr << "Usage: cp <dest> <src>\n";
                goto out;
            }
            string content;
            string filename;
            if (cmds[1].starts_with("@PC:")) {
                fs::path src(cmds[1].substr(4));
                ifstream in(src);
                stringstream ss;
                ss << in.rdbuf();
                content = ss.str();
                filename = src.filename().string();
            } else {
                auto src = currDir.find_dest_dir(cmds[1]);
                if (src.inode_n == -1) {
                    fmt::print("{} not found\n", src.name);
                    goto out;
                }
                if (!src.is_file()) {
                    fmt::print("{} is not a file\n", src.name);
                    goto out;
                }
                content = src.get_content();
                filename = src.filename();
            }


            auto dest = currDir.find_dest_dir(cmds[2]);
            if (dest.inode_n == -1) {
                if (cmds[2].ends_with(dest.name)) {
                    filename = dest.name;
                    dest = currDir.find_dest_dir(cmds[2].substr(0, cmds[2].rfind('/') + 1));
//                    dest.copy_file(filename, src.inode_n);
                    dest.create_file(filename, content);
                } else {
                    fmt::print("{} not found\n", dest.name);
                    goto out;
                }
            } else {
                if (dest.is_file()) {
                    fmt::print("{} already exits\n", dest.filename());
                    goto out;
                } else {
                    dest.create_file(filename, content);
//                    dest.copy_file(src.filename(), src.inode_n);
                }
            }
        }

        else
            fmt::print("Unknown command \"{}\"\n", cmds[0]);

        out:
        locker.unlock();
        fmt::print("simdisk {}>", currDir.name);
    }
//    auto startTime = chrono::high_resolution_clock::now();

//    auto endTime = chrono::high_resolution_clock::now();
//    fmt::print("writing total time {}", chrono::duration_cast<chrono::milliseconds>(endTime - startTime));
//    cout << DISK.tellp() << endl;

    return 0;
}
