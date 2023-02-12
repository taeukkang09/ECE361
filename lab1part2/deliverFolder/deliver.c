#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

#define MAX_LEN 512

int main(int argc, char *argv[]) {
    // Check if proper number of arguments passed in.
    if (argc != 3) {
        fprintf(stderr,"Usage: deliver <server address> <server port number>\n");
        exit(1);
    }

    // Parse server address and server port from arguments.
    char *server_address = argv[1];
    int server_port = atoi(argv[2]);

    char input[MAX_LEN];
    char filename[MAX_LEN];

    // Read filename from user input.
    printf("Enter the name of file to transfer(ftp <filename>): ");
    fgets(input, sizeof(input), stdin);

    // Remove newline characters from input. "strcspn" returns the index of the first newline character, then removes it.
    input[strcspn(input, "\n")] = 0;

    // Use space char as the delimiter, and tokenize the input to parse the "ftp" and filename.
    char* token = strtok(input, " ");

    // Check if the first token is "ftp".
    if (strcmp(token, "ftp") == 0) {
        // Get the next token, the filename value.
        token = strtok(NULL, " ");
        strncpy(filename, token, MAX_LEN);
        // Check if the file exists.
        if(access(token, F_OK) == -1) {
            printf("The filename \"%s\" does not exist.\n", filename);
            exit(1);
        } 
    } else {
        printf("Command must start with \"ftp\".\n");
        exit(1);
    }

    // Creates a UDP socket. 
    // AF_INET = IPv4 Internet Protocols, SOCK_DGRAM = UDP sockets, the protocol identifier is 0 for UDP.
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("Error creating socket: ");
        exit(1);
    }

    // Set up server address structure
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port); // Converts IP address from host byte order -> network byte order.
    if (inet_aton(server_address, &server_addr.sin_addr) == 0) { // Converts dot-notation server address into binary form. 
        printf("Invalid server address.\n");
        close(sock_fd);
        exit(1);
    }

    // Send "ftp" message to server
    char message[] = "ftp";
    if (sendto(sock_fd, message, strlen(message), 0, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error sending message: ");
        close(sock_fd);
        exit(1);
    }

    // Receive response from server
    char response[MAX_LEN];
    socklen_t server_addr_len = sizeof(server_addr);
    if (recvfrom(sock_fd, response, MAX_LEN, 0, (struct sockaddr *) &server_addr, &server_addr_len) < 0) {
        perror("Error receiving message");
        close(sock_fd);
        exit(1);
    }

    // Print appropriate message based on server response
    if (strcmp(response, "yes") == 0) {
        printf("File transfer started.\n");
    } else {
        printf("File transfrer rejected by server.\n");
    }

    close(sock_fd);
    return 0;
}