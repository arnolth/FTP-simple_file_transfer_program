/*******************************************************************************
 * Author: Theodora Arnold
 * Program: Programming Assignment #2 - simple file transfer system
 * Class: CS_372_400_F2019 - Introduction to Networks
 * Desc: Server Side of the ftp system
 * Last Modified: 12.01.2019
 * Sources: http://beej.us/guide/bgnet/html/
 *          https://www.geeksforgeeks.org/socket-programming-cc/
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <unistd.h>

#define BUFFER 255

/* function prototypes  - start up, handle request */
int startup (char*);
void handleReq(int);
void sendList(int, int);
void sendFile(char*, int, int);
int initDataConnection(char*);

/* main method, takes argument of port number for control connection */
int main (int argc, char *argv[]) {
    int sockFD, newSockFD;
    struct sockaddr_storage clientAddr;
    socklen_t addrSize;

    sockFD = startup(argv[1]);
    printf("Server open on %s\n", argv[1]);

    /* wait for connections until reviece SIGINT (^C) from keyboard */
    while (1){
        addrSize = sizeof(clientAddr);
        /* accept incoming control connection */
        newSockFD = accept(sockFD, (struct sockaddr*) &clientAddr, &addrSize);
        if (newSockFD == -1){
            fprintf(stderr, "accept() failure\n");
            exit(1);
        } else {
            printf("control connection accepted\n");
        }
        handleReq(newSockFD);
        close(newSockFD);
    }

    return 0;
}

/*******************************************************************************
 * startup - creates, binds and listens given a port number in the form of a 
 *           char* 
 * param: takes char* of the port number for creating a socket
 * returns: integer of file descriptor of the socket which is listening for up 
 *          to 3 connections
*******************************************************************************/
int startup (char* port){
    int check, sockFD;
    int yes = 1;
    struct addrinfo hints,
                    *clientAddr;

    memset(&hints, '\0', sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    check = getaddrinfo(NULL, port, &hints, &clientAddr);
    if (check){
        fprintf(stderr, "getaddrinfo() failure\n");
        exit(1);
    }
    /* create socket*/
    sockFD = socket(clientAddr->ai_family, clientAddr->ai_socktype, clientAddr->ai_protocol);
    if (sockFD == -1){
        fprintf(stderr, "socket() failure\n");
        exit(1);
    }

    check = setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (check == -1){
        fprintf(stderr, "setsockopt() failure\n");
        exit(1);
    }

    /* bind socket */
    check = bind(sockFD, clientAddr->ai_addr, clientAddr->ai_addrlen);
    if (check == -1){
        close(sockFD);
        fprintf(stderr, "bind() failure\n");
        exit(1);
    }

    /* listen */
    check = listen(sockFD, 3);
    if (check == -1){
        close(sockFD);
        fprintf(stderr, "listen() failure\n");
        exit(1);
    }

    /* free addrinfo struct */
    freeaddrinfo(clientAddr);
    return sockFD;
}

/*******************************************************************************
 * handleReq - parses request from client and sends corresponding data, either 
 *             a list files in the current directory or the contents of one of 
 *             those files. 
 * Param: int that is the file descriptor for the control connection
*******************************************************************************/
void handleReq(int sockFD){
    char request[BUFFER];
    char *filename;
    char *dataPort;
    int dataFD;

    memset(request, '\0', BUFFER);

    if(recv(sockFD, request, BUFFER, 0) == -1){
        /* error in receiving request, exit */
        fprintf(stderr, "error receiving request\n");
        exit(1);
    }

    /* get command */
    char *token = strtok(request, " ");
    if (token == NULL){
        return;
    }

    if (strstr(token, "l")){
        /* if client request list (-l) */
        dataPort = strtok(NULL, " ");
        printf("List directory requested on port %s.\n", dataPort);
        /* create data port socket and send list */
        dataFD = initDataConnection(dataPort);
        sendList(dataFD, sockFD);
        close(dataFD);
    } else if (strstr(token, "g")){
        /* if client request get file (-g) */
        filename = strtok(NULL, " ");
        dataPort = strtok(NULL, " ");
        printf("File \"%s\" requested on port %s.\n", filename, dataPort);
        dataFD = initDataConnection(dataPort);
        sendFile(filename, dataFD, sockFD);
        close(dataFD);
    } else {
        /* invalid command */
        char *errMsg = "command not recognized\n";
        send(sockFD, errMsg, strlen(errMsg), 0);
    }
}

/*******************************************************************************
 * sendList - makes a string of all filenames in the current directory and sends
 *            them to the client
 * Param: dataFD is the file descriptor for the data connection
 *        sockFD is the file descriptor for the control connection
*******************************************************************************/
void sendList(int dataFD, int sockFD){
    DIR *dir;
    struct dirent *file;
    char list[2048];

    memset(list, '\0', sizeof(list));
    /* open current directory */
    /* source: https://www.geeksforgeeks.org/c-program-list-files-sub-directories-directory/ */
    /*        *used in cs344 Prog2 */
    dir = opendir(".");
    if (dir == NULL){
        /* send error message to client */
        char *errMsg = "could not open directory\n";
        fprintf(stderr, errMsg);
    } else {
        /* add files to list (array) */
        while ((file = readdir(dir)) != NULL){
            /* if file is not . or .. */
            if (strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0){
                strcat(list, file->d_name);
                strcat(list, "\n");
                if (strlen(list) > 2000){
                    /* resize list array */
                }
            }
        }
        /* remove the final newline */
        list[strlen(list) - 1] = '\0';

        /* send directory contents to client */
        printf("Sending directory contents to client\n");
        send(dataFD, list, strlen(list), 0);
    }
}

/*******************************************************************************
 * sendFile - sends the contents of a requested file to the client, if the file
 *            doesn't exists, sends FILE NOT FOUND message to the client
 * Param: filename - name of file client has requested
 *        dataFD is the file descriptor for the data connection
 *        sockFD is the file descriptor for the control connection
*******************************************************************************/
void sendFile(char *filename, int dataFD, int sockFD){
    FILE *file;
    char *lineBuf = NULL;
    size_t lineBufSize = 0;
    ssize_t lineSize;

    file = fopen(filename, "r");
    if (!file){
        /* if file fails to open send message to the client */
        char *errMsg = "FILE NOT FOUND.";
        fprintf(stderr, errMsg);
        fprintf(stderr, " Sending error message to client.\n");
        send(sockFD, errMsg, strlen(errMsg), 0);
    } else {
        /* file open succeeded, send file contents to client */
        printf("Sending \"%s\" to client.\n", filename);
        send(sockFD, filename, strlen(filename), 0);

        /* source: https://riptutorial.com/c/example/8274/get-lines-from-a-file-using-getline-- */
        lineSize = getline(&lineBuf, &lineBufSize, file);
        /* loop through the whole file line by line */
        while (lineSize >= 0){
            /* send the line to the client */
            send(dataFD, lineBuf, lineSize, 0);
            /* get next line */
            lineSize = getline(&lineBuf, &lineBufSize, file);
        }

        /* free line buffer from getline function and close the file */
        free(lineBuf);
        fclose(file);
    }
}

/*******************************************************************************
 * initDataConnection - creates, binds and listens given a port number in the 
 *                      form of a char* 
 * param: port is the char* of the port number for creating a socket for the 
 *             data connection
 * returns: the integer value file descriptor of the data connection 
*******************************************************************************/
int initDataConnection(char* port){
    int sockFD, dataFD;
    struct sockaddr_storage clientAddr;
    socklen_t addrSize;

    /* start up data connection */
    sockFD = startup(port);
    addrSize = sizeof(clientAddr);

    /* accept data connection */
    dataFD = accept(sockFD, (struct sockaddr*) &clientAddr, &addrSize);
    if (dataFD == -1){
        fprintf(stderr, "accept() failure\n");
        exit(1);
    } else {
        printf("data connection accepted\n");
    }
    return dataFD;
}
