#ifndef STATS_H
#define STATS_H

#include <stdint.h>

typedef struct IPNode {
    uint32_t ip;          
    uint64_t count;
    struct IPNode *left, *right;
} IPNode;

IPNode* insert_ip(IPNode *root, uint32_t ip);
IPNode* find_ip(IPNode *root, uint32_t ip);
void print_stats(IPNode *root, int client_fd);
void free_stats(IPNode *root);

#endif
