/*
 * echoclient.c - An echo client
 */
#include "csapp.h"

#define MAX_NAME_LEN 256

int main(int argc, char **argv)
{
    int clientfd, port, fd, n, file_size;
    char *host, buf[MAXBUF], buf_file_name[MAX_NAME_LEN], buf_file_content[MAXBUF];
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

    if (Fgets(buf_file_name, MAX_NAME_LEN, stdin) != NULL) {

        //There is a '\n' at the end of the line that need to be removed before sending it to the server
        buf_file_name[strlen(buf_file_name) - 1] = '\0';        

        Rio_writen(clientfd, buf_file_name, strlen(buf_file_name));

        //Opening (creating) a file to store server's response
        fd = Open(buf_file_name, O_WRONLY | O_CREAT, 0644);
        
        if ((n = Rio_readnb(&rio, buf, MAXBUF)) != 0) {
            file_size = atoi(buf);
        }
        else
        {
            fprintf(stderr, "Error: Couldn't get file size\n");
            Close(fd);
            Close(clientfd);
            exit(0);
        }

        while((n = Rio_readnb(&rio, buf_file_content, MAXBUF)) != 0) {
            printf("client received %u bytes\n", (unsigned int)n);
            Rio_writen(fd, buf_file_content, n);
            file_size -= n;
        }
        
        if (file_size != 0) {
            fprintf(stderr, "Error: File missing parts\n");
        }
        Close(fd);
    }


    Close(clientfd);
    exit(0);
}
