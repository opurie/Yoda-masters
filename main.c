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
    if(id < countOfX){
        master = X;}
    else if(id >= countOfX && id < countOfX+countOfY){
        master = Y;}
    else{
        master = Z;}
    //set hyperspace full
    hyperSpace = MAX_ENERGY;    
}
void initCustomMessage(){
    int block[4] = {1,1,1,1};
    MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Aint offsets[4];
    offsets[0] = offsetof(struct Message, sender);
    offsets[1] = offsetof(struct Message, timestamp);
    offsets[2] = offsetof(struct Message, type);
    offsets[3] = offsetof(struct Message, inQue);
    MPI_Type_create_struct(4, block, offsets, types, &mpi_message_type);
    MPI_Type_commit(&mpi_message_type);
}
void sendMessage(int receiver, int type, int in){
    struct Message message;
    message.sender= id;
    message.timestamp = timestamp;
    message.type = type;
    message.inQue = in;
    MPI_Send(&message, 1, mpi_message_type, receiver, 100, MPI_COMM_WORLD);
}
struct Message receiveMessage(){
    struct Message message;
    MPI_Status status;
    MPI_Recv(&message, 1, mpi_message_type, MPI_ANY_SOURCE, 100, MPI_COMM_WORLD, &status);
    return message;
}
void incrementTimestamp(int income){
    if(income > timestamp)
        timestamp = income+1;
    else
        timestamp++;
}
void sendToGroup(int messageType, masters master, int n){
    if(master == X){
        for(int i = 0; i<countOfX;i++){
            if(id == i)
                continue;
            sendMessage(i, messageType, n);
        }
    }else if(master == Y){
        for(int i = 0; i < countOfY; i++){
            if(id == i+countOfX)
                continue;
            sendMessage(i+countOfX, messageType, n);
        }
    }else if(master == Z){
        for(int i = 0; i < countOfZ; i++){
            if(id == i+countOfX+countOfY)
                continue;
            sendMessage(i+countOfX+countOfY, messageType, n);
        }
    }
}
int queuePlace(int acks, masters master, int *queue, int *inQue){
    int zeros = 0, k = 1;
    if(master == X){
        if(acks == countOfX -1){
            state = waitingForY;
            for(int i = 0; i<countOfX; i++){
                if(i == id)
                    continue;
                if(queue[i] < sended_ts)
                    k++;
                else if(queue[i] == sended_ts && id < i)
                    k++;
                else if(queue[i]>sended_ts && inQue[i]==1)
                    k++;
            }
        }else 
            k = 0;
    }
    return k;
}
/*void *listeningX(){
    struct Message message;
    int k;
    while(1){
        message = receiveMessage();
        if(message.type == REQ){
            pthread_mutex_lock(&tsMutex);
            incrementTimestamp(message.timestamp);
            queue[message.sender] = message.timestamp;
            inQue[message.sender] = 1;
            sendMessage(message.sender, ACK);
            pthread_mutex_unlock(&tsMutex);
        }else if(message.type == ACK){
            if(message.timestamp>sended_ts)
                receivedACKs++;
                k = queuePlace(receivedACKs, X);
                if(k==0) continue;
                if(k <= countOfY){
                    pthread_mutex_lock(&tsMutex);
                    incrementTimestamp(message.timestamp);
                    state = waitingForY;
                    sendToGroup(GROUP_ME, Y);
                    pthread_mutex_unlock(&tsMutex);
                }
        }else if(message.type == RELEASE_X){
            inQue[message.sender]=0;
        }else if(message.type == JOINED){
            state = farming;
        }else if(message.type == RELEASE_Y){
            state = queueing;
        }
    }
}*/
void runningX(){
    int *queue= malloc(countOfX * sizeof(int));
    char *inQue= malloc(countOfX * sizeof(char)); 
    memset(inQue, 0, countOfX);
    memset(queue, 0, countOfX);
    struct Message message;
    state = queueing;
    int k=0, sendedToY=0;
    //pthread_t thread;
    //pthread_create(&thread, NULL, listeningX, NULL);
start:
    incrementTimestamp(0);
    sended_ts = timestamp;
    sendToGroup(REQ, X, 0);
    receivedACKs = 0;
    queue[id]=sended_ts;
    state = waitingForXs;
    
    while(1){
        message = receiveMessage();
        //Jeśli nie jesteś zakolejkowany inQue=0, jeśli jesteś inQue=1
        if(id==0)
        printf("%d <- %d, %d\n", id, message.sender, message.type);
        if(message.type == REQ){
            incrementTimestamp(message.timestamp);
            queue[message.sender] = message.timestamp;
            if(state == waitingForY || state == farming)
                sendMessage(message.sender, ACK, 1);
            else
                sendMessage(message.sender, ACK, 0);
                if(id==0)
            printf("%d -> %d, ACK\n", id, message.sender);
        }else if(message.type == ACK){
            if(message.timestamp>sended_ts)
                receivedACKs++;
            inQue[message.sender] = message.inQue;
            incrementTimestamp(message.timestamp);
            if(receivedACKs==countOfX-1)
                state = waitingForY;
checkpoint:
            if(receivedACKs==countOfX-1)
                k = queuePlace(receivedACKs, X, queue, inQue);
            if(k>0 && k <= countOfY && sendedToY==0){
                incrementTimestamp(message.timestamp);
                printf("Jestem %d, numer w kolejce %d, ts %d\n",id,k, timestamp);
                sendToGroup(GROUP_ME, Y, k);
                sendedToY=1;
            }
        }else if(message.type == RELEASE_X){
            inQue[message.sender]=0;
            incrementTimestamp(message.timestamp);
            goto checkpoint;
        }else if(message.type == JOINED){
            incrementTimestamp(message.timestamp);
            state = farming;
            groupedProcess_id = message.sender;
        }else if(message.type == RELEASE_Y){
            state = queueing;
            goto start;
        }
    }

}
int main(int argc, char **argv){
    MPI_Init(&argc, &argv);
    initCustomMessage();
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    init();
    if(master == X)
        runningX();

    MPI_Finalize();
    return 0;
}