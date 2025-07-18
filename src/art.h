#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include "ngx-queue.h"

#ifndef ART_H
#define ART_H

#define NODE4   0
#define NODE10  1
#define NODE16  4
#define NODE48  2
#define NODE48_origin 5
#define NODE256 3

#define NODE4_len 1 //node 4 length is a fundamental unit in a track
#define NODE10_len 2
#define NODE48_len 8
#define NODE256_len 33

#define MIN_UNIT 32 // 32B

#define MAX_PREFIX_LEN_origin 4
#define MAX_PREFIX_LEN_4 10
#define MAX_PREFIX_LEN_10 12
#define MAX_PREFIX_LEN_48 14
#define MAX_PREFIX_LEN_256 30

#define MAX_DOMAIN_LEN 4U // 0~3 => total 4 len
#define MAX_DOMAIN 3U // 0 ~ 3 => start from 0, 32B, 64B, 96B => total length 128*8 = 2^10 in a track

static const int WORD_SIZE = 8; // 8 byte


typedef int (*art_callback)(void *data, const char *key, uint32_t key_len, void *value);

/** 
 * This struct is included as part
 * of all the various node sizes
 */
typedef struct {
    uint8_t prefix_too_long;
    uint8_t type;
    uint16_t partial_len;   //prefix length
    uint8_t num_children;
    uint32_t track_domain_id;
    char partial[MAX_PREFIX_LEN_origin]; //prefix string
} art_node;

/**
 * Small node with only 4 children
 */
typedef struct {
    art_node n;
    unsigned char keys[4];
    art_node *children[4];
    uint32_t child_track_domain_id[4];
    char partial[MAX_PREFIX_LEN_4]; 
} art_node4;

/**
 * Node with 10 children
 */
typedef struct {
    art_node n;
    unsigned char keys[10];
    art_node *children[10];
    uint32_t child_track_domain_id[10];
    char partial[MAX_PREFIX_LEN_10]; 
} art_node10;

/**
 * Node with 16 children
 */
typedef struct {
    art_node n;
    unsigned char keys[16];
    art_node *children[16];
} art_node16;

typedef struct {
    art_node n;
    unsigned char keys[256];
    art_node *children[48];
} art_node48_origin;

/**
 * Node with 48 children, but
 * a full 256 byte field.
 */
typedef struct {
    art_node n; //art_node n結構成員是物件（struct)用 ., art_node *n 結構成員是指標（struct*）用 ->
    // unsigned char keys[256];
    unsigned char keys[48];
    art_node *children[48]; //connected to 48 children
    uint32_t child_track_domain_id[48];
    char partial[MAX_PREFIX_LEN_48]; 
} art_node48;

/**
 * Full node with 256 children
 */
typedef struct {
    art_node n;
    art_node *children[256];
    uint32_t child_track_domain_id[256];
    char partial[MAX_PREFIX_LEN_256]; 
} art_node256;

/**
 * Represents a leaf. These are
 * of arbitrary size, as they include the key.
 */
typedef struct {
    void *value;
    // uint8_t type;
    uint32_t key_len;
    uint32_t track_domain_id;
    unsigned char key[];
} art_leaf;

/**
 * Main struct, points to root.
 */
typedef struct {
    art_node *root;
    bool origin;
    uint64_t size;
} art_tree;

/**
 * Iterator over ART tree.
 */
typedef struct {
    art_node *node;
    uint32_t pos;
    ngx_queue_t queue;
    bool is_leaf;
} art_iterator;


 typedef struct {
    uint32_t track_domain_id;
    unsigned char prefix[];
 } prefix_long;
/*
Skyrmion operation
*/
/*
get latency and energy
*/
void art_get_latency_energy();

/**
 * Initializes an ART tree
 * @return 0 on success.
 */
int init_art_tree(art_tree *t);

/**
 * Destroys an ART tree
 * @return 0 on success.
 */
int destroy_art_tree(art_tree *t);

/**
 * Returns the size of the ART tree.
 */
inline uint64_t art_size(art_tree *t) {
    return t->size;
}

/**
 * Inserts a new value into the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @arg value Opaque value.
 * @return NULL if the item was newly inserted, otherwise
 * the old value pointer is returned.
 */
void* art_insert(art_tree *t, char *key, int key_len, void *value, bool origin);

/**
 * Deletes a value from the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void* art_delete(art_tree *t, char *key, int key_len);

/**
 * Searches for a value in the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void* art_search(art_tree *t, char *key, int key_len);

/**
 * Returns the minimum valued leaf
 * @return The minimum leaf or NULL
 */
art_leaf* art_minimum(art_tree *t);

/**
 * Returns the maximum valued leaf
 * @return The maximum leaf or NULL
 */
art_leaf* art_maximum(art_tree *t);

/**
 * Iterates through the entries pairs in the map,
 * invoking a callback for each. The call back gets a
 * key, value for each and returns an integer stop value.
 * If the callback returns non-zero, then the iteration stops.
 * @arg t The tree to iterate over
 * @arg cb The callback function to invoke
 * @arg data Opaque handle passed to the callback
 * @return 0 on success, or the return of the callback.
 */
int art_iter(art_tree *t, art_callback cb, void *data);

/**
 * Iterates through the entries pairs in the map,
 * invoking a callback for each that matches a given prefix.
 * The call back gets a key, value for each and returns an integer stop value.
 * If the callback returns non-zero, then the iteration stops.
 * @arg t The tree to iterate over
 * @arg prefix The prefix of keys to read
 * @arg prefix_len The length of the prefix
 * @arg cb The callback function to invoke
 * @arg data Opaque handle passed to the callback
 * @return 0 on success, or the return of the callback.
 */
int art_iter_prefix(art_tree *t, char *prefix, int prefix_len, art_callback cb, void *data);

/**
 * Creates a copy of an ART tree. The two trees will
 * share the internal leaves, but will NOT share internal nodes.
 * This allows leaves to be added and deleted from each tree
 * individually.
 *
 * @arg dst The destination tree. Not initialized yet.
 * @arg src The source tree, must be initialized.
 * @return 0 on success.
 */
int art_copy(art_tree *dst, art_tree *src);

/**
 * Create and initializes an ART tree iterator.
 * @return 0 on success.
 */
art_iterator* create_art_iterator(art_tree *tree);

/**
 * Destroys an ART tree iterator.
 * @return 0 on success.
 */
int destroy_art_iterator(art_iterator *iterator);

/**
 * Return next leaf element.
 * @return The next leaf or NULL
 */
art_leaf* art_iterator_next(art_iterator *iterator); 

/*
 * free node in skyrmion space
 */
void free_node(int type, uint32_t track_domain_id);


#endif
