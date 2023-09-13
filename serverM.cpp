/**
 * @file serverM.cpp
 * @author Yi-Hsuan Chen
 * 
 * The code is adapted from server.c, talker.c, and listener.c in Beej's Guide to Network Programming.
 * 
 */

#include <cstdlib>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <netdb.h>
#include <sys/types.h>
#include <set>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define HOST_NAME "127.0.0.1"
#define UDPPORT "23280"
#define TCPPORT "24280"

#define SERVER_A "21280"
#define SERVER_B "22280"

#define BACKLOG 10 //Size of pending connections queue
#define MAXSIZE 5000

using namespace std;

void sigchld_handler(int s){
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

set<string> nameToSet(string names){
    set<string> nameList;
    string name;
    stringstream ss(names);
    while(getline(ss, name, ',')){
        nameList.insert(name);
    }
    return  nameList;
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

string printSlot(vector<int> times){
    stringstream ss;
    ss << "[";
    int first = -1;
    int last = -1;
    for(int i=0; i<times.size(); i++){
        if(times[i] == 1){
            if(first == -1){
                first = i;
            }
            else{
                last = i;
            }
        }
        if(times[i] == 0 && first != last && first != -1 && last != -1){
            ss << "[" << first << ", " << last << "], ";
            first = -1;
            last = -1;
        }
    }
    ss << "]";
    string intersection = ss.str();
    
    if(intersection.length() == 2){
        return intersection;
    }

    return intersection.replace(intersection.length()-3, 2, "");
}

int main(void){
    int UDP_socketfd;
    int TCP_socketfd;
    int child_socketfd;
    int listA_size;
    int listB_size;
    int client_input_size;
    struct sockaddr_in serverM_UDP_addr;
    struct sockaddr_in serverM_TCP_addr;
    struct sockaddr_in child_addr;
    struct sockaddr_storage serverA_addr;
    struct sockaddr_storage serverB_addr;
    socklen_t addrA_len;
    socklen_t addrB_len;
    char nameList_A[MAXSIZE];
    char nameList_B[MAXSIZE];
    char input_client[MAXSIZE];
    set<string> list_A;
    set<string> list_B;

    /**
     * Create the UDP socket for Server M.
     * - Using IPv4
     * - Socket Descriptor = UDP_socketfd
     * - Use to communicate with Server A and Server B
     */
    if((UDP_socketfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("SERVER M: fail to create UDP socket");
        exit(1);
    }
    memset(&serverM_UDP_addr, 0, sizeof serverM_UDP_addr);
    serverM_UDP_addr.sin_family = AF_INET;
    serverM_UDP_addr.sin_addr.s_addr = inet_addr(HOST_NAME);
    serverM_UDP_addr.sin_port = (__uint16_t)(stoi(UDPPORT));
    if(::bind(UDP_socketfd, (const struct sockaddr*)&serverM_UDP_addr, sizeof serverM_UDP_addr) == -1){
        perror("SERVER M: fail to bind the UDP socket");
        exit(1);
    }
    printf("Main is up and running.\n");

    //Receive the namelist from Server A
    addrA_len = sizeof serverA_addr; 
    if((listA_size = recvfrom(UDP_socketfd, nameList_A, MAXSIZE-1, 0, (struct sockaddr *)&serverA_addr, &addrA_len)) == -1){
        perror("SERVER M: fail to receive name list from server A");
        exit(1);
    }
    nameList_A[listA_size] = '\0';
    string nameA(nameList_A);
    list_A = nameToSet(nameA);
    printf("Main Server received the username list from server A using UDP over port %s\n", UDPPORT);

    //Receive the name list from server B
    addrB_len = sizeof serverB_addr;
    if((listB_size = recvfrom(UDP_socketfd, nameList_B, MAXSIZE-1, 0, (struct sockaddr *)&serverB_addr, &addrB_len)) == -1){
        perror("SERVER M: fail to receive name list from server B");
        exit(1);
    }
    nameList_B[listB_size] = '\0';
    string nameB(nameList_B);
    list_B = nameToSet(nameB);
    printf("Main Server received the username list from server B using UDP over port %s\n", UDPPORT);

    /**
     * Create the TCP socket for Server M.
     * - Using IPv4
     * - Socket Descriptor = TCP_socketfd
     * - Use to communicate with Client
     */
    if((TCP_socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("SERVER M: fail to create TCP socket");
        exit(1);
    }
    memset(&serverM_TCP_addr, 0, sizeof serverM_TCP_addr);
    serverM_TCP_addr.sin_family = AF_INET;
    serverM_TCP_addr.sin_addr.s_addr = inet_addr(HOST_NAME);
    serverM_TCP_addr.sin_port = (__uint16_t)(stoi(TCPPORT));
    if(::bind(TCP_socketfd, (const struct sockaddr*)&serverM_TCP_addr, sizeof serverM_TCP_addr) == -1){
        perror("SERVER M: fail to bind the TCP socket");
        exit(1);
    }
    if(listen(TCP_socketfd, BACKLOG) == -1){
        perror("SERVER M: fail to listen for client");
        exit(1);
    }

    /**
     * Keep communicating with client, server A, and server B
     * - Receive the errorless input from client through TCP
     * - Find where the name resides in (server A or B)
     * 
     * ## If there exists the name not exist in A or B:
     * - Send the name list back to client through TCP
     * 
     * - Send name list (if exist) to server A or B through UDP
     * - Receive the intersection from server A or B
     * - Intersect the intersections received from server A and B
     * - Send the result back to client through TCP
     */
    while(true){
        /**
         * Accept the TCP connection from client,
         * and create a child socket to perform the operation.
         * - Socket Descriptor = child_socketfd
         */
        socklen_t child_addr_len;
        child_addr_len = sizeof child_addr;
        if((child_socketfd = ::accept(TCP_socketfd, (struct sockaddr*) &child_addr, &child_addr_len)) == -1){
            perror("SERVER M: fail to accept TCP connection from client");
            exit(1);
        }

        //Start interacting with connected client
        while(true){
            //Receive the errorless participants list from client
            if((client_input_size = recv(child_socketfd, input_client, MAXSIZE-1, 0)) == -1){
                perror("SERVER M: fail to receive input from client");
                exit(1);
            }
            printf("Main Server received the request from client using TCP over port %s.\n", TCPPORT);

            //Parse the partiicipants list into vector<string>
            input_client[client_input_size] = '\0';
            string inputString(input_client);
            string temp;
            stringstream ss(inputString);
            vector<string> client_input_vec;
            while(getline(ss, temp, ',')){
                client_input_vec.push_back(temp);
            }

            //Find the where the name resides in (Server A/ServerB/Not exist)
            vector<string> search_A;
            vector<string> search_B;
            vector<string> exist_list;
            vector<string> not_exist;
            bool invalidUser = false;
            bool emptyVec = true;

            //Telling the client if there is not exist user in the namelist or not
            not_exist.push_back("f");
        
            for(string name:client_input_vec){
                auto a_end = list_A.end();
                auto b_end = list_B.end();
                auto find_A = list_A.find(name);
                auto find_B = list_B.find(name);

                //Case 1. Names are in Server A
                if(find_A != a_end){
                    search_A.push_back(name);
                    exist_list.push_back(name);
                    emptyVec = false;
                }
                //Case 2. Names are in Server B
                else if(find_B != b_end){
                    search_B.push_back(name);
                    exist_list.push_back(name);
                    emptyVec = false;
                }
                //Case 3. Names do not exist in the list
                else if((find_A == a_end) && (find_B == b_end)){
                    not_exist.push_back(name);
                    invalidUser = true;
                }
            }

            /**
             * If there is not exist user in the namelist
             * - Show the message on Main Server.
             * - Send the not exist namelist to client.
             */ 
            if(invalidUser){
                string printInvalid = printVector(not_exist);
                printf("%s do not exist. Send a reply to the client.\n", printInvalid.substr(3,printInvalid.length()-3).c_str());
            }
            else{
                not_exist.clear();
            }

            string invalidNames = "";
            invalidNames = nameToString(not_exist);
            if(send(child_socketfd, invalidNames.c_str(), sizeof invalidNames, 0) == -1){
                perror("Server M: fail to send not existed name list to client");
                exit(1);
            }

            /**
             * Send exist participants:
             * - send the namelist to Server A or Server B to find the intersection
             * - receive the result back from Server A or Server B
             * - calculate the final intersetion
             */
            vector<int> intersect_A_vec;
            vector<int> intersect_B_vec;
            if(!search_A.empty()){
                string nameToA = nameToString(search_A);
                string printA = printVector(search_A);
                if(sendto(UDP_socketfd, nameToA.c_str(), sizeof nameToA, 0, (struct sockaddr *)&serverA_addr, addrA_len) == -1){
                    perror("SERVER M: fail to send name list to server A");
                    exit(1);
                }
                printf("Found %s located at Server A. Send to Server A.\n", printA.c_str());
                usleep(1000);
            }
            if(!search_B.empty()){
                string nameToB = nameToString(search_B);
                string printB = printVector(search_B);
                if(sendto(UDP_socketfd, nameToB.c_str(), sizeof nameToB, 0, (struct sockaddr *)&serverB_addr, addrB_len) == -1){
                    perror("Server M: fail to send name list to server B");
                    exit(1);
                }
                printf("Found %s located at Server B. Send to Server B.\n", printB.c_str());
            }

            if(!search_A.empty()){
                //Receive the intervals from A
                int timeLength_A;
                char intersect_A[MAXSIZE];
                if((timeLength_A = recvfrom(UDP_socketfd, intersect_A, MAXSIZE-1, 0, (struct sockaddr *)&serverA_addr, &addrA_len)) == -1){
                    perror("Server M: fail to receive time intersection from Server A");
                    exit(1);
                }
                intersect_A[timeLength_A] = '\0';
                string intersect_A_string(intersect_A);
                string timeA;
                stringstream ssA(intersect_A_string);
                while(getline(ssA, timeA, ',')){
                    intersect_A_vec.push_back(stoi(timeA)-1);
                }
                string printTimeA = printSlot(intersect_A_vec);
                printf("Main Server received from Server A the intersection result using UDP over port %s: %s.\n", UDPPORT, printTimeA.c_str());
            }
            if(!search_B.empty()){
                //Receive the intervals from B
                int timeLength_B;
                char intersect_B[MAXSIZE];
                if((timeLength_B = recvfrom(UDP_socketfd, intersect_B, MAXSIZE-1, 0, (struct sockaddr *)&serverB_addr, &addrB_len)) == -1){
                    perror("Server M: fail to receive time intersection from Server B");
                    exit(1);
                }
                intersect_B[timeLength_B] = '\0';
                string intersect_B_string(intersect_B);
                string timeB;
                stringstream ssB(intersect_B_string);
                while(getline(ssB, timeB, ',')){
                    intersect_B_vec.push_back(stoi(timeB)-1);
                }
                string printTimeB = printSlot(intersect_B_vec);
                printf("Main Server received from Server B the intersection result using UDP over port %s: %s.\n", UDPPORT, printTimeB.c_str());
            }

            //intersect the intervals to get the final result
            vector<int> slot_final(101, 0);
            if(!search_A.empty()){
                slot_final = intersect_A_vec;
            }
            else if(!search_B.empty()){
                slot_final = intersect_B_vec;
            }
            else if(!(search_A.empty() || search_B.empty())){
                for(int i=0; i<intersect_A_vec.size(); i++){
                    slot_final[i] = intersect_A_vec[i] & intersect_B_vec[i];
                }
            }

            //Send the final result to client
            string printFinalTime;
            if(!emptyVec){
                printFinalTime = printSlot(slot_final);
                printf("Found the intersection between the results from server A and B:\n%s\n", printFinalTime.c_str());
            }
            else{
                //Mark the intersection time list with "e" if there is no result (for client)
                printFinalTime = "e";
            } 

            usleep(1000);

            if(send(child_socketfd, printFinalTime.c_str(), (size_t)printFinalTime.length(), 0) == -1){
                perror("Server M: fail to send available time slots to client");
                exit(1);
            }

            usleep(1000);

            //Send the exist user namelist to client
            if(!emptyVec){
                string finalNameVec = printVector(exist_list);
                if(send(child_socketfd, finalNameVec.c_str(), (size_t)finalNameVec.length(), 0) == -1){
                    perror("Server M: fail to send final namelist to client");
                    exit(1);
                }
            }
            printf("Main Server sent the result to the client.\n");
        }
        close(child_socketfd);
    }
    close(UDP_socketfd);
    close(TCP_socketfd);
    return 0;
}