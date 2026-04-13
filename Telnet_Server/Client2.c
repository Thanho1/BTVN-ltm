/*******************************************************************************
 * @file    Client2.c
 * @brief   Telnet client dùng select()
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int client;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    client = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(client, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("Connected to server on port %d...\n", PORT);

    fd_set readfds;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(client, &readfds);

        int maxfd = (client > STDIN_FILENO) ? client : STDIN_FILENO;

        select(maxfd + 1, &readfds, NULL, NULL, NULL);

        // nhận từ server
        if (FD_ISSET(client, &readfds)) {
            int len = recv(client, buffer, BUFFER_SIZE - 1, 0);
            if (len <= 0) {
                printf("Server disconnected.\n");
                break;
            }

            buffer[len] = 0;
            printf("%s", buffer);
            fflush(stdout);
        }

        // nhập từ bàn phím
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            fgets(buffer, BUFFER_SIZE, stdin);
            send(client, buffer, strlen(buffer), 0);
        }
    }

    close(client);
    return 0;
}