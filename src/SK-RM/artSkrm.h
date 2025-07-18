
#ifndef ARTSKRM_H
#define ARTSKRM_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_TRACK (1U << 30) //2^30 = 1073741824

typedef struct {
    int WORD_SIZE;

    int _Parallel_Write;
    int _Permutation_Write;
    int _Sky_Tree;
    int _Naive;
    int _Bit_Comparasion;
    int _degree_limit;

    int shift;  //Using "bit" unit to count shift
    int remove;
    int inject;
    int detect;
    int inject_energy;
    int remove_energy;
    int detect_energy;
    int inject_latency;
    int remove_latency;
    int detect_latency;
    int cycle;
} ARTskrm;

extern ARTskrm* global_artskrm;

void init_artskrm();
void art_clear();
void art_shift(int count);
void art_inject(int count);
void art_detect(int count);
void art_remove(int count);
void art_inject_e(int count);
void art_inject_l(int count);
int art_str_to_int_fast(const char* str);
int art_skyrmions_counter(uint32_t number);
int art_skyrmions_counter_char(char c);
int art_skyrmions_counter_str(char * str);
unsigned int * art_get_energy();
unsigned int * art_get_latency();
uint32_t art_track_domain_trans(uint32_t track, uint16_t domain);
void art_trans_track_domain(uint32_t track_domain_id, uint32_t* track, uint16_t* domain);
int trans_into_prefix_info(bool prefix_too_long, uint8_t type, uint8_t prefix_len);

void art_insert_leaf(const char* key, const char* value, uint32_t track_domain_id);
void art_insert_node256(uint32_t old_track, uint32_t new_track);
void art_insert_node256_all_child(int num_children, uint32_t * child_track_domain_id);
void art_delete_node48_key_child_pair(int num_children, unsigned char * keys, uint32_t * child_track_domain_id);
void art_trans_node10_to_node48(uint32_t old_track, uint16_t old_domain, uint32_t new_track) ;
void art_trans_node4_to_node10(uint32_t old_track, uint16_t old_domain, uint32_t new_track, uint16_t new_domain) ;                    
void art_insert_node4(bool prefix_too_long, uint8_t prefix_len, uint8_t type); 

void art_delete_leaf(unsigned char* key, void* value, uint32_t key_len, uint32_t val_len);
void art_delete_node256(uint32_t old_track, uint32_t new_track) ;
void art_delete_node256_all_children(int num_children, uint32_t * child_track_domain_id);
void art_insert_node48_key_child_pair(int num_children, unsigned char * keys, uint32_t * child_track_domain_id);
void art_delete_node48_to_node10(uint32_t old_track, uint16_t old_domain, uint32_t new_track) ;
void art_delete_node10_to_node4(uint32_t old_track, uint16_t old_domain, uint32_t new_track, uint16_t new_domain);

void art_insert_new_pair(unsigned char new_key, uint32_t new_val);
void art_delete_pair(unsigned char key, uint32_t val) ;
void art_insert_child_id(uint32_t new);
void art_change_type(unsigned char old_type, unsigned char new_type);

int shift_count(int compare1_len, int compare2_len);
void print_char_binary(unsigned char c);
void compare_and_insert(unsigned char old, unsigned char new);

#endif
