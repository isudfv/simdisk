//
// Created by yieatn on 2021/09/30.
//

#include <iostream>
#include <cstdlib>
#include "inode.h"
using namespace std;

int main() {
    cout << sizeof(inode) << endl;
    void *startPos = malloc(100<<20);
    cout << sizeof(dir_entry) << endl;
    return 0;
}