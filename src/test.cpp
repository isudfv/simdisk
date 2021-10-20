//
// Created by yieatn on 2021/09/30.
//

#include <iostream>
#include <iomanip>
#include <ctime>
#include "inode.h"
#include <fmt/core.h>
#include <fmt/chrono.h>
using namespace std;

int main()
{
//    std::locale::global(std::locale("zh_CN.utf8"));
    std::time_t t = std::time(nullptr);
    stringstream fmtedTime;
    fmtedTime << put_time(localtime(&t), "%a %d %H:%M");
    cout << fmtedTime.str();
    cout << endl <<  sizeof(dir_entry) << endl;
//    cout << fmt::format("{}", put_time(localtime(&t), "%a %d %H:%S"));
}