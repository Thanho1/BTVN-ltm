#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define BUF_SIZE 1024

int main() {
    int client = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(client, (struct sockaddr*)&addr, sizeof(addr));

    printf("Connected to server\n");
    printf("Nhap: client_id: name\n");

    struct pollfd fds[2];

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = client;
    fds[1].events = POLLIN;

    char buf[BUF_SIZE];

    while (1) {
        poll(fds, 2, -1);

        if (fds[0].revents & POLLIN) {
            fgets(buf, sizeof(buf), stdin);
            send(client, buf, strlen(buf), 0);
        }

        if (fds[1].revents & POLLIN) {
            int len = recv(client, buf, sizeof(buf)-1, 0);
            if (len <= 0) break;

            buf[len] = 0;
            printf("%s", buf);
        }
    }

    close(client);
    return 0;
}