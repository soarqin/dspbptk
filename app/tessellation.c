#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "libdspbptk.h"

// TODO 排序：根据id顺序
typedef enum {
    none = -1,
    smelter,
    assembler,
    chemical,
    lab,
    collider,
    refine,
    MODULE_NUM
} module_enum_t;

typedef struct {
    double dx;
    double dy;
    blueprint_t blueprint;
} module_t;

typedef double need_t[MODULE_NUM];
typedef module_enum_t chromosome_t[32];

double score(need_t need, chromosome_t chromosome) {
}

int main(void) {
    module_t module[MODULE_NUM];
    // TODO 初始化模块
}
