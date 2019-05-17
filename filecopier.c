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

/* elimina del arreglo de archivos el archivo borrado de la carpeta*/
void eliminarArchivo(int index) {
    for(int i = index; i < count-1; i++) {
        archivos[i] = archivos[i+1];
    }
    count--;
}

/* imprime el nombre de todos los archivos registrados
   para comprobar que se estan rastreando de manera correcta */
void allFiles() {
    printf("TODOS LOS ARCHIVOS:\n");
    for(int i = 0; i < count; i++) {
        printf("%s\n", archivos[i].title);
    }
    printf("\n");
}

void sendArchivo(int i) {
    write(peer_socket, archivos[i].title, 50);
    FILE *fp = fopen(archivos[i].title, "rb");
    fseek(fp, 0L, SEEK_END);
    int filesize = ftell(fp);
    rewind(fp);
    printf("Filesize: %d\n", filesize);

    int data = htonl(filesize);
    write(peer_socket, &data, sizeof(data));

    if (fp == NULL) {
        printf("File open error");
    } else {
        while(1) {
            unsigned char buff[1024] = {0};
            int n = fread(buff, 1, filesize, fp);
            if (n > 0) {
                write(peer_socket, buff, n);
            } 
            if (n < filesize) {
                if (feof(fp)) {
                    write(peer_socket, "-", 1);
                }
                if (ferror(fp)) {
                    printf("Error reading\n");
                }
                break;
            }
        }
    }
    fclose(fp);
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
        }
        else if (file_is_modified(archivos[i].title, archivos[i].modifiedTime) == 1)
        {
            setModifiedTime(i);
            printf("%s - Fue cambiado\n", archivos[i].title);
            sendArchivo(i);
        }
        else
        {
            //printf("%s\n", archivos[i].title);
        }
    }

    for (int i = 0; i < count; i++)
        if (archivos[i].using == 0)
        {
            printf("%s - BORRADO\n", archivos[i].title);
            write(peer_socket, "eliminado", 50);
            write(peer_socket, archivos[i].title, 50);
            eliminarArchivo(i);
            continue;
        }
        else
        {
            archivos[i].using = 0;
        }
}

void startServer() {
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
    while (1)
    {
        readFiles();
        sleep(5);
    }

    close(peer_socket);
    close(server_socket);
}

int main(void) {
        /* Our process ID and Session ID */
        pid_t pid, sid;
        
        /* Fork off the parent process */
        pid = fork();
        if (pid < 0) {
                exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
                exit(EXIT_SUCCESS);
        }

        /* Change the file mode mask */
        umask(0);
                
        /* Open any logs here */        

        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }
        
        /* Change the current working directory */
        if ((chdir("/Users/diegomartinez/Desktop/PrograAvanzada/Mision6/Dropbox-clone")) < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }

        /* Close out the standard file descriptors */
        // close(STDIN_FILENO);
        // close(STDOUT_FILENO);
        // close(STDERR_FILENO);

        /* Daemon-specific initialization goes here */

        startServer();
        while(1) {
            sleep(10);
        }

   exit(EXIT_SUCCESS);
}