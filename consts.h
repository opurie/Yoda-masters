#ifndef CONSTSH
#define CONSTSH

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stddef.h>
#include <time.h>
#include <string.h>


//Messages
#define REQ 10
#define ACK 11
#define RELEASE_X 12
#define RELEASE_Y 13
#define RELEASE_Z 14
#define GROUP_ME 15
#define JOINED 16
#define FULL 17
#define EMPTY 18

//percentage of each masters type, sum must be 1.0
#define X_MASTERS 0.35
#define Y_MASTERS 0.35
#define Z_MASTERS 0.3

#define MAX_ENERGY 20
#define TIME_IN 1

struct Message{
    //nadawca
    int sender;
    //timestamp
    int timestamp;
    //typ wiadomości, linijki 16-24
    int type;
    int inQue;
};
//każdy proces ma przydzieloną jedną wartość
typedef enum {X=100,Y,Z} masters;
#endif
