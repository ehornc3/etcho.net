#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdbool.h>
#include <netdb.h>

// #define DEBUG

void *sendProcess();
void *recvProcess();
pthread_t recv_tid;
pthread_t send_tid;
int sock;

int main(int argc, char *argv[]) {
    if (argc != 3)
    {
        printf("usage: %s <ip address/hostname> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    bool number = true;
    for (int i = 0; i < strlen(argv[2]); i++)
    {
        if (!isdigit(argv[2][i]))
        {
            number = false;
        }
    }
    if (!number)
    {
        printf("Port argument is not a number\n");
        exit(EXIT_FAILURE);
    }

    const int PORT = atoi(argv[2]);
    if (PORT < 0 || PORT > 65536)
    {
        printf("Port argument is not a valid port number\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    int addrlen = sizeof(serv_addr);

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket");
        close(sock);
        exit(EXIT_FAILURE);
    }

#ifdef DEBUG
    printf("Socket created, file descriptor %d\n", sock);
#endif /* ifdef DEBUG */

    // Connect to the server
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0)
    {
        struct addrinfo hints, *res;
        char ipstr[INET6_ADDRSTRLEN];
        int status;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if ((status = getaddrinfo(argv[1], NULL, &hints, &res)) != 0)
        {
            perror("getaddrinfo");
            close(sock);
            exit(EXIT_FAILURE);
        }

        if (res->ai_family == AF_INET)
        {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
            serv_addr.sin_addr = ipv4->sin_addr;
        }
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }


    if (pthread_create(&send_tid, NULL, sendProcess, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&recv_tid, NULL, recvProcess, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    pthread_join(recv_tid, NULL);
    pthread_kill(send_tid, SIGTERM);

    close(sock);
    exit(EXIT_SUCCESS);
}

void *sendProcess() {
    char outgoing[1024] = {0};

    printf("Enter username: ");
    while (true)
    {
        fgets(outgoing, sizeof(outgoing), stdin);
        send(sock, outgoing, strlen(outgoing), 0);
        memset(outgoing, 0, sizeof(outgoing));
    }
}

void *recvProcess() {
    char incoming[1024] = {0};

    while (true)
    {
        // Receive from the server
        int valread = read(sock, incoming, sizeof(incoming));
        if (valread <= 0) { pthread_exit(NULL); }

        incoming[valread] = '\0';
        printf("%s", incoming); // Newlines are NOT our responsibility.

        valread = -1;

        memset(incoming, 0, sizeof(incoming));
    }
}
