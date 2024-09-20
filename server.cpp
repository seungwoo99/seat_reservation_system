/* Compile method
    g++ server.cpp -o server -Wall
*/

//server.cpp
//CS470
//Simon Choi
/* Honor code: "I pledge that I have neither given nor received help from anyone 
other than the instructor/TA for all program components included here!" */

//header files
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <sstream>
#include <iostream>
#include <pthread.h>
#define BUFFER_SIZE 1024
#define PORT_NUMBER 5678
#define DEFAULT_ROW 10
#define DEFAULT_COLUMN 10

using namespace std;

//global variables
int MAP_ROWS; //map rows
int MAP_COLUMNS; //map columns
int **map; //map
pthread_mutex_t myMutex;

void printMap(){
    printf("<Current map status>\n");
    for(int i=0; i<MAP_ROWS; i++){
        for(int j=0; j<MAP_COLUMNS; j++){
            cout << map[i][j] << " | ";
        }cout << endl;
    }
}

//check if the map is full
bool checkMap(){
    bool full = true;
    for(int i=0; i<MAP_ROWS; i++){
        for(int j=0; j<MAP_COLUMNS; j++){
            if(map[i][j] == 0){
                full = false;
            }
        }
    }
    return full;
}

//thread function
void* handleClient(void* connfd){
    int clientSocket = (int)(long)connfd;
    printf("Client %d entered.\n", clientSocket);
    while(1){
        printMap();
        //receive coordinate from the client
        int coordinate[2];
        ssize_t receivedByte = recv(clientSocket, coordinate, sizeof(coordinate), 0);
        if(receivedByte == -1){
            cerr << "Error in receiving data from the client.\n";
            break;
        }else if(receivedByte == 0){
            break;
        }else {
            int x = coordinate[0];
            int y = coordinate[1];
            string result = "";
            pthread_mutex_lock(&myMutex);
            if(x == -1 || y == -1){
                result = "Client closed the connection.\n";
            }else if((0<=x && x<MAP_ROWS) && (0<=y && y<MAP_ROWS)){
                if(map[x][y] == 0){
                    map[x][y] = 1;
                    printf("Client %d purchased (%d, %d) seat.\n\n", clientSocket, x, y);
                    result = "Allocated Successfully.";
                }else{
                    printf("Client %d tried to purchase already allocated seat (%d, %d).\n\n", clientSocket, x, y);
                    result = "The seat is already booked.";
                }
                if(checkMap()){
                    result += "\nThe seats are sold out.";
                }
            }else{
                printf("Client %d tried to purchase a seat outside of scope (%d, %d).\n\n", clientSocket, x, y);
                result = "Out of range. Try again.";
            }
            pthread_mutex_unlock(&myMutex);
            const char* message = result.c_str();
            //send result message
            if(send(clientSocket, message, strlen(message), 0) == -1){
                cerr << "Failed to send message to the client.\n";
                break;
            }
        }
    }
    //close client socket
    close(clientSocket);
    printf("Client %d connection is closed.\n", clientSocket);
    pthread_exit(NULL);

    return (void*)0;
}

//main function
int main(int argc, char *argv[]){
    //read command line argument
    if(argc == 1){
        MAP_ROWS = DEFAULT_ROW;
        MAP_COLUMNS = DEFAULT_COLUMN;
    }else if(argc == 3){
        //store row input
        istringstream ss1(argv[1]);
        if(!(ss1 >> MAP_ROWS)){
            printf("Invalid number for row size: %s\n", argv[1]);
        }else{
            if(MAP_ROWS <= 0){
                printf("Row size cannot be less than or equal to 0.\n");
            }
        }
        //store column input
        istringstream ss2(argv[2]);
        if(!(ss2 >> MAP_COLUMNS)){
            printf("Invalid number for column size: %s\n", argv[2]);
        }else{
            if(MAP_COLUMNS <= 0){
                printf("Column size cannot be less than or equal to 0.\n");
            }
        }
        if(MAP_ROWS <= 0 || MAP_COLUMNS <= 0){
            return 0;
        }
    }else {
        printf("\n Usage: %s [Map row size] [Map column size] \n",argv[0]);
        return 0;
    }
    
    //create a map
    map = new int*[MAP_ROWS];
    for(int i=0; i<MAP_ROWS; i++){
        map[i] = new int[MAP_COLUMNS];
    }
    //place 0s in the map
    for(int i=0; i<MAP_ROWS; i++){
        for(int j=0; j<MAP_COLUMNS; j++){
            map[i][j] = 0;
        }
    }

    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr; //structure that stores address of the server

    listenfd = socket(AF_INET, SOCK_STREAM, 0); //create a socket

    memset(&serv_addr, '0', sizeof(serv_addr)); //memory value setting

    serv_addr.sin_family = AF_INET; //assign to server address structure
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT_NUMBER);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); //binding socket
    listen(listenfd, 10); //socket in waiting status

    printf("Server has started...\n");

    while(1){ //use while for multiple clients access
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); //get socket when client access

        //check if the map is full
        if(checkMap()){
            close(connfd);
            continue;
        }

        //send map size to the client
        int mapSize[2] = {MAP_ROWS, MAP_COLUMNS};
        if(send(connfd, mapSize, sizeof(mapSize), 0) < 0){
            cerr << "Error in sending map size to the client.\n";
        }

        //create a thread for a client
        pthread_t threadID;
        pthread_create(&threadID, NULL, handleClient, (void*)(long)connfd);

        sleep(1);
    }
    
    return 0;
}
