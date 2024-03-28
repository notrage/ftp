#include "csapp.h"

#define MAX_NAME_LEN 256
#define MAX_BUF_CONTENT 512
#define NB_PROC 3
#define SERVER_DIR "./fichiers/"

// pour pouvoir gérer la terminaison propre du serveur
int fd_proc_using[NB_PROC], table_proc[NB_PROC];
int nb_proc_restant = NB_PROC;

// give the index of the process in the table_proc
int get_idx_proc(){
    for (int i = 1; i < NB_PROC+1; i++) {
        if (table_proc[i] == getpid()) {
            return i;
        }
    }
    return -1;
}

// handler for the signal SIGCHLD
void sigchildhandler(int sig){
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        nb_proc_restant--;
    }
    // if all the children are terminated, the server can be terminated properly
    if(nb_proc_restant == 0)
        exit(0);
}

// handler for the signal SIGINT
void siginthandler(int sig){
    int i = get_idx_proc();
    if (fd_proc_using[i] != -1) {
        if (close(fd_proc_using[i]) < 0) {
            fprintf(stderr, "Error: can't close the connection\n");
            exit(0);
        }
    }
    if (close(fd_proc_using[0]) < 0) {
        fprintf(stderr, "Error: can't close the connection\n");
        exit(0);
    }
    exit(0);
}

// create the children processes
void creer_fils(){
    pid_t pid;

    table_proc[0] = getpid();
    for (int i = 1; i < NB_PROC+1; i++) {
        if ((pid = Fork()) == 0){
            Signal(SIGINT, siginthandler);
            break;
        }
        else {
            table_proc[i] = pid;
            fd_proc_using[i] = -1;
        }
    }
    
    return;
}

// function to treat the client's request
void traiter_demande(int connfd) {

    char buf_file_name[MAX_NAME_LEN], buf_file_content[MAX_BUF_CONTENT],
         buf_file_path[MAX_NAME_LEN] = SERVER_DIR;
    uint32_t buf_taille[1];
    rio_t rio;
    size_t n;
    int fd;
    
    struct stat *stats = malloc(sizeof(struct stat));

    // reading number of char of this file name and file name 
    if ((rio_readn(connfd, buf_taille, sizeof(uint32_t)) != 0)) {

        if ((n = rio_readn(connfd, buf_file_name, buf_taille[0])) != 0) {
            
            // checking if transfer was complete
            if (strlen(buf_file_name)+1 != buf_taille[0]) {
                fprintf(stderr, "Error: invalid name received\n");
                return;
            }
    
            // adding the path to open the asked file
            strcat(buf_file_path, buf_file_name);
            printf("file to send : %s\n", buf_file_path);
    
            // opening the asked file and initialisation of the buffer on opened file
            fd = open(buf_file_path, O_RDONLY, 0);
            if (fd == -1) {
                fprintf(stderr, "Error: can't open the file: %s\n", buf_file_path);
                return;
            }

            // intinialisation of the rio buffer
            rio_readinitb(&rio, fd); 
    
            // getting the size of the asked file
            if (fstat(fd, stats) == -1) {
                fprintf(stderr,"Error:  fstat bad address\n");
                return;
            }
            buf_taille[0] = stats->st_size;

            // sending it to the client
            if (rio_writen(connfd, buf_taille, sizeof(uint32_t)) < 0) {
                fprintf(stderr, "Error: can't send the file size\n");
                return;
            }
            free(stats);

            // while we can read something in the opened file
            while((n = rio_readnb(&rio, buf_file_content, MAX_BUF_CONTENT)) != 0) {
            
                // sending to the client the readen file content
                if (rio_writen(connfd, buf_file_content, n) < 0) {
                    fprintf(stderr, "Error: can't send the file content\n");
                    return;
                }
                printf("server read and sent %u bytes\n", (unsigned int)n);
            }
            if (close(fd) < 0) {
                fprintf(stderr, "Error: can't close the file: %s\n", buf_file_path);
                return;
            }
        } else {
            fprintf(stderr, "Error: invalid name received\n");
            return;
        }
    } else {
        fprintf(stderr, "Error: invalid name lenght received\n");
        return;
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
    
    if ((listenfd = open_listenfd(port)) < 0) {
        fprintf(stderr, "Error: can't open the listenfd\n");
        exit(0);
    }

    // father's pid is stored in table_proc[0]
    creer_fils();

    if (getpid() != table_proc[0]) {
        while (1) {
            while ((connfd = accept(listenfd, (SA *)&clientaddr, &clientlen)) < 0);

            fd_proc_using[get_idx_proc(table_proc)] = connfd;

            /* determine the name of the client */
            if (getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAX_NAME_LEN, 0, 0, 
                    0) != 0) {
                fprintf(stderr, "Error: can't determine the name of the client\n");
                exit(0);
            }

            /* determine the textual representation of the client's IP address */
            if (inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string, INET_ADDRSTRLEN) == NULL) {
                fprintf(stderr, "Error: can't determine the IP address of the client\n");
                exit(0);
            }

            printf("server connected to %s (%s)\n", client_hostname, client_ip_string);

            traiter_demande(connfd);
            if (close(connfd) < 0) {
                fprintf(stderr, "Error: can't close the connection\n");
                exit(0);
            }
            fd_proc_using[get_idx_proc(table_proc)] = -1;
            printf("client ended connection\n");
        }
    } else {
        fd_proc_using[0] = listenfd;
        if (close(listenfd) < 0) {
            fprintf(stderr, "Error: can't close the connection\n");
            exit(0);
        }
        pause();
    }
    exit(0);
}