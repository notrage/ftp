/*
 * echoclient.c - An echo client
 */
#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd, port, fd, n;
    char *host, buf[MAXLINE], buf_file_content[MAXBUF];
    rio_t rio;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = atoi(argv[2]);

    /*
     * Note that the 'host' can be a name or an IP address.
     * If necessary, Open_clientfd will perform the name resolution
     * to obtain the IP address.
     */
    clientfd = Open_clientfd(host, port);
    
    /*
     * At this stage, the connection is established between the client
     * and the server OS ... but it is possible that the server application
     * has not yet called "Accept" for this connection
     */
    printf("client connected to server OS\n");
    
    Rio_readinitb(&rio, clientfd);

    if (Fgets(buf, MAXLINE, stdin) != NULL) {
        buf[strlen(buf) - 1] = '\0';
        Rio_writen(clientfd, buf, strlen(buf));
        fd = Open(buf, O_WRONLY | O_CREAT, 0644);
        while((n = Rio_readnb(&rio, buf_file_content, MAXBUF)) != 0) {
            printf("client read %u bytes\n", (unsigned int)n);
            Rio_writen(fd, buf_file_content, n);
            printf("client wrote %u bytes\n", (unsigned int)n);
        }
        Close(fd);
    }


    Close(clientfd);
    exit(0);
}
