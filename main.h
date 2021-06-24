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
int countReqs = 1;
pthread_mutex_t hsMutex;//     }
int hyperSpace;
int sended_ts;
int groupedProcess_id;//for X and Y

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

//ma zapisany aktualny stan procesu X
State state;
void changeState(State s);

//inicjacja customowych wiadomości
void initCustomMessage();

//zwraca pozycje w kolejce(narazie dla X)
int queuePlace(masters master, int *queue, int *inQue);

/*k - miejsce w kolejce Y, xtab - tablica z numerami kolejek Xsów*/
int findX(int k, int *xtab);
void updatextab(int k, int *xtab);

/*Pomocnicza funkcja dla Y, 
zwraca 2 jeśli wysłało EMPTY, 
1 jeśli tylko pobrało energię, 
0 jeśli zgrupowało się lub nic nie zrobiło*/
int farmingY(int k, int* queue, int *inQue, int* xtab);

//zarządzają procesami
void runningX();
void runningY();
void runningZ();

//tpyowy main
int main(int argc, char **argv);
#endif
