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
#include "users.h"
using namespace std;
namespace fs = std::filesystem;

//std::mutex locker;

void startOver( ){
    for (int i = 0; i < 16; ++i) {
        bitmap[i] = UINT64_MAX;
    }
    bitmap[16] = 0x7FFF;
    DISK.seekp(FREEBLOCKS);
    DISK.write((char *)bitmap, sizeof(bitmap[0]) * BITMAPNUM);
    for (auto & inode : inodes) {
        memset(&inode, 0, sizeof(inode));
    }
    DISK.seekp(INDEXNODE);
    DISK.write((char *)inodes, sizeof(inodes[0]) * 1600);
    inodes[0].i_size = 2;
    inodes[0].i_zone[0] = ROOTDIR;
    inodes[0].i_mode |= IS_DIRECTORY;
    inodes[0].i_uid = 0;
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
//    startOver();
//    return 0;


    DISK.seekg(INDEXNODE);
    DISK.read((char *)&inodes, sizeof(inodes));

    DISK.seekg(FREEBLOCKS);
    DISK.read((char *)bitmap, sizeof(bitmap));

    DISK.seekg(USERS);
    DISK.read((char *)users, sizeof(users));

//    char curr_username[24];
    cout << fmt::format("Log in as: ");
    while (cin >> curr_username) {
        for (auto & user : users) {
            if (user.password == 0) continue;
            if (strcmp(curr_username, user.name) == 0) {
                cout << fmt::format("password: ");
                string password;
                while (cin >> password) {
                    if (hash<string>{}(password) == user.password) {
                        curr_uid = user.uid;
                        goto start;
                    } else {
                        std::cout << fmt::format("wrong password, please try again: ");
                    }
                }
            }
        }
        std::cout << fmt::format("user not found, please try again: ");
    }

    start:
    cin.ignore(std::numeric_limits< streamsize >::max(), '\n');

    path currDir;
    string cmdline;

    /*std::thread writeIntoDisk([](){
        locker.lock();
        while (true){
            locker.lock();
            DISK.seekp(0);
            DISK.write(startPos, 100<<20);
        }
    });
    writeIntoDisk.detach();*/
//    cout << "simdisk " << currDir.dir() << ">" ;

    auto writeIntoDisk = []() {
        DISK.seekp(INDEXNODE);
        DISK.write((char *)&inodes, sizeof(inodes));

        DISK.seekp(FREEBLOCKS);
        DISK.write((char *)bitmap, sizeof(bitmap));

        DISK.seekp(USERS);
        DISK.write((char *)users, sizeof(users));
    };


    cout << fmt::format("{}@simdisk:{}>", curr_username, currDir.name);
    while (getline(cin, cmdline)) {
        auto cmds = split(cmdline);
        if (cmds.empty()) {
            cout << fmt::format("\n");
            goto out;
            continue;
        }
        if (cmds[0] == "cd") {
            if (cmds.size() == 1)
                cmds.emplace_back("/");
            auto dest = currDir.find_dest_dir(cmds[1]);
            if (dest.inode_n == -1){
                std::cout << fmt::format("No directory named {}\n", dest.name);
            } else if (!dest.is_directory()) {
                std::cout << fmt::format("{} not a directory\n", dest.name);
            }
            else
                currDir = dest;
        }

        else if (cmds[0] == "ls") {
            auto findArgsDir = [&cmds]() {
                vector<string> dirs;
                for (int i = 1; i < cmds.size(); ++i) {
                    if (cmds[i].starts_with("-"))
                        continue;
                    dirs.push_back(cmds[i]);
                }
                return dirs;
            };

            auto findParam = [&cmds]() {
                string params;
                for (int i = 1; i < cmds.size(); ++i) {
                    if (cmds[i].starts_with("-"))
                        params.append(cmds[i].substr(cmds[i].find('-') + 1));
                }
                return params;
            };

            auto dirs = findArgsDir();

            auto params = findParam();

            if (dirs.empty()) {
                auto subdirs = currDir.list(params);
                for (auto &p : subdirs) {
                    cout << p << endl;
                }
                goto out;
            }

            for (const auto &dir : dirs) {
                auto dest = currDir.find_dest_dir(dir);
                if (!dest.is_directory()) {
                    std::cout << fmt::format("{} is not a directory\n", dest.name);
                    continue;
                }
                cout << fmt::format("{}:\n", dest.name);
                auto subdirs = dest.list(params);
                for (auto &p : subdirs) {
                    cout << p << endl;
                }
                if (&dir - &dirs[0] != dirs.size() - 1)
                    cout << endl;
            }
        }

        else if (cmds[0] == "mkdir") {
            auto pos = cmds[1].rfind('/') + 1;
            auto filename = cmds[1].substr(pos);
            cmds[1].erase(pos);
            auto dest = currDir.find_dest_dir(cmds[1]);
            if (dest.inode_n == -1) {
                std::cout << fmt::format("No directory named {}\n", dest.name);
            } else {
                dest.create_dir(filename);
            }
        }

        else if (cmds[0] == "mkfile") {
            if (cmds.size() == 2)
                cmds.emplace_back();
            if (cmds.size() != 3) {
                std::cout << "Usage: mkfile <file> \"content\"\n";
                goto out;
            }
            auto pos = cmds[1].rfind('/') + 1;
            auto filename = cmds[1].substr(pos);
            cmds[1].erase(pos);
            auto dest = currDir.find_dest_dir(cmds[1]);
            if (dest.inode_n == -1) {
                std::cout << fmt::format("No directory named {}\n", dest.name);
            } else {
                dest.create_file(filename, cmds[2]);
            }
        }

        else if (cmds[0] == "cat") {
            if (cmds.size() != 2) {
                std::cout << "Usage: cat <file>\n";
                goto out;
            }
            auto dest = currDir.find_dest_dir(cmds[1]);
            if (dest.inode_n == -1) {
                std::cout << fmt::format("No directory named {}\n", dest.name);
            } else if (!dest.is_file()){
                std::cout << fmt::format("{} is not a file\n", dest.filename());
            } else
                cout << dest.get_content() << endl;
        }

        else if (cmds[0] == "rm") {
            if (cmds.size() != 2) {
                std::cout << "Usage: rm <dir>\n";
                goto out;
            }

            auto pos = cmds[1].rfind('/') + 1;
            auto filename = cmds[1].substr(pos);
            cmds[1].erase(pos);
            auto dest = currDir.find_dest_dir(cmds[1]);
            if (dest.inode_n == -1) {
                std::cout << fmt::format("No directory named {}\n", dest.name);
            } else {
                dest.remove_dir(filename);
            }
        }

        else if (cmds[0] == "exit") {
            break;
        }

        else if (cmds[0] == "cp") {
            if (cmds.size() != 3) {
                std::cout << "Usage: cp <dest> <src>\n";
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
                    std::cout << fmt::format("{} not found\n", src.name);
                    goto out;
                }
                if (!src.is_file()) {
                    std::cout << fmt::format("{} is not a file\n", src.name);
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
                    dest.create_file(filename, content);
                } else {
                    std::cout << fmt::format("{} not found\n", dest.name);
                    goto out;
                }
            } else {
                if (dest.is_file()) {
                    std::cout << fmt::format("{} already exits\n", dest.filename());
                    goto out;
                } else {
                    dest.create_file(filename, content);
                }
            }
        }

        else if (cmds[0] == "addusr") {

            if (userNotFull() && !userExists(cmds[1])) {
                cout << fmt::format("password: ");
                string password;
                cin >> password; getchar();
                for (uint16_t i = 0; i < USERNUM; ++i) {
                    if (users[i].password == 0) {
                        users[i].uid = i;
                        users[i].password = hash<string>{}(password);
                        strcpy(users[i].name, cmds[1].c_str());
                        break;
                    }
                }
            } else {
                std::cout << fmt::format("user already exists\n");
            }
        }

        else if (cmds[0] == "rmusr") {
            if (curr_uid != 0) {
                std::cout << fmt::format("only root user can delete a user\n");
            } else {
                for (auto &user : users) {
                    if (user.name == cmds[1]) {
                        user.uid = UINT16_MAX;
                        user.password = 0;
                        strcpy(user.name, "");
                    }
                }
            }
        }

        else if (cmds[0] == "su") {
            if (cmds.size() != 2) {
                std::cout << fmt::format("Usage: su <usrname>\n");
                goto out;
            }
            if (!userExists(cmds[1])) {
                std::cout << fmt::format("User {} does not exist\n", cmds[1]);
                goto out;
            }
            for (auto &user : users) {
                if (user.name == cmds[1]) {
                    cout << fmt::format("password: ");
                    string password;
                    while (cin >> password) {
                        getchar();
                        if (hash<string>{}(password) == user.password) {
                            strcpy(curr_username, cmds[1].data());
                            curr_uid = user.uid;
                            break;
                        } else {
                            std::cout << fmt::format("wrong password, please try again: ");
                        }
                    }
                }
            }
        }

        else if (cmds[0] == "info") {
            cout << fmt::format("simdisk by 韩希贤, 201930341045\n"
                                "用户:   {}/{}\n"
                                "inode: {}/{}\n"
                                "block: {}/{}\n",
                                userNum(), USERNUM,
                                inodeNum(), INODENUM,
                                blockNum(), BLOCKNUM);
        }

        else
            std::cout << fmt::format("Unknown command \"{}\"\n", cmds[0]);

        out:
        writeIntoDisk();
        cout << fmt::format("{}@simdisk:{}>", curr_username, currDir.name);
    }

    return 0;
}
