/* Client code */
/* TODO : Modify to meet your need */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

#define PORT_NUMBER 5000
#define SERVER_ADDRESS "127.0.0.1"

int client_socket;
ssize_t len;
struct sockaddr_in remote_addr;
char buffer[1024];
int file_size;
FILE *received_file;
int remain_data = 0;

void saveArchivo() {
    FILE *fp;
    char fname[50];
    int received;
    int filesize;
    read(client_socket, fname, 50);
    read(client_socket, &received, sizeof(received));
    filesize = ntohl(received);

    printf("%s recibido\n", fname);
    printf("filesize: %d\n", filesize);
    fp = fopen(fname, "w");
    int bytesRcv;
    if(fp == NULL) {
        printf("Error opening file\n");
    } else {
        while((bytesRcv = read(client_socket, buffer, filesize)) > 0) {
            if (bytesRcv == 1) {
                break;
            } else {
                fwrite(buffer, 1, bytesRcv, fp);
            }
        }
    }
    fclose(fp);
}

int main(int argc, char **argv)
{
    chdir("Folder2");

    /* Zeroing remote_addr struct */
    memset(&remote_addr, 0, sizeof(remote_addr));

    /* Construct remote_addr struct */
    remote_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_ADDRESS, &(remote_addr.sin_addr));
    remote_addr.sin_port = htons(PORT_NUMBER);

    /* Create client socket */
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        fprintf(stderr, "Error creating socket --> %s\n", strerror(errno));

        exit(EXIT_FAILURE);
    }

    /* Connect to the server */
    if (connect(client_socket, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1)
    {
        fprintf(stderr, "Error on connect --> %s\n", strerror(errno));

        exit(EXIT_FAILURE);
    }

    while (1)
    {
        saveArchivo();
    }
    close(client_socket);

    return 0;
}