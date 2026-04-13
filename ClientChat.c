//Chat Client

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <locale.h>

int main() {
    setlocale(LC_ALL, "");

    int client = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8080);

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Ket noi that bai");
        return 1;
    }

    printf("Da ket noi den server port 8080...\n");

    fd_set fdread;
    char buf[256];

    while (1) {
        FD_ZERO(&fdread);
        FD_SET(STDIN_FILENO, &fdread);
        FD_SET(client, &fdread);

        if (select(client + 1, &fdread, NULL, NULL, NULL) < 0) break;

        if (FD_ISSET(STDIN_FILENO, &fdread)) {
            if (fgets(buf, sizeof(buf), stdin) != NULL) {
                send(client, buf, strlen(buf), 0);
            }
        }

        if (FD_ISSET(client, &fdread)) {
            int n = recv(client, buf, sizeof(buf) - 1, 0);
            if (n <= 0) {
                printf("Mat ket noi voi server.\n");
                break;
            }
            buf[n] = 0;
            printf("%s", buf); 
        }
    }

    close(client);
    return 0;
}