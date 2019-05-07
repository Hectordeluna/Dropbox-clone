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
char buffer[BUFSIZ];
int file_size;
FILE *received_file;
int remain_data = 0;

void saveArchivo()
{

    recv(client_socket, buffer, BUFSIZ, 0);
    char *fileName = buffer;
    fileName[strlen(fileName)] = '\0';
    printf("%s \n", fileName);
    received_file = fopen(fileName, "w+");
    if (received_file == NULL)
    {
        fprintf(stderr, "Failed to open file --> %s\n", strerror(errno));

        exit(EXIT_FAILURE);
    }
    /* Receiving file size */
    recv(client_socket, buffer, BUFSIZ, 0);
    file_size = atoi(buffer);
    //fprintf(stdout, "\nFile size : %d\n", file_size);

    remain_data = file_size;

    while ((remain_data > 0) && ((len = recv(client_socket, buffer, BUFSIZ, 0)) > 0))
    {
        fwrite(buffer, sizeof(char), len, received_file);
        remain_data -= len;
        fprintf(stdout, "Receive %d bytes and we hope :- %d bytes\n", len, remain_data);
    }
    len = 0;
    fclose(received_file);
    bzero(buffer, sizeof(buffer));
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