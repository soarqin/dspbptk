#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "libdspbptk.h"

#define CHROMOSOME_LENGTH 32

// 枚举所有结构
typedef enum {
    assembler,
    // refine,
    // collider,
    smelter,
    // chemical,
    // lab,
    MODULE_COUNT
} module_enum_t;

typedef struct {
    double dx;
    double dy;
    blueprint_t blueprint;
} module_t;

void init_module(module_t* module) {
    // TODO 自动读取
    module[smelter].dx = (2.31118 + 0.0001);
    module[smelter].dy = (5.31118 + 0.0001);
    module[assembler].dx = (3.0416 + 0.0001);
    module[assembler].dy = (6.7416 + 0.0001);
    // TODO 自动读取end
}

void print_chromosome(module_enum_t* chromosome, size_t length) {
    for (size_t i = 0; i < length; i++)
        printf("%d", chromosome[i]);
    putchar('\n');
}

void search(module_enum_t* chromosome, size_t index) {
    for (module_enum_t i = 0; i < MODULE_COUNT; i++) {
        chromosome[index] = i;
        print_chromosome(chromosome, index + 1);
        if (index + 1 < CHROMOSOME_LENGTH)
            search(chromosome, index + 1);  // 尾递归，gcc自己会优化
    }
}

int main(void) {
    // TODO 输入参数处理

    // 读取模块
    module_t module[MODULE_COUNT];
    init_module(module);

    // 构造所有排列
    module_enum_t chromosome[CHROMOSOME_LENGTH] = {0};
    search(chromosome, 0);
}