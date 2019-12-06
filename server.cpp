// This is the agent application that will be used to send actions to the server.

// It takes 3 arguments, server IP or host, port, and the action requested.

// List of includes
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <ctime>
#include <string>
#include <cstring>
#include <cstdlib>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <vector>
#include <algorithm>
#include <sstream>

#define MAXPORT 6
#define MAXBUFFER 1024


void WriteLog(std::string line) {
    std::ofstream log("log.txt", std::ios::app);
    time_t t = time(NULL);
    char *timeStamp = ctime(&t);
    timeStamp[strlen(timeStamp) - 1] = '\0';
    log << "\"" << timeStamp << "\": " << line << "\n";
    log.close();
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("\nNot enough arguments\n");
        return -1;
    }

    int port = atoi(argv[1]);
    printf("Setting up server on port: %i", port);
    char server_message[256] = "You have reached the server!";

    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 1) {
        printf("\nSocket setup failed\n");
        return -1;
    }

    //define main variables
    std::vector<time_t> timeStamps;
    std::vector<std::string> agents;
    char buffer[MAXBUFFER];
    std::string actions[] = {"#JOIN",
                             "#LEAVE",
                             "#LIST",
                             "#LOG"};
    std::string ipAddress;
    struct sockaddr_in clientAddress{};
    int clientAddrLen = sizeof(clientAddress);
    struct sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        printf("Error: Binding socket failed");
        return -1;
    }

    int maxClients = 5;
    if (listen(server_socket, maxClients) < 0) {
        printf("Listening failed");
        return -1;
    }

    //cout << "Listening on socket: " << server_socket;

    int client_socket;
    int clientIp;
    std::string str;
    std::string log;

    while (true) {

        client_socket = accept(server_socket, (struct sockaddr *) &clientAddress, (socklen_t *) &clientAddrLen);
        if (client_socket < 0) {
            std::cerr << "Failed to accept client";
            return -1;
        }

        //store ip
        ipAddress = inet_ntoa(clientAddress.sin_addr);
        memset(buffer, 0, MAXBUFFER);
        read(client_socket, buffer, MAXBUFFER);

        //check for actions
        if (strcmp(buffer, actions[0].c_str()) == 0) { // #JOIN
            //add new member
            //print $OK or $ALREADY MEMBER
            WriteLog("Received a  \"#JOIN\" request from agent \"" + ipAddress + "\"");

            if (std::find(agents.begin(), agents.end(), ipAddress) != agents.end()) {
                str = "$AREADY MEMEBR";
                WriteLog("Responded to agent \"" + ipAddress + "\" with \"" + str + "\"");
            } else {
                str = "$OK";
                agents.push_back(ipAddress);
                timeStamps.push_back(time(NULL));
                WriteLog("Responded to agent \"" + ipAddress + "\" with \"" + str + "\"");
            }
            write(client_socket, str.c_str(), str.length());

        } else if (strcmp(buffer, actions[1].c_str()) == 0) { //#LEAVE
            //remove member
            //print $OK or $NOT MEMBER

            std::string log = "Received a  \"#LEAVE\" request from agent \"" + ipAddress + "\"";
            WriteLog(log);
            bool foundAndRemoved = false;
            for (int i = 0; i < agents.size(); i++) {
                //if match
                if (agents[i] == ipAddress) {
                    agents.erase(agents.begin() + i);
                    str = "$OK";
                    log = "Responded to agent \"" + ipAddress + "\" with \"" + str + "\"";
                    //no duplicates so safe to return
                    foundAndRemoved = true;
                }
            }

            if (!foundAndRemoved){
                str = "$NOT MEMBER";
                log = "Responded to agent \"" + ipAddress + "\" with \"" + str + "\"";
            }

            write(client_socket, str.c_str(), str.length());
            WriteLog(log);


        } else if (strcmp(buffer, actions[2].c_str()) == 0) { //#LIST
            //break if not member
            //send list of all active agents
            //list of <clientIP, seconds online>
            WriteLog("Received a  \"#LIST\" request from agent \"" + ipAddress + "\"");

            std::string log = "Received a  \"#LEAVE\" request from agent \"" + ipAddress + "\"";
            WriteLog(log);

            int index = -1;
            for (int i = 0; i < agents.size(); i++) {
                //if match
                if (agents[i] == ipAddress) {
                    index = i;
                }
            }

            if (index != -1) {
                //save agent entry and its corresponding time stamp to log
                double elapsedTime = difftime(time(NULL), timeStamps[index]);

                //double to string
                std::stringstream s;
                s << elapsedTime;
                std::string timeInseconds = s.str();

                //write
                str = "<" + agents[index] + ", " + timeInseconds + ">\n";
                write(client_socket, str.c_str(), str.length());
                WriteLog("Responded to agent \"" + ipAddress + "\" with list of active agents.");

            } else
                WriteLog(log = "No response is supplied to agent \"" + ipAddress + "\"");

            write(client_socket, str.c_str(), str.length());

        } else if (strcmp(buffer, actions[3].c_str()) == 0) { //#LOG
            //break if not member
            //send list of all active agents
            //list contains IP and active duration
            WriteLog("Responded to agent \"" + ipAddress + "\" with \"log.txt.\"");

            //agent active
            if (std::find(agents.begin(), agents.end(), ipAddress) != agents.end()) {

                WriteLog("Responded to agent \"" + ipAddress + "\" with \"log.txt.\"");
                std::ifstream in;
                in.open("log.txt", std::ifstream::in);
                if (!in.is_open()) {
                    std::cerr << "\nNot log.txt file\n";
                    return -1;
                }

                while (!in.eof()) {
                    memset(buffer, 0, MAXBUFFER);
                    in.read(buffer, MAXBUFFER);
                    write(client_socket, buffer, sizeof(buffer));
                }
                in.close();
            } else {
                WriteLog("No response is supplied to agent \"" + ipAddress + "\"");
            }
        }

        write(client_socket, str.c_str(), str.length());


    }
    //send message
    send(client_socket, server_message, sizeof(server_message), 0);
//close socket
    close(server_socket);

    return 0;
}