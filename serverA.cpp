/**
 * @file serverA.cpp
 * @author Yi-Hsuan Chen
 * 
 * The code is adapted from talker.c and listener.c in Beej's Guide to Network Programming.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <fstream>
#include <regex>
#include <vector>
#include <unordered_map>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define HOST_NAME "127.0.0.1"
#define SERVEA_PORT "21280" //UDP port number for server A
#define SERVEM_PORT "23280" //UDP port number for server M
#define MAXBUFLEN 5000

using namespace std;

struct UsersInfo{
    vector<string> nameList;
    unordered_map<string, vector<vector<int> > > timeList;
};

vector<vector<int> > intVec(vector<string> stringVec){
    size_t inputSize = stringVec.size();
    vector<vector<int> > res((int)inputSize, vector<int>(2));
    int curr = 0;
    
    for(string slot:stringVec){
        stringstream ss(slot);
        string temp;
        vector<int> oneSlot;
        while(getline(ss, temp, ',')){
            oneSlot.push_back(stoi(temp));
        }
        for(int i=0; i<2; i++){
            res[curr][i] = oneSlot[i];
        }
        curr++;
    }
    return res;
}

UsersInfo retrieveData(string fileName){
    ifstream inFile;
    string outString;
    UsersInfo info;
    
    inFile.open(fileName);
    if(inFile.is_open()){
        string line;
        regex r("\\s+");
        while(getline(inFile, line)){
            string name;
            string trimedName;
            string timeString;
            vector<string> time;
            vector<vector<int> > timeSlot;

            name = line.substr(0,line.find(';'));
            trimedName = regex_replace(name, r, "");
            info.nameList.push_back(trimedName);

            timeString = line.substr(line.find(';')+2, line.find_last_of(']')-((line.find(';'))+2));
            int pointer = 1;
            while(timeString.length()>0){
                size_t lastPos = timeString.find(']');
                string temp = timeString.substr(pointer,lastPos-1);
                time.push_back(temp);
                if(lastPos==timeString.length()-1){
                    timeString = "";
                    break;
                }
                timeString = timeString.substr(lastPos+2);
            }
            timeSlot = intVec(time);
            info.timeList[trimedName] = timeSlot;
        }
    }
    return info;
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

vector<int> intersect(vector<string> names, unordered_map<string,vector<vector<int> > > timeMap){
    vector<int> res(101,0);
    vector<vector<int> > firstInterval = timeMap[names[0]];
    for(auto slot:firstInterval){
        for(int i=slot[0]; i<slot[1]+1; i++){
            res[i] = 1;
        }
    }
    for(string name:names){
        vector<int> curr(101,0);
        vector<vector<int> > currInterval = timeMap[name];
        for(auto slot:currInterval){
            for(int j=slot[0]; j<slot[1]+1; j++){
                curr[j] = 1;
            }
        }
        for(int k=0; k<res.size(); k++){
            res[k] = res[k] & curr[k];
        }
    }
    return res;
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

string intVecToString(vector<int> times){
    stringstream ss;
    for(int time:times){
        time +=1;
        if(time != times.back()){
            ss << time << ",";
        }
        else{
            ss << time;
        }
    }
    return ss.str();
}

int main(){
	int socketfd;
    int numBytes;
    int inputSize;
    struct sockaddr_in serverA_addr;
    struct sockaddr_in serverM_addr;
    char inputBuf[MAXBUFLEN];

    /**
     * Create the UDP socket for Server A.
     * - Using IPv4
     * - Socket Descriptor = socketfd
     */
    if((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("SERVER A: fail to create a socket");
        exit(1);
    }
    memset(&serverA_addr, 0, sizeof serverA_addr);
    serverA_addr.sin_family = AF_INET;
    serverA_addr.sin_addr.s_addr = inet_addr(HOST_NAME);
    serverA_addr.sin_port = (__uint16_t)(stoi(SERVEA_PORT));

    if(::bind(socketfd, (struct sockaddr*)&serverA_addr, sizeof serverA_addr) == -1){
        perror("SERVER A: fail to bind the socket");
        exit(1);
    }
    printf("Server A is up and running using UDP on port %s.\n", SERVEA_PORT);

    /**
     * Retrieve data from the input file: a.txt
     * Store the namelist and available time slots into struct UserInfo
     */
    UsersInfo info = retrieveData("a.txt");

    //Set the connection information for Main Server
    memset(&serverM_addr, 0, sizeof serverM_addr);
    serverM_addr.sin_family = AF_INET;
    serverM_addr.sin_addr.s_addr = inet_addr(HOST_NAME);
    serverM_addr.sin_port = (__uint16_t)(stoi(SERVEM_PORT));

    //Send the namelist to Main Server
    string nameString;
    nameString = nameToString(info.nameList);
    if((numBytes = sendto(socketfd, nameString.c_str(), nameString.length(), 0, (const struct sockaddr *) &serverM_addr, sizeof serverM_addr)) == -1){
        perror("SERVER A: fail to send message to the main server");
        exit(1);
    } 
    printf("Server A finished sending a list of usernames to Main Server.\n");

	//Keep receiving the namelist from Main Server
    while(true){
        socklen_t serverM_len = sizeof serverM_addr;
        if((inputSize = recvfrom(socketfd, inputBuf, MAXBUFLEN-1, 0, (struct sockaddr*) &serverM_addr, &serverM_len)) == -1){
            perror("Server A: fail to receive name list from Main server");
            exit(1);
        }
        printf("Server A received the usernames from Main Server using UDP over port %s.\n", SERVEA_PORT);

        /**
        * 1. Parse the input received from Main server into string vector
        * 2. Find the time intersection of participants
        */
        inputBuf[inputSize] = '\0';
        string inputString(inputBuf);
        string temp;
        stringstream ss(inputString);
        vector<string> participants;
        vector<int> commonTime;
        while(getline(ss, temp, ',')){
            participants.push_back(temp);
        }
        commonTime = intersect(participants, info.timeList);
        string result_time = printSlot(commonTime);
        string result_name = printVector(participants);
        printf("Found the intersection result: %s for %s.\n", result_time.c_str(), result_name.c_str());

        //Send the result in prototype back to Main Server
        string output = intVecToString(commonTime);
        if(sendto(socketfd, output.c_str(), output.length(), 0, (const struct sockaddr *) &serverM_addr, sizeof serverM_addr) == -1){
            perror("SERVER A: fail to send output time list to the main server");
            exit(1);
        }
        printf("Server A finished sending the response to Main Server.\n");
    }

    close(socketfd);
    return 0;
}