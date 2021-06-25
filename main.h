#ifndef MAIN_H
#define MAIN_H

#include "consts.h"
//MPIowe typy
int size, id;
MPI_Datatype mpi_message_type;

//liczba procesów poszczegolnej grupy
int countOfX, countOfY, countOfZ;

//zapisany aktualny stan procesu X
State state;

masters master;
int timestamp = 0;
int countReqs = 0;

int hyperSpace;
int groupedProcess_id;//for X and Y

//Initialize numbers of masters, set hyperspace full and assign master type to process
void init();
void initCustomMessage();
void incrementTimestamp(int income);
void sendMessage(int receiver, int type, int in);
void sendToGroup(int messageType, masters master, int n);
struct Message receiveMessage();

void changeState(State s);

/*k - miejsce w kolejce Y, xtab - tablica z numerami kolejek Xsów*/
int findX(int k, int *xtab);

/*Pomocnicza funkcja dla Y, 
zwraca 2 jeśli wysłało EMPTY, 
1 jeśli tylko pobrało energię, 
0 jeśli zgrupowało się lub nic nie zrobiło*/
int farmingY(int k, int* queue, int* xtab);

//zarządzają procesami
void runningX();
void runningY();
void runningZ();

//tpyowy main
int main(int argc, char **argv);
#endif
