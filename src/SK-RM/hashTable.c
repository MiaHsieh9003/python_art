#include "hashTable.h"

#define INITIAL_BUCKETS 256
#define INITIAL_CAPACITY 4

HashTable *create_table() {
    HashTable *table = (HashTable *)malloc(sizeof(HashTable));
    if (table) {
        for (int i = 0; i < 4; i++) {
            table->heads[i] = NULL;
        }
    }
    return table;
}

void set_with_node(HashTable *table, int key, uint32_t track, uint16_t domain, art_node* n) {
    if (!table) return;
    if (key < 0 || key > 3) {
        printf("Invalid key: %d\n", key);
        return;
    }

    TrackDomain *new_entry = (TrackDomain *)malloc(sizeof(TrackDomain));
    new_entry->track = track;
    new_entry->domain = domain;
    new_entry->next = NULL;
    // switch(type){
    //     case NODE4:
    //         new_entry->node = (art_node4 *)n;
    //     case NODE10:
    //         new_entry->node = (art_node10 *) n;
    //     case NODE48:
    //         new_entry->node = (art_node48 *)n;
    //     case NODE256:
    //         new_entry->node = (art_node256 *)n;
    // }
    new_entry->node = n;
    // 插到鏈結串列頭
    if (table->heads[key] == NULL) {
        table->heads[key] = new_entry;
    } else {
        TrackDomain *ptr = table->heads[key];
        while(ptr->next != NULL){
            ptr = ptr->next;
        }
        ptr->next = new_entry;
        // new_entry->next = table->heads[key];
    }
}


void set(HashTable *table, int key, uint32_t track, uint16_t domain) {
    if (!table) return;
    if (key < 0 || key > 3) {
        printf("Invalid key: %d\n", key);
        return;
    }

    TrackDomain *new_entry = (TrackDomain *)malloc(sizeof(TrackDomain));
    new_entry->track = track;
    new_entry->domain = domain;
    new_entry->next = NULL;
    new_entry->node = NULL;

    // 插到鏈結串列頭
    if (table->heads[key] == NULL) {
        table->heads[key] = new_entry;
    } else {
        TrackDomain *ptr = table->heads[key];
        while(ptr->next != NULL){
            ptr = ptr->next;
        }
        ptr->next = new_entry;
        // new_entry->next = table->heads[key];
    }
}
void get_NODE10(HashTable *table, uint16_t old_domain, uint32_t *track, uint16_t *domain, bool *found) {
    if (!table) return;
    TrackDomain *current = table->heads[NODE10];
    TrackDomain *prev = NULL;
    *found = false;
    if (current == NULL) {
        printf("No entries for key %d\n", NODE10);
        return;
    }
    // old_domain is original domain from node4
    while (current && abs(current->domain-old_domain) > 1) {
        prev = current;
        current = current->next;
    }
    if(current){
        printf("  (Track: %u, Domain: %u)\n", current->track, current->domain);
        *track = current->track;
        *domain = current->domain;
        prev->next = current->next; // remove current from linked list
        free(current);
        *found = true;
    }
        
}

void get(HashTable *table, int key, uint32_t *track, uint16_t *domain, bool *found) {
    if (!table) return;
    if (key < 0 || key > 3) {
        printf("Invalid key: %d\n", key);
        return;
    }

    TrackDomain *current = table->heads[key];
    if (current == NULL) {
        printf("No entries for key %d\n", key);
        *found = false;
    }else{
        printf("  (Track: %u, Domain: %u)\n", current->track, current->domain);
        *track = current->track;
        *domain = current->domain;
        table->heads[key] = current->next;
        free(current);
        *found = true;
    }
}

void print_table(HashTable *table) {
    for (int i = 0; i < 4; i++) {
        printf("Key %d:\n", i);
        TrackDomain *current = table->heads[i];
        while (current) {
            printf("  (Track: %u, Domain: %u)\n", current->track, current->domain);
            current = current->next;
        }
    }
}

void free_table(HashTable *table) {
    if (!table) return;
    for (int i = 0; i < 4; i++) {
        TrackDomain *current = table->heads[i];
        while (current) {
            TrackDomain *tmp = current;
            current = current->next;
            free(tmp);
        }
        table->heads[i] = NULL;
    }
    free(table);
}