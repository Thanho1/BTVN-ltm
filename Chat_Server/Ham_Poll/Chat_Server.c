#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define MAX_CLIENTS 1000
#define BUF_SIZE 1024

struct client_info {
    int fd;
    char name[50];   // dùng name để hiển thị
    int registered;
};

struct client_info clients[MAX_CLIENTS];

void remove_client(struct pollfd fds[], int *nfds, int i) {
    close(fds[i].fd);

    for (int j = i; j < *nfds - 1; j++) {
        fds[j] = fds[j + 1];
        clients[j] = clients[j + 1];
    }
    (*nfds)--;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(listener, 5) < 0) {
        perror("listen");
        return 1;
    }

    struct pollfd fds[MAX_CLIENTS];
    int nfds = 1;

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    printf("Chat Server is listening on port 8080...\n");

    char buf[BUF_SIZE];

    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret < 0) {
            perror("poll");
            break;
        }

        // CLIENT MỚI
        if (fds[0].revents & POLLIN) {
            int client_fd = accept(listener, NULL, NULL);
            if (client_fd < 0) continue;

            if (nfds < MAX_CLIENTS) {
                fds[nfds].fd = client_fd;
                fds[nfds].events = POLLIN;

                clients[nfds].fd = client_fd;
                clients[nfds].registered = 0;

                nfds++;

                printf("New connection: %d\n", client_fd);
            } else {
                close(client_fd);
            }
        }

        // XỬ LÝ CLIENT
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                int len = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);

                if (len <= 0) {
                    printf("Client %d disconnected\n", fds[i].fd);
                    remove_client(fds, &nfds, i);
                    i--;
                    continue;
                }

                buf[len] = 0;

                // CHƯA ĐĂNG KÝ
                if (!clients[i].registered) {
                    char id[50], name[50];

                    if (sscanf(buf, "%[^:]: %s", id, name) == 2) {
                        strcpy(clients[i].name, name);
                        clients[i].registered = 1;

                        printf("Client %d registered as %s\n",
                               fds[i].fd, name);

                        send(fds[i].fd,
                             "Dang ky thanh cong. Moi ban chat!\n",
                             40, 0);
                    } else {
                        send(fds[i].fd,
                             "Sai cu phap. Nhap: client_id: client_name\n",
                             50, 0);
                    }
                    continue;
                }

                // CHAT MESSAGE
                char msg[BUF_SIZE];

                snprintf(msg, sizeof(msg),
                         "%s: %.900s",
                         clients[i].name,
                         buf);

                // IN SERVER (giống bài mẫu)
                printf("%s", msg);

                // GỬI CHO CLIENT KHÁC
                for (int j = 1; j < nfds; j++) {
                    if (j != i && clients[j].registered) {
                        send(fds[j].fd, msg, strlen(msg), 0);
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}