#include "inode.h"
#include "path.h"
#include "users.h"
#include <chrono>
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <string>
#include "ipc.h"
#include <string_view>
#include <sys/shm.h>
#include <sys/types.h>
#include "unistd.h"
#include <thread>
using namespace std;
namespace fs = std::filesystem;

//std::mutex locker;



int main() {
//    startOver();
//    return 0;


    //to create an area of shared memory that takes users for logging in
    if ((shmid_out = shmget(key_out, (1<<20), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    if ((shm_out = shmat(shmid_out, nullptr, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    if ((shmid_in = shmget(key_in, (1<<20), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    if ((shm_in = shmat(shmid_in, nullptr, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }
    memset(shm_out, 0, (1<<20));
    memset(shm_in, 0, (1<<20));
//    *(char *)shm_in = 0;
//    *(char *)shm_out = 0;

    key_t shms[2];
//fork a process to deal with a new user
while (inFromSHM(shms, shm_in, sizeof(shms))) {
    cout << "received\n";
    pid_t pid = fork();
    if (pid != 0) continue;
//    if (fork() == 0) continue;
    cout << "into child process\n";
    cout << shms[0] << endl << shms[1] << endl;
    //create shared memory to transmitting commands and output
    if ((shmid_out = shmget(shms[0], (1 << 20), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    if ((shm_out = shmat(shmid_out, nullptr, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    if ((shmid_in = shmget(shms[1], (1 << 20), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    if ((shm_in = shmat(shmid_in, nullptr, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    //to read inode bitmap user from simdisk to memory
    DISK.seekg(INDEXNODE);
    DISK.read((char *) &inodes, sizeof(inodes));

    DISK.seekg(FREEBLOCKS);
    DISK.read((char *) bitmap, sizeof(bitmap));

    DISK.seekg(USERS);
    DISK.read((char *) users, sizeof(users));

    //    char curr_username[24];
    //    outToSHM(fmt::format("Log in as: "), shm_out);
    //to log in
    outToSHM("Log in as: ", shm_out);
    allout(shm_out);
    while (inFromSHM(curr_username, shm_in)) {
        preparing(shm_out);
        for (auto &user : users) {
            if (user.password == 0) continue;
            if (strcmp(curr_username, user.name) == 0) {
                outToSHM("password: ", shm_out);
                //                outToSHM(fmt::format("password: "), shm_out);
                string password;
                allout(shm_out);
                while (inFromSHM(password, shm_in)) {
                    preparing(shm_out);
                    if (hash<string>{}(password) == user.password) {
                        curr_uid = user.uid;
                        goto start;
                    } else {
                        //                        outToSHM(fmt::format("wrong password, please try again: "), shm_out);
                        outToSHM("wrong password, please try again: ", shm_out);
                    }
                    allout(shm_out);
                }
            }
        }
        outToSHM(fmt::format("user not found, please try again: "), shm_out);
        allout(shm_out);
    }

    start:
    //    cin.ignore(std::numeric_limits< streamsize >::max(), '\n');

    path currDir;
    string cmdline;
    //to write changes back to simdisk
    auto writeIntoDisk = []() {
        DISK.seekp(INDEXNODE);
        DISK.write((char *) &inodes, sizeof(inodes));

        DISK.seekp(FREEBLOCKS);
        DISK.write((char *) bitmap, sizeof(bitmap));

        DISK.seekp(USERS);
        DISK.write((char *) users, sizeof(users));
    };


    //take in commands
    outToSHM(fmt::format("{}@simdisk:{}>", curr_username, currDir.name), shm_out);
    allout(shm_out);
    while (inFromSHM(cmdline, shm_in)) {
        preparing(shm_out);
        auto cmds = split(cmdline);
        if (cmds.empty()) {
            outToSHM(fmt::format("\n"), shm_out);
            goto out;
            continue;
        }

        if (cmds[0] == "cd") {
            if (cmds.size() == 1)
                cmds.emplace_back("/");
            auto dest = currDir.find_dest_dir(cmds[1]);
            if (dest.inode_n == -1) {
                outToSHM(fmt::format("No directory named {}\n", dest.name), shm_out);
            } else if (!dest.is_directory()) {
                outToSHM(fmt::format("{} not a directory\n", dest.name), shm_out);
            } else
                currDir = dest;
        }

        else if (cmds[0] == "ls") {
            //to find destination directory
            auto findArgsDir = [&cmds]() {
                vector<string> dirs;
                for (int i = 1; i < cmds.size(); ++i) {
                    if (cmds[i].starts_with("-"))
                        continue;
                    dirs.push_back(cmds[i]);
                }
                return dirs;
            };
            //to get parameters of ls command
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
            //if dir is empty, then dest directory is current directory
            if (dirs.empty()) {
                auto subdirs = currDir.list(params);
                /*for (auto &p : subdirs) {
                outToSHM(p << endl, shm_out);
            }*/
                outToSHM(subdirs, shm_out);
                goto out;
            }

            //iterate each directory
            for (const auto &dir : dirs) {
                auto dest = currDir.find_dest_dir(dir);
                if (dest.inode_n == -1) {
                    outToSHM(fmt::format("No directory named {}\n", dest.name), shm_out);
                    continue;
                }
                //dest must be a directory
                if (!dest.is_directory()) {
                    outToSHM(fmt::format("{} is not a directory\n", dest.name), shm_out);
                    continue;
                }
                outToSHM(fmt::format("{}:\n", dest.name), shm_out);
                auto subdirs = dest.list(params);
                /*for (auto &p : subdirs) {
                outToSHM(p << endl, shm_out);
            }
            if (&dir - &dirs[0] != dirs.size() - 1)
                outToSHM(endl, shm_out);*/
                outToSHM(subdirs, shm_out);
                if (&dir != &dirs.back())
                    outToSHM("\n", shm_out);
            }
        }

        else if (cmds[0] == "mkdir") {
            auto pos = cmds[1].rfind('/') + 1;
            auto filename = cmds[1].substr(pos);
            cmds[1].erase(pos);
            auto dest = currDir.find_dest_dir(cmds[1]);
            if (dest.inode_n == -1) {
                outToSHM(fmt::format("No directory named {}\n", dest.name), shm_out);
            } else {
                dest.create_dir(filename);
            }
        }

        else if (cmds[0] == "mkfile") {
            if (cmds.size() == 2)
                cmds.emplace_back();
            if (cmds.size() != 3) {
                outToSHM("Usage: mkfile <file> \"content\"\n", shm_out);
                goto out;
            }
            //to separate directory from filename
            auto pos = cmds[1].rfind('/') + 1;
            auto filename = cmds[1].substr(pos);
            cmds[1].erase(pos);
            auto dest = currDir.find_dest_dir(cmds[1]);
            if (dest.inode_n == -1) {
                outToSHM(fmt::format("No directory named {}\n", dest.name), shm_out);
            } else {
                dest.create_file(filename, cmds[2]);
            }
        }

        else if (cmds[0] == "cat") {
            if (cmds.size() != 2) {
                outToSHM("Usage: cat <file>\n", shm_out);
                goto out;
            }
            auto dest = currDir.find_dest_dir(cmds[1]);
            if (dest.inode_n == -1) {
                outToSHM(fmt::format("No directory named {}\n", dest.name), shm_out);
            } else if (!dest.is_file()) {
                outToSHM(fmt::format("{} is not a file\n", dest.filename()), shm_out);
            } else
                outToSHM(fmt::format("{}\n", dest.get_content()), shm_out);
        }
        //to append a string to dest file
        else if (cmds[0] == "app") {
            if (cmds.size() != 2) {
                outToSHM("Usage: app <file> \"content\"\n", shm_out);
                goto out;
            }
            auto dest = currDir.find_dest_dir(cmds[1]);
            if (dest.inode_n == -1) {
                outToSHM(fmt::format("No directory named {}\n", dest.name), shm_out);
            } else if (!dest.is_file()) {
                outToSHM(fmt::format("{} is not a file\n", dest.filename()), shm_out);
            } else {
                std::string content;
                outToSHM("content: ", shm_out);
                allout(shm_out);
                inodes[dest.inode_n].i_mode |= IS_WRITTING;
                writeIntoDisk();
                inFromSHM(content, shm_in);
                preparing(shm_out);
                dest.apppend(content);
                inodes[dest.inode_n].i_mode &= (~IS_WRITTING);
            }
        }

        else if (cmds[0] == "rm") {
            if (cmds.size() != 2) {
                outToSHM("Usage: rm <dir>\n", shm_out);
                goto out;
            }

            auto pos = cmds[1].rfind('/') + 1;
            auto filename = cmds[1].substr(pos);
            cmds[1].erase(pos);
            auto dest = currDir.find_dest_dir(cmds[1]);
            if (dest.inode_n == -1) {
                outToSHM(fmt::format("No directory named {}\n", dest.name), shm_out);
            } else {
                dest.remove_dir(filename);
            }
        }

        else if (cmds[0] == "exit") {
            break;
        }

        else if (cmds[0] == "cp") {
            if (cmds.size() != 3) {
                outToSHM("Usage: cp <dest> <src>\n", shm_out);
                goto out;
            }
            string content;
            string filename;
            //from host
            if (cmds[1].starts_with("@PC:")) {
                fs::path src(cmds[1].substr(4));
                ifstream in(src);
                stringstream ss;
                ss << in.rdbuf();
                content = ss.str();
                filename = src.filename().string();
            } else {
                //from simdisk
                auto src = currDir.find_dest_dir(cmds[1]);
                if (src.inode_n == -1) {
                    outToSHM(fmt::format("{} not found\n", src.name), shm_out);
                    goto out;
                }
                if (!src.is_file()) {
                    outToSHM(fmt::format("{} is not a file\n", src.name), shm_out);
                    goto out;
                }
                content = src.get_content();
                filename = src.filename();
            }

            //where to put the file
            auto dest = currDir.find_dest_dir(cmds[2]);
            if (dest.inode_n == -1) {// if the last directory is filename
                if (cmds[2].ends_with(dest.name)) {
                    filename = dest.name;
                    dest = currDir.find_dest_dir(cmds[2].substr(0, cmds[2].rfind('/') + 1));
                    dest.create_file(filename, content);
                } else {
                    outToSHM(fmt::format("{} not found\n", dest.name), shm_out);
                    goto out;
                }
            } else {
                if (dest.is_file()) {
                    outToSHM(fmt::format("{} already exits\n", dest.filename()), shm_out);
                    goto out;
                } else {
                    dest.create_file(filename, content);
                }
            }
        }

        else if (cmds[0] == "addusr") {

            if (cmds.size() != 2) {
                outToSHM("Usage: addusr <usrname>\n", shm_out);
                goto out;
            }

            if (userNotFull() && !userExists(cmds[1])) {
                outToSHM(fmt::format("password: "), shm_out);
                string password;
                //                cin >> password; getchar();
                allout(shm_out);
                inFromSHM(password, shm_in);
                preparing(shm_out);
                for (uint16_t i = 0; i < USERNUM; ++i) {
                    if (users[i].password == 0) {
                        users[i].uid = i;
                        //hash user password
                        users[i].password = hash<string>{}(password);
                        strcpy(users[i].name, cmds[1].c_str());
                        break;
                    }
                }
            } else {
                outToSHM(fmt::format("user already exists\n"), shm_out);
            }
        }

        else if (cmds[0] == "rmusr") {
            if (curr_uid != 0) {
                outToSHM(fmt::format("only root user can delete a user\n"), shm_out);
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
                outToSHM(fmt::format("Usage: su <usrname>\n"), shm_out);
                goto out;
            }
            if (!userExists(cmds[1])) {
                outToSHM(fmt::format("User {} does not exist\n", cmds[1]), shm_out);
                goto out;
            }
            //switch to another user
            for (auto &user : users) {
                if (user.name == cmds[1]) {
                    outToSHM(fmt::format("password: "), shm_out);
                    string password;
                    allout(shm_out);
                    while (inFromSHM(password, shm_in)) {
                        preparing(shm_out);
                        //                        getchar();
                        if (hash<string>{}(password) == user.password) {
                            strcpy(curr_username, cmds[1].data());
                            curr_uid = user.uid;
                            break;
                        } else {
                            outToSHM(fmt::format("wrong password, please try again: "), shm_out);
                            allout(shm_out);
                        }
                    }
                }
            }
        }

        else if (cmds[0] == "info") {
            //format output
            outToSHM(fmt::format("simdisk by 韩希贤, 201930341045\n"
                                 "用户:   {}/{}\n"
                                 "inode: {}/{}\n"
                                 "block: {}/{}\n",
                                 userNum(), USERNUM,
                                 inodeNum(), INODENUM,
                                 blockNum(), BLOCKNUM),
                     shm_out);
        }

        else
            outToSHM(fmt::format("Unknown command \"{}\"\n", cmds[0]), shm_out);

        out:
        writeIntoDisk();
        outToSHM(fmt::format("{}@simdisk:{}>", curr_username, currDir.name), shm_out);
        allout(shm_out);
    }
    //to free sharen memory
    shmctl(shmid_out, IPC_RMID, nullptr);
    return 0;
}
}
