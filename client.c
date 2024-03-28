/*
 * echoclient.c - An echo client
 */
#include "csapp.h"

#define MAX_NAME_LEN 256
#define MAX_BUF_CONTENT 512
#define CLIENT_DIR "./files/"
//#define CURRENT_FILE "client"


int main(int argc, char **argv)
{
    size_t n;
    int clientfd, port, fd;
    uint32_t buf[1], file_size;
    char *host, buf_file_name[MAX_NAME_LEN], buf_file_content[MAX_BUF_CONTENT], 
        buf_file_path[MAX_NAME_LEN] = CLIENT_DIR;

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
    if ((clientfd = open_clientfd(host, port)) < 0) {
        fprintf(stderr, "Error: couldn't connect to the server\n");
        exit(0);
    }
    
    /*
     * At this stage, the connection is established between the client
     * and the server OS ... but it is possible that the server application
     * has not yet called "Accept" for this connection
     */
    printf("client connected to server OS\n");

    // reading the file name from the user
    if (fgets(buf_file_name, MAX_NAME_LEN, stdin) != NULL) {

        //There is a '\n' at the end of the line that need to be removed before sending it to the server
        buf_file_name[strlen(buf_file_name)-1] = '\0';

        buf[0] = strlen(buf_file_name)+1;

        // according to our protocol, we first send the size of the file name and the file name
        if (rio_writen(clientfd, buf, sizeof(uint32_t)) < 0) {
            fprintf(stderr, "Error: couldn't send the file name size\n");
            Close(clientfd);
            exit(0);
        }
        if (rio_writen(clientfd, buf_file_name, buf[0]) < 0) {
            fprintf(stderr, "Error: couldn't send the file name\n");
            Close(clientfd);
            exit(0);
        }

        // adding the path to open the asked file
        strcat(buf_file_path, buf_file_name);

        // opening (creating) a file to store server's response
        if ((fd = Open(buf_file_path, O_WRONLY | O_CREAT, 0644)) < 0) {
            fprintf(stderr, "Error: couldn't open the file: %s\n", buf_file_path);
            Close(clientfd);
            exit(0);
        }
        
        // getting wanted size of wanted file
        if ((n = rio_readn(clientfd, buf, sizeof(uint32_t))) != 0) {
            file_size = buf[0];
        } else {
            fprintf(stderr, "Error: Couldn't get file size\n");
            if (close(fd) < 0) {
                fprintf(stderr, "Error: couldn't close the file\n");
            }
            if (close(clientfd) < 0) {
                fprintf(stderr, "Error: couldn't close the connection\n");
            }
            exit(0);
        }

        printf("requesting to download: %d bytes...\n", file_size);
        sleep(1);

        // while we can read something from server
        while((n = rio_readn(clientfd, buf_file_content, MAX_BUF_CONTENT)) != 0) {

            printf("client received %u bytes from server\n", (unsigned int)n);

            // writing the readen file content to the opened file
            if (rio_writen(fd, buf_file_content, n) < 0) {
                fprintf(stderr, "Error: couldn't write the file content\n");
                if (close(fd) < 0) {
                    fprintf(stderr, "Error: couldn't close the file\n");
                }
                if (close(clientfd) < 0) {
                    fprintf(stderr, "Error: couldn't close the connection\n");
                }
                exit(0);
            }

            

            file_size -= n;
        }
        // verifying if the file is complete
        if (file_size == 0) {
            fprintf(stdout, "The requested file is complete... ending transmission\n");
        } else if (file_size > 0) {
            fprintf(stderr, "Error: File missing parts\n");
        } else {
            fprintf(stderr, "Error: Too much bytes read\n");
        }
        if (close(fd) < 0) {
            fprintf(stderr, "Error: couldn't close the file\n");
        }
    } else {
        fprintf(stderr, "Error: couldn't read the given file name\n");
    }

    if (close(clientfd) < 0) {
        fprintf(stderr, "Error: couldn't close the connection\n");
    }
    exit(0);
}