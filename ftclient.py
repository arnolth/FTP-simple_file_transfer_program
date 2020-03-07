#!/bin/python3
"""
Author: Theodora Arnold
Program: Programming Assignment #2 - simple file transfer system
Class: CS_372_400_F2019 - Introduction to Networks
Desc: Client Side of the ftp system
Last Modified: 12.01.2019
Sources: Computer Networking - A Top-Down Approach v7: Section 2.7.2 Socket Programming with TCP

"""

from socket import *
from sys import *
import os


# functions - initiate contact, make request, recieve data

def initContact(argv):
    """
    Connect to server given host and port number in argv
    Param: argv[1] is the server's hostname
           argv[2] is the port number 
    Return: int file description of control connection
    """
    host = argv[1]
    port = int(argv[2])
    clientSocket = socket(AF_INET, SOCK_STREAM)
    clientSocket.connect((host, port))
    return clientSocket

def makeReq(connection, argv):
    """
    Parse argv and send request to server
    Param: connection is the int file descriptor of control connection
           argv[1] is the server's hostname
           argv[2] is the port number
           argv[3] is the command type should be either -l or -g
           argv[4] is filename if argv[3] == -g
                   or data connection port number if argv[3] == -l
           argv[5] if exists is data connection port number if argv[3] == -g 
    Return: True is request is sent
            False if bad request 
    """
    fileExists = False
    # if argv[3] (command) is not -l or -g return false
    if argv[3] != "-l" and argv[3] != "-g":
        return False

    # set up basic request
    request = argv[3] + " " + argv[4]

    # if request is get file
    if len(argv) == 6:
        #source: https://stackoverflow.com/questions/11968976/list-files-only-in-the-current-directory
        filename = argv[4]
        # make array of all files in current directory
        files = [f for f in os.listdir('.')]
        # loop through all files to see if file already exists
        for f in files:
            if filename == f:
                fileExists = True
        # add final argv to request to send to server
        request += " " + argv[5]


    if fileExists:
        print("File already exists. Remove file and try again.")
        return False # request not sent
    else:
        #send request to server and return True
        connection.send(bytes(request, "utf-8"))
        return True

def recData(ctrlConn, argv):
    """
    Gets data from the server and prints if it is a directory structure or 
    writes to file
    Param: ctrlConn is the int file descriptor of control connection
           argv[1] is the server's hostname
           argv[2] is the port number
           argv[3] is the command type should be either -l or -g
           argv[4] is filename if argv[3] == -g
                   or data connection port number if argv[3] == -l
           argv[5] if exists is data connection port number if argv[3] == -g
    """
    host = argv[1]
    port = int(argv[len(argv) - 1])
    command = argv[3];
    if len(argv) == 6:
        filename = argv[4]

    # create data connection
    dataSocket = socket(AF_INET, SOCK_STREAM)
    dataSocket.connect((host, port))
    if command == '-l':
        print("Receiving directory structure from server")
        data = dataSocket.recv(4096)
        print (data.decode("utf-8"))
    if command == '-g':
        # check to see if file exists on server
        checkCtrl = ctrlConn.recv(4096)
        checkCtrl = checkCtrl.decode("utf-8")
        if "FILE NOT FOUND" in checkCtrl:
            print(checkCtrl)
        else: 
            print ("Receiving " + checkCtrl + " from server.")
            data = dataSocket.recv(4096)
            writeFile = open(filename, "w")
            # while still receiving data continue receiving data
            while data: 
                writeFile.write(data.decode("utf-8"))
                data = dataSocket.recv(4096)
            # close the file and print complete message
            writeFile.close()
            print("File transfer complete.")


####           main          ####

if len(argv) < 5 or len(argv) > 6:
    print ("invalid number of arguments")
    exit(1)

command = argv[3]

# connect to server's control socket
ctrlConn = initContact(argv)

if command == '-l' or command == '-g':
    goodRequest = makeReq(ctrlConn, argv)
    if goodRequest:
        recData(ctrlConn, argv)
    else:
        ctrlConn.close()
else:
    print ("Invalid command.")
