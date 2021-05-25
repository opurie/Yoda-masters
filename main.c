#include "main.h"

void init(){
    //init number of Yodas
    countOfX = size*X_MASTERS;
    countOfY = size*Y_MASTERS;
    countOfZ = size*Z_MASTERS;
    int suma = countOfZ + countOfY + countOfX;
    if(suma < size){
        countOfZ += size - suma;}
    else if(suma > size){
        if((suma-size)%2==0){
            countOfX -= (suma-size)/2;
            countOfY -= (suma-size)/2;
        }else{
            countOfX -= (suma-size)/2;
            countOfY -= ((suma-size)/2 + 1);
        }
    }
    //assign master type
    if(rank < countOfX){
        master = X;
        queque = malloc(countOfX*sizeof(int));}
    else if(rank >= countOfX && rank < countOfX+countOfY){
        master = Y;
        queque = malloc(countOfY*sizeof(int));}
    else{
        master = Z;
        queque = malloc(countOfZ*sizeof(int));}
    //set hyperspace full
    hyperSpace = MAX_ENERGY;
    if(rank == ROOT)
        printf("X: %d, Y: %d, Z: %d\n",countOfX,countOfY,countOfZ);
    
}
void X_stuff(){
    
}
int main(int argc, char **argv){
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    init();

    if(master == X)
        X_stuff();
    else if(master == Y)
        Y_stuff();
    else 
        Z_stuff();

    MPI_Finalize();
    return 0;
}