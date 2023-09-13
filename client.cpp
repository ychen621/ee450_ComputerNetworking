#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define HOST_NAME "127.0.0.1"
#define SERVER_PORT "24280"
#define MAXSIZE 5000

using namespace std;

//Global variable;
bool noError;

bool error_check(string input){
    bool res = true;
    //Small alphabets are in range of 97-122
    for(int i=0; i<input.length(); i++){
        if(input[i]<97 || input[i]>122){
            res = false;
            break;
        }
    }
    return res;
}

vector<string> parse_and_check(string input){
    noError = true;
    vector<string> user_list;
    stringstream ss(input);
    string name;
    while(ss >> name){
        noError = error_check(name);
        if(!noError){
            break;
        }
        user_list.push_back(name);
    }
    return user_list;
}

string nameToString(vector<string> nameList){
    stringstream ss;
    for(string name:nameList){
        if(name != nameList.back()){
            ss << name << ",";
        }
        else{
            ss << name;
        }
    }
    return ss.str();
}

string printVector(vector<string> nameList){
    stringstream ss;
    for(string name:nameList){
        if(name != nameList.back()){
            ss << name << ", ";
        }
        else{
            ss << name;
        }
    }
    return ss.str();
}

int main(int argc, char *argv[]){
    int client_socketfd;
    struct sockaddr_in serverM_TCP_addr;

    /**
     * Create the TCP socket for Client.
     * - Using IPv4
     * - Socket Descriptor = client_socketfd
     * - Use to communicate with Server M
     * 
     * Connect to Main Server
     */
    if((client_socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Client: fail to create a TCP socket");
        exit(1);
    }
    memset(&serverM_TCP_addr, 0, sizeof serverM_TCP_addr);
    serverM_TCP_addr.sin_family = AF_INET;
    serverM_TCP_addr.sin_addr.s_addr = inet_addr(HOST_NAME);
    serverM_TCP_addr.sin_port = (__uint16_t)(stoi(SERVER_PORT));

    connect(client_socketfd, (const struct sockaddr*)&serverM_TCP_addr, sizeof serverM_TCP_addr);

    printf("Client is up and running.\n");
    printf("Please enter the usernames to check schedule availability:\n");

    //Keep listening to user input
    string userInput;
    while(getline(cin, userInput)){
        vector<string> user_list;
        string nameListString;
        string printName;

        user_list = parse_and_check(userInput);
        if(!noError){
            printf("Error: Invalid input\n");
            printf("Input names should have only small letters and no special characters.\n");
            printf("-----Start a new request-----\n");
            printf("Please enter the usernames to check schedule availability:\n");
            continue;
        }
        nameListString = nameToString(user_list);
        printName = printVector(user_list);

        //Send the errorless user input (participants) to Main Server
        if(send(client_socketfd, nameListString.c_str(), sizeof nameListString, 0) == -1){
            perror("Client: fail to send name list to the main server");
            exit(1);
        }
        printf("Client finished sending the usernames to Main Server.\n");

        //Get the client socket information
        struct sockaddr_storage client_addr;
        socklen_t client_len;
        client_len = sizeof client_addr;
        if(getsockname(client_socketfd, (struct sockaddr *)&client_addr, &client_len)){
            perror("Client: fail to get socket name");
            exit(1);
        }
        struct sockaddr_in *sin_client = (struct sockaddr_in *)&client_addr;
        char *portNum = (char *)&sin_client->sin_port;

        //Receive the not exist namelist from Main Server
        char noExistList[MAXSIZE];
        int listSize;
        if((listSize = recv(client_socketfd, noExistList, MAXSIZE-1, 0)) == -1){
            perror("Client: fail to receive not exist name list from the main server");
            exit(1);
        }
        noExistList[listSize] = '\0';
        string noExistString(noExistList);
        
        /**
         * If the first char is "f" (marked in Main Server)
         * - parse the not exist list
         * - print the not exist list
         */
        if(noExistList[0] == 'f'){
            vector<string> resultVec;
            stringstream ss(noExistString);
            string temp;
            string notExist;
            while(getline(ss,temp, ',')){
                if(temp == "f"){
                    continue;
                }
                resultVec.push_back(temp);
            }
            notExist = printVector(resultVec);
            printf("Client received the reply from Main Server using TCP over port %d:\n", portNum[0]);
            printf("%s do not exist.\n", notExist.c_str());
        }

        char resultList[MAXSIZE];
        int resultSize;
        if((resultSize = recv(client_socketfd, resultList, MAXSIZE-1, 0)) == -1){
            perror("Client: fail to receive result list from the main server");
            exit(1);
        }
        resultList[resultSize] = '\0';
        string resultString(resultList);

        if(resultList[0] != 'e'){
            char finalNameList[MAXSIZE];
            int nameSize;
            if((nameSize = recv(client_socketfd, finalNameList, MAXSIZE-1, 0)) == -1){
                perror("Client: fail to receive final name list from main server");
                exit(1);
            }
            finalNameList[nameSize] = '\0';
            string finalNameString(finalNameList);
            printf("Client received the reply from Main Server using TCP over port %d:\n", portNum[0]);
            printf("Time intervals %s works for %s.\n", resultString.c_str(), finalNameString.c_str());
        }

        printf("-----Start a new request-----\n");
        printf("Please enter the usernames to check schedule availability:\n");
    }
    return 0;
}