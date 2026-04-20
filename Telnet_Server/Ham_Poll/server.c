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
int state[MAX_CLIENTS]; // 0: login, 1: command
int nClients = 0;

// ===== TRIM INPUT =====
void trim(char *str) {
    // xóa \r\n
    str[strcspn(str, "\r\n")] = 0;

    // xóa space cuối
    int len = strlen(str);
    while (len > 0 && str[len - 1] == ' ') {
        str[len - 1] = 0;
        len--;
    }

    // xóa space đầu
    int start = 0;
    while (str[start] == ' ') start++;

    if (start > 0)
        memmove(str, str + start, strlen(str + start) + 1);
}

// ===== CHECK LOGIN =====
int check_login(char *user, char *pass) {
    FILE *f = fopen("users.txt", "r");
    if (!f) return 0;

    char u[100], p[100];
    while (fscanf(f, "%99s %99s", u, p) == 2) {
        if (strcmp(u, user) == 0 && strcmp(p, pass) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

// ===== REMOVE CLIENT =====
void removeClient(int i) {
    close(clients[i]);
    if (i < nClients - 1) {
        clients[i] = clients[nClients - 1];
        state[i] = state[nClients - 1];
    }
    nClients--;
}

// ===== CHECK COMMAND =====
int is_valid_command(char *cmd) {
    return strcmp(cmd, "ls") == 0 ||
           strcmp(cmd, "pwd") == 0 ||
           strcmp(cmd, "date") == 0 ||
           strcmp(cmd, "whoami") == 0;
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

        // add clients
        for (int i = 0; i < nClients; i++) {
            FD_SET(clients[i], &readfds);
            if (clients[i] > maxfd)
                maxfd = clients[i];
        }

        select(maxfd + 1, &readfds, NULL, NULL, NULL);

        // ===== NEW CONNECTION =====
        if (FD_ISSET(listener, &readfds)) {
            int client = accept(listener, NULL, NULL);
            if (nClients < MAX_CLIENTS) {
                clients[nClients] = client;
                state[nClients] = 0;
                nClients++;

                char *msg = "Vui long dang nhap (user pass): ";
                send(client, msg, strlen(msg), 0);
            } else {
                close(client);
            }
        }

        // ===== HANDLE CLIENT =====
        for (int i = 0; i < nClients; i++) {
            if (FD_ISSET(clients[i], &readfds)) {

                int ret = recv(clients[i], buf, BUFFER_SIZE - 1, 0);
                if (ret <= 0) {
                    removeClient(i);
                    i--;
                    continue;
                }

                buf[ret] = 0;

                // 🔥 CLEAN INPUT
                trim(buf);

                // loại ký tự rác
                int j = 0;
                for (int k = 0; buf[k] != '\0'; k++) {
                    if ((unsigned char)buf[k] >= 32 && (unsigned char)buf[k] <= 126) {
                        buf[j++] = buf[k];
                    }
                }
                buf[j] = '\0';

                // ===== LOGIN =====
                if (state[i] == 0) {
                    char u[100], p[100];

                    if (sscanf(buf, "%99s %99s", u, p) == 2 &&
                        check_login(u, p)) {

                        state[i] = 1;

                        char menu[] =
                            "\n--- DANG NHAP THANH CONG ---\n"
                            "Goi y lenh: ls, pwd, date, whoami\n"
                            "----------------------------\n"
                            "[HUST_Telnet_Server]$ ";

                        send(clients[i], menu, strlen(menu), 0);
                    } else {
                        char *err = "Sai tai khoan! Nhap lai: ";
                        send(clients[i], err, strlen(err), 0);
                    }
                }

                // ===== COMMAND MODE =====
                else {

                    // enter trống → chỉ hiện prompt
                    if (strlen(buf) == 0) {
                        char *prompt = "[HUST_Telnet_Server]$ ";
                        send(clients[i], prompt, strlen(prompt), 0);
                        continue;
                    }

                    // check command hợp lệ
                    if (!is_valid_command(buf)) {
                        char *err = "Lenh khong hop le!\n[HUST_Telnet_Server]$ ";
                        send(clients[i], err, strlen(err), 0);
                        continue;
                    }

                    // 🔥 FIX WARNING snprintf
                    char cmd[BUFFER_SIZE];
                    int max_len = BUFFER_SIZE - strlen(" > out.txt 2>&1") - 1;

                    snprintf(cmd, sizeof(cmd),
                             "%.*s > out.txt 2>&1",
                             max_len, buf);

                    system(cmd);

                    // đọc file
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