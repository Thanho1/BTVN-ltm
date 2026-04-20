#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define BUF_SIZE 1024

int main() {
    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }

    printf("Connected to server...\n");

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = client;
    fds[1].events = POLLIN;

    char buf[BUF_SIZE];

    while (1) {
        poll(fds, 2, -1);

        // INPUT
        if (fds[0].revents & POLLIN) {
            if (fgets(buf, sizeof(buf), stdin) == NULL)
                break;

            send(client, buf, strlen(buf), 0);
        }

        // RECEIVE
        if (fds[1].revents & POLLIN) {
            int len = recv(client, buf, sizeof(buf) - 1, 0);

            if (len <= 0) {
                printf("Server disconnected.\n");
                break;
            }

            buf[len] = 0;
            printf("%s", buf);
            fflush(stdout);
        }
    }

    close(client);
    return 0;
}