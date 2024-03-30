#include "csapp.h"

#define SERVER_DIR "./fichiers/"
#define MAX_BUF_CONTENT 512
#define MAX_NAME_LEN 256
#define NB_PROC 3

// to manage the clean termination of the server
int fd_proc_using[NB_PROC + 1], table_proc[NB_PROC + 1];
int nb_proc_restant = NB_PROC;

// give the index of the process in the table_proc
int get_idx_proc()
{
    for (int i = 1; i < NB_PROC + 1; i++)
    {
        if (table_proc[i] == getpid())
        {
            return i;
        }
    }
    return -1;
}

// handler for the signal SIGCHLD
void sigchildhandler(int sig)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
        nb_proc_restant--;
    }
    // if all the children are terminated, the server can be terminated properly
    if (nb_proc_restant == 0) 
    {
        exit(0);
    }
}

// handler for the signal SIGINT
void siginthandler(int sig)
{
    int idx = get_idx_proc();
    if (fd_proc_using[idx] != -1)
    {
        if (close(fd_proc_using[idx]) < 0)
        {
            fprintf(stderr, "Error: can't close the connection\n");
            exit(0);
        }
    }
    if (close(fd_proc_using[0]) < 0)
    {
        fprintf(stderr, "Error: can't close the connection\n");
        exit(0);
    }
    exit(0);
}

// create the children processes
void creer_fils()
{
    pid_t pid;
    int idx;

    table_proc[0] = getpid();
    for (idx = 1; idx < NB_PROC + 1; idx++)
    {
        if ((pid = Fork()) == 0)
        {
            Signal(SIGINT, siginthandler);
            break;
        }
        else
        {
            table_proc[idx] = pid;
            fd_proc_using[idx] = -1;
        }
    }

    return;
}

// function to treat the client's request
void traiter_requete(int connfd)
{
    printf("\nAble to send a file\n");
    char buf_file_name[MAX_NAME_LEN], buf_file_path[MAX_NAME_LEN],
        buf_file_content[MAX_BUF_CONTENT];
    uint32_t buf_int;
    int fd, i;
    rio_t rio;
    size_t n;

    // initialisation of the stats structure
    struct stat *stats = malloc(sizeof(struct stat));

    // reading number of char of this file name and file name
    while ((rio_readn(connfd, &buf_int, sizeof(uint32_t)) != 0))
    {
        buf_int = ntohl(buf_int);

        if ((n = rio_readn(connfd, buf_file_name, buf_int)) != 0)
        {

            // checking if transfer was complete
            if (strlen(buf_file_name) + 1 != buf_int)
            {
                fprintf(stderr, "Error: invalid name received\n");
                free(stats);
                return;
            }

            // formating the path to open the asked file
            strcpy(buf_file_path, SERVER_DIR);
            strcat(buf_file_path, buf_file_name);

            // opening the asked file and initialisation of the buffer on opened file
            fd = open(buf_file_path, O_RDONLY, 0);
            if (fd == -1)
            {
                fprintf(stderr, "Error: can't open the file: %s\n", buf_file_path);
                free(stats);
                return;
            }

            // intinialisation of the rio buffer
            rio_readinitb(&rio, fd);

            // getting the size of the asked file
            if (fstat(fd, stats) == -1)
            {
                fprintf(stderr, "Error:  fstat failed\n");
                free(stats);
                return;
            }

            // receiving the file size on the client side
            if (rio_readn(connfd, &buf_int, sizeof(uint32_t)) != 0)
            {
                // if file was already partially downloaded, we begin the download where it stopped
                lseek(fd, ntohl(buf_int), SEEK_SET);

                // calculate the remaining size to send
                buf_int = htonl(stats->st_size - ntohl(buf_int));
            }
            else
            {
                fprintf(stderr, "Error: can't get the file size\n");
                free(stats);
                return;
            }

            // sending it to the client
            if (rio_writen(connfd, &buf_int, sizeof(uint32_t)) < 0)
            {
                fprintf(stderr, "Error: can't send the file size\n");
                free(stats);
                return;
            }

            printf("server will send %u bytes:\n", ntohl(buf_int));

            i = 0;
            // while we can read something in the opened file
            while ((n = rio_readnb(&rio, buf_file_content, MAX_BUF_CONTENT)) != 0)
            {

                // sending to the client the readen file content
                if (rio_writen(connfd, buf_file_content, n) < 0)
                {
                    fprintf(stderr, "Error: can't send the file content\n");
                    free(stats);
                    return;
                }
                if (n == MAX_BUF_CONTENT)
                {
                    printf("\rserver read and sent %u bytes [x%d]", (unsigned int)n, ++i);
                }
                else 
                {
                    printf("\nserver read and sent %u bytes", (unsigned int)n);
                }
            }
            printf("\n");

            if (close(fd) < 0)
            {
                fprintf(stderr, "Error: can't close the file: %s\n", buf_file_path);
                free(stats);
                return;
            }
        }
        else
        {
            fprintf(stderr, "Error: invalid name received\n");
            free(stats);
            return;
        }
        printf("\nAble to send another file\n");
    }
    free(stats);
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

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    port = atoi(argv[1]);
    clientlen = (socklen_t)sizeof(clientaddr);

    if ((listenfd = open_listenfd(port)) < 0)
    {
        fprintf(stderr, "Error: can't open the listenfd\n");
        exit(0);
    }

    // father's pid is stored in table_proc[0]
    creer_fils();

    if (getpid() != table_proc[0])
    {
        while (1)
        {
            while ((connfd = accept(listenfd, (SA *)&clientaddr, &clientlen)) < 0)
                ;

            fd_proc_using[get_idx_proc(table_proc)] = connfd;

            /* determine the name of the client */
            if (getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAX_NAME_LEN, 0, 0,
                            0) != 0)
            {
                fprintf(stderr, "Error: can't determine the name of the client\n");
            }

            /* determine the textual representation of the client's IP address */
            if (inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string, INET_ADDRSTRLEN) == NULL)
            {
                fprintf(stderr, "Error: can't determine the IP address of the client\n");
            }

            printf("server connected to %s (%s)\n", client_hostname, client_ip_string);

            traiter_requete(connfd);
            if (close(connfd) < 0)
            {
                fprintf(stderr, "Error: can't close the connection\n");
            }
            fd_proc_using[get_idx_proc(table_proc)] = -1;
            printf("client ended connection\n");
        }
    }
    else
    {
        fd_proc_using[0] = listenfd;
        if (close(listenfd) < 0)
        {
            fprintf(stderr, "Error: can't close the connection\n");
        }
        pause();
    }
    exit(0);
}