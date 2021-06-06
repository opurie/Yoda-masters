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

#define ROOT 0

//Messages
#define REQ 10
#define ACK 11
#define RELEASE_X 12
#define RELEASE_Y 13
#define RELEASE_Z 14
#define JOINED 15
#define GROUP_ME 16
#define FULL 17
#define EMPTY 18

//percentage of each masters type, sum must be 1.0
#define X_MASTERS 0.35
#define Y_MASTERS 0.35
#define Z_MASTERS 0.3

#define MAX_ENERGY 50
#define MIN_ENERGY 0


struct Message{
    int sender;
    int timestamp;
    int type;
    int inQue;
};

typedef enum {X=100,Y,Z} masters;
#endif