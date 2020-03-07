Author: Theodora Arnold
Project 2: File Transfer
CS372_400_F2019 - Introduction to Networks

Instructions:
1. Add execution permissions to ftclient
   $ make client

2. Compile the ftserver
   $ make server

3. Start the server (port example 20202)
   $ ftserver <port number P>

4a. Client requests directory list 
    $ ftclient.py <host> <port number P> -l <port number Q>
    host example: flip1
    port number P - port number on which server is currently running
    port number Q* - new port number for data connection

4b. Client requests file transfer 
    $ ftclient.py <host> <port number P> -g <filename> <port number Q>
    host example: flip1
    port number P - port number on which server is currently running
    filename - must be file in server's current directory and not have 
               matching file in the client's current directory 
    port number Q* - new port number for data connection

5. Close the server by sending SIGINT (ctrl+c)
 
