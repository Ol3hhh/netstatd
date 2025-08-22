#include "stats.h"
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

IPNode* insert_ip(IPNode *root, uint32_t ip) {
    if (!root) {
        IPNode *node = malloc(sizeof(IPNode));
        node->ip = ip;
        node->count = 1;
        node->left = node->right = NULL;
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

void print_stats(IPNode *root) {
    if (!root) return;
    print_stats(root->left);

    struct in_addr addr;
    addr.s_addr = root->ip;
    printf("%s -> %lu\n", inet_ntoa(addr), root->count);

    print_stats(root->right);
}

void free_stats(IPNode *root) {
    if (!root) return;
    free_stats(root->left);
    free_stats(root->right);
    free(root);
}
