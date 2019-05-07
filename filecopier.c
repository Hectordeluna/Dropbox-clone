#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/uio.h>

struct Archivo
{
    int id;
    char title[50];
    time_t modifiedTime;
    int using;
};

struct Archivo archivos[50];
int count = 0;

#define PORT_NUMBER 5000
#define SERVER_ADDRESS "127.0.0.1"

int server_socket;
int peer_socket;
socklen_t sock_len;
ssize_t len;
struct sockaddr_in server_addr;
struct sockaddr_in peer_addr;
char file_size[256];
struct stat file_stat;
off_t offset;
int remain_data;

void sendArchivo(int index)
{
    int fd;
    int sent_bytes = 0;

    fd = open(archivos[index].title, O_RDONLY);
    if (fd == -1)
    {
        fprintf(stderr, "Error opening file --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }

    /* Get file stats */
    if (fstat(fd, &file_stat) < 0)
    {
        fprintf(stderr, "Error fstat --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "File Size: \n%d bytes\n", file_stat.st_size);

    sprintf(file_size, "%d", file_stat.st_size);

    len = send(peer_socket, archivos[index].title, sizeof(archivos[index].title) + 1, 0);
    if (len < 0)
    {
        fprintf(stderr, "Error on sending greetings --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }

    /* Sending file size */
    len = send(peer_socket, file_size, sizeof(file_size), 0);
    if (len < 0)
    {
        fprintf(stderr, "Error on sending greetings --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server sent %d bytes for the size\n", len);

    offset = 0;
    remain_data = file_stat.st_size;
    /* Sending file data */
    while (((sendfile(fd, peer_socket, sent_bytes, &sent_bytes, NULL, 0)) == 0) && (remain_data > 0))
    {
        fprintf(stdout, "1. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, offset, remain_data);
        remain_data -= sent_bytes;
        fprintf(stdout, "2. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, offset, remain_data);
    }
    close(fd);
}

void setModifiedTime(int index)
{
    struct stat file_stat;

    int err = stat(archivos[index].title, &file_stat);
    archivos[index].modifiedTime = file_stat.st_mtime;
}

void createArchivo(char *name, int index)
{
    strcpy(archivos[index].title, name);
    setModifiedTime(index);
    printf("%s - Es nuevo\n", archivos[index].title);
    count++;
    archivos[index].using = 1;
}

int file_is_modified(char *path, time_t oldMTime)
{

    struct stat file_stat;
    int err = stat(path, &file_stat);
    if (err != 0)
    {
        printf("Error %i ", err);
    }

    return file_stat.st_mtime > oldMTime;
}

void readFiles()
{
    int len;
    struct dirent *pDirent;
    DIR *pDir;

    // abre la direccion
    pDir = opendir(".");
    if (pDir == NULL)
    {
        return;
    }

    while ((pDirent = readdir(pDir)) != NULL)
    {
        int i;

        if (strcmp(".", pDirent->d_name) == 0 || strcmp("..", pDirent->d_name) == 0)
            continue;

        for (i = 0; i < count; i++)
        {

            if (strcmp(archivos[i].title, pDirent->d_name) == 0)
            {
                archivos[i].using = 1;
                break;
            }
        }

        if (i == count)
        {
            createArchivo(pDirent->d_name, i);
            sendArchivo(i);
        }
        else if (file_is_modified(archivos[i].title, archivos[i].modifiedTime) == 1)
        {
            setModifiedTime(i);
            sendArchivo(i);
            printf("%s - Fue cambiado\n", archivos[i].title);
        }
        else
        {
            printf("%s\n", archivos[i].title);
        }
    }

    for (int i = 0; i < count; i++)
        if (archivos[i].using == 0)
        {
            printf("%s - BORRADO\n", archivos[i].title);
            continue;
        }
        else
        {
            archivos[i].using = 0;
        }
}

int main()
{
    // Cambia el dir al folder que usamos
    chdir("Folder1");

    /* Create server socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        fprintf(stderr, "Error creating socket --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }

    /* Zeroing server_addr struct */
    memset(&server_addr, 0, sizeof(server_addr));
    /* Construct server_addr struct */
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_ADDRESS, &(server_addr.sin_addr));
    server_addr.sin_port = htons(PORT_NUMBER);

    /* Bind */
    if ((bind(server_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))) == -1)
    {
        fprintf(stderr, "Error on bind --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }

    /* Listening to incoming connections */
    if ((listen(server_socket, 5)) == -1)
    {
        fprintf(stderr, "Error on listen --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }

    sock_len = sizeof(struct sockaddr_in);
    /* Accepting incoming peers */
    peer_socket = accept(server_socket, (struct sockaddr *)&peer_addr, &sock_len);
    if (peer_socket == -1)
    {
        fprintf(stderr, "Error on accept --> %s", strerror(errno));

        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Accept peer --> %s\n", inet_ntoa(peer_addr.sin_addr));

    // Loop infinito para leer cada 5 segundos los files
    int tr = 0;
    while (tr == 0)
    {
        readFiles();
        sleep(5);
    }

    close(peer_socket);
    close(server_socket);

    return 0;
}