#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    none = 0,
    smelter,
    assembler,
    chemical,
    lab,
    collider,
    refine,
    MODULE_NUM = refine
} module_t;

typedef double need_t[MODULE_NUM + 1];
typedef module_t chromosome_t[32];

double score(need_t need, chromosome_t chromosome) {
}

int main(void) {
}
