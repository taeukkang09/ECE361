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
#include <fcntl.h>

struct packet{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char filename[1000];
    char filedata[1000];
};

double uniform_rand(){
    /*
    Generates a uniformly distributed random number between 0 and 1
    */
    return (rand() % 100)/100;
}

void parse_packet(struct packet *packet, char* data){
    /*
    Parses packet read into data into packet
    */
    //Using strtok
    char buffer[4096];
    strcpy(buffer, data);
    char *ptr = strtok(buffer, ":");
    int i = 0, j = 0;

    for(i = 0; i < 4; i++){
        j += (int) strlen(ptr);
        if(i == 0){
            packet->total_frag = atoi(ptr);
        }
        else if(i == 1){
            packet->frag_no = atoi(ptr);
        }
        else if(i == 2){
            packet->size = atoi(ptr);
        }
        else if(i == 3){
            strcpy(packet->filename, ptr);
        }
        ptr = strtok(NULL, ":");
    }
    for(i = 0; i < packet->size; i++){
        packet->filedata[i] = data[j+i+4];
    }
}

int main(int argc, char* argv[]) {
    // Check if proper number of arguments passed in.
    if (argc != 2) {
        fprintf(stderr,"Usage: <UDP listen port>\n");
        exit(1);
    }

    //Read in packet, send ack or drop, repeat
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
    char buffer2[4096];
    struct sockaddr_in client_addr;             //Create client address structure
    socklen_t client_addr_len = sizeof(client_addr);
    int fd, i;
    char *ptr;

    //Prepare packet struct
    struct packet packet_arr[4096];
    int total_frag = 1;
    int cur_frag = 0;

    //Read in packet, send ack or nack, repeat
    //Write to filestream at the end
    while(cur_frag < total_frag){
        //Read and check for errors
        if(recvfrom(sock_fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &client_addr, &client_addr_len) < 0) {
            continue;
        }
        //Determine whether to drop the packet or not
        if(uniform_rand() < 1e-2){
            printf("DROPPING PACKET\n");
            continue;
        }

        //Determine which packet number this is
        //We can use strcpy here since we won't get to the data field
        strcpy(buffer2, buffer);
        ptr = strtok(buffer2, ":");
        ptr = strtok(NULL, ":");

        //Populate cur_packet
        parse_packet(&packet_arr[atoi(ptr)], buffer);
        total_frag = packet_arr[atoi(ptr)].total_frag;
        cur_frag ++;    //ASSUMING NO REPEATED PACKETS

        //Send ACK
        sprintf(buffer, "%d", packet_arr[atoi(ptr)].frag_no);
        if(sendto(sock_fd, buffer, strlen(buffer), 0, (struct sockaddr *) &client_addr, client_addr_len) == -1){
            int err = errno;
            perror("sending ACK");
            exit(err);
        };
    }

    //Setup with user read/write/execute access, write, create, and overwrite mode.
    fd = open(packet_arr[1].filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);

    //Write to fd
    for(cur_frag = 1; cur_frag <= total_frag; cur_frag++){
        if(write(fd, packet_arr[cur_frag].filedata, packet_arr[cur_frag].size) == -1){
            int err = errno;
            perror("writing to fd");
            exit(err);  
        }
    };
    close(fd);
}
