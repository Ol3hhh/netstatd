#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/netstatd.sock"

static int connect_daemon() {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    return sock;
}

static void usage(const char *prog) {
    printf("Usage: %s <command>\n", prog);
    printf("Commands:\n");
    printf("  start                  Start daemon (if not running)\n");
    printf("  stop                   Stop packet sniffing\n");
    printf("  show [ip] count        Show number of packets from ip\n");
    printf("  select iface [iface]   Select interface for sniffing\n");
    printf("  stat [iface]           Show stats for iface (or all)\n");
    printf("  --help                 Show this help\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0) {
        usage(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "start") == 0) {
        printf("Starting daemon...\n");
        int ret = system("./bin/daemon &");
        if (ret == -1) {
            perror("system");
            return 1;
        }
        sleep(1); 
        printf("Daemon started successfully\n");
        return 0;
    }

    int sock = connect_daemon();
    if (sock < 0) {
        fprintf(stderr, "Cannot connect to daemon. Is it running?\n");
        return 1;
    }


    char cmd[256];
    memset(cmd, 0, sizeof(cmd));

    if (strcmp(argv[1], "stop") == 0) {
        snprintf(cmd, sizeof(cmd), "stop");
    } else if (strcmp(argv[1], "show") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s show <ip> count\n", argv[0]);
            close(sock);
            return 1;
        }
        snprintf(cmd, sizeof(cmd), "show %s", argv[2]);
    } else if (strcmp(argv[1], "select") == 0 && argc >= 4 && strcmp(argv[2], "iface") == 0) {
        snprintf(cmd, sizeof(cmd), "select %s", argv[3]);
    } else if (strcmp(argv[1], "stat") == 0) {
        if (argc == 3)
            snprintf(cmd, sizeof(cmd), "stat %s", argv[2]);
        else
            snprintf(cmd, sizeof(cmd), "stat");
    } else {
        usage(argv[0]);
        close(sock);
        return 1;
    }

    if (send(sock, cmd, strlen(cmd), 0) < 0) {
        perror("send");
        close(sock);
        return 1;
    }
    char buffer[4096];
    ssize_t n;
    while ((n = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
    }

    close(sock);
    return 0;
}
