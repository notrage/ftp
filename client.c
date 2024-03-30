#include "csapp.h"

#define MAX_NAME_LEN 256
#define MAX_BUF_CONTENT 512
#define CLIENT_DIR "./files/"

int min(int a, int b)
{
    if (a < b)
    {
        return a;
    }
    else
    {
        return b;
    }
}

void traiter_echange_server(int clientfd)
{

    size_t n;
    int fd, i;
    uint32_t buf, file_size, net_buf;
    char buf_file_name[MAX_NAME_LEN], buf_file_content[MAX_BUF_CONTENT],
        buf_file_path[MAX_NAME_LEN] = CLIENT_DIR;
    struct stat *stats = malloc(sizeof(struct stat));

    // reading the file name from the user
    while (fgets(buf_file_name, MAX_NAME_LEN, stdin) != NULL)
    {

        // There is a '\n' at the end of the line that need to be removed before sending it to the server
        buf_file_name[strlen(buf_file_name) - 1] = '\0';
        buf = strlen(buf_file_name) + 1;
        net_buf = htonl(buf);

        // according to our protocol, we first send the size of the file name and the file name
        if (rio_writen(clientfd, &net_buf, sizeof(net_buf)) < 0)
        {
            fprintf(stderr, "Error: couldn't send the file name size\n");
            Close(clientfd);
            exit(0);
        }
        if (rio_writen(clientfd, buf_file_name, buf) < 0)
        {
            fprintf(stderr, "Error: couldn't send the file name\n");
            Close(clientfd);
            exit(0);
        }

        // adding the path to open the asked file
        strcpy(buf_file_path, CLIENT_DIR);
        strcat(buf_file_path, buf_file_name);

        // opening (creating) a file to store server's response
        if ((fd = open(buf_file_path, O_WRONLY | O_CREAT, 0644)) < 0)
        {
            fprintf(stderr, "Error: couldn't open the file: %s\n", buf_file_path);
            Close(clientfd);
            exit(0);
        }

        // getting open file's size. If not 0, it means previous downloading was stopped during
        // process, implying we must restart its download where it stopped
        if (fstat(fd, stats) == -1)
        {
            fprintf(stderr, "Error:  fstat failed\n");
            free(stats);
            return;
        }
        buf = htonl(stats->st_size);

        if (rio_writen(clientfd, &buf, sizeof(uint32_t)) < 0)
        {
            fprintf(stderr, "Error: couldn't send the local file size\n");
            Close(clientfd);
            exit(0);
        }

        lseek(fd, ntohl(buf), SEEK_SET);

        // getting size of wanted file
        if ((n = rio_readn(clientfd, &buf, sizeof(uint32_t))) != 0)
        {
            file_size = ntohl(buf);
        }
        else
        {
            fprintf(stderr, "Error: Couldn't get file size\n");
            if (close(fd) < 0)
            {
                fprintf(stderr, "Error: couldn't close the file\n");
            }
            if (close(clientfd) < 0)
            {
                fprintf(stderr, "Error: couldn't close the connection\n");
            }
            exit(0);
        }

        printf("requesting to download: %d bytes...\n", file_size);
        sleep(1);

        i = 0;
        // while we can read something from server
        while ((n = rio_readn(clientfd, buf_file_content, min(MAX_BUF_CONTENT, file_size))) != 0)
        {

            printf("\rclient read %u bytes [x%d]", (unsigned int)n, ++i);

            // writing the readen file content to the opened file
            if (rio_writen(fd, buf_file_content, n) < 0)
            {
                fprintf(stderr, "Error: couldn't write the file content\n");
                if (close(fd) < 0)
                {
                    fprintf(stderr, "Error: couldn't close the file\n");
                }
                if (close(clientfd) < 0)
                {
                    fprintf(stderr, "Error: couldn't close the connection\n");
                }
                exit(0);
            }

            file_size -= n;
        }

        // verifying if the file is complete
        if (file_size == 0)
        {
            fprintf(stdout, "The requested file is complete... ending transmission\n");
        }
        else if (file_size > 0)
        {
            fprintf(stderr, "Error: File missing parts\n");
        }
        else
        {
            fprintf(stderr, "Error: Too much bytes read\n");
        }
        if (close(fd) < 0)
        {
            fprintf(stderr, "Error: couldn't close the file\n");
        }
        printf("\nEnter the name of the file you want to download: \n");
    }
    free(stats);
    return;
}

int main(int argc, char **argv)
{

    char *host;
    int clientfd, port;

    if (argc != 3)
    {
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
    if ((clientfd = open_clientfd(host, port)) < 0)
    {
        fprintf(stderr, "Error: couldn't connect to the server\n");
        exit(0);
    }

    /*
     * At this stage, the connection is established between the client
     * and the server OS ... but it is possible that the server application
     * has not yet called "Accept" for this connection
     */
    printf("client connected to server OS\n");
    printf("Enter the name of the file you want to download: \n");

    traiter_echange_server(clientfd);

    if (close(clientfd) < 0)
    {
        fprintf(stderr, "Error: couldn't close the connection\n");
    }
    exit(0);
}