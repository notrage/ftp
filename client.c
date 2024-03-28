/*
 * echoclient.c - An echo client
 */
#include "csapp.h"

#define MAX_NAME_LEN 256
#define MAX_BUF_CONTENT 512

int main(int argc, char **argv)
{
    size_t n;
    int clientfd, port, fd;
    uint32_t buf[1], file_size;
    char *host, buf_file_name[MAX_NAME_LEN], buf_file_content[MAX_BUF_CONTENT];
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

    if (Fgets(buf_file_name, MAX_NAME_LEN, stdin) != NULL) {

        //There is a '\n' at the end of the line that need to be removed before sending it to the server
        buf_file_name[strlen(buf_file_name)-1] = '\0';

        buf[0] = strlen(buf_file_name)+1;

        Rio_writen(clientfd, buf, sizeof(uint32_t));
        Rio_writen(clientfd, buf_file_name, buf[0]);

        //Opening (creating) a file to store server's response
        fd = Open(buf_file_name, O_WRONLY | O_CREAT, 0644);
        
        //Getting wanted size of wanted file
        if ((n = Rio_readn(clientfd, buf, sizeof(uint32_t))) != 0) {
            file_size = buf[0];
        }
        else
        {
            fprintf(stderr, "Error: Couldn't get file size\n");
            Close(fd);
            Close(clientfd);
            exit(0);
        }

        printf("%d\n", file_size);
        Rio_readinitb(&rio, clientfd);

        while((n = Rio_readnb(&rio, buf_file_content, MAX_BUF_CONTENT)) != 0) {
            printf("client received %u bytes\n", (unsigned int)n);
            Rio_writen(fd, buf_file_content, n);
            file_size -= n;
        }

        
        if (file_size == 0)
        {
            fprintf(stdout, "Ending transmission\n");
        }
        else if (file_size > 0)
        {
            fprintf(stderr, "Error: File missing parts\n");
            printf("%d\n", file_size);
        }
        else
        {
            fprintf(stderr, "Error: Too much bytes read\n");
        }
         
        Close(fd);
    }

    Close(clientfd);
    exit(0);
}