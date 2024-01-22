#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "libdspbptk.h"

#define CHROMOSOME_LENGTH 32

// TODO 排序：根据id顺序
typedef enum {
    none = -1,
    smelter,
    assembler,
    // chemical,
    // lab,
    // collider,
    // refine,
    MODULE_NUM
} module_enum_t;

typedef struct {
    double dx;
    double dy;
    blueprint_t blueprint;
} module_t;

typedef double need_t[MODULE_NUM];
typedef module_enum_t chromosome_t[CHROMOSOME_LENGTH];

double score(need_t need, chromosome_t chromosome) {
    for
}

void init_module(module_t* module) {
    // TODO 自动读取
    module[smelter].dx = (2.31118 + 0.0001);
    module[smelter].dy = (5.31118 + 0.0001);
    module[assembler].dx = (3.0416 + 0.0001);
    module[assembler].dy = (6.7416 + 0.0001);
    // TODO 自动读取end
}

int main(void) {
    module_t module[MODULE_NUM];
    init_module(module);

}
