/*
 * echoserveri.c - An iterative echo server
 */

#include "csapp.h"

#define MAX_NAME_LEN 256
#define MAX_BUF_CONTENT 512
#define NB_PROC 3
#define SERVER_DIR "./fichiers/"

// pour pouvoir gérer la terminaison propre du serveur
int nb_proc_restant = NB_PROC;
int fd_proc_using[NB_PROC];
int table_proc[NB_PROC];

void sigchildhandler(int sig){
    // lorsqu'un fils meurt
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        //printf("killing child n°%i\n", nb_proc_restant);
        nb_proc_restant--;
    }
    // si aucun fil ne reste en vie 
    if(nb_proc_restant == 0)
        exit(0);
}

void echo(int connfd);

void creer_fils(int *proc_table){
    pid_t pid;

    proc_table[0] = getpid();
    for (int i = 1; i < NB_PROC+1; i++) {
        if ((pid = Fork()) == 0){
            Signal(SIGINT, SIG_DFL);
            break;
        }
        else {
            proc_table[i] = pid;
        }
    }
    
    return;
}

void traiter_demande(int connfd){
    size_t n;
    uint32_t buf_taille[1];
    char buf_file_name[MAX_NAME_LEN];
    char buf_file_path[MAX_NAME_LEN] = SERVER_DIR;
    char buf_file_content[MAX_BUF_CONTENT];
    rio_t rio;
    int fd;
    struct stat *stats = malloc(sizeof(struct stat));

    // reading number of char of this file name and file name 
    if ((Rio_readn(connfd, buf_taille, sizeof(uint32_t)) != 0)) 
    {
        if ((n = Rio_readn(connfd, buf_file_name, buf_taille[0])) != 0) 
        { 
            printf("%d\n", buf_taille[0]);
            printf("|%s|\n", buf_file_name);
            // checking if transfer was complete
            if (strlen(buf_file_name)+1 != buf_taille[0])
            {
                fprintf(stderr, "Error: invalid name received\n");
                return;
            }
    
            // adding the path to open the asked file
            strcat(buf_file_path, buf_file_name);
            printf("file to send : %s\n", buf_file_path);
    
            // opening the asked file and initialisation of the buffer on opened file
            fd = Open(buf_file_path, O_RDONLY, 0);
            Rio_readinitb(&rio, fd); 
    
            // getting the size of the asked file
            if (fstat(fd, stats) == -1) {
                fprintf(stderr,"Error:  fstat bad address\n");
                return;
            }
            buf_taille[0] = stats->st_size;
            // sending it to the client
            Rio_writen(connfd, buf_taille, sizeof(uint32_t)); 
            free(stats);

            // while we can read something in the opened file
            while((n = Rio_readnb(&rio, buf_file_content, MAX_BUF_CONTENT)) != 0) {
            
                // sending to the client the readen file content
                Rio_writen(connfd, buf_file_content, n);
                printf("server read and sent %u bytes\n", (unsigned int)n);
            }
            Close(fd);
        }
    }

    return;
}

/* 
 * Note that this code only works with IPv4 addresses
 * (IPv6 is not supported)
 */
int main(int argc, char **argv)
{
    int listenfd, connfd, port;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN], client_hostname[MAX_NAME_LEN];

    Signal(SIGCHLD, sigchildhandler);
    Signal(SIGINT, SIG_IGN);
    
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    port = atoi(argv[1]);
    clientlen = (socklen_t)sizeof(clientaddr);   
    listenfd = Open_listenfd(port);

    //Father's pid is stored in table_proc[0]
    creer_fils(table_proc);

    if (getpid() != table_proc[0]) {
        while (1) {
            while ((connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen)) < 0);
            /* determine the name of the client */
            Getnameinfo((SA *) &clientaddr, clientlen,
                    client_hostname, MAX_NAME_LEN, 0, 0, 0);

            /* determine the textual representation of the client's IP address */
            Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                    INET_ADDRSTRLEN);

            printf("server connected to %s (%s)\n", client_hostname,
                    client_ip_string);


            traiter_demande(connfd);
            Close(connfd);
            printf("client ended connection\n");
        }
    } else {
        pause();
    }
    exit(0);
}