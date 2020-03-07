.PHONY: ftserver ftclient

server:
	gcc ftserver.c -o ftserver

client: 
	chmod +x ftclient

