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
#include <time.h>

#define MAX_LEN 512
#define PACKET_LEN 1000

struct packet{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char filename[1000];
    char filedata[1000];
};

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
    struct packet packet;

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

    strcpy(packet.filename, filename);
    // packet.filename = filename;
    printf("Filename: %s\n", packet.filename);

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

    char packet_buf[4096];

    // Open the file in read mode.
    FILE *file = fopen(packet.filename, "rb");
    if (file == NULL) {
        perror("Error opening file: ");
        close(sock_fd);
        exit(1);
    }

    // Gets the file size.
    fseek(file, 0, SEEK_END); // SEEK_END points to the end of file. 
    int file_size = ftell(file); // ftell() returns the current file position. 
    rewind(file); 

    // Calculate number of fragments required to send the file
    packet.total_frag = file_size / 1000 + 1;
    packet.frag_no = 1;
    char buf[100];
    socklen_t server_addr_len = sizeof(server_addr);

    // Start timer.
    clock_t start, end;
    start = clock();
    double duration;

    while((packet.size = fread(packet.filedata, sizeof(char), PACKET_LEN, file))) {
        // Convert packet struct to string
        unsigned int len = sprintf(packet_buf, "%d:%d:%d:%s:", packet.total_frag, packet.frag_no, packet.size, packet.filename);

        for(int i = len; i < packet.size + len; i++){
            packet_buf[i] = packet.filedata[i - len];
        }

        if (sendto(sock_fd, packet_buf, packet.size + len, 0, (struct sockaddr*)&server_addr, server_addr_len) == -1) {
            printf("Error sending filedata 1.\n");
        }

        while(1) {
            int response_len = recvfrom(sock_fd, buf, 100, 0, (struct sockaddr *) &server_addr, &server_addr_len);
            if (response_len > 0) {   
                buf[response_len] = '\0';
                char frag_no_buf[100];
                sprintf(frag_no_buf, "%d", packet.frag_no);
                if (strcmp(buf, frag_no_buf) == 0) { // Server sends frag_no if ACK.
                    printf("Package transfer successful.\n");
                } else { // Server sends NACK.
                    printf("%s\n", buf);
                }
                if(atoi(frag_no_buf) == 1){
                    end = clock();
                    duration = ((double)end - start) * 1000 /CLOCKS_PER_SEC;
                }
                break;
            } else {
                if (sendto(sock_fd, packet_buf, packet.size + len, 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
                    printf("Error sending filedata 2.\n");
                }
            }
        }
        packet.frag_no += 1;
    }

    
    printf("RTT: %.3f ms.\n", duration);
        
    close(sock_fd);
    fclose(file);
    return 0;
}