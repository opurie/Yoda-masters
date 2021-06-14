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
//k = {1..countOf(XYZ)}
int queuePlace(masters master, int *queue, int *inQue){
    int k = 1;
    int ys = countOfX, zs = countOfX+countOfY;
    if(master == X){
        state = waitingForY;
        for(int i = 0; i<countOfX; i++){
            if(i == id) continue;
            if(queue[i] < sended_ts)
                k++;
            else if(queue[i] == sended_ts && i < id)
                k++;
            else if(queue[i] >= sended_ts && inQue[i]==1)
                k++;
        }
    }
    if(master == Y){
        for(int i = 0; i<countOfY; i++){
            if(id - ys == i) continue;
            if(queue[i] < sended_ts)
                k++;
            else if(queue[i] == sended_ts && i < id - ys )
                k++;
            else if(queue[i] >= sended_ts && inQue[i]==1)
                k++;
        }
    }
    if(master == Z){
        for(int i = 0; i<countOfZ;i++){
            if(id - zs == i) continue;
            if(queue[i] < sended_ts)
                k++;
            else if(queue[i] == sended_ts && i < id-zs)
                k++;
            else if(queue[i] >= sended_ts && inQue[i]==1)
                k++;
        }
    }
    return k;
}
void runningX(){
    int *queue= malloc(countOfX * sizeof(int));
    char *inQue= malloc(countOfX * sizeof(char)); 
    memset(inQue, 0, countOfX);
    memset(queue, 0, countOfX);
    struct Message message;
    state = queueing;

    //początek, proces rozsyła żądanie do Xs aby otrzymać Y
start:
    incrementTimestamp(0);
    sended_ts = timestamp;
    sendToGroup(REQ, X, 0);
    receivedACKs = 0;
    queue[id]=sended_ts;
    state = queueing;
    groupedProcess_id = -1;
    int k=0, sendedToY=0;
    //pętla zarządzająca odbiorem wiadomości
    while(1){
        message = receiveMessage();
        incrementTimestamp(message.timestamp);
        if(message.type == REQ){
            queue[message.sender] = message.timestamp;
            if(state == waitingForY || state == farming)
                sendMessage(message.sender, ACK, 1);
            else
                sendMessage(message.sender, ACK, 0);
        }else if(message.type == ACK){
            if(message.timestamp > sended_ts)
                receivedACKs++;
            inQue[message.sender] = message.inQue;
            if(receivedACKs==countOfX-1){
                k = queuePlace(X, queue, inQue);}
            if(k>0 && k <= countOfY && sendedToY==0){
                incrementTimestamp(0);
                sendToGroup(GROUP_ME, Y, k);
                state = waitingForY;
                sendedToY=1;
            }
        }else if(message.type == RELEASE_X){
            inQue[message.sender] = 0;
            if(receivedACKs == countOfX - 1)
                k = queuePlace(X, queue, inQue);
            if(k>0 && k <= countOfY && sendedToY==0){
                incrementTimestamp(message.timestamp);
                sendToGroup(GROUP_ME, Y, k);
                state = waitingForY;
                sendedToY=1;
            }
        }else if(message.type == JOINED){
            state = farming;
            if(groupedProcess_id > 0)
                printf("[ERROR X - %d] grouped - %d, want to group - %d\n", id, groupedProcess_id, message.sender);
            groupedProcess_id = message.sender;
        }else if(message.type == RELEASE_Y){
            incrementTimestamp(0);
            sendToGroup(RELEASE_X, X, 0);
            goto start;
        }
    }

}
int findX(int k, int *xtab){
    for(int i=0;i<countOfX;i++){
        if(k == xtab[i])
            return i;
    }
    return -1;
}
void updatextab(int k, int *xtab){
    int j = xtab[k];
    for(int i=0;i<countOfX;i++){
        if(xtab[i] > j) xtab[i]--;
    }
}
char farmingY(int k, int* queue, int *inQue, int* xtab){
    /*if(state == waitingForX){
        groupedProcess_id = findX(k, xtab);
        if(groupedProcess_id>=0){
            incrementTimestamp(0);
            sendMessage(groupedProcess_id, JOINED, 0);
            printf("[Y - %d] readyToFarm, x - %d, k - %d\n", id, groupedProcess_id, k);
            state = readyToFarm;
        }
    }*/
    groupedProcess_id = findX(k, xtab);
    if(state == waitingForX && k <= hyperSpace && groupedProcess_id != -1){
        incrementTimestamp(0);
        sendMessage(groupedProcess_id, JOINED, 0);

        printf("[Y - %d] farming, x - %d, hyperspace - %d\n", id, groupedProcess_id, hyperSpace);
        state = farming;
        hyperSpace--;
        incrementTimestamp(0);
        sendMessage(groupedProcess_id, RELEASE_Y, 0);
        sendToGroup(RELEASE_Y, Y, groupedProcess_id);
        if(hyperSpace - k == 0){
            incrementTimestamp(0);
            printf("[Y - %d] =======EMPTY========\n",id);
            sleep(1);
            sendToGroup(EMPTY, Z, 0);
        }
        return 1;
    }
    return 0;
}

void runningY(){
    int *queue= malloc(countOfY * sizeof(int));
    int *xtab = malloc(countOfX * sizeof(int));
    char *inQue= malloc(countOfY * sizeof(char)); 
    memset(inQue, 0, countOfY);
    memset(xtab,-1,countOfX);
    memset(queue, 0, countOfY);

    struct Message message;
    int k=0, sendedToX=0;
    
    int ys = countOfX;
start:
    printf("[Y - %d] queueing\n", id);
    state = queueing;
    incrementTimestamp(0);
    sended_ts = timestamp;
    sendToGroup(REQ, Y, 0);
    receivedACKs = 0;
    queue[id-ys]=sended_ts;
    k=0;
    while(1){
        message = receiveMessage();
        incrementTimestamp(message.timestamp);
        if(message.type == REQ){
            queue[message.sender - ys] = message.timestamp;
            if(state == readyToFarm || state == farming || state == waitingForX)
                sendMessage(message.sender, ACK, 1);
            else
                sendMessage(message.sender, ACK, 0);
        }else if(message.type == ACK){
            if(message.timestamp > sended_ts)
                receivedACKs++;
            inQue[message.sender - ys] = message.inQue;
            if(receivedACKs == countOfY-1){   
                state = waitingForX;
                k = queuePlace(Y, queue, inQue);
                if(farmingY(k, queue, inQue, xtab)==1){
                    state = queueing;
                    goto start;
                }
            }
            
        }else if(message.type == GROUP_ME){
            xtab[message.sender] = message.inQue;
            if(receivedACKs == countOfY-1)
                k = queuePlace(Y, queue, inQue);
            if(farmingY(k, queue, inQue, xtab)==1){
                state = queueing;
                goto start;
            }

        }else if(message.type == RELEASE_Y){
            inQue[message.sender-ys]=0;
            updatextab(message.inQue, xtab);
            if(receivedACKs == countOfY-1)
                k = queuePlace(Y, queue, inQue);
            if(hyperSpace>0)
                hyperSpace -= 1;
            if(farmingY(k, queue, inQue, xtab)==1){
                state = queueing;
                goto start;
            }
            
        }else if(message.type == EMPTY){
        }else if(message.type == FULL){
            hyperSpace = MAX_ENERGY;
            if(receivedACKs == countOfY-1)
                k = queuePlace(Y, queue, inQue);
            if(farmingY(k, queue, inQue, xtab)==1){
                state = queueing;
                goto start;
            }
        }
    }
}
void runningZ(){
    int *queue= malloc(countOfZ * sizeof(int));
    char *inQue= malloc(countOfZ * sizeof(char)); 
    memset(inQue, 0, countOfZ);
    memset(queue, 0, countOfZ);
    struct Message message;
    state = chilling;
    int k=0;
    int zs = countOfY+countOfX;
    goto secondStart;
    //początek, proces rozsyła żądanie do Xs aby otrzymać Y
start:
    printf("\t\t\t\t\t\t[Z - %d] queueing\n", id);
    incrementTimestamp(0);
    sended_ts = timestamp;
    sendToGroup(REQ, Z, 0);
    receivedACKs = 0;
    queue[id - zs]=sended_ts;
    state = waitingForZ;
secondStart:
    k=0;
    //pętla zarządzająca odbiorem wiadomości
    while(1){
        message = receiveMessage();
        incrementTimestamp(message.timestamp);
        //Jeśli nie jesteś zakolejkowany inQue=0, jeśli jesteś inQue=1
        if(message.type == REQ){
            queue[message.sender - zs] = message.timestamp;
            if(state == readyToFarm || state == farming){
                sendMessage(message.sender, ACK, 1);
            }else{
                sendMessage(message.sender, ACK, 0);
            }
        }else if(message.type == ACK){
            if(message.timestamp > sended_ts)
                receivedACKs++;
            inQue[message.sender - zs] = message.inQue;
            if(receivedACKs == countOfZ - 1){
                k = queuePlace(Z, queue, inQue);
                state = readyToFarm;
            }
            if(k>0 && k + hyperSpace <= MAX_ENERGY){
                printf("\t\t\t\t\t\t[Z - %d] farming, hyperspace - %d\n", id, hyperSpace);
               state = farming;
               incrementTimestamp(0);
               hyperSpace++;
               sendToGroup(RELEASE_Z, Z, 0); 
               if(hyperSpace + k == MAX_ENERGY - 1){
                   sleep(1);
                    sendToGroup(FULL, Z, 0);
                    sendToGroup(FULL, Y, 0);
                    memset(inQue, 0, countOfZ);
                    memset(queue, 0, countOfZ);
                    state = chilling;
                    hyperSpace = MAX_ENERGY;
                    goto secondStart;
                }
               goto start;
            }
        }else if(message.type == RELEASE_Z){
            hyperSpace++;
            inQue[message.sender - zs] = 0;
            if(receivedACKs == countOfZ - 1)
                k = queuePlace(Z, queue, inQue);
            if(k > 0 && k + hyperSpace <= MAX_ENERGY){
                state = farming;
                printf("\t\t\t\t\t\t[Z - %d] farming, hyperspace - %d\n", id, hyperSpace);
                incrementTimestamp(0);
                hyperSpace++;
                sendToGroup(RELEASE_Z, Z, 0);
                if(hyperSpace + k == MAX_ENERGY - 1){
                    sleep(1);
                    sendToGroup(FULL, Z, 0);
                    sendToGroup(FULL, Y, 0);
                    memset(inQue, 0, countOfZ);
                    memset(queue, 0, countOfZ);
                    state = chilling;
                    hyperSpace = MAX_ENERGY;
                    goto secondStart;
                }
                goto start; 
            }

        }else if(message.type == FULL){
            printf("\t\t\t\t\t\t[Z - %d] chilling\n", id);
            state = chilling;
            goto secondStart;
        }else if(message.type == EMPTY){
            state = queueing;
            hyperSpace = 0;
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
    if(master == Y)
        runningY();
    if(master == Z)
        runningZ();

    MPI_Finalize();
    return 0;
}