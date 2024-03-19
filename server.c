/*
 * echoserveri.c - An iterative echo server
 */

#include "csapp.h"

#define MAX_NAME_LEN 256
#define NB_PROC 10

void sighandler(int sig){
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void echo(int connfd);

void creer_fils(int *tabfils){
    int n;

    for (int i = 0; i < NB_PROC; i++) {
        if ((n = Fork()) == 0)
            break;
        else
            tabfils[i] = n;
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
    int tabfils[NB_PROC];

    Signal(SIGCHLD, sighandler);
    
    if (argc != 1) {
        fprintf(stderr, "usage: %s\n", argv[0]);
        exit(0);
    }

    clientlen = (socklen_t)sizeof(clientaddr);    

    creer_fils(tabfils);

    listenfd = Open_listenfd(50000);
    while (1) {
        
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        if(Fork() == 0) { //fils
            Close(listenfd);
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
            exit(0);
        }

        Close(connfd); 
    }
    exit(0);
}