//Chat Server

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include <locale.h>

#define MAX_CLIENTS 1020

// Cấu trúc để lưu thông tin bổ sung cho mỗi client
typedef struct {
    int fd;
    char name[64];
    int registered; 
} ClientInfo;

// Hàm xóa client khỏi danh sách 
void removeClient(ClientInfo *clients, int *nClients, int i) {
    close(clients[i].fd);
    if (i < *nClients - 1) {
        clients[i] = clients[*nClients - 1];
    }
    *nClients -= 1;
}

int main() {
    setlocale(LC_ALL, "");
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) { perror("socket() failed"); return 1; }

    // Cho phép sử dụng lại địa chỉ port ngay lập tức
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed"); return 1;
    }

    if (listen(listener, 5)) {
        perror("listen() failed"); return 1;
    }

    printf("Chat Server is listening on port 8080...\n");

    ClientInfo clients[MAX_CLIENTS];
    int nClients = 0;
    fd_set fdread;
    char buf[2048];

    while (1) {
        FD_ZERO(&fdread);
        FD_SET(listener, &fdread);
        int maxdp = listener + 1;

        for (int i = 0; i < nClients; i++) {
            FD_SET(clients[i].fd, &fdread);
            if (maxdp < clients[i].fd + 1) maxdp = clients[i].fd + 1;
        }

        int ret = select(maxdp, &fdread, NULL, NULL, NULL);
        if (ret < 0) { perror("select() failed"); break; }

        // Chấp nhận kết nối mới
        if (FD_ISSET(listener, &fdread)) {
            int client = accept(listener, NULL, NULL);
            if (nClients < MAX_CLIENTS) {
                clients[nClients].fd = client;
                clients[nClients].registered = 0; // Chưa đăng ký tên
                nClients++;
                printf("New connection: %d\n", client);
                char *msg = "Hay nhap ten: client_id: client_name\n";
                send(client, msg, strlen(msg), 0);
            } else {
                close(client);
            }
        }

        // Xử lý dữ liệu từ các client
        for (int i = 0; i < nClients; i++) {
            if (FD_ISSET(clients[i].fd, &fdread)) {
                ret = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);
                if (ret <= 0) {
                    printf("Client %s (%d) disconnected\n", clients[i].name, clients[i].fd);
                    removeClient(clients, &nClients, i);
                    i--;
                    continue;
                }

                buf[ret] = 0;
                buf[strcspn(buf, "\r\n")] = 0; // Xóa ký tự xuống dòng

                if (clients[i].registered == 0) {
                    // XỬ LÝ ĐĂNG KÝ
                    char id_tag[32], name[64];
                    if (sscanf(buf, "%31[^:]: %63s", id_tag, name) == 2 && strcmp(id_tag, "client_id") == 0) {
                        strcpy(clients[i].name, name);
                        clients[i].registered = 1;
                        char *ok = "Dang ky thanh cong. Moi ban chat!\n";
                        send(clients[i].fd, ok, strlen(ok), 0);
                    } else {
                        char *err = "Sai cu phap! Nhap lai: client_id: client_name\n";
                        send(clients[i].fd, err, strlen(err), 0);
                    }
                } else {
                    // XỬ LÝ CHAT (BROADCAST)
                    time_t t = time(NULL);
                    struct tm *tm_info = localtime(&t);
                    char time_str[32], send_buf[2200];
                    strftime(time_str, sizeof(time_str), "%Y/%m/%d %I:%M:%S%p", tm_info);

                    sprintf(send_buf, "%s %s: %s\n", time_str, clients[i].name, buf);
                    printf("%s", send_buf);

                    for (int j = 0; j < nClients; j++) {
                        // Gửi cho người khác và chỉ người đã đăng ký mới nhận được
                        if (i != j && clients[j].registered == 1) {
                            send(clients[j].fd, send_buf, strlen(send_buf), 0);
                        }
                    }
                }
            }
        }
    }
    close(listener);
    return 0;
}