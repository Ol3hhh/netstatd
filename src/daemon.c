#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <pcap.h>

#include "db.h"

#define SOCKET_PATH "/tmp/netstatd.sock"
#define DB_PATH "stats.db"

static volatile int running = 1;
static pcap_t *handle = NULL;
static char current_iface[64] = "eth0"; 

void packet_handler(u_char *args, const struct pcap_pkthdr *header,
                    const u_char *packet) {
    (void)args;
    (void)header;

    if (header->caplen < 34) return; 

    struct iphdr {
        unsigned char ihl:4, version:4;
        unsigned char tos;
        unsigned short tot_len;
        unsigned short id;
        unsigned short frag_off;
        unsigned char ttl;
        unsigned char protocol;
        unsigned short check;
        unsigned int saddr;
        unsigned int daddr;
    };

    struct iphdr *ip = (struct iphdr *)(packet + 14);
    if (ip->version != 4) return; 

    char src[INET_ADDRSTRLEN];
    char dst[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip->saddr, src, sizeof(src));
    inet_ntop(AF_INET, &ip->daddr, dst, sizeof(dst));

    db_increment(current_iface, src);
    db_increment(current_iface, dst);
}


int start_sniffer(const char *iface) {
    char errbuf[PCAP_ERRBUF_SIZE];
    handle = pcap_open_live(iface, BUFSIZ, 1, 1000, errbuf);
    if (!handle) {
        fprintf(stderr, "pcap_open_live failed on %s: %s\n", iface, errbuf);
        return -1;
    }
    printf("Sniffing on %s...\n", iface);
    return 0;
}

void stop_sniffer() {
    if (handle) {
        pcap_close(handle);
        handle = NULL;
    }
}


void handle_signal(int sig) {
    (void)sig;
    running = 0;
}


void safe_write(int fd, const char *buf, size_t len) {
    ssize_t n = write(fd, buf, len);
    if (n < 0) {
        if (errno != EPIPE && errno != ECONNRESET) {
            perror("write");
        }
    }
}

void handle_client(int client_fd, char *cmd) {
    if (strncmp(cmd, "stop", 4) == 0) {
        safe_write(client_fd, "Stopping daemon\n", 16);
        running = 0;

    } else if (strncmp(cmd, "select iface", 12) == 0) {
        char iface[64];
        if (sscanf(cmd, "select iface %63s", iface) == 1) {
            strncpy(current_iface, iface, sizeof(current_iface));
            stop_sniffer();
            if (start_sniffer(current_iface) == 0) {
                safe_write(client_fd, "Interface changed\n", 18);
            } else {
                safe_write(client_fd, "Failed to change interface\n", 27);
            }
        }

    } else if (strncmp(cmd, "stat", 4) == 0) {
        char iface[64];
        if (sscanf(cmd, "stat %63s", iface) == 1) {
            db_print_stats(iface, client_fd);
        } else {
            db_print_stats(NULL, client_fd);
        }

    } else if (strncmp(cmd, "show", 4) == 0) {
        char ip[64];
        if (sscanf(cmd, "show %63s", ip) == 1) {
            int count;
            if (db_get_count(current_iface, ip, &count) == 0) {
                char buf[128];
                int n = snprintf(buf, sizeof(buf),
                                 "%s %s -> %d\n", current_iface, ip, count);
                safe_write(client_fd, buf, n);
            } else {
                safe_write(client_fd, "No data\n", 8);
            }
        } else {
            safe_write(client_fd, "Usage: show <ip>\n", 17);
        }

    } else {
        safe_write(client_fd, "Unknown command\n", 16);
    }
}

int main() {
    signal(SIGPIPE, SIG_IGN);    
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (db_init(DB_PATH) != 0) return 1;
    if (start_sniffer(current_iface) != 0) return 1;

    unlink(SOCKET_PATH);
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        return 1;
    }

    printf("Daemon started, listening on %s\n", SOCKET_PATH);

    fd_set readfds;
    while (running) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int maxfd = server_fd;

        struct timeval tv = {1, 0};
        int ret = select(maxfd + 1, &readfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(server_fd, &readfds)) {
            int client_fd = accept(server_fd, NULL, NULL);
            if (client_fd >= 0) {
                char buf[256];
                int n = read(client_fd, buf, sizeof(buf) - 1);
                if (n > 0) {
                    buf[n] = '\0';
                    handle_client(client_fd, buf);
                }
                close(client_fd);
            }
        }

        if (handle) pcap_dispatch(handle, -1, packet_handler, NULL);
    }

    printf("Shutting down daemon...\n");
    stop_sniffer();
    db_close();
    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}
