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
        for(int i = 0; i < countOfX; i++){
            if(i == id) continue;
            if(queue[i] < queue[id] && inQue[i] == 1)
                k++;
            else if(queue[i] == queue[id] && i < id && inQue[i] == 1)
                k++;
        }
    }
    else if(master == Y){
        for(int i = 0; i < countOfY; i++){
            if(id - ys == i) continue;
            if(queue[i] < queue[id - ys] && inQue[i] == 1)
                k++;
            else if(queue[i] == queue[id - ys] && i < id - ys && inQue[i] == 1)
                k++;
        }
    }
    else if(master == Z){
        for(int i = 0; i<countOfZ;i++){
            if(id - zs == i) continue;
            if(queue[i] < queue[id - zs] && inQue[i]==1)
                k++;
            else if(queue[i] == queue[id - zs] && i < id-zs && inQue[i]==1)
                k++;
        }
    }
    return k;
}
void changeState(State s){
    state = s;
}
void runningX(){
    int *queue= malloc(countOfX * sizeof(int));
    char *inQue= malloc(countOfX * sizeof(char)); 
    memset(inQue, 1, countOfX);
    memset(queue, 0, countOfX);
    struct Message message;
    int receivedACKs, k, minimum;
    if(countOfY > countOfX)
        minimum = countOfX;
    else
        minimum = countOfY; 
start:
    sendToGroup(REQ, X, 0);
    queue[id] = timestamp;
    receivedACKs = 0;
    k = 0;
    changeState(queueing);
    while(1){
        message = receiveMessage();
        incrementTimestamp(message.timestamp);
        switch (message.type)
        {
        case REQ:
            queue[message.sender] = message.timestamp;
            inQue[message.sender] = 1;
            sendMessage(message.sender, ACK, 0);
            break;
        case ACK:
            receivedACKs++;
            if(receivedACKs == countOfX - 1 && state == queueing){
                changeState(readyToFarm);
                k = queuePlace(X, queue, inQue);
                if(k > 0 && k <= minimum){
                    sendToGroup(GROUP_ME, Y, k);
                    changeState(waitingForY);
                }
            }
            break;
        case RELEASE_X:
            inQue[message.sender] = 0;
            if(state == readyToFarm){
                k = queuePlace(X, queue, inQue);
                if(k > 0 && k <= minimum){
                    sendToGroup(GROUP_ME, Y, k);
                    changeState(waitingForY);
                }
            }
            break;
        case JOINED:
            if(state == waitingForY){
                changeState(farming);
                groupedProcess_id = message.sender;
            }else{
                printf("[ERROR X - %d] JOINED\n", id);
            }
            break;
        case RELEASE_Y:
            if(state == farming){
                changeState(queueing);
                sendToGroup(RELEASE_X, X, 0);
                goto start;
            }
            else{
                printf("[ERROR X - %d] RELEASE_Y\n", id);
            }
            break;
        default:
            break;
        }
    }
}
int findX(int k, int *xtab){
    for(int i = 0; i < countOfX; i++){
        if(xtab[i]==k)
            return i;
    }
    return -1;
}
void updateXtab(int i, int* xtab){
    for(int j = 0; j < countOfX; j++){
        if(xtab[j] > xtab[i]) xtab[j]--;
    }
    xtab[i]=0;
}
void goInHyperSpaceY(int k){
    changeState(farming);
    hyperSpace--;
    sleep(TIME_IN);
    sendMessage(groupedProcess_id, RELEASE_X, 0);
    sendToGroup(RELEASE_Y, Y, groupedProcess_id);
    if(k == hyperSpace - 1){
        sendToGroup(EMPTY, Y, 0);
        sendToGroup(EMPTY, Z, 0);
        sendEmptys = 1;
    }
    changeState(queueing);
}
void goInHyperSpaceZ(int k){
    changeState(farming);
    hyperSpace++;
    sleep(TIME_IN);
    sendToGroup(RELEASE_Z, Z, 0);
    if(k + hyperSpace == MAX_ENERGY - 1){
        sendToGroup(FULL, Z, 0);
        sendToGroup(FULL, Y, 0);
        sendFulls = 1;
    }
    changeState(queueing);
}
void runningY(){
    int *queue= malloc(countOfY * sizeof(int));
    int *xtab = malloc(countOfX * sizeof(int));
    char *inQue= malloc(countOfY * sizeof(char)); 
    memset(inQue, 1, countOfY);
    memset(queue, 0, countOfY);
    memset(xtab,-1,countOfX);
    struct Message message;
    int receivedACKs, receivedFULLs, k, minimum;
    int ys = countOfX;
    if(countOfY > countOfX)
        minimum = countOfX;
    else
        minimum = countOfY;

start:
    sendToGroup(REQ, Y, 0);
    queue[id - ys] = timestamp;
    receivedACKs = 0;
    receivedFULLs = 0;
    k = 0;
    sendEmptys = 0;
    changeState(queueing);
    while(1){
        message = receiveMessage();
        incrementTimestamp(message.timestamp);
        switch (message.type)
        {
        case REQ:
            queue[message.sender - ys] = message.timestamp;
            inQue[message.sender - ys] = 1;
            sendMessage(message.sender, ACK, 0);
            break;
        case ACK:
            receivedACKs++;
            if(receivedACKs == countOfY - 1 && state == queueing){
                changeState(waitingForX);
                k = queuePlace(Y, queue, inQue);
                groupedProcess_id = findX(k, xtab);
                if(k>0 && k <= minimum && groupedProcess_id >= 0){
                    sendMessage(groupedProcess_id, JOINED, 0);
                    changeState(readyToFarm);
                }
                if(state == readyToFarm && k <= hyperSpace){
                    goInHyperSpaceY(k);
                    goto start;
                }
            }
            break;
        case RELEASE_Y:
            inQue[message.sender - ys] = 0;
            updateXtab(message.inQue, xtab);
            hyperSpace--;
            k = queuePlace(Y, queue, inQue);
            if(state == waitingForX){
                if(groupedProcess_id == -1)
                    groupedProcess_id = findX(k, xtab);
                if(k>0 && k <= minimum && groupedProcess_id >= 0){
                    sendMessage(groupedProcess_id, JOINED, 0);
                    changeState(readyToFarm);
                }
            }
            if(state == readyToFarm && k <= hyperSpace){
                goInHyperSpaceY(k);
                goto start;
            }
            if(hyperSpace == 0 && sendEmptys == 0){
                sendToGroup(EMPTY, Z, 0);
                sendEmptys = 1;
            }
            break;
        case GROUP_ME:
            xtab[message.sender] = message.inQue;
            hyperSpace--;
            k = queuePlace(Y, queue, inQue);
            if(state == waitingForX){
                if(groupedProcess_id == -1)
                    groupedProcess_id = findX(k, xtab);
                if(k>0 && k <= minimum && groupedProcess_id >= 0){
                    sendMessage(groupedProcess_id, JOINED, 0);
                    changeState(readyToFarm);
                }
            }
            if(state == readyToFarm && k <= hyperSpace){
                goInHyperSpaceY(k);
                goto start;
            }
            break;
        case FULL:
            receivedFULLs++;
            if(receivedFULLs == countOfZ){
                sendEmptys = 0;
                hyperSpace = MAX_ENERGY;
                if(state == readyToFarm && k <= hyperSpace){
                    goInHyperSpaceY(k);
                    goto start;
                }
            }
            break;
        case EMPTY:
            if(hyperSpace == 0 && sendEmptys == 0){
                sendToGroup(EMPTY, Z, 0);
                sendEmptys = 1;
            }
            break;
        
        default:
            break;
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
    int receivedACKs, k;
    int zs = countOfY+countOfX;
    int receivedEMPTYs = 0;

    goto gotoLoop;
start:
    sendToGroup(REQ, Z, 0);
    queue[id - zs] = timestamp;
    receivedACKs = 0;
    k = 0;
    changeState(queueing);
gotoLoop:
    while (1)
    {
        message = receiveMessage();
        incrementTimestamp(message.timestamp);
        switch (message.type)
        {
        case REQ:
            queue[message.sender - zs] = message.timestamp;
            inQue[message.sender - zs] = 1;
            sendMessage(message.sender, ACK, 0);
            break;
        case ACK:
            receivedACKs++;
            if(receivedACKs == countOfZ - 1 && state == queueing){
                changeState(readyToFarm);
                k = queuePlace(Z, queue, inQue);
                if(k > 0 && k + hyperSpace <= MAX_ENERGY){
                    goInHyperSpaceZ(k);
                    goto start;
                }
            }
            break;
        case RELEASE_Z:
            hyperSpace++;
            inQue[message.sender - zs] = 0;
            k = queuePlace(Z, queue,inQue);
            if(k > 0 && k + hyperSpace <= MAX_ENERGY){
                    goInHyperSpaceZ(k);
                    goto start;
            }
            if(hyperSpace == MAX_ENERGY && sendFulls == 0){
                sendToGroup(FULL, Y, 0);
            }
            break;
        case EMPTY:
            receivedEMPTYs++;
            if(receivedEMPTYs == countOfY){
                sendFulls = 0;
                hyperSpace = 0;
                goto start;
            }
            break;
        case FULL:
            if(hyperSpace == MAX_ENERGY && sendFulls == 0){
                sendToGroup(FULL, Y, 0);
                sendFulls = 1;
            }
            break;
        
        default:
            break;
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