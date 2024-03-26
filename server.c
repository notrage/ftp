/*
 * echoserveri.c - An iterative echo server
 */

#include "csapp.h"

#define MAX_NAME_LEN 256
#define NB_PROC 10

// pour pouvoir gérer la terminaison propre du serveur
int nb_proc_restant = NB_PROC;

void sighandler(int sig){
    // lorsqu'un fils meurt
    if (sig == SIGCHLD) {
        while (waitpid(-1, NULL, WNOHANG) > 0) {
            //printf("killing child n°%i\n", nb_proc_restant);
            nb_proc_restant--;
        }
        // si aucun fil ne reste en vie 
        if(nb_proc_restant == 0)
            exit(0);
    }
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
    char buf_file_name[MAX_NAME_LEN];
    char buf_file_path[MAXBUF] = "./fichiers/";
    char buf_file_content[MAXBUF];
    rio_t rio;
    int fd, file_size;
    struct stat *stats = NULL;

    // initialisation du descripteur de socket pour communiquer avec le client
    Rio_readinitb(&rio, connfd);

    // lecture du nom de fichier envoyé par le client
    if (Rio_readlineb(&rio, buf_file_name, MAX_NAME_LEN) != 0){

        // ajout de path devant le nom de fichier
        strcat(buf_file_path, buf_file_name);
        printf("file to send : %s\n", buf_file_path);

        // ouverture en lecture fichier demandé
        fd = Open(buf_file_path, O_RDONLY, 0);
        
        Fstat(fd, stats);
        file_size = stats->st_size;
        Rio_writen(connfd, itoa(file_size), strlen(itoa(file_size)));

        // initialisation du buffer rio pour lire dans le fichier demandé
        Rio_readinitb(&rio, fd); 

        // tant que l'on lit quelque chose dans le fichier on lit son contenu (8192 bytes max)
        while((n = Rio_readnb(&rio, buf_file_content, MAXBUF)) != 0) {

            // écriture sur le descripteur de socket du client
            Rio_writen(connfd, buf_file_content, n);
            printf("server read and sent %u bytes\n", (unsigned int)n);
        }
        Close(fd);
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