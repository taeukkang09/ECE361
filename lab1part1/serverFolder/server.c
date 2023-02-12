#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char* argv[]) {
    // Check if proper number of arguments passed in.
    if (argc != 2) {
        fprintf(stderr,"Usage: <UDP listen port>\n");
        exit(1);
    }

    // Create socket address structure
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    //Create socket
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    //Bind socket to address
    if(bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket: ");
        exit(1);
    }

    //Process incoming message
    char buffer[4096];
    memset(buffer, '\0', sizeof(buffer));       //Clear buffer
    struct sockaddr_in client_addr;             //Create client address structure
    socklen_t client_addr_len = sizeof(client_addr);
    
    int bytes_received = recvfrom(sock_fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &client_addr, &client_addr_len);
    if(bytes_received < 0) {
        perror("Error receiving message: ");
        exit(1);
    }

    if(strcmp(buffer, "ftp") == 0) {
        sendto(sock_fd, "yes", sizeof("yes"), 0, (struct sockaddr *) &client_addr, client_addr_len);
    }
    else{
        sendto(sock_fd, "no", sizeof("no"), 0, (struct sockaddr *) &client_addr, client_addr_len);
    }  
}
