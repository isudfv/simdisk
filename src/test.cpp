//
// Created by yieatn on 2021/09/30.
//

#include <iostream>
#include <iomanip>
#include <ctime>
#include "inode.h"
//#include <fmt/core.h>
//#include <fmt/chrono.h>
#include "ipc.h"
using namespace std;

int main()
{
//    vector<string> strs{"123", "abc"};
//    cout << getIPCString(strs);
    cout << sizeof(inode) << endl << sizeof(dir_entry);
    cout << endl << INODENUM << endl;
}