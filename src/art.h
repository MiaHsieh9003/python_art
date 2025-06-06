#include <stdint.h>
#include <stdbool.h>
#include "ngx-queue.h"
#ifndef ART_H
#define ART_H

#define NODE4   1
#define NODE10  5
#define NODE16  2
#define NODE48  3
#define NODE256 4

// #define MAX_PREFIX_LEN 12
#define MAX_PREFIX_LEN_4 9
#define MAX_PREFIX_LEN_10 11
#define MAX_PREFIX_LEN_48 13
#define MAX_PREFIX_LEN_256 29

typedef int(*art_callback)(void *data, const char *key, uint32_t key_len, void *value);

/** 
 * This struct is included as part
 * of all the various node sizes
 */
typedef struct {
    uint8_t type;
    uint8_t num_children;
    uint16_t partial_len;   //prefix length
    // char partial[MAX_PREFIX_LEN]; //prefix string
} art_node;

/**
 * Small node with only 4 children
 */
typedef struct {
    art_node n;
    unsigned char keys[4];
    art_node *children[4];
    char partial[MAX_PREFIX_LEN_4]; 
} art_node4;

/**
 * Node with 10 children
 */
typedef struct {
    art_node n;
    unsigned char keys[10];
    art_node *children[10];
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

/**
 * Node with 48 children, but
 * a full 256 byte field.
 */
typedef struct {
    art_node n;
    // unsigned char keys[256];
    unsigned char keys[48];
    art_node *children[48]; //connected to 48 children
    char partial[MAX_PREFIX_LEN_48]; 
} art_node48;

/**
 * Full node with 256 children
 */
typedef struct {
    art_node n;
    art_node *children[256];
    char partial[MAX_PREFIX_LEN_256]; 
} art_node256;

/**
 * Represents a leaf. These are
 * of arbitrary size, as they include the key.
 */
typedef struct {
    void *value;
    uint32_t key_len;
    unsigned char key[];
} art_leaf;

// typedef struct art_leaf_link{
//     //add by Mia
//     struct art_leaf_link *next;
//     void *value;
//     uint32_t key_len;
//     unsigned char key[];
// } art_leaf_link;

/**
 * Main struct, points to root.
 */
typedef struct {
    art_node *root;
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

/*
* add by Mia
* @return node type
*/
static uint8_t find_child_type(art_node *n);

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
void* art_insert(art_tree *t, char *key, int key_len, void *value);

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

#endif
