/*
* shm-shell - shell program to demonstrate shared memory.
*/
#include <cstring>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include "ipc.h"


#define SHMSZ     27
using namespace std;
int main()
{
    int shmid_out, shmid_in;
    key_t key_out = 41046, key_in = 41045;
    void *shm_out, *shm_in;

   // to connect to simdisk
    sleep(1);
    if ((shmid_out = shmget(key_out, (1<<20), 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    if ((shm_out = shmat(shmid_out, nullptr, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    if ((shmid_in = shmget(key_in, (1<<20), 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    if ((shm_in = shmat(shmid_in, nullptr, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

   /*
    * Now read what the server put in the memory.
    */

   string str, cmd;
   while (inFromSHM(str, shm_in)) {
       cout << str;
//       cin >> cmd;
       getline(cin, cmd);
       preparing(shm_out);
       outToSHM(cmd.c_str(), shm_out);
       allout(shm_out);
       if (cmd == "exit")
           break;
   }
//   *(char *)shm_out = '*';

   exit(0);
}
