#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../art.h"
#ifndef HASHTABLE_H
#define HASHTABLE_H

// 每一個 (track, domain) pair
typedef struct TrackDomain {
    uint32_t track;
    uint16_t domain;
    void * node;
    struct TrackDomain *next;  // 用 linked list 接續
} TrackDomain;

// 每個 key 對應的 linked list 頭指標
typedef struct {
    TrackDomain *heads[4]; // key 只能是 0, 1, 2, 3
} HashTable;


HashTable *create_table() ;
void set(HashTable *table, int key, uint32_t track, uint16_t domain) ;
void set_with_node(HashTable *table, int key, uint32_t track, uint16_t domain, art_node* n) ;
void get(HashTable *table, int key, uint32_t *track, uint16_t *domain, bool *found) ;
void get_NODE10(HashTable *table, uint16_t old_domain, uint32_t *track, uint16_t *domain, bool *found) ;
void print_table(HashTable *table) ;
void free_table(HashTable *table) ;

#endif