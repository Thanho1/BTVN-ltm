#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

int clients[MAX_CLIENTS];
int state[MAX_CLIENTS]; // 0: auth (user+pass), 1: command mode
int nClients = 0;

// Hàm kiểm tra login
int check_login(char *user, char *pass) {
    FILE *f = fopen("users.txt", "r");
    if (!f) return 0;

    char u[100], p[100];
    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (strcmp(u, user) == 0 && strcmp(p, pass) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

// Hàm xóa client khi ngắt kết nối
void removeClient(int i) {
    close(clients[i]);
    if (i < nClients - 1) {
        clients[i] = clients[nClients - 1];
        state[i] = state[nClients - 1];
    }
    nClients--;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        return 1;
    }
    listen(listener, 5);

    printf("Telnet Server running on port %d...\n", PORT);

    fd_set readfds;
    char buf[BUFFER_SIZE];

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(listener, &readfds);
        int maxfd = listener;

        for (int i = 0; i < nClients; i++) {
            FD_SET(clients[i], &readfds);
            if (clients[i] > maxfd) maxfd = clients[i];
        }

        select(maxfd + 1, &readfds, NULL, NULL, NULL);

        // Chấp nhận kết nối mới
        if (FD_ISSET(listener, &readfds)) {
            int client = accept(listener, NULL, NULL);
            if (nClients < MAX_CLIENTS) {
                clients[nClients] = client;
                state[nClients] = 0; // Trạng thái chờ đăng nhập
                nClients++;
                char *welcome = "Vui long dang nhap (user pass): ";
                send(client, welcome, strlen(welcome), 0);
            } else {
                close(client);
            }
        }

        // Xử lý dữ liệu từ các client
        for (int i = 0; i < nClients; i++) {
            if (FD_ISSET(clients[i], &readfds)) {
                int ret = recv(clients[i], buf, BUFFER_SIZE - 1, 0);
                if (ret <= 0) {
                    removeClient(i); i--; continue;
                }

                buf[ret] = 0;

                buf[strcspn(buf, "\r\n")] = 0;

                int j = 0;
                for (int k = 0; buf[k] != '\0'; k++) {
                    if ((unsigned char)buf[k] >= 32 && (unsigned char)buf[k] <= 126) {
                        buf[j++] = buf[k];
                    }
                }
                buf[j] = '\0';

                if (strlen(buf) == 0 && state[i] == 1) {
                    char *prompt = "[HUST_Telnet_Server]$ ";
                    send(clients[i], prompt, strlen(prompt), 0);
                    continue;
                }

                if (state[i] == 0) {
                    char u[100], p[100];
                    if (sscanf(buf, "%s %s", u, p) == 2 && check_login(u, p)) {
                        state[i] = 1; // Đăng nhập thành công
                        char menu[512];
                        snprintf(menu, sizeof(menu),
                            "\n--- DANG NHAP THANH CONG ---\n"
                            "Goi y lenh: ls, pwd, date, whoami\n"
                            "----------------------------\n"
                            "[HUST_Telnet_Server]$ ");
                        send(clients[i], menu, strlen(menu), 0);
                    } else {
                        char *err = "Sai tai khoan! Nhap lai (user pass): ";
                        send(clients[i], err, strlen(err), 0);
                    }
                } 
                else {
                    // Thực thi lệnh và chuyển hướng kết quả ra file
                    char cmd[BUFFER_SIZE + 64];
                    snprintf(cmd, sizeof(cmd), "%s > out.txt 2>&1", buf);
                    system(cmd);

                    // Đọc file kết quả và gửi trả client
                    FILE *f = fopen("out.txt", "r");
                    if (f) {
                        char line[512];
                        while (fgets(line, sizeof(line), f)) {
                            send(clients[i], line, strlen(line), 0);
                        }
                        fclose(f);
                    }
                    
                    char *prompt = "[HUST_Telnet_Server]$ ";
                    send(clients[i], prompt, strlen(prompt), 0);
                }
            }
        }
    }
    close(listener);
    return 0;
}