
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "artSkrm.h"

ARTskrm* global_artskrm = NULL;

int node4_skyrmion = 0;
int total_node4 = 0;
int node10_skyrmion = 0;
int total_node10 = 0;


static void ARTskrm_init(ARTskrm* self) {
    self->WORD_SIZE =  8; // 8 byte for inter port distance    
    self->shift = 0;
    self->remove = 0;
    self->inject_energy = 0;
    self->remove_energy = 0;
    self->detect_energy = 0;
    self->inject_latency = 0;
    self->remove_latency = 0;
    self->detect_latency = 0;
    self->origin_space = 0;
    self->modify_space = 0;
    self->origin_method = false;
    self->cycle = 0;
}

void init_artskrm() {
    // printf("init_artskrm\n");
    if (!global_artskrm) {
        global_artskrm = (ARTskrm*)malloc(sizeof(ARTskrm));
        if (!global_artskrm) {
            fprintf(stderr, "Failed to allocate ARTskrm\n");
            exit(1);
        }
        ARTskrm_init(global_artskrm);
    }
}

void art_clear() {
    if (global_artskrm) {
        ARTskrm_init(global_artskrm);
    }
}

void art_shift(int count){
    global_artskrm->shift += count;
}

void art_remove(int count){
    global_artskrm->remove += count;
    global_artskrm->remove_energy += count;
    global_artskrm->remove_latency += count;
}

void art_detect(int count){
    if (!global_artskrm) return;
    global_artskrm->detect += count;
    global_artskrm->detect_energy += count;
    global_artskrm->detect_latency += count;
}

void art_inject(int count) {
    if (!global_artskrm) return;
    global_artskrm->inject += count;
    global_artskrm->inject_latency += count;
    global_artskrm->inject_energy += count;
}

void art_inject_e(int count) {
    if (!global_artskrm) return;
    global_artskrm->inject_energy += count;
}

void art_inject_l(int count) {
    if (!global_artskrm) return;
    global_artskrm->inject_latency += count;
}

int art_str_to_int_fast(const char* str) {
    int result = 0;
    while (*str) {
        result = (result << 8) | (unsigned char)(*str);
        str++;
    }
    return result;
}

int art_skyrmions_counter_str(char * str) {
    int count = 0;
    for(unsigned int i = 0; i < strlen(str); i++) {
        count += art_skyrmions_counter((unsigned char)str[i]);
    }
    return count;
}

int art_skyrmions_counter_char(char c) {
    int count = 0;
    while (c) {
        c &= c - 0x1;
        count++;
    }
    return count;
}

int art_skyrmions_counter(uint32_t number) {
    int count = 0;
    while (number) {
        number &= number - 0x1;
        count++;
    }
    return count;
}

float get_avg_node4_skyr(int type){
    return (float) node4_skyrmion/total_node4;
}

float get_avg_node10_skyr(){
    return (float) node10_skyrmion/total_node10;
}

unsigned int * art_get_energy() {
    if (!global_artskrm) return NULL;
    unsigned int *result = malloc(sizeof(unsigned int)*5);
    result[0] = 2.0 * global_artskrm->detect_energy;
    result[1] = 20.0 * global_artskrm->shift;
    result[2] = 20.0 * global_artskrm->remove_energy;
    result[3] = 200.0 * global_artskrm->inject_energy;
    result[4] = result[0] + result[1] + result[2] + result[3];
    return result;
}

unsigned int * art_get_latency() {
    if (!global_artskrm) return NULL;
    unsigned int *result = malloc(sizeof(unsigned int)*5);
    result[0] = 0.1 * global_artskrm->detect_latency;
    result[1] = 0.5 * global_artskrm->shift;
    result[2] = 0.8 * global_artskrm->remove_latency;
    result[3] = 1.0 * global_artskrm->inject_latency;
    result[4] = result[0] + result[1] + result[2] + result[3];
    return result;
}

//return child_pointer
uint64_t art_child_pointer_trans(uint32_t track, uint16_t domain) {
    // printf("skrm art_track_domain_trans\n");
    // printf("skrm key track: %d, domain: %d\n", track, domain);

    return (track << 7) + domain;
}

//return track_domain_id
uint32_t art_track_domain_trans(uint32_t track, uint16_t domain) {
    // printf("skrm art_track_domain_trans\n");
    // printf("skrm key track: %d, domain: %d\n", track, domain);

    return (track << 2) + domain;
}
// get track and domain from track_domain_id
void art_trans_track_domain(uint32_t track_domain_id, uint32_t* track, uint16_t* domain) {
    *track = track_domain_id >> 2;
    *domain = (track_domain_id % 4);
}
// track_domain_id trans into real physical address track and domain
void art_trans_track_domain_physical(uint32_t track_domain_id, uint32_t* physical_track, uint16_t* physical_domain) {
    *physical_track = track_domain_id >> 2;
    *physical_domain = (track_domain_id % 4) * 32 * 8;
}

int trans_into_prefix_info(bool prefix_too_long, uint8_t type, uint8_t prefix_len) {
    return ((prefix_too_long ? 1 : 0) << 7) + (type << 5) + prefix_len;
}

void art_delete_leaf(unsigned char* key, void* value, uint32_t key_len, uint32_t val_len){
    if (!global_artskrm) return;
    // printf("[DELETE LEAF]\n");
    int skyrmions_count = 0;
    // printf("key_len: %d, val_len:%d\n", key_len, val_len);
    // printf("key: %s, val:%s\n", key, (char*)value);
    // count key inject skyrmion bit
    for (unsigned int i = 0; i < key_len; i++) {
        // printf("key[%d]=%d\n", i, key[i]);
        skyrmions_count += art_skyrmions_counter((unsigned char)key[i]);
    }
    // count value inject skyrmion bit
    for (unsigned int i = 0; i < val_len; i++) {
        // printf("value[%d] = %d\n", i, ((char *)value)[i]);
        skyrmions_count += art_skyrmions_counter(((char *) value)[i]);
    }

    art_shift(shift_count(key_len, val_len)); // global_artskrm->WORD_SIZE * 2;
    art_detect(shift_count(key_len, val_len));
    art_remove(skyrmions_count);
    global_artskrm->cycle += 1;
}

void art_insert_leaf_origin(const char* key, const char* value) {
    if (!global_artskrm) return;
    // printf("[INSERT LEAF] Insert key: %s val: %s\n", key, value);

    int key_len = strlen(key);
    int val_len = strlen(value);
    int skyrmions_count = 0;

    // count key inject skyrmion bit
    for (int i = 0; i < key_len; i++) {
        skyrmions_count += art_skyrmions_counter((unsigned char)key[i]);
    }
    // count value inject skyrmion bit
    for (int i = 0; i < val_len; i++) {
        skyrmions_count += art_skyrmions_counter((unsigned char)value[i]);
    }
    art_inject(skyrmions_count);
    art_shift(global_artskrm->WORD_SIZE * 8 * 2);

    global_artskrm->cycle += 1;
}

void art_insert_leaf(const char* key, const char* value, uint32_t track_domain_id) {
    if (!global_artskrm) return;
    // printf("[INSERT LEAF] Insert key: %s val: %s\n", key, value);

    int key_len = strlen(key);
    int val_len = strlen(value);
    int skyrmions_count = 0;

    // count key inject skyrmion bit
    for (int i = 0; i < key_len; i++) {
        skyrmions_count += art_skyrmions_counter((unsigned char)key[i]);
    }
    // count value inject skyrmion bit
    for (int i = 0; i < val_len; i++) {
        skyrmions_count += art_skyrmions_counter((unsigned char)value[i]);
    }
    
    // count physical address    
    // uint32_t physical_track;
    // uint16_t physical_domain;
    // art_trans_track_domain_physical(track_domain_id, &physical_track, &physical_domain);

    // add key to skyrmion
    // if ((physical_track + (physical_domain + key_len * 8) / (128 * 8)) < MAX_TRACK) {
    //     physical_track += (physical_domain + key_len * 8) / (128 * 8);
    //     physical_domain = (physical_domain + key_len * 8) % (128 * 8);
    // }
    // tracer_skyrmion_insert(physical_track, physical_domain, key, global_artskrm->cycle);

    // add value to skyrmion
    // if ((physical_track + (physical_domain + val_len * 8) / (128 * 8)) < MAX_TRACK) {
    //     physical_track += (physical_domain + val_len * 8) / (128 * 8);
    //     physical_domain = (physical_domain + val_len * 8) % (128 * 8);
    // }
    // tracer_skyrmion_insert(physical_track, physical_domain, value, global_artskrm->cycle);

    // printf("key: \n");
    // for(int i = 0; i < key_len; ++i) {
    //     print_char_binary(key[i]);
    // }
    // printf("val: ");
    // for(int i = 0; i < val_len; ++i) {
        // print_char_binary(value[i]);
    // }
    art_inject(skyrmions_count);
    // printf("skyrmions_count: %d\n", skyrmions_count);
    // printf("before skrm shift: %d\n", global_artskrm->shift);
    art_shift(shift_count(key_len, val_len)); // global_artskrm->WORD_SIZE * 2;
    // printf("after skrm shift: %d\n", global_artskrm->shift);

    global_artskrm->cycle += 1;
}
// trans node48 prefix_info, prefix to node256
void art_insert_node256(uint32_t old_track, uint32_t new_track){
    printf("[skrm NODE256]\n");
    int vertical_shift = ((abs(new_track - old_track)) * global_artskrm->WORD_SIZE * 2) * 8;

    printf("old_track: %d, new_track:%d\n", old_track, new_track);
    printf("vertical: %d\n", vertical_shift);
    art_shift(vertical_shift);
    global_artskrm->cycle += 1;
}

// trans node256 prefix_info, prefix to node48
void art_delete_node256(uint32_t old_track, uint32_t new_track) {
    printf("[skrm delete NODE256]\n");
    int vertical_shift = ((abs(new_track - old_track)) * global_artskrm->WORD_SIZE * 2) * 8;

    printf("old_track: %d, new_track:%d\n", old_track, new_track);
    printf("vertical: %d\n", vertical_shift);
    art_shift(vertical_shift);

    global_artskrm->cycle += 1;
}   

void art_insert_node48_key_child_pair(int num_children, unsigned char * keys, uint32_t * child_track_domain_id){
    int skyrmions_count = 0;
    for(int i=0; i<num_children; i++){
        skyrmions_count += art_skyrmions_counter(keys[i]);
        skyrmions_count += art_skyrmions_counter(child_track_domain_id[i]);
    }
    art_inject(skyrmions_count);
    art_shift(global_artskrm->WORD_SIZE * 8 + 1);

    global_artskrm->cycle += 1;
}

// delete all node48 key value pair because node256 will inject all new child_idd
void art_delete_node48_key_child_pair(int num_children, unsigned char * keys, uint32_t * child_track_domain_id){
    int skyrmions_count = 0;
    for(int i=0; i<num_children; i++){
        skyrmions_count += art_skyrmions_counter(keys[i]);
        skyrmions_count += art_skyrmions_counter(child_track_domain_id[i]);
    }
    art_remove(skyrmions_count);
    art_shift(global_artskrm->WORD_SIZE * 8 + 1);
    art_detect(global_artskrm->WORD_SIZE * 8);
    global_artskrm->cycle += 1;
}


void art_insert_node256_all_child(int num_children, uint32_t * child_track_domain_id){
    int child_id_skyrmion_count = 0;
    for(int i=0; i<num_children; i++){
        child_id_skyrmion_count += art_skyrmions_counter(child_track_domain_id[i]);
    }
    art_inject(child_id_skyrmion_count);
    art_shift( global_artskrm->WORD_SIZE * 8 + 1);

    global_artskrm->cycle += 1;
}

void art_delete_node256_all_children(int num_children, uint32_t * child_track_domain_id){
    int child_id_skyrmion_count = 0;
    for(int i=0; i<num_children; i++){
        child_id_skyrmion_count += art_skyrmions_counter(child_track_domain_id[i]);
    }
    art_remove(child_id_skyrmion_count);
    art_shift(global_artskrm->WORD_SIZE * 8 + 1);
    art_detect(global_artskrm->WORD_SIZE * 8);
    
    global_artskrm->cycle += 1;
}

void art_trans_node10_to_node48(uint32_t old_track, uint16_t old_domain, uint32_t new_track) {
    printf("[skrm NODE48]\n");
    int vertical_shift = ((abs(new_track - old_track)) * global_artskrm->WORD_SIZE * 2) * 8;
    int align_shift = 0;
    if(old_domain == 0){
        align_shift = 40 * 8 + 1;   // *8 means byte to bit
    }else{
        align_shift = 64 * 8 + 1;
    }
    // printf("old_track: %d, old_domain:%d, new_track:%d\n", old_track, old_domain, new_track);
    printf("vertical: %d, align: %d\n", vertical_shift, align_shift);
    art_shift(vertical_shift + align_shift);

    global_artskrm->cycle += 1;
}

void art_delete_node48_to_node10(uint32_t old_track, uint16_t old_domain, uint32_t new_track) {
    int vertical_shift = ((abs(new_track - old_track)) * global_artskrm->WORD_SIZE * 2) * 8;
    int align_shift = 0;
    if(old_domain == 0){
        align_shift = 40 * 8 + 1;   // *8 means byte to bit
    }else{
        align_shift = 64 * 8 + 1;
    }
    printf("vertical: %d, align: %d\n", vertical_shift, align_shift);
    art_shift(vertical_shift + align_shift);

    global_artskrm->cycle += 1;
}

void art_delete_node10_to_node4(uint32_t old_track, uint16_t old_domain, uint32_t new_track, uint16_t new_domain){
    int vertical_shift = ((abs(new_track - old_track)) * global_artskrm->WORD_SIZE * 2) * 8;
    int align_shift = 0;
    if(old_domain == new_domain){
        align_shift = 8 * 8 + 1;   // *8 means byte to bit
    }else{
        align_shift = 32 * 8 + 1;
    }
    art_shift(vertical_shift + align_shift);
    global_artskrm->cycle += 1;
}

bool art_trans_node4_to_node10(uint32_t old_track, uint16_t old_domain, uint32_t new_track, uint16_t new_domain) {
    // printf("[skrm NODE10]\n");
    int vertical_shift = 0;
    int align_shift = 0;
    
    if(old_domain == new_domain && abs(new_track - old_track) < 27){
        // part of shift? Ans.insert nonskyrmion, insert nonskyrmion energy == shift
        align_shift = 8 * 8 + 1;
        vertical_shift = ((abs(new_track - old_track)) * global_artskrm->WORD_SIZE * 2) * 8;
    }else if(old_domain != new_domain && abs(new_track - old_track) < 25){ // domain difference is 1
        align_shift = 32 * 8 + 1; 
        vertical_shift = ((abs(new_track - old_track)) * global_artskrm->WORD_SIZE * 2) * 8;  
    }else{
        return false;
    }
    
    art_shift(vertical_shift + align_shift);
    global_artskrm->cycle += 1;
    return true;
}

void art_insert_node4(bool prefix_too_long, uint8_t prefix_len, uint8_t type) {
    if (!global_artskrm) return;
    // printf("[INSERT NODE4]\n");
    unsigned char prefix_info = trans_into_prefix_info(prefix_too_long, type, prefix_len);
    // inject skyrmion of "prefix_info"
    int skyrmions_count = art_skyrmions_counter(prefix_info); //  + art_skyrmions_counter(num_of_child)
    
    // printf("prefix_info: ");
    // print_char_binary(prefix_info);
    // int num_of_child = 1;
    // printf("num_of_child: ");
    // print_char_binary(num_of_child);
    // printf("skyrmions_count: %d\n", skyrmions_count);
    // printf("before skrm node4 shift: %d\n", global_artskrm->shift);
    art_shift(5 * 8 + 1);    // 5 = prefix_info + num_of_child + prefix + key(only one key)
    // printf("after skrm node4 shift: %d\n", global_artskrm->shift);
    art_inject(skyrmions_count);

    global_artskrm->cycle += 1;
}

// for original type delete node
void art_delete_node_origin(uint8_t prefix_too_long, uint8_t type, uint16_t partial_len, char * partial, uint8_t num_children, unsigned char* key, uint64_t *child_pointers) {
    int skyrmions_count = 0;
    skyrmions_count += art_skyrmions_counter(prefix_too_long);
    skyrmions_count += art_skyrmions_counter(type);
    skyrmions_count += art_skyrmions_counter(partial_len);
    skyrmions_count += art_skyrmions_counter(num_children);
    for(int j=0; j<partial_len; j++){
        skyrmions_count += art_skyrmions_counter(partial[j]);
    }
    for(int i=0; i<num_children; i++){
        skyrmions_count += art_skyrmions_counter(key[i]) + art_skyrmions_counter(child_pointers[i]);        
    }
    if(type == 0){
        node4_skyrmion += skyrmions_count;
        total_node4 += 1;
    }else if(type == 1){
        node10_skyrmion += skyrmions_count;
        total_node10 += 1;
    }
    art_remove(skyrmions_count);
    art_shift(global_artskrm->WORD_SIZE * 8 * 2);
    art_detect(global_artskrm->WORD_SIZE * 8);
    
    global_artskrm->cycle += 1;
}

// for original type create node
void art_create_node_origin(uint8_t prefix_too_long, uint8_t type, uint16_t partial_len, char * partial) {
    int skyrmions_count = 0;
    skyrmions_count += art_skyrmions_counter(prefix_too_long);
    skyrmions_count += art_skyrmions_counter(type);
    skyrmions_count += art_skyrmions_counter(partial_len);
    for(int j=0; j<partial_len; j++){
        skyrmions_count += art_skyrmions_counter(partial[j]);
    }
    art_inject(skyrmions_count);
    art_shift(global_artskrm->WORD_SIZE * 8 * 2);
    
    global_artskrm->cycle += 1;
}


int shift_count(int compare1_len, int compare2_len) {
    int max_shift = (compare1_len < compare2_len) ? compare2_len  : compare1_len;
    return (max_shift > global_artskrm->WORD_SIZE) ? global_artskrm->WORD_SIZE * 8 : max_shift * 8;
}

void print_char_binary(unsigned char c) {
    for (int i = 7; i >= 0; i--) {
        printf("%d", (c >> i) & 1);
    }
    printf("\n");
}

void art_insert_child_id(uint32_t id){
    int skyrmions_count = art_skyrmions_counter(id);
    art_inject(skyrmions_count);

    global_artskrm->cycle += 1;
}

//naive write with delete and inject operation
void compare_and_insert_origin(unsigned char old, unsigned char new){
    art_remove(art_skyrmions_counter(old));    
    art_inject(art_skyrmions_counter(new));

    global_artskrm->cycle += 1;
}

//permutation write with xor operation
void compare_and_insert(unsigned char old, unsigned char new){
    int skyrmions_count = 0;
    // xor operation
    skyrmions_count += art_skyrmions_counter(old ^ new);
    art_inject(skyrmions_count);

    global_artskrm->cycle += 1;
}

// modify node type 
void art_change_type(unsigned char old_type, unsigned char new_type){
    int skyrmions_count = 0;
    // xor operation
    skyrmions_count += art_skyrmions_counter(old_type ^ new_type);
    art_inject(skyrmions_count);

    global_artskrm->cycle += 1;
}


void art_insert_new_pair(unsigned char new_key, uint32_t new_val){
    int skyrmions_count = 0;
    skyrmions_count += art_skyrmions_counter(new_key);
    skyrmions_count += art_skyrmions_counter(new_val);
    art_inject(skyrmions_count);    

    // printf("key: ");
    // print_char_binary(new_key);

    global_artskrm->cycle += 1;
}

void art_delete_pair(unsigned char key, uint32_t val) {
    int skyrmions_count = 0;
    skyrmions_count += art_skyrmions_counter(key);
    skyrmions_count += art_skyrmions_counter(val);

    printf("key: ");
    print_char_binary(key);

    global_artskrm->cycle += 1;
    global_artskrm->remove += skyrmions_count;
    global_artskrm->shift += global_artskrm->WORD_SIZE + 1; //將目前的key, value pair空缺往前補
}

