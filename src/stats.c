#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "stats.h"

IPNode* insert_ip(IPNode *root, uint32_t ip) {
    if (!root) {
        IPNode *node = calloc(1, sizeof(IPNode));
        node->ip = ip;
        node->count = 1;
        return node;
    }
    if (ip == root->ip) {
        root->count++;
    } else if (ip < root->ip) {
        root->left = insert_ip(root->left, ip);
    } else {
        root->right = insert_ip(root->right, ip);
    }
    return root;
}

IPNode* find_ip(IPNode *root, uint32_t ip) {
    if (!root) return NULL;
    if (ip == root->ip) return root;
    if (ip < root->ip) return find_ip(root->left, ip);
    return find_ip(root->right, ip);
}

void print_stats(IPNode *root, int client_fd) {
    if (!root) return;
    print_stats(root->left, client_fd);
    struct in_addr addr;
    addr.s_addr = root->ip;
    char buf[128];
    int n = snprintf(buf, sizeof(buf), "%s -> %lu\n",
                     inet_ntoa(addr), root->count);
    write(client_fd, buf, n);
    print_stats(root->right, client_fd);
}

void free_stats(IPNode *root) {
    if (!root) return;
    free_stats(root->left);
    free_stats(root->right);
    free(root);
}
