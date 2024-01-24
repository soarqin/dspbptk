#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "libdspbptk.h"

// 枚举所有结构在遗传算法中基因的编码，注意none不是零而是-1
typedef enum {
    none = -1,
    assembler,
    // refine,
    // collider,
    smelter,
    // chemical,
    // lab,
    MODULE_NUM
} module_enum_t;

// 缓存一个结构的相关数据
typedef struct {
    double dx;
    double dy;
    double half_dx_pow2;  // half_dx_pow2 = (dx/2)^2
    blueprint_t blueprint;
} module_t;

// 需求向量
typedef double need_t[MODULE_NUM];

// 染色体
#define CHROMOSOME_LENGTH 32
typedef module_enum_t chromosome_t[CHROMOSOME_LENGTH];

// 缓存评价函数中一行结构的相关数据
typedef struct {
    double row_y_max;
    size_t buildings_count;
} row_data_t[CHROMOSOME_LENGTH];

/**
 * @brief 计算一个结构因纬度导致变形后的y跨度，单位格
 *
 * @param half_dx_pow2 预计算参数，half_dx_pow2 = (dx/2)^2
 * @param dy 结构在赤道时的y跨度
 * @param row_y_min 当前行y最小值点
 * @return double y跨度
 */
double row_height_direct(double half_dx_pow2, double dy, double row_y_min) {
    // 计算当前位置距离地轴的半径长度
    double r_row_y_min = cos(row_y_min * (M_PI / 500.0)) * (500.0 / M_PI);
    // 计算y跨度补偿值
    // TODO 这里使用最保守的补偿方法，或许可以考虑进一步优化
    double y_corrected = r_row_y_min - sqrt(r_row_y_min * r_row_y_min - half_dx_pow2);
    return dy + y_corrected;
}

double row_height(module_enum_t module_enum, const module_t* module_list, double row_y_min) {
    if (module_enum == none)
        return 0.0;
    else
        return row_height_direct(module_list[module_enum].dx, module_list[module_enum].dy, row_y_min);
}

double score(double need_x_max, double need_y_max, const need_t need, const chromosome_t chromosome, const module_t* module_list, row_data_t row_data) {
    // 计算y轴跨度
    row_data[0].row_y_max = row_height(chromosome[0], module_list, 0.0);
    for (int i = 1; i < CHROMOSOME_LENGTH; i++)
        row_data[i].row_y_max = row_data[i - 1].row_y_max + row_height(chromosome[i], module_list, row_data[i - 1].row_y_max);
    // y跨度超过约束条件的直接给零分
    if (row_data[CHROMOSOME_LENGTH - 1].row_y_max > need_y_max)
        return 0.0;

    // 模块总数
    size_t total_module[MODULE_NUM] = {0};

    // 计算每层的模块数量

    // 找出瓶颈的模块，作为最终评分
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
