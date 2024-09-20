/* Compile method
    g++ client.cpp -o client -Wall
*/

//client.cpp
//CS470
//Simon Choi
/* Honor code: "I pledge that I have neither given nor received help from anyone 
other than the instructor/TA for all program components included here!" */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <random>
#define BUFFER_SIZE 1024
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 5678
#define DEFAULT_TIMEOUT 10

using namespace std;

//global variables
int mode; //store client mode - 0: manual 1: automatic
string IP_address; //store IP address in txt file
int port_number, timeout; //store port number and timeout value in txt file

//main function
int main(int argc, char *argv[]){
    //read command line argument
    if(argc == 2 && strcmp(argv[1], "manual") == 0){
        printf("The client is accessed by manual mode.\n");
        mode = 0; //manual
    }else if(argc == 3 && strcmp(argv[1], "automatic") == 0){
        printf("The client is accessed by automatic mode.\n");
        mode = 1; //automatic
        //read text file that stores IP address, port number, timeout
        vector<string> txtInfo;
        ifstream file(argv[2]);
        if(file.is_open()){
            string line;
            for(int i=0; i<4; i++){
                getline(file, line);
                if(i == 0){ //skip section line
                    continue;
                }
                string value1, value2, value3;
                istringstream iss(line);
                iss >> value1 >> value2 >> value3;
                txtInfo.push_back(value3);
            }
            //store values in global variable
            IP_address = txtInfo.at(0);
            istringstream ssPort(txtInfo.at(1));
            if(!(ssPort >> port_number)){
                printf("Invalid port number from %s file: %s\n", argv[2], txtInfo.at(1).c_str());
                return 0;
            }
            istringstream ssTimeout(txtInfo.at(2));
            if(!(ssTimeout >> timeout)){
                printf("Invalid timeout value from %s file: %s\n", argv[2], txtInfo.at(2).c_str());
                return 0;
            }
        }else {
            cout << "Failed to open the file.\n";
            return 0;
        }
    }else {
        printf("\n Usage: %s [manual | automatic] [txt file] \n",argv[0]);
        return 0;
    }

    int sockfd = 0;
    struct sockaddr_in serv_addr; //structure that stores address of the server
    memset(&serv_addr, '0', sizeof(serv_addr));

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){ //create a socket
        printf("\n Error : Could not create socket \n");
        return 1;
    }
    
    //assign to server address structure
    if(mode == 0){ //manual - use default IP address and port number
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(DEFAULT_PORT);
        if(inet_pton(AF_INET, DEFAULT_IP, &serv_addr.sin_addr)<=0){
            printf("\n inet_pton error occured\n");
            return 1;
        }
    }else{ //automatic - use IP address and port number in text file
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port_number);
        if(inet_pton(AF_INET, IP_address.c_str(), &serv_addr.sin_addr)<=0){
            printf("\n inet_pton error occured\n");
            return 1;
        }
    }
    
    //connect to the server
    int TIMEOUT_SECONDS;
    if(mode == 0){
        TIMEOUT_SECONDS = DEFAULT_TIMEOUT;
    }else {
        TIMEOUT_SECONDS = timeout;
    }
    printf("Connecting to the server...\n");
    bool connection = false;
    time_t startTime = time(nullptr); //get current time
    while(!connection){
        time_t currentTime = time(nullptr); //get current time
        time_t gapTime = currentTime - startTime;
        if(gapTime >= TIMEOUT_SECONDS){
            printf("Timeout reached. Failed connection to the server.\n");
            return 0;
        }
        if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0){
            printf("Successfully connected!\n");
            connection = true;
        }
    }

    //get map size from the server
    int mapSize[2];
    int MAP_ROWS, MAP_COLUMNS;
    ssize_t mapByte = recv(sockfd, mapSize, sizeof(mapSize), 0);
    if(mapByte <0){
        cerr << "Fail to get map size.\n";
        return 0;
    }else if(mapByte == 0){
        cerr << "There are no seats available.\nDisconnect connection\n";
        close(sockfd);
        return 0;
    }else{
        MAP_ROWS = mapSize[0];
        MAP_COLUMNS = mapSize[1];
        printf("<Size of map: %d Row, %d Column.>\n", MAP_ROWS, MAP_COLUMNS);
    }

    bool disconnect = false;
    mt19937 mt(time(NULL));
    //start communication
    printf("-----------------------------------------------------------\n");
    while(!disconnect){
        int x, y; //coordinate to pass
        if(mode == 0){ //manual
            bool valid = false;
            while(!valid){
                //ask user to input coordinate
                cout << "Enter coordinate (EX. 2,2) or (-1,-1 to exit): ";
                string coord;
                getline(cin, coord);
                vector<int> input;
                stringstream s_stream(coord);
                //add user input coordinate into vector
                while(s_stream.good()){
                    int number;
                    string substr;
                    getline(s_stream, substr, ','); //get first string delimited by comma
                    istringstream ss(substr);
                    if(!(ss >> number)){
                        printf("Invalid number for coordinate: %s\n", substr.c_str());
                    }else{
                        input.push_back(number);
                    }
                }
                if(input.size() < 2){
                    printf("Need more value. Try again.\n");
                }else if(input.size() > 2){ //too many inputs
                    printf("Too many inputs. Try again.\n");
                }else{
                    x = input.at(0);
                    y = input.at(1);
                    valid = true;
                    if(x == -1 || y == -1){
                        disconnect = true;
                    }
                }
            }
        }else{//automatic
            //generate random numbers for coordinate
            x = mt() % MAP_ROWS;
            y = mt() % MAP_COLUMNS;
            printf("Request a seat for (%d, %d).\n", x, y);
        }

        //send the coordinate to the server
        int coordinate[2] = {x,y};
        if(send(sockfd, coordinate, sizeof(coordinate), 0) < 0){
            cerr << "Error in sending data to the server.\n";
            return 1;
        }else{
            //recieve confirmation message from the server
            char buffer[BUFFER_SIZE];
            ssize_t receivedByte = recv(sockfd, buffer, sizeof(buffer), 0);
            if(receivedByte < 0){
                cerr << "Error in receiving message from the server.\n";
                break;
            }else if(receivedByte == 0){
                cerr << "Server closed connection.\n";
                disconnect = true;
                continue;
            }else {
                buffer[receivedByte] = '\0';
                cout << "Received message from server: " << buffer << endl;
                if(receivedByte > 35){ //message length is more than 35: seats are sold out
                    disconnect = true;
                }
            }
        }

        if(mode == 0){
            printf("-----------------------------------------------------------\n");
        }else if(mode == 1){ //random delay for automatic mode
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> randNum(1, 3);
            int num = randNum(gen);
            int delay = 2 * num + 1; //delays: 3/5/7 seconds
            printf("Delay time: %d.\n", delay);
            printf("-----------------------------------------------------------\n");
            sleep(delay*0);
        }
    }

    close(sockfd);
    printf("Disconnect connection.\n");
    return 0;
}
