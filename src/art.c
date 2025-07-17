#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <emmintrin.h>
#include <assert.h>
#include <math.h>
#define _GNU_SOURCE
#include <string.h>

#include "art.h"
#include "SK-RM/artSkrm.h"
#include "SK-RM/hashTable.h"

/**
 * Macros to manipulate pointer tags
 */
// Tag definitions
#define TAG_LEAF        0x1  // 01
// #define TAG_LEAF_LINK   0x3  // 11
#define TAG_MASK        0x3  //   // 二進位 11，用來選取兩個 tag bits

//若為 1 則視為 leaf 節點
#define IS_LEAF(x) (((uintptr_t)x & 1))
// #define IS_LEAF_LINK(x)   (((uintptr_t)(x) & TAG_MASK) == TAG_LEAF_LINK)
//將指標 x 標記為 leaf：把最低位設為 1。
#define SET_LEAF(x) ((void*)((uintptr_t)x | 1))
// #define IS_LEAF_LINK(x)   (((uintptr_t)(x) & TAG_MASK) == TAG_LEAF_LINK)
//還原成真實的 leaf 指標（清除最低位元）

// ~1 是 bitwise NOT，會變成 ...11111110
// x & ~1 的效果是把 x 的最低位元（bit 0）變成 0，其餘不變
// 把處理後的整數再轉回成 void pointer 指標型別。
#define LEAF_RAW(x) ((void*)((uintptr_t)x & ~1))

#define MAX(a,b) ((a) > (b) ? (a) : (b))

// #define LEAF_LINK_RAW(n) ((art_leaf_link *)((uintptr_t)(n) & ~1))
uint32_t track = 0;  
uint16_t domain = 0; //is 0~3

uint32_t node4_track = 0;
uint16_t node4_domain = 0;

uint32_t node10_track = 0;
uint16_t node10_domain = 0;

uint32_t node48_track = 0;
uint16_t node48_domain = 0;

uint32_t node256_track = 0;
uint16_t node256_domain = 0;

uint32_t leaf_track = 0;
uint16_t leaf_domain = 0;

bool origin_method;

HashTable * hole_space; //save the skip or delete space

/**
 * Allocates a node4,
 * initializes to zero and sets the type.
 */
static inline art_node4* alloc_node4(void) {
    art_node4* node = calloc(1, sizeof(art_node4));
    node->n.type = NODE4;
    return node;
}

/**
 * Allocates a node8,
 * initializes to zero and sets the type.
 */
static inline art_node10* alloc_node10(void) {
    art_node10* node = calloc(1, sizeof(art_node10));
    node->n.type = NODE10;
    return node;
}


/**
 * Allocates a node16,
 * initializes to zero and sets the type.
 */
static inline art_node16* alloc_node16(void) {
    art_node16* node = calloc(1, sizeof(art_node16));
    node->n.type = NODE16;
    return node;
}

/**
 * Allocates a node48_origin,
 * initializes to zero and sets the type.
 */
static inline art_node48_origin* alloc_node48_origin(void) {
    art_node48_origin* node = calloc(1, sizeof(art_node48_origin));
    node->n.type = NODE48_origin;
    return node;
}

/**
 * Allocates a node48,
 * initializes to zero and sets the type.
 */
static inline art_node48* alloc_node48(void) {
    art_node48* node = calloc(1, sizeof(art_node48));
    node->n.type = NODE48;
    return node;
}

/**
 * Allocates a node256,
 * initializes to zero and sets the type.
 */
static inline art_node256* alloc_node256(void) {
    art_node256* node = calloc(1, sizeof(art_node256));
    node->n.type = NODE256;
    return node;
}

void art_get_latency_energy(){
    printf("[GET LATENCY ENERGY]\n");
    unsigned int * energy = art_get_energy();
    unsigned int * latency = art_get_latency();

    if(energy == NULL || latency == NULL){
        printf("energy or latency is NULL!!\n");
        return;
    }
    // printf("detect shift remove inject\n");
    for(int i=0; i<4; i++){
        if(i == 0){
            printf("[DETECT] ");
        }else if(i == 1){
            printf("[SHIFT] ");
        }else if(i == 2){
            printf("[REMOVE] ");
        }else{
            printf("[INJECT] ");
        }
        printf("energy: %d, ",energy[i]);
        printf("latency: %d\n",latency[i]);
    }
}
/**
 * Initializes an ART tree
 * @return 0 on success.
 */
int init_art_tree(art_tree *t) {
    t->root = NULL;
    t->size = 0;
    t->origin = false;
    origin_method = false;
    hole_space = create_table();
    init_artskrm(); 
    return 0;
}

char *get_node_partial(art_node *n) {
    switch (n->type) {
        case NODE4:
            return ((art_node4*)n)->partial;
        case NODE10:
            return ((art_node10*)n)->partial;
        case NODE48_origin:
            return ((art_node48_origin*)n)->n.partial;
        case NODE48:
            return ((art_node48*)n)->partial;
        case NODE256:
            return ((art_node256*)n)->partial;
        default:
            return NULL;
    }
}

int get_max_partial_len(art_node *n) {
    int max_partial_len = 0;
    switch (n->type) {
        case NODE4:
            max_partial_len = MAX_PREFIX_LEN_4;
            break;
        case NODE10:
            max_partial_len = MAX_PREFIX_LEN_10;
            break;
        case NODE48:
            max_partial_len = MAX_PREFIX_LEN_48;
            break;
        case NODE256:
            max_partial_len = MAX_PREFIX_LEN_256;
            break;
        default:
            break;
    }
    return max_partial_len;
}
// Recursively destroys the tree
static void destroy_node(art_node *n) {
    // printf("[DESTROY NODE]\n");
    // 在remove_child有處理skyrmion問題，在此先忽略leaf delete
    // Break if null
    if (!n) return;

    // Special case leafs
    if (IS_LEAF(n)) {
        free(LEAF_RAW(n));
        return;
    }

    // Handle each node type
    int i;
    union {
        art_node4 *p1;
        art_node10 *p5;
        art_node16 *p2;
        art_node48_origin *p6;
        art_node48 *p3;
        art_node256 *p4;
    } p;
    switch (n->type) {
        case NODE4:
            p.p1 = (art_node4*)n;
            for (i=0;i<n->num_children;i++) {
                destroy_node(p.p1->children[i]);
            }
            break;

        case NODE10:
            p.p5 = (art_node10*)n;
            for (i=0;i<n->num_children;i++) {
                destroy_node(p.p5->children[i]);
            }
            break;

        case NODE16:
            p.p2 = (art_node16*)n;
            for (i=0;i<n->num_children;i++) {
                destroy_node(p.p2->children[i]);
            }
            break;

        case NODE48_origin:
            p.p6 = (art_node48_origin*)n;
            for (i=0;i<n->num_children;i++) {
                destroy_node(p.p3->children[i]);
            }
            break;

        case NODE48:
            p.p3 = (art_node48*)n;
            for (i=0;i<n->num_children;i++) {
                destroy_node(p.p3->children[i]);
            }
            break;

        case NODE256:
            p.p4 = (art_node256*)n;
            for (i=0;i<256;i++) {
                if (p.p4->children[i])
                    destroy_node(p.p4->children[i]);
            }
            break;

        default:
            abort();
    }
    // Free ourself on the way up
    free(n);
}

/**
 * Destroys an ART tree
 * @return 0 on success.
 */
int destroy_art_tree(art_tree *t) {
    destroy_node(t->root);
    free_table(hole_space);
    return 0;
}

/**
 * Returns the size of the ART tree.
 */
extern inline uint64_t art_size(art_tree *t);



static art_node** find_child(art_node *n, unsigned char c) {
    int i, mask, bitfield;
    union {
        art_node4 *p1;
        art_node10 *p5;
        art_node16 *p2;
        art_node48 *p3;
        art_node48_origin *p6;
        art_node256 *p4;
    } p;
    switch (n->type) {
        case NODE4:
            p.p1 = (art_node4*)n;
            for (i=0;i < n->num_children; i++) {
                if (p.p1->keys[i] == c)
                    return &p.p1->children[i];
            }
            break;        

        case NODE10:
            p.p5 = (art_node10*)n;
            for (i=0;i < n->num_children; i++) {
                if (p.p5->keys[i] == c)
                    return &p.p5->children[i];
            }
            break;

        {
        __m128i cmp;
        case NODE16:
            p.p2 = (art_node16*)n;

            // Compare the key to all 16 stored keys
            cmp = _mm_cmpeq_epi8(_mm_set1_epi8(c),
                    _mm_loadu_si128((__m128i*)p.p2->keys));

            // Use a mask to ignore children that don't exist
            mask = (1 << n->num_children) - 1;
            bitfield = _mm_movemask_epi8(cmp) & mask;

            /*
             * If we have a match (any bit set) then we can
             * return the pointer match using ctz to get
             * the index.
             */
            if (bitfield)
                return &p.p2->children[__builtin_ctz(bitfield)];
            break;
        }

        case NODE48_origin:
            p.p6 = (art_node48_origin*)n;
            i = p.p6->keys[c];
            if (i)
                return &p.p6->children[i-1];
            break;

        case NODE48:
            p.p3 = (art_node48*)n;
            for (i=0;i < n->num_children; i++) {
                if (p.p3->keys[i] == c){
                    return &p.p3->children[i];
                }
            }
            break;

        case NODE256:
            p.p4 = (art_node256*)n;
            if (p.p4->children[c])
                return &p.p4->children[c];
            break;

        default:
            abort();
    }
    art_shift(WORD_SIZE * 2 * 8);   // check all children at one time => shift WORD_SIZE * 2
    art_detect(WORD_SIZE * 8);
    return NULL;
}

// Simple inlined if
static inline int min(int a, int b) {
    return (a < b) ? a : b;
}

/**
 * Returns the number of prefix characters shared between
 * the key and node.
 */
static int check_prefix(art_node *n, char *key, int key_len, int depth) {
    int max_cmp = 0;
    int idx = 0;
    char *partial = NULL;
    int max_partial_len = 0;
    
    switch (n->type) 
    {
        case NODE4:
            partial = ((art_node4*)n)->partial;
            max_partial_len = MAX_PREFIX_LEN_4;
            break;
        case NODE10:
            partial = ((art_node10*)n)->partial;
            max_partial_len = MAX_PREFIX_LEN_10;
            break;
        case NODE16:
            partial = ((art_node16*)n)->n.partial;
            max_partial_len = MAX_PREFIX_LEN_origin;
            break;
        case NODE48_origin:
            partial = ((art_node48_origin*)n)->n.partial;
            max_partial_len = MAX_PREFIX_LEN_origin;
            break;
        case NODE48:
            partial = ((art_node48*)n)->partial;
            max_partial_len = MAX_PREFIX_LEN_48;
            break;
        case NODE256:
            partial = ((art_node256*)n)->partial;
            max_partial_len = MAX_PREFIX_LEN_256;
            break;
        default:
            break;
    }
    max_cmp = min(min(n->partial_len, max_partial_len), key_len - depth);
    for (idx=0; idx < max_cmp; idx++) {
        if (partial[idx] != key[depth+idx])
            return idx;
    }
    return idx;
}

/**
 * Checks if a leaf matches
 * @return 0 on success.
 */
static int leaf_matches(art_leaf *n, char *key, int key_len, int depth) {
    //(void)明確標記 depth 為未使用參數，避免編譯器警告
    (void)depth;
    // Fail if the key lengths are different
    // skurmion did not compare key_len
    if (n->key_len != (uint32_t)key_len) return 1;

    // skyrmion key shift and detect to compare
    if(key_len < global_artskrm->WORD_SIZE){
        art_shift(key_len * 2 * 8);
        art_detect(key_len * 8);
    }
    else{
        art_shift(global_artskrm->WORD_SIZE * 2 * 8);
        art_detect(global_artskrm->WORD_SIZE * 8);
    }
   
    // Compare the keys starting at the depth
    //int memcmp(const void *str1, const void *str2, size_t n)
    // 如果返回值 = 0，则表示 str1 等于 str2 
    return memcmp(n->key, key, key_len);
}
// add by Mia
/*
static int leaf_matches_link(art_leaf_link *n, char *key, int key_len, int depth) {
    (void)depth;
    // Fail if the key lengths are different
    if (n->key_len != (uint32_t)key_len) return 1;

    // Compare the keys starting at the depth
    return memcmp(n->key, key, key_len);
}
*/
/**
 * Searches for a value in the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void* art_search(art_tree *t, char *key, int key_len) {
    art_node **child;
    art_node *n = t->root;
    int prefix_len, depth = 0;
    int max_partial_len = 0;
    while (n) {
        // Might be a leaf
        if (IS_LEAF(n)) {
            n = LEAF_RAW(n);
            // Check if the expanded path matches
            if (!leaf_matches((art_leaf*)n, key, key_len, depth)) {
                return ((art_leaf*)n)->value;
            }
            return NULL;
        }

        // Bail if the prefix does not match
        if (n->partial_len) {     
            art_shift(1 * 8 * 2); //shift to get partial len in prefix_info(the first byte)  
            art_detect(1 * 8);   // partial len is at the first byte of node in prefix_info
            max_partial_len = get_max_partial_len(n);
            prefix_len = check_prefix(n, key, key_len, depth);
            if (prefix_len != min(max_partial_len, n->partial_len))  //partial_len為prefix length
                return NULL;
            depth = depth + n->partial_len;
        }

        // Recursively search
        child = find_child(n, key[depth]);
        n = (child) ? *child : NULL;
        depth++;
    }
    return NULL;
}

// Find the minimum leaf under a node
static art_leaf* minimum(art_node *n) {
    // Handle base cases
    if (!n) return NULL;
    if (IS_LEAF(n)) return LEAF_RAW(n);

    int idx;
    switch (n->type) {
        case NODE4:
            return minimum(((art_node4*)n)->children[0]);
        case NODE10:
            return minimum(((art_node10*)n)->children[0]);
        case NODE16:
            return minimum(((art_node16*)n)->children[0]);

        case NODE48_origin:
            idx=0;
            while (!((art_node48_origin*)n)->keys[idx]) idx++;
            idx = ((art_node48_origin*)n)->keys[idx] - 1;
            return minimum(((art_node48_origin*)n)->children[idx]);
 
        case NODE48:
            return minimum(((art_node48*)n)->children[0]);
        case NODE256:
            idx=0;
            while (!((art_node256*)n)->children[idx]) idx++;
            return minimum(((art_node256*)n)->children[idx]);
        default:
            abort();
    }
}

// Find the maximum leaf under a node
static art_leaf* maximum(art_node *n) {
    // Handle base cases
    if (!n) return NULL;
    if (IS_LEAF(n)) return LEAF_RAW(n);

    int idx;
    switch (n->type) {
        case NODE4:
            return maximum(((art_node4*)n)->children[n->num_children-1]);
        case NODE10:
            return maximum(((art_node10*)n)->children[n->num_children-1]);
        case NODE16:
            return maximum(((art_node16*)n)->children[n->num_children-1]);
        
        case NODE48_origin:
            idx=255;
            while (!((art_node48_origin*)n)->keys[idx]) idx--;
            idx = ((art_node48_origin*)n)->keys[idx] - 1;
            return maximum(((art_node48_origin*)n)->children[idx]);
 
        case NODE48:
            return maximum(((art_node48*)n)->children[n->num_children-1]);
        case NODE256:
            idx=255;
            while (!((art_node256*)n)->children[idx]) idx--;
            return maximum(((art_node256*)n)->children[idx]);
        default:
            abort();
    }
}

/**
 * Returns the minimum valued leaf
 */
art_leaf* art_minimum(art_tree *t) {
    return minimum((art_node*)t->root);
}

/**
 * Returns the maximum valued leaf
 */
art_leaf* art_maximum(art_tree *t) {
    return maximum((art_node*)t->root);
}

static art_leaf* make_leaf(char *key, int key_len, void *value) {
    int val_len = strlen((char *) value);
    // printf("make_leaf, val: %s\n", (char *) value);
    art_leaf *l = malloc(sizeof(art_leaf)+key_len);
    l->value = value;
    l->key_len = key_len;
    memcpy(l->key, key, key_len);
    // printf("(key_len + val_len)  (MIN_UNIT * 4): %d\n", (key_len + val_len) % (MIN_UNIT * 4));
    // printf("(key_len + val_len) / (MIN_UNIT * 4): %f\n", (float)(key_len + val_len) / (MIN_UNIT * 4));
    // printf("(char *)value: %c\n", ((char *)value)[0]);
    if(track <= MAX_TRACK - ceil((float)(key_len + val_len)/(MIN_UNIT * 4))){
        leaf_track = track;
        if((key_len + val_len) % (MIN_UNIT * 4) != 0){
            int hole_domain = ceil((float)((key_len + val_len) % (MIN_UNIT * 4))/MIN_UNIT);
            int hole_track = leaf_track + (key_len + val_len) / (MIN_UNIT * 4);
            if(hole_domain == 1){
                set(hole_space, NODE4, hole_track, hole_domain);
                set(hole_space, NODE10, hole_track, hole_domain + 1);
            }else if(hole_domain == 2){
                set(hole_space, NODE10, hole_track, hole_domain + 1);
            }else {
                set(hole_space, NODE4, hole_track, hole_domain);
            }
        }
        track += ceil((float)(key_len + val_len)/(MIN_UNIT * 4));
    }else {
        // TODO
        // when memory full
        // check hole space when memory full
        printf("MEMORY FULL\n");
    }
    l->track_domain_id = art_track_domain_trans(leaf_track, leaf_domain);
    art_insert_leaf(key, value, l->track_domain_id);
    return l;
}

static unsigned int longest_common_prefix(art_leaf *l1, art_leaf *l2, int depth) {
    int max_cmp = min(l1->key_len, l2->key_len) - depth;
    int idx;
    for (idx=0; idx < max_cmp; idx++) {
        if (l1->key[depth+idx] != l2->key[depth+idx])
            return idx;
    }
    return idx;
}

// save all items in art_node except "type"
static void copy_header(art_node *dest, art_node *src) {
    dest->prefix_too_long = src->prefix_too_long;
    dest->partial_len = src->partial_len;
    dest->num_children = src->num_children;
    dest->track_domain_id = src->track_domain_id;
    // memcpy(dest->partial, src->partial, min(MAX_PREFIX_LEN, src->partial_len));
}

static void add_child256(art_node256 *n, art_node **ref, unsigned char c, void *child) {
    // printf("[add NODE256]\n n->n.num_children = %d\n", n->n.num_children);
    if(n->n.num_children == 48){

        uint32_t old_track = n->n.track_domain_id >> 2; //get first 30 bits
        
        if(track <= MAX_TRACK - ceil((float)NODE256_len/MAX_DOMAIN_LEN)){  //ceil(NODE256_len/MAX_DOMAIN_LEN) = 9
            node256_track = track;
            track += ceil((float)NODE256_len/MAX_DOMAIN_LEN);
            
            // save a node256 generate 3 node4 hole space
            set(hole_space, NODE4, node256_track+ceil((float)NODE256_len/MAX_DOMAIN_LEN)-1, 1);    //at domain == 1
            set(hole_space, NODE10, node256_track+ceil((float)NODE256_len/MAX_DOMAIN_LEN)-1, 2);   //at domain == 2
            // add current track_domain_id
            n->n.track_domain_id = art_track_domain_trans(node10_track, node10_domain);
            // trans node48 prefix_info, prefix to node256
            art_insert_node256(old_track, node256_track);
        }else{
            // when memory full need to find "hole"
            uint32_t *track = NULL;
            uint16_t *domain = NULL;
            bool *found = NULL;
            get(hole_space, NODE256, track, domain, found);
            if(*found){
                n->n.track_domain_id = art_track_domain_trans(*track, *domain);
            }
            else{
                printf("MEMORY FULL!\n");
                // TODO
                // when memory full need to find leaf "hole"
                return;
            }
            // insert node48 prefix_info, prefix to node256
            art_insert_node256(old_track, *track); 
        }

        int old_num_children = n->n.num_children;        
        // skyrmion inject 
        art_insert_node256_all_child(n->n.num_children, n->child_track_domain_id);    //save all children id before
        compare_and_insert(old_num_children, n->n.num_children);    //modify num_children        
        art_detect(2 * 8);  // detect num_children at the secound byte of node

        // only modify the 'type' difference
        art_change_type(NODE48, NODE256);
    } 
    (void)ref;
    n->children[c] = child;
    n->child_track_domain_id[c] = n->children[c]->track_domain_id;
    art_insert_child_id(n->child_track_domain_id[c]);   // skyrmion save new track_domain_id of new child in skyrmion
    n->n.num_children++;
}

static void add_child256_origin(art_node256 *n, art_node **ref, unsigned char c, void *child) {
    (void)ref;
    n->n.num_children++;
    n->children[c] = child;
}

static void add_child48_origin(art_node48_origin *n, art_node **ref, unsigned char c, void *child) {
    if (n->n.num_children < 48) {
        printf("[< 48] add origin child48\n");
        int pos = 0;
        while (n->children[pos]) pos++;
        n->children[pos] = child;
        n->keys[c] = pos + 1;
        n->n.num_children++;
    } else {
        art_node256 *new = alloc_node256();
        for (int i=0;i<256;i++) {
            if (n->keys[i]) {
                new->children[i] = n->children[n->keys[i] - 1];
            }
        }
        copy_header((art_node*)new, (art_node*)n);
        *ref = (art_node*)new;
        free(n);
        add_child256_origin(new, ref, c, child);
    }
}


static void add_child48(art_node48 *n, art_node **ref, unsigned char c, void *child) {
    if (n->n.num_children < 48) {
        // printf("[add NODE48] \nadd_child48 < 48: n->keys = '%c' (%d)\n", n->keys[0], n->keys[0]);   
        if(n->n.num_children == 10){

            uint8_t old_domain = n->n.track_domain_id % 4;  //get last 2 bits
            uint32_t old_track = n->n.track_domain_id >> 2; //get first 30 bits
            if(track > MAX_TRACK - NODE48_len/MAX_DOMAIN_LEN){
                uint32_t *track = NULL;
                uint16_t *domain = NULL;
                bool *found = NULL;
                // when memory full need to find "hole"
                get(hole_space, NODE48, track, domain, found);
                if(*found){
                    n->n.track_domain_id = art_track_domain_trans(*track, *domain);
                }else{
                    // TODO
                    // when memory still full need to find node256 or leaf "hole"
                    printf("MEMORY FULL!\n");
                    return;
                }
                art_trans_node10_to_node48(old_track, old_domain, *track);
            }else{
                if(node48_domain == 0){
                    node48_track = track;
                    track += NODE48_len/MAX_DOMAIN_LEN;                

                    // add current track_domain_id
                    n->n.track_domain_id = art_track_domain_trans(node48_track, node48_domain);
                    // save the skyrmion trans between node10 and node48
                    art_trans_node10_to_node48(old_track, old_domain, node48_track);
                }
            }              
            // only modify the 'type' difference
            art_change_type(NODE10, NODE48);
        }
        int old_num_children = n->n.num_children;
        int idx = 0;
        while ((n->keys[idx] < c)&& (idx < n->n.num_children)) idx++; //find the first empty position

        memmove(n->keys+idx+1, n->keys+idx, n->n.num_children - idx);
        memmove(n->children+idx+1, n->children+idx,
                (n->n.num_children - idx)*sizeof(void*));
        n->keys[idx] = c;
        n->children[idx] = child;
        // save child track_domain_id in tree
        n->child_track_domain_id[idx] = n->children[idx]->track_domain_id;
        n->n.num_children++;

        // skyrmion inject 
        art_insert_new_pair(n->keys[idx], n->children[idx]->track_domain_id);   // save child track_domain_id in skyrmion
        compare_and_insert(old_num_children, n->n.num_children);    //modify num_children
    } else {
        // printf("add_child48 > 48: n->n.num_children = %d\n", n->n.num_children);
        art_node256 *new = alloc_node256();
        for (int i=0;i<n->n.num_children;i++) {
            new->children[n->keys[i]] = n->children[i]; //related to "n->keys[c] = pos + 1;", so needs "-1"
        }
        copy_header((art_node*)new, (art_node*)n);      //save all art_node n information(including prefix_too_long, partial_len)
        memcpy(new->partial, n->partial, n->n.partial_len);
        *ref = (art_node*)new;
        // skyrmion delete all node48 key value pair because node256 will inject all new child_idd
        art_delete_node48_key_child_pair(n->n.num_children, n->keys, n->child_track_domain_id);
        // skyrmion mark the node track, domain and remain for free
        free_node(NODE48, n->n.track_domain_id);
        free(n);
        add_child256(new, ref, c, child);
    }
}

static void add_child16(art_node16 *n, art_node **ref, unsigned char c, void *child) {
    printf("in add_child16\n");
    if (n->n.num_children < 16) {
        __m128i cmp;
        printf("[< 16]\n");
        // Compare the key to all 16 stored keys
        cmp = _mm_cmplt_epi8(_mm_set1_epi8(c),
                _mm_loadu_si128((__m128i*)n->keys));
        // Use a mask to ignore children that don't exist
        unsigned mask = (1 << n->n.num_children) - 1;
        unsigned bitfield = _mm_movemask_epi8(cmp) & mask;
        // Check if less than any
        unsigned idx;
        if (bitfield) {
            idx = __builtin_ctz(bitfield);
            if (idx < 16) {
                memmove(n->keys+idx+1,n->keys+idx,n->n.num_children-idx);
                memmove(n->children+idx+1,n->children+idx,
                        (n->n.num_children-idx)*sizeof(void*));
            }
        } else{
            idx = n->n.num_children;
        }
        // Set the child
        n->keys[idx] = c;
        n->children[idx] = child;
        n->n.num_children++;
    } else {
        printf("[> 16]\n");
        art_node48_origin *new = alloc_node48_origin();
        // Copy the child pointers and populate the key map
        memcpy(new->children, n->children,
                sizeof(void*)*n->n.num_children);
        for (int i=0;i<n->n.num_children;i++) {
            new->keys[n->keys[i]] = i + 1;
        }
        copy_header((art_node*)new, (art_node*)n);
        *ref = (art_node*)new;
        free(n);
        add_child48_origin(new, ref, c, child);
    }
}

static void add_child10(art_node10 *n, art_node **ref, unsigned char c, void *child) {
    printf("[add NODE10]\n");
    if (n->n.num_children < 10) {
        // printf("[<10] add_child10 < 10: n->n.num_children = %d, c = %c\n", n->n.num_children, c);
        // update address 當node10_domain == 0，分配新的track給node10
        if(n->n.num_children == 4)
        {
            uint8_t old_domain = n->n.track_domain_id % 4;  //get last 2 bits
            uint32_t old_track = n->n.track_domain_id >> 2; //get first 30 bits
            if(track > MAX_TRACK-1){
                // when memory full need to find "hole"
                uint32_t *track = NULL;
                uint16_t *domain = NULL;
                bool * found = NULL;
                get_NODE10(hole_space, old_domain, track, domain, found) ;
                if(*found){
                    n->n.track_domain_id = art_track_domain_trans(*track, *domain);
                }else{
                    printf("MEMORY FULL!\n");
                    // TODO
                    // when memory still full need to find node48, node256 or leaf "hole"
                    return;
                }
                art_trans_node4_to_node10(old_track, old_domain, *track, *domain);
            }else {
                if(node10_domain == 0){
                    node10_track = track;
                    track += 1;                
                    // check if the old_domain is near node10_domain
                    if(abs(old_domain - node10_domain) > 1){
                        set(hole_space, NODE10, node10_track, node10_domain);   // 跳過這個space, 先紀錄著，等到memory full再分配
                        node10_domain = (node10_domain + NODE10_len) % MAX_DOMAIN_LEN;
                        if(node10_domain == 0){
                            if(track < MAX_TRACK-1){
                                track += 1;
                                node10_track = track;
                            }
                        }
                    }
                }else{
                    // update address 更新每次的node10_domain
                    node10_domain = (node10_domain + NODE10_len) % MAX_DOMAIN_LEN;
                }     
                // add current track_domain_id
                n->n.track_domain_id = art_track_domain_trans(node10_track, node10_domain);
                // compare 'track', 'domain' difference between node4 and node10
                art_trans_node4_to_node10(old_track, old_domain, node10_track, node10_domain);
            }       
            // only modify the 'type' difference
            art_change_type(NODE4, NODE10);
        }
        int old_num_children = n->n.num_children;
        int idx = 0;    // insert position
        while (n->keys[idx] < c && idx < n->n.num_children) idx++;
        // insert key in ART by order
        for(int i = n->n.num_children; i >= idx; i--){
            n->keys[i+1] = n->keys[i];
            n->children[i+1] = n->children[i];
        }
        n->keys[idx] = c;
        n->children[idx] = child;
        // save child track_domain_id in tree
        n->child_track_domain_id[idx] = n->children[idx]->track_domain_id;
        n->n.num_children++;

        // skyrmion inject 
        art_insert_new_pair(n->keys[idx], n->children[idx]->track_domain_id);   // save child track_domain_id in skyrmion
        compare_and_insert(old_num_children, n->n.num_children);    //modify num_children

    }else{
        // printf("add_child10 > 10: n->n.num_children = %d, c = %c\n", n->n.num_children, c);
        // new = 指向 art_node48 結構的指標
        art_node48 *new = alloc_node48();
        // Copy the child pointers and the key map
        // memcpy(b, a, sizeof(a)); // 把 a 的內容複製到 b
        memcpy(new->children, n->children,
                sizeof(void*)*n->n.num_children);
        memcpy(new->keys, n->keys,
            sizeof(unsigned char)*n->n.num_children);
        copy_header((art_node*)new, (art_node*)n);    // include prefix_too_long, track_domain_id is saving
           
        memcpy(new->child_track_domain_id, n->child_track_domain_id,
                sizeof(uint32_t)* n->n.num_children);
        memcpy(new->partial, n->partial, n->n.partial_len);
        *ref = (art_node*)new;
        // free node10 space before growing to node48
        free_node(NODE10, n->n.track_domain_id);
        free(n);
        add_child48(new, ref, c, child);
    }
    print_table(hole_space);
}

static void add_child4_origin(art_node4 *n, art_node **ref, unsigned char c, void *child) {
    printf("in add_child4_origin\n");
    if (n->n.num_children < 4) {
        printf("[<4] origin\n");
        int idx;
        for (idx=0; idx < n->n.num_children; idx++) {
            if (c < n->keys[idx]) break;
        }

        // Shift to make room
        memmove(n->keys+idx+1, n->keys+idx, n->n.num_children - idx);
        memmove(n->children+idx+1, n->children+idx,
                (n->n.num_children - idx)*sizeof(void*));

        // Insert element
        n->keys[idx] = c;
        n->children[idx] = child;
        n->n.num_children++;

    } else {
        printf("[>4] origin\n");
        art_node16 *new = alloc_node16();

        // Copy the child pointers and the key map
        memcpy(new->children, n->children,
                sizeof(void*)*n->n.num_children);
        memcpy(new->keys, n->keys,
                sizeof(unsigned char)*n->n.num_children);
        copy_header((art_node*)new, (art_node*)n);
        *ref = (art_node*)new;
        free(n);
        add_child16(new, ref, c, child);
    }
}


static void add_child4_modify(art_node4 *n, art_node **ref, unsigned char c, void *child) {
    //why isn't n->n.num_children <=4 but <4? Because 如果已經有 4 個，那就不能再加入了
    printf("[add NODE4]\n");
    if (n->n.num_children < 4) {
        // printf("[<4] add_child4 < 4, c = %c, n->n.num_children = %d, n->partial_len = %d\n",c, n->n.num_children, n->n.partial_len);
        // update address if node4 need new track allocate
        if(n->n.num_children == 0)
        {      
            if(track > MAX_TRACK-1){
                // when memory full need to find "hole"
                uint32_t *track = NULL;
                uint16_t *domain = NULL;
                bool *found = NULL;
                get(hole_space, NODE4, track, domain, found) ;
                if(*found){
                    n->n.track_domain_id = art_track_domain_trans(*track, *domain);
                }else{
                    printf("MEMORY FULL!\n");
                    // TODO
                    // when memory still full need to find node10, node48, node256 or leaf "hole"
                    return;
                }

                // TODO
                // 下次重新reuse free_node_n的時候用permutation write將原來的skyrmion數量根號來的比較，單位為word size
            }else {
                if(node4_domain == 0){
                    node4_track = track;
                    track += 1;
                }else{
                    // update address in skyrmion
                    node4_domain = (node4_domain+NODE4_len) % MAX_DOMAIN_LEN;
                }
                // add current track_domain_id
                n->n.track_domain_id = art_track_domain_trans(node4_track, node4_domain);
            }
            // insert_node4 in skyrmion
            art_insert_node4(n->n.prefix_too_long, n->n.partial_len, n->n.type);
        }
        int old_num_children = n->n.num_children;
        int idx;
        for (idx=0; idx < n->n.num_children; idx++) {
            // printf("in for in add node4");
            if (c < n->keys[idx]) break;
        }

        // Shift to make room
        memmove(n->keys+idx+1, n->keys+idx, n->n.num_children - idx);
        memmove(n->children+idx+1, n->children+idx,
                (n->n.num_children - idx)*sizeof(void*));

        // Insert element
        n->keys[idx] = c;
        n->children[idx] = child;
        // save child track_domain_id
        n->child_track_domain_id[idx] = n->children[idx]->track_domain_id;
        n->n.num_children++;
        
        // skyrmion inject of node
        art_insert_new_pair(n->keys[idx], n->children[idx]->track_domain_id);   //key, value pair of node
        compare_and_insert(old_num_children, n->n.num_children);    //change num_children
    } else {
        // printf("[>4] add_child4>4, c = %c, n->n.num_children = %d\n", c, n->n.num_children);
        
        //origin node16 part
        art_node16 *new16 = alloc_node16();
        memcpy(new16->children, n->children,
                sizeof(void*)*n->n.num_children);
        memcpy(new16->keys, n->keys,
                sizeof(unsigned char)*n->n.num_children);
        copy_header((art_node*)new16, (art_node*)n);
        *ref = (art_node*)new16;
        add_child16(new16, ref, c, child);

        // modeify node10 part
        art_node10 *new = alloc_node10();
        memcpy(new->children, n->children,
                sizeof(void*)*n->n.num_children);
        memcpy(new->keys, n->keys,
                sizeof(unsigned char)*n->n.num_children);
        copy_header((art_node*)new, (art_node*)n);  // include prefix_too_long, track_domain_id is saving
        memcpy(new->child_track_domain_id, n->child_track_domain_id,
                sizeof(uint32_t)* n->n.num_children);
        memcpy(new->partial, n->partial, n->n.partial_len);
        *ref = (art_node*)new;
        // free node4 track domain
        free_node(NODE4, n->n.track_domain_id);
        free(n);
        add_child10(new, ref, c, child);
    }
}


static void add_child4( art_node4 *n, art_node **ref, unsigned char c, void *child){
    if(origin_method){
        add_child4_origin(n, ref, c, child);
    }else{
        add_child4_modify(n, ref, c, child);
    }
}

void free_node(int type, uint32_t track_domain_id){
    uint32_t free_track = track_domain_id >> 2;
    uint16_t free_domain = track_domain_id % 4;
    set(hole_space, type, free_track, free_domain);  
    printf("free node n.track_domain_id: %d\n", track_domain_id);
}

void free_node_n(int type, uint32_t track_domain_id, void* n){
    uint32_t free_track = track_domain_id >> 2;
    uint16_t free_domain = track_domain_id % 4;
    set_with_node(hole_space, type, free_track, free_domain, n);  
    printf("free node count n.track_domain_id: %d\n", track_domain_id);
}
static void add_child(art_node *n, art_node **ref, unsigned char c, void *child) {
    // printf("add child\n");
    n->prefix_too_long = false;
    switch (n->type) {
        case NODE4:
            return add_child4( (art_node4*)n, ref, c, child);
        case NODE10:
            return add_child10((art_node10*)n, ref, c, child);
        case NODE16:
            return add_child16((art_node16*)n, ref, c, child);
        case NODE48_origin:
            return add_child48_origin((art_node48_origin*)n, ref, c, child);
        case NODE48:
            return add_child48((art_node48*)n, ref, c, child);
        case NODE256:
            return add_child256((art_node256*)n, ref, c, child);
        default:
            abort();
    }
}

/**
 * Calculates the index at which the prefixes mismatch
 */
static int prefix_mismatch(art_node *n, char *key, int key_len, int depth) {
    // printf("prefix_mismatch\n");
    int max_cmp = 0;
    int idx = 0;
    char *partial = NULL;
    int max_partial_len = 0;

    switch (n->type) 
    {
        case NODE4:
            partial = ((art_node4*)n)->partial;
            max_partial_len = MAX_PREFIX_LEN_4;
            break;
        case NODE10:
            partial = ((art_node10*)n)->partial;
            max_partial_len = MAX_PREFIX_LEN_10;
            break;
        case NODE16:
            partial = ((art_node16*)n)->n.partial;
            max_partial_len = MAX_PREFIX_LEN_origin;
        case NODE48_origin:
            partial = ((art_node48_origin*)n)->n.partial;
            max_partial_len = MAX_PREFIX_LEN_origin;
        case NODE48:
            partial = ((art_node48*)n)->partial;
            max_partial_len = MAX_PREFIX_LEN_48;
            break;
        case NODE256:
            partial = ((art_node256*)n)->partial;
            max_partial_len = MAX_PREFIX_LEN_256;
            break;
        default:
            break;
    }
    max_cmp = min(min(max_partial_len, n->partial_len), key_len - depth);
    for (idx=0; idx < max_cmp; idx++) {
        if (partial[idx] != key[depth+idx])
            return idx;
    }

    // If the prefix is short we can avoid finding a leaf
    if (n->partial_len > max_partial_len) {
        // Prefix is longer than what we've checked, find a leaf
        art_leaf *l = minimum(n);
        max_cmp = min(l->key_len, key_len)- depth;
        for (; idx < max_cmp; idx++) {
            if (l->key[idx+depth] != key[depth+idx])
                return idx; //回傳max length of相同prefix的idx
        }
    }
    return idx;
}
char *my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}

static void* recursive_insert(art_node *n, art_node **ref, char *key, unsigned int key_len, void *value, unsigned int depth, int *old) {
    // printf("in recursive_insert\n");
    int max_partial_len = 0;
    char *n_partial = NULL;
    // If we are at a NULL node, inject a leaf
    // when initial creating the tree, t->root == n == NULL
    if (!n) {
        // printf("NULL node\n");
        // 用 SET_LEAF(...) 標記成 tagged pointer（低位元 turn into 1）
        // 得到的 *ref 是一個「非對齊」的指標, so can't initialize *ref directly
        art_leaf *l = make_leaf(key, key_len, value);
        *ref = (art_node*)SET_LEAF(l);
        return NULL;
    }

    // If we are at a leaf, we need to replace it with a node
    if (IS_LEAF(n)) {
        art_leaf *l = LEAF_RAW(n);
        // printf("IS_LEAF, old_val:%s\n", (char *)l->value);

        // Check if we are updating an existing value 
        //leaf_matches() ==  0 if (l->key == key)
        if (!leaf_matches(l, key, key_len, depth)) {
            // printf("if leaf_matches\n");
            *old = 1;
            char *old_val = my_strdup(l->value);
            // printf("leaf_matches old_value:%s, new_val:%s\n",(char *)old_val, ((char *)value));
            l->value = value;   //replace original data to value from input
            // permutation write
            // printf("old val skrm count:%d\n", art_skyrmions_counter_str((char *)old_val));
            // printf("new val skrm count:%d\n", art_skyrmions_counter_str((char *)l->value));
            // printf("remove: %d\n",MAX(art_skyrmions_counter_str((char *)old_val) - art_skyrmions_counter_str((char *)l->value), 0));
            art_shift((global_artskrm->WORD_SIZE * 8 + 1) * 2 + MAX(art_skyrmions_counter_str(((char *)old_val)) - art_skyrmions_counter_str(((char *)l->value)), 0)); //shift to get val_len in prefix_info(the first byte)
            art_detect(global_artskrm->WORD_SIZE * 8); //detect if prefix_info
            art_remove(MAX(art_skyrmions_counter_str((char *)old_val) - art_skyrmions_counter_str((char *)l->value), 0)); //remove skyrmion if needed
            art_inject(MAX(art_skyrmions_counter_str((char *)l->value) - art_skyrmions_counter_str((char *)old_val), 0)); //inject skyrmion if needed

            return old_val; //return original data
        }
        // printf("if not leaf_matches\n");

        // New value, we must split the leaf into a node4
        art_node4 *new = alloc_node4();
        max_partial_len = MAX_PREFIX_LEN_4;
        // Create a new leaf
        art_leaf *l2 = make_leaf(key, key_len, value);

        // Determine longest prefix
        unsigned int longest_prefix = longest_common_prefix(l, l2, depth);
        new->n.partial_len = longest_prefix;
        memcpy(new->partial, key+depth, min(max_partial_len, longest_prefix));
        // Add the leafs to the new node4
        *ref = (art_node*)new;
        // Check bounds, prefix length can be equal to key length (foo and foobar)
        add_child4( new, ref,
            l->key_len > depth+longest_prefix ? l->key[depth+longest_prefix] : 0x00,
            SET_LEAF(l));
        add_child4( new, ref,
            l2->key_len > depth+longest_prefix ? l2->key[depth+longest_prefix] : 0x00,
            SET_LEAF(l2));
        return NULL;
    }

    // Check if given node has a prefix
    if (n->partial_len) {
        n_partial = get_node_partial(n);
        // Determine if the prefixes differ, since we need to split
        int prefix_diff = prefix_mismatch(n, key, key_len, depth);
        if ((uint32_t)prefix_diff >= n->partial_len) {
            depth += n->partial_len;
            goto RECURSE_SEARCH;
        }

        // Create a new node
        art_node4 *new = alloc_node4();
        max_partial_len = MAX_PREFIX_LEN_4;
        *ref = (art_node*)new;
        new->n.partial_len = prefix_diff;
        memcpy(new->partial, n_partial, min(max_partial_len, prefix_diff));

        // Adjust the prefix of the old node
        if (n->partial_len <= max_partial_len) {
            add_child4(new, ref, n_partial[prefix_diff], n);
            n->partial_len -= (prefix_diff+1);
            memmove(n_partial, n_partial+prefix_diff+1,
                    min(max_partial_len, n->partial_len));
        } else {    
            // if partial_len too long, only save the remaining part of prefix
            n->partial_len -= (prefix_diff+1);
           
            // if still too long after 減掉重複的partial len
            if(n->partial_len > max_partial_len){
                // TODO: give new space for prefix
                n->prefix_too_long = true;
                printf("PREFIX TOO LONG!!\n");
            }
            
            art_leaf *l = minimum(n);
            add_child4(new, ref, l->key[depth+prefix_diff], n);
            // 
            memcpy(n_partial, l->key+depth+prefix_diff+1,       
                    min(max_partial_len, n->partial_len));
        }
        // Insert the new leaf
        art_leaf *l = make_leaf(key, key_len, value);
        add_child4(new, ref, key[depth+prefix_diff], SET_LEAF(l));
        return NULL;
    }

RECURSE_SEARCH:;
    // printf("RECURSE_SEARCH\n");
    // Find a child to recurse to
    art_node **child = find_child(n, key[depth]);
    if (child) {
        return recursive_insert(*child, child, key, key_len, value, depth+1, old);
    }

    // No child, node goes within us
    art_leaf *l = make_leaf(key, key_len, value);
    add_child(n, ref, key[depth], SET_LEAF(l));
    return NULL;
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
void* art_insert(art_tree *t, char *key, int key_len, void *value, bool origin) {
    // printf("[ART INSERT]\n");
    char *copy_value = my_strdup((char *)value);    //** 一開始就複製一次 value, 確保不會受到共用pointer影響
    int old_val = 0;
    t->origin = origin;
    origin_method = origin;

    void *old = recursive_insert(t->root, &t->root, key, key_len, copy_value, 0, &old_val);
    if (!old_val) t->size++;
    return old;    
}

static void remove_child256_origin(art_node256 *n, art_node **ref, unsigned char c) {
    n->children[c] = NULL;
    n->n.num_children--;

    // Resize to a node48 on underflow, not immediately to prevent
    // trashing if we sit on the 48/49 boundary
    if (n->n.num_children == 37) {
        art_node48_origin *new = alloc_node48_origin();
        *ref = (art_node*)new;
        copy_header((art_node*)new, (art_node*)n);

        int pos = 0;
        for (int i=0;i<256;i++) {
            if (n->children[i]) {
                new->children[pos] = n->children[i];
                new->keys[i] = pos + 1;
                pos++;
            }
        }
        free(n);
    }
}

static void remove_child256(art_node256 *n, art_node **ref, unsigned char c) {
    n->children[c] = NULL;
    n->n.num_children--;
    int max_partial_len = 0;
    // Resize to a node48 on underflow, not immediately to prevent
    // trashing if we sit on the 48/49 boundary
    // 已經明顯低於 Node48 容量，值得執行縮減，可避免資料結構在邊界值來回震盪時造成效能問題。
    if (n->n.num_children <= 45) {
        max_partial_len = MAX_PREFIX_LEN_48;
        art_node48 *new = alloc_node48();
        *ref = (art_node*)new;
        copy_header((art_node*)new, (art_node*)n);
        int pos = 0;
        // tree add key, value pair in node48
        for (int i=0;i<256;i++) {
            if (n->children[i]) {
                new->keys[pos] = i;
                new->children[pos] = n->children[i];
                new->child_track_domain_id[pos] = n->child_track_domain_id[i];
                pos++;
            }
        }
        memcpy(new->partial, n->partial, min(max_partial_len, n->n.partial_len));
        free(n);
    }
}

static void remove_child48_origin(art_node48_origin *n, art_node **ref, unsigned char c) {
    int pos = n->keys[c];
    n->keys[c] = 0;
    n->children[pos-1] = NULL;
    n->n.num_children--;

    if (n->n.num_children == 12) {
        art_node16 *new = alloc_node16();
        *ref = (art_node*)new;
        copy_header((art_node*)new, (art_node*)n);

        int child = 0;
        for (int i=0;i<256;i++) {
            pos = n->keys[i];
            if (pos) {
                new->keys[child] = i;
                new->children[child] = n->children[pos - 1];
                child++;
            }
        }
        free(n);
    }
}

// static void remove_child48(art_node48 *n, art_node **ref, unsigned char c) {
static void remove_child48(art_node48 *n, art_node **ref, art_node **l) {
    int max_partial_len = 0;
    int pos = l - n->children;
    if(n->n.num_children == 45){
        // del_count = 0;

        // uint8_t old_domain = n->n.track_domain_id % 4;  //old_domain must be 0
        uint32_t old_track = n->n.track_domain_id >> 2; //get first 30 bits
        if(node48_domain == 0 && track <= MAX_TRACK - NODE48_len/MAX_DOMAIN_LEN){
            node48_track = track;
            track += NODE48_len/MAX_DOMAIN_LEN;
        }else{
            // TODO
            // when memory full need to find "hole"
            printf("MEMORY FULL!\n");
        }
        // add current track_domain_id
        n->n.track_domain_id = art_track_domain_trans(node48_track, node48_domain);
        // only modify the 'type' difference
        art_change_type(NODE256, NODE48);
        // trans node256 prefix_info, prefix to node48
        art_delete_node256(old_track, node48_track);   
        // skyrmion add key value pair in node48   
        art_insert_node48_key_child_pair(n->n.num_children, n->keys, n->child_track_domain_id);   
    }

    // delete key value pair in skyrmion and 往前移補上刪除空缺
    art_delete_pair(n->keys[pos], n->child_track_domain_id[pos]);
    memmove(n->keys+pos, n->keys+pos+1, n->n.num_children - 1 - pos);
    memmove(n->children+pos, n->children+pos+1, (n->n.num_children - 1 - pos)*sizeof(void*));
    n->keys[n->n.num_children-1] = 0;
    n->n.num_children--;

    if (n->n.num_children <= 8) {
        max_partial_len = MAX_PREFIX_LEN_10;
        art_node10 *new = alloc_node10();
        *ref = (art_node*)new;
        copy_header((art_node*)new, (art_node*)n);
        // tree shift children from node48 to node10
        for(int i=0; i<n->n.num_children; i++){
            new->keys[i] = n->keys[i];
            new->children[i] = n->children[i];
            new->child_track_domain_id[i] = n->child_track_domain_id[i];
        }
        // skyrmion check prefix
        if(n->n.partial_len > max_partial_len){
            new->n.prefix_too_long = true;
            printf("PRFIX TOO LONG !!\n");
            // need to find new place to save partial
            // memcmp(new->partial, new_track_domain_id, sizeof(uint32_t));
        }
        memcpy(new->partial, n->partial, min(max_partial_len, n->n.partial_len));
        free_node(NODE48, n->n.track_domain_id);
        free(n);
    }
}

static void remove_child16(art_node16 *n, art_node **ref, art_node **l) {
    int pos = l - n->children;
    memmove(n->keys+pos, n->keys+pos+1, n->n.num_children - 1 - pos);
    memmove(n->children+pos, n->children+pos+1, (n->n.num_children - 1 - pos)*sizeof(void*));
    n->n.num_children--;
    if (n->n.num_children == 3) {
        art_node4 *new = alloc_node4();
        *ref = (art_node*)new;
        copy_header((art_node*)new, (art_node*)n);
        memcpy(new->keys, n->keys, 4);
        memcpy(new->children, n->children, 4*sizeof(void*));
        free(n);
    }
}

static void remove_child10(art_node10 *n, art_node **ref, art_node **l) {
    int max_partial_len = 0;
    int pos = l - n->children;
    int old_num_children = n->n.num_children;
    // change position and type when node10 is created
    if(n->n.num_children == 8){
        // del_count = 0;  // init del_count for counting skyrmion by deleted key value pairs

        uint8_t old_domain = n->n.track_domain_id % 4;  //get last 2 bits
        uint32_t old_track = n->n.track_domain_id >> 2; //get first 30 bits
        if(node10_domain == 0 && track <= MAX_TRACK-1){
            node10_track = track;
            track += 1;
        }else{
            // TODO
            // find space for node10 but memory full need to find "hole"
            printf("MEMORY FULL!\n");   
        }
        // add current track_domain_id
        n->n.track_domain_id = art_track_domain_trans(node10_track, node10_domain);
        // only modify the 'type' difference
        art_change_type(NODE48, NODE10);
        // delete node48 in skyrmion
        // skyrmion shift children from node48 to node10
        art_delete_node48_to_node10(old_track, old_domain, node10_track); 
    }

    // delete key value pair in skyrmion
    art_delete_pair(n->keys[pos], n->child_track_domain_id[pos]);
    
    for(int i=pos;i<n->n.num_children-1;i++){
        n->keys[i] = n->keys[i+1];
        n->children[i] = n->children[i+1];
    }
    n->keys[n->n.num_children-1] = 0;
    n->n.num_children--;
    // modify num_children
    compare_and_insert(old_num_children, n->n.num_children); //modify num_children
    if (n->n.num_children <= 3) {
        max_partial_len = MAX_PREFIX_LEN_4;
        art_node4 *new = alloc_node4();
        *ref = (art_node*)new;
        copy_header((art_node*)new, (art_node*)n);
        memcpy(new->keys, n->keys, 4);
        memcpy(new->children, n->children, 4*sizeof(void*));
        memcpy(new->child_track_domain_id, n->child_track_domain_id,
                sizeof(uint32_t)* n->n.num_children);
        // (art_node4*)new->partial = n->partial;
        memcpy(new->partial, n->partial, min(max_partial_len, n->n.partial_len));

        free_node(NODE10, n->n.track_domain_id);
        free(n);
    }
}

static void remove_child4(art_node4 *n, art_node **ref, art_node **l) {
    int pos = l - n->children;
    char *child_partial = NULL; 
    int max_partial_len = MAX_PREFIX_LEN_4;
    if(n->n.num_children == 3){
        uint8_t old_domain = n->n.track_domain_id % 4;  //get last 2 bits
        uint32_t old_track = n->n.track_domain_id >> 2; //get first 30 bits
        if(node4_domain == 0 && track <= MAX_TRACK-1){
            node4_track = track;
            track += 1;
        }else{
            // TODO
            // find space for node4 but memory full need to find "hole"
            printf("MEMORY FULL!\n");
        }
        //  check node4_domain and node10_domain position
        if(abs(old_domain - node4_domain) > 1){
            if(node4_domain == 0 || node4_domain == 2){ // node4_domain == 0 or 1, so need to set hole_space
                set(hole_space, NODE4, node4_track, node4_domain);   // 跳過這個space, 先紀錄著，等到memory full再分配
                set(hole_space, NODE4, node4_track, node4_domain+NODE4_len);   // 跳過這個space, 先紀錄著，等到memory full再分配
                node4_domain = (node4_domain + NODE4_len * 2) % MAX_DOMAIN_LEN;
            }
            else{   //if (node4_domain == 1 || node4_domain == 3)
                set(hole_space, NODE4, node4_track, node4_domain);   // 跳過這個space, 先紀錄著，等到memory full再分配
                node4_domain = (node4_domain + NODE4_len) % MAX_DOMAIN_LEN;
            }
            if(old_domain == 0){   
                if(track < MAX_TRACK-1){
                    track += 1;
                    node4_track = track;
                }else{
                    // TODO
                    // when memory full need to find "hole"
                    printf("MEMORY FULL!\n");
                }
            }
        }
        // TODO
        // if partial len > node4 partial max len
        art_delete_node10_to_node4(old_track, old_domain, node4_track, node4_domain); // delete node10 in skyrmion
        // add current track_domain_id
        n->n.track_domain_id = art_track_domain_trans(node4_track, node4_domain);
        // only modify the 'type' difference
        art_change_type(NODE10, NODE4);
        // update address in skyrmion
        node4_domain = (node4_domain+NODE4_len) % MAX_DOMAIN_LEN;
    }
    memmove(n->keys+pos, n->keys+pos+1, n->n.num_children - 1 - pos);
    memmove(n->children+pos, n->children+pos+1, (n->n.num_children - 1 - pos)*sizeof(void*));
    n->n.num_children--;
    // Remove nodes with only a single child
    if (n->n.num_children == 1) {
        // int count = 0;
        art_node *child = n->children[0];
        child_partial = get_node_partial(child);
        if (!IS_LEAF(child)) {
            // Concatenate the prefixes
            int prefix = n->n.partial_len;
            if (prefix < max_partial_len) {
                n->partial[prefix] = n->keys[0];
                prefix++;
            }else{
                // TODO
                child->prefix_too_long = true;
                printf("PREFIX TOO LONG!!\n");
            }
            if (prefix < max_partial_len) {
                int sub_prefix = min(child->partial_len, max_partial_len - prefix);
                memcpy(n->partial+prefix, child_partial, sub_prefix);
                prefix += sub_prefix;
            }else{
                // TODO
                child->prefix_too_long = true;
                printf("PREFIX TOO LONG!!\n");
            }

            // Store the prefix in the child
            memcpy(child_partial, n->partial, min(prefix, max_partial_len));
            child->partial_len += n->n.partial_len + 1;
        }
        // count prefix_info
        // count += art_skyrmions_counter(trans_into_prefix_info(n->n.prefix_too_long, n->n.partial_len, n->n.type));
        // count num_children
        // count += art_skyrmions_counter(n->n.num_children);
        // count keys and child_track_domain_id
        // for(int i=0; i<n->n.num_children; i++){
        //     count += art_skyrmions_counter(n->keys[i]);
        //     count += art_skyrmions_counter(n->child_track_domain_id[i]);
        // }
        free_node_n(NODE4, n->n.track_domain_id, n);

        *ref = child;
        // free(n);
    }
}

static void remove_child(art_node *n, art_node **ref, unsigned char c, art_node **l) {
    switch (n->type) {
        case NODE4:
            return remove_child4((art_node4*)n, ref, l);
        case NODE10:
            return remove_child10((art_node10*)n, ref, l);
        case NODE16:
            return remove_child16((art_node16*)n, ref, l);
        case NODE48_origin:
            return remove_child48_origin((art_node48_origin*)n, ref, c);
        case NODE48:
            return remove_child48((art_node48*)n, ref, l);
        case NODE256:
            return remove_child256((art_node256*)n, ref, c);
        default:
            abort();
    }
}

static art_leaf* recursive_delete(art_node *n, art_node **ref, char *key, int key_len, int depth) {
    // Search terminated
    if (!n) return NULL;
    int max_prefix_len = get_max_partial_len(n);

    // Handle hitting a leaf node
    if (IS_LEAF(n)) {
        art_leaf *l = LEAF_RAW(n);
        if (!leaf_matches(l, key, key_len, depth)) {
            *ref = NULL;
            return l;
        }
        return NULL;
    }

    // Bail if the prefix does not match
    if (n->partial_len) {
        int prefix_len = check_prefix(n, key, key_len, depth);
        if (prefix_len != min(max_prefix_len, n->partial_len)) {
            return NULL;
        }
        depth += n->partial_len;
    }

    // Find child node
    art_node **child = find_child(n, key[depth]);
    if (!child) return NULL;

    // If the child is leaf, delete from this node
    if (IS_LEAF(*child)) {
        art_leaf *l = LEAF_RAW(*child);
        if (!leaf_matches(l, key, key_len, depth)) {
            remove_child(n, ref, key[depth], child);
            return l;
        }
        return NULL;

    // Recurse
    }

    return recursive_delete(*child, child, key, key_len, depth+1);
}

/**
 * Deletes a value from the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void* art_delete(art_tree *t, char *key, int key_len) {
    // printf("[art delete]\n");
    art_leaf *l = recursive_delete(t->root, &t->root, key, key_len, 0);
    int val_len = strlen((char *) l->value);
    if (l) {
        t->size--;
        void *old = l->value;
        // remove skyrmion in key and val and count shift ,detect and remove
        art_delete_leaf(l->key, l->value, l->key_len, val_len);
        // TODO
        // free_node(track, domain, 依照leaf len而free掉由不同node type組合的space)，例如leaf len長度3 x 32B，則free掉1 node4 + 1 node10
        free(l);
        return old;
    }
    return NULL;
}

// Recursively iterates over the tree
static int recursive_iter(art_node *n, art_callback cb, void *data) {
    // Handle base cases
    if (!n) return 0;
    if (IS_LEAF(n)) {
        art_leaf *l = LEAF_RAW(n);
        return cb(data, (const char*)l->key, l->key_len, l->value);
    }

    int res, idx;
    switch (n->type) {
        case NODE4:
            for (int i=0; i < n->num_children; i++) {
                res = recursive_iter(((art_node4*)n)->children[i], cb, data);
                if (res) return res;
            }
            break;

        case NODE10:
            for (int i=0; i < n->num_children; i++) {
                res = recursive_iter(((art_node10*)n)->children[i], cb, data);
                if (res) return res;
            }
            break;

        case NODE16:
            for (int i=0; i < n->num_children; i++) {
                res = recursive_iter(((art_node16*)n)->children[i], cb, data);
                if (res) return res;
            }
            break;

        case NODE48_origin:
            for (int i=0; i < 256; i++) {
                idx = ((art_node48_origin*)n)->keys[i];
                if (!idx) continue;

                res = recursive_iter(((art_node48_origin*)n)->children[idx-1], cb, data);
                if (res) return res;
            }
            break;

        case NODE48:
            for(int i=0; i < n->num_children; i++) {
                res = recursive_iter(((art_node48*)n)->children[i], cb, data);
                if (res) return res;
            }
            break;

        case NODE256:
            for (int i=0; i < 256; i++) {
                if (!((art_node256*)n)->children[i]) continue;
                res = recursive_iter(((art_node256*)n)->children[i], cb, data);
                if (res) return res;
            }
            break;

        default:
            abort();
    }
    return 0;
}

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
int art_iter(art_tree *t, art_callback cb, void *data) {
    return recursive_iter(t->root, cb, data);
}

/**
 * Checks if a leaf prefix matches
 * @return 0 on success.
 */
static int leaf_prefix_matches(art_leaf *n, char *prefix, int prefix_len) {
    // Fail if the key length is too short
    if (n->key_len < (uint32_t)prefix_len) return 1;

    // Compare the keys
    return memcmp(n->key, prefix, prefix_len);
}

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
int art_iter_prefix(art_tree *t, char *key, int key_len, art_callback cb, void *data) {
    art_node **child;
    art_node *n = t->root;
    int prefix_len, depth = 0;
    while (n) {
        // Might be a leaf
        if (IS_LEAF(n)) {
            n = LEAF_RAW(n);
            // Check if the expanded path matches
            if (!leaf_prefix_matches((art_leaf*)n, key, key_len)) {
                art_leaf *l = (art_leaf*)n;
                return cb(data, (const char*)l->key, l->key_len, l->value);
            }
            return 0;
        }

        // If the depth matches the prefix, we need to handle this node
        if (depth == key_len) {
            art_leaf *l = minimum(n);
            if (!leaf_prefix_matches(l, key, key_len))
               return recursive_iter(n, cb, data);
            return 0;
        }

        // Bail if the prefix does not match
        if (n->partial_len) {
            prefix_len = prefix_mismatch(n, key, key_len, depth);

            // If there is no match, search is terminated
            if (!prefix_len) {
                return 0;

            // If we've matched the prefix, iterate on this node
            } else if (depth + prefix_len == key_len) {
                return recursive_iter(n, cb, data);
            }

            // if there is a full match, go deeper
            depth = depth + n->partial_len;
        }

        // Recursively search
        child = find_child(n, key[depth]);
        n = (child) ? *child : NULL;
        depth++;
    }
    return 0;
}


// Recursively copies a tree
static art_node* recursive_copy(art_node *n) {
    // Handle the NULL nodes
    if (!n) return NULL;

    // Handle leaves
    if (IS_LEAF(n)) {
        art_leaf *l = LEAF_RAW(n);
        // Copy leaf
        l = make_leaf((char *)l->key, l->key_len, l->value);
        n = (art_node*)SET_LEAF(l);
        return n;
    }

    union {
        art_node4 *node4;
        art_node10 *node10;
        art_node16 *node16;
        art_node48_origin *node48_origin;
        art_node48 *node48;
        art_node256 *node256;
    } p;
    switch (n->type) {
        case NODE4:
            p.node4 = alloc_node4();
            copy_header((art_node*)p.node4, n);
            memcpy(p.node4->keys, ((art_node4*)n)->keys, 4);
            memcpy(p.node4->partial, ((art_node4*)n)->partial, MAX_PREFIX_LEN_4);
            for (int i=0; i < n->num_children; i++) {
                p.node4->children[i] = recursive_copy(((art_node4*)n)->children[i]);
            }
            return (art_node*)p.node4;

        case NODE10:
            p.node10 = alloc_node10();
            copy_header((art_node*)p.node10, n);
            memcpy(p.node10->keys, ((art_node10*)n)->keys, 10);
            memcpy(p.node10->partial, ((art_node10*)n)->partial, MAX_PREFIX_LEN_10);
            for (int i=0; i < n->num_children; i++) {
                p.node4->children[i] = recursive_copy(((art_node10*)n)->children[i]);
            }
            return (art_node*)p.node10;

        case NODE16:
            p.node16 = alloc_node16();
            copy_header((art_node*)p.node16, n);
            memcpy(p.node16->keys, ((art_node16*)n)->keys, 16);
            for (int i=0; i < n->num_children; i++) {
                p.node16->children[i] = recursive_copy(((art_node16*)n)->children[i]);
            }
            return (art_node*)p.node16;

        case NODE48_origin:
            p.node48_origin = alloc_node48_origin();
            copy_header((art_node*)p.node48_origin, n);
            memcpy(p.node48_origin->keys, ((art_node48_origin*)n)->keys, 256);
            for (int i=0; i < n->num_children; i++) {
                p.node48_origin->children[i] = recursive_copy(((art_node48_origin*)n)->children[i]);
            }
            return (art_node*)p.node48_origin;

        case NODE48:
            p.node48 = alloc_node48();
            copy_header((art_node*)p.node48, n);
            memcpy(p.node48->keys, ((art_node48*)n)->keys, 48);
            memcpy(p.node48->partial, ((art_node48*)n)->partial, MAX_PREFIX_LEN_48);
            for (int i=0; i < n->num_children; i++) {
                p.node48->children[i] = recursive_copy(((art_node48*)n)->children[i]);
            }
            return (art_node*)p.node48;

        case NODE256:
            p.node256 = alloc_node256();
            copy_header((art_node*)p.node256, n);
            memcpy(p.node256->partial, ((art_node256*)n)->partial, MAX_PREFIX_LEN_256);
            for (int i=0; i < 256; i++) {
                p.node256->children[i] = recursive_copy(((art_node256*)n)->children[i]);
            }
            return (art_node*)p.node256;

        default:
            abort();
    }
}

/**
 * Creates a copy of an ART tree. The two trees will
 * share the internal leaves, but will NOT share internal nodes.
 * This allows leaves to be added and deleted from each tree
 * individually. It is important that concurrent updates to
 * a given key has no well defined behavior since the leaves are
 * shared.
 * @arg dst The destination tree. Not initialized yet.
 * @arg src The source tree, must be initialized.
 * @return 0 on success.
 */
int art_copy(art_tree *dst, art_tree *src) {
    dst->size = src->size;
    dst->root = recursive_copy(src->root);
    return 0;
}

/**
 * Initializes an ART tree iterator.
 * @return Pointer to iterator or NULL.
 */
art_iterator* create_art_iterator(art_tree *tree) {
    art_iterator* iterator = malloc(sizeof(art_iterator));
    if (!iterator) {
        return NULL;
    }
    iterator->node = tree->root;
    iterator->pos = 0;
    iterator->is_leaf = false;
    ngx_queue_init(&iterator->queue);
    return iterator;
}

/**
 * Remove all queue's element and free memory.
 */
static void destroy_queue(ngx_queue_t *h) {
    ngx_queue_t *q;
    art_iterator *iterator;

    while(!ngx_queue_empty(h)) {
        q = ngx_queue_head(h);
        iterator = ngx_queue_data(q, art_iterator, queue);
        ngx_queue_remove(q);
        free(iterator);
    }
}

/**
 * Destroys an ART iterator.
 * @return 0 on success.
 */
int destroy_art_iterator(art_iterator *iterator) {
    destroy_queue(&iterator->queue);
    free(iterator);
    return 0;
}

// get next child of node
static inline art_node* iterator_get_child_node(art_iterator *iterator) {
    art_node *next, *node;
    node = iterator->node;

    if (IS_LEAF(node)) {
        // node is leaf
        if (iterator->is_leaf) return NULL;
        iterator->is_leaf = true;
        return node;
    }

    int idx;
    next = NULL;
    switch (node->type) {
        case NODE4:
            for (; iterator->pos < node->num_children; iterator->pos++) {
                next = ((art_node4*)node)->children[iterator->pos];
                if (next) break;
            }
            break;

        case NODE10:
            for (; iterator->pos < node->num_children; iterator->pos++) {
                next = ((art_node10*)node)->children[iterator->pos];
                if (next) break;
            }
            break;

        case NODE16:
            for (; iterator->pos < node->num_children; iterator->pos++) {
                next = ((art_node16*)node)->children[iterator->pos];
                if (next) break;
            }
            break;

        case NODE48_origin:
            for (; iterator->pos < 256; iterator->pos++) {
                idx = ((art_node48_origin*)node)->keys[iterator->pos];
                if (!idx) continue;
                next = ((art_node48_origin*)node)->children[idx-1];
                if (next) break;
            }
            break;

        case NODE48:
            for(; iterator->pos < node->num_children; iterator->pos++) {
                next = ((art_node48*)node)->children[iterator->pos];
                if (next) break;
            }
            break;

        case NODE256:
            for (; iterator->pos < 256; iterator->pos++) {
                next = ((art_node256*)node)->children[iterator->pos];
                if (next) break;
            }
            break;

        default:
            abort();
    }
    iterator->pos++;
    return next;
}

/**
 * Return next leaf element.
 * @return The next leaf or NULL
 */
art_leaf* art_iterator_next(art_iterator *iterator) {
    if (!iterator->node) return NULL;

    ngx_queue_t *q;
    art_iterator *current;
    art_node *node;
    do {
        q = ngx_queue_head(&iterator->queue);
        current = ngx_queue_data(q, art_iterator, queue);
        node = iterator_get_child_node(current);

        if ((node != NULL) & IS_LEAF(node)) {
            // we found leaf, return it
            return LEAF_RAW(node);
        } else if (node) {
            // we found node, go into it
            current = malloc(sizeof(art_iterator));
            current->pos = 0;
            current->is_leaf = false;
            current->node = node;
            ngx_queue_insert_tail(q, &current->queue);
        } else if (!ngx_queue_empty(&iterator->queue)) {
            // we found nothing, got to top
            ngx_queue_remove(q);
            free(current);
            continue;
        } else {
            // work is done
            return NULL;
        }

    } while(1);

    return NULL;
}
