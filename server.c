/*
 * echoserveri.c - An iterative echo server
 */

#include "csapp.h"

#define MAX_NAME_LEN 256
#define NB_PROC 2

void sighandler(int sig){
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void echo(int connfd);

void creer_fils(int *proc_table){
    pid_t pid;

    proc_table[0] = getpid();
    for (int i = 1; i < NB_PROC+1; i++) {
        if ((pid = Fork()) == 0)
            break;
        else {
            proc_table[i] = pid;
            printf("created %i'th child\n", pid);
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
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    int table_proc[NB_PROC];
    int port;

    Signal(SIGCHLD, sighandler);
    
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    port = atoi(argv[1]);

    clientlen = (socklen_t)sizeof(clientaddr);    

    listenfd = Open_listenfd(port);

    //Father's pid is stored in table_proc[0]
    creer_fils(table_proc);

    while (1) {
        if(getpid() != table_proc[0]) //fils
        { 
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            if (connfd != -1) 
            {
                /* determine the name of the client */
                Getnameinfo((SA *) &clientaddr, clientlen,
                        client_hostname, MAX_NAME_LEN, 0, 0, 0);

                /* determine the textual representation of the client's IP address */
                Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                        INET_ADDRSTRLEN);

                printf("server connected to %s (%s)\n", client_hostname,
                        client_ip_string);

                echo(connfd);
                Close(connfd);
                printf("client ended connection\n");
            }
        }  // sinon pere fait rien
        sleep(1);
    }
    exit(0);
}