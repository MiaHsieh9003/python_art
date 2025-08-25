#ifndef NODEDEF_H
#define NODEDEF_H

#define MAX_TRACK (1U << 30) //2^30 = 1073741824

// number for type
#define NODE4   0
#define NODE10  1
#define NODE16  4
#define NODE48  2
#define NODE48_origin 5
#define NODE256 3

// length in fundamental unit for modify ART
#define NODE4_len 1 //node 4 length is a fundamental unit in a track
#define NODE10_len 2
#define NODE48_len 8
#define NODE256_len 33

// length in byte for origin ART
#define NODE4_len_origin 48 // 48 byte a node
#define NODE16_len_origin 156
#define NODE48_len_origin 652
#define NODE256_len_origin 2060

// 32B for a unit
#define MIN_UNIT 32

// prefix length limit for origin node
#define MAX_PREFIX_LEN_origin 8
// prefix length limit for modify node
#define MAX_PREFIX_LEN_4 10
#define MAX_PREFIX_LEN_10 12
#define MAX_PREFIX_LEN_48 14
#define MAX_PREFIX_LEN_256 30

#define MAX_DOMAIN_LEN 4U // 0~3 => total 4 len
#define MAX_DOMAIN 3U // 0 ~ 3 => start from 0, 32B, 64B, 96B => total length 128*8 = 2^10 in a track

#define MAX_DOMAIN_LEN_origin 128 // 128B in a track

#endif
