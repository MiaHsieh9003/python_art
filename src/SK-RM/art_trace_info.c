#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

// 定義 tracer 結構
typedef struct {
    FILE *trace_file;
    int enable;
    int domain_shift;
    int track_shift;
    int trace_count;
} tracer;

// 初始化
void tracer_init(tracer *t, const char *filename) {
    t->enable = 1;
    t->domain_shift = 6;
    t->track_shift = t->domain_shift + (int)(log2(4096)); // default 12
    t->trace_count = 0;

    t->trace_file = fopen(filename, "a");
    if (t->trace_file) {
        fprintf(t->trace_file, "NVMV 2\n");
        printf("[trace enabled]\n");
    }
}

// 設定 domain
void tracer_set_domain(tracer *t, int domain) {
    t->track_shift = t->domain_shift + (int)(log2(domain));
}

// 關閉 trace
void tracer_close(tracer *t) {
    if (t->trace_file) {
        fclose(t->trace_file);
    }
}

// address 計算
uint64_t tracer_calc_address(tracer *t, uint32_t track, uint32_t domain) {
    return ((uint64_t)track << t->track_shift) + ((uint64_t)domain << t->domain_shift);
}

// 字串轉 hex，填充 128 個字元
void str_to_hex(const char *value, char *hex_out) {
    if (!value || strlen(value) == 0) {
        memset(hex_out, '0', 128);
        hex_out[128] = '\0';
        return;
    }

    char bin_str[1024] = {0};
    for (size_t i = 0; i < strlen(value); i++) {
        char bin[9];
        snprintf(bin, sizeof(bin), "%08b", (unsigned char)value[i]);
        strcat(bin_str, bin);
    }

    // 將 binary string 轉 hex string
    unsigned char tmp[256] = {0};
    size_t bin_len = strlen(bin_str);
    for (size_t i = 0; i < bin_len; i += 4) {
        char chunk[5] = {0};
        strncpy(chunk, bin_str + i, 4);
        int val = (int)strtol(chunk, NULL, 2);
        sprintf((char *)tmp + strlen((char *)tmp), "%x", val);
    }

    // 填充長度
    strncpy(hex_out, (char *)tmp, 128);
    size_t tmp_len = strlen(hex_out);
    if (tmp_len < 128) {
        memset(hex_out + tmp_len, '0', 128 - tmp_len);
    }
    hex_out[128] = '\0';
}

// skyrmion insert
void tracer_skyrmion_insert(tracer *t, uint32_t track, uint32_t domain, const char *newvalue, int cycle) {
    if (!t->enable || !t->trace_file) return;

    uint64_t addr = tracer_calc_address(t, track, domain);

    char new_hex[129];
    str_to_hex(newvalue, new_hex);
    char zero[129];
    memset(zero, '0', 128);
    zero[128] = '\0';

    fprintf(t->trace_file, "%d I 0x%lX %s %s 0\n", cycle, addr, new_hex, zero);
}

// skyrmion delete
void tracer_skyrmion_delete(tracer *t, uint32_t track, uint32_t domain, const char *oldvalue, int cycle) {
    if (!t->enable || !t->trace_file) return;

    uint64_t addr = tracer_calc_address(t, track, domain);

    char old_hex[129];
    str_to_hex(oldvalue, old_hex);
    char zero[129];
    memset(zero, '0', 128);
    zero[128] = '\0';

    fprintf(t->trace_file, "%d D 0x%lX %s %s 0\n", cycle, addr, zero, old_hex);
}

// skyrmion read
void tracer_skyrmion_read(tracer *t, uint32_t track, uint32_t domain, const char *value, int cycle) {
    if (!t->enable || !t->trace_file) return;

    uint64_t addr = tracer_calc_address(t, track, domain);

    char hex_val[129];
    str_to_hex(value, hex_val);

    fprintf(t->trace_file, "%d R 0x%lX %s %s 0\n", cycle, addr, hex_val, hex_val);
}

// skyrmion write
void tracer_skyrmion_write(tracer *t, uint32_t track, uint32_t domain, const char *oldvalue, const char *newvalue, int cycle) {
    if (!t->enable || !t->trace_file) return;

    uint64_t addr = tracer_calc_address(t, track, domain);

    char old_hex[129];
    str_to_hex(oldvalue, old_hex);
    char new_hex[129];
    str_to_hex(newvalue, new_hex);

    fprintf(t->trace_file, "%d W 0x%lX %s %s 0\n", cycle, addr, new_hex, old_hex);
}

// 測試用
int main() {
    tracer t;
    tracer_init(&t, "trace_output.nvt");

    tracer_skyrmion_read(&t, 1, 64, "read_value", 100);
    tracer_skyrmion_write(&t, 1, 1024, "old_value", "new_value", 101);
    tracer_skyrmion_insert(&t, 1, 1024, "data_value", 102);
    tracer_skyrmion_delete(&t, 1, 1024, "data_value", 103);

    tracer_close(&t);

    return 0;
}
