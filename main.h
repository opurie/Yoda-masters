#ifndef MAIN_H
#define MAIN_H

#include "consts.h"
//MPIowe typy
int size, id;
MPI_Datatype mpi_message_type;

//liczba procesów poszczegolnej grupy
int countOfX, countOfY, countOfZ;

masters master;
pthread_mutex_t tsMutex;// }
int timestamp=0;//         }
pthread_mutex_t hsMutex;//     }
int hyperSpace;
int groupedProcess_id;//for X and Y
int sendEmptys, sendFulls;
//Initialize numbers of masters, set hyperspace full and assign master type to process
void init();
void incrementTimestamp(int income);
void sendMessage(int receiver, int type, int in);
void sendToGroup(int messageType, masters master, int n);

struct Message receiveMessage();

typedef enum{queueing, readyToFarm, farming, chilling,
             waitingForX,
             waitingForY, 
             waitingForZ} State;

State state;
void changeState(State s);
void initCustomMessage();
int queuePlace(masters master, int *queue, int *inQue);
int findX(int k, int *xtab);
char farmingY(int k, int* queue, int *inQue, int* xtab);
void updateXtab(int i, int* xtab);
void goInHyperSpaceY(int k);
void goInHyperSpaceZ(int k);
//zarządzają procesami
void runningX();
void runningY();
void runningZ();

//tpyowy main
int main(int argc, char **argv);
#endif
