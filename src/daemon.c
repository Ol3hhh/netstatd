#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pcap.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include "stats.h"

#define DEFAULT_IFACE "wlp3s0"

IPNode *stats_root = NULL;
pcap_t *handle = NULL;

void handle_signal(int sig) {
    printf("\nCollected statistics:\n");
    print_stats(stats_root);
    free_stats(stats_root);
    if (handle) {
        pcap_close(handle);
    }
    exit(0);
}

void packet_handler(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {
    struct iphdr *ip = (struct iphdr*)(packet + 14);
    stats_root = insert_ip(stats_root, ip->saddr);
}

int main(int argc, char **argv) {
    char errbuf[PCAP_ERRBUF_SIZE];
    const char *iface = DEFAULT_IFACE;

    if (argc > 1) {
        iface = argv[1];
    }

    signal(SIGINT, handle_signal);

    handle = pcap_open_live(iface, BUFSIZ, 1, 1000, errbuf);
    if (!handle) {
        fprintf(stderr, "Error opening interface %s: %s\n", iface, errbuf);
        return 1;
    }

    printf("Sniffing on interface %s... Press Ctrl+C to stop.\n", iface);

    if (pcap_loop(handle, 0, packet_handler, NULL) < 0) {
        fprintf(stderr, "pcap_loop failed: %s\n", pcap_geterr(handle));
        pcap_close(handle);
        return 1;
    }

    pcap_close(handle);
    free_stats(stats_root);
    return 0;
}
