#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "libdspbptk.h"

#define CHROMOSOME_LENGTH 64

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
    double area;
    double pow2_half_dx;
    blueprint_t blueprint;
} module_t;

void init_module(module_t* module) {
    // TODO 自动读取
    module[smelter].dx = (2.31118 + 0.0001);
    module[smelter].dy = (5.31118 + 0.0001);
    module[assembler].dx = (3.0416 + 0.0001);
    module[assembler].dy = (6.7416 + 0.0001);
    // TODO 自动读取end

    for (module_enum_t i = 0; i < MODULE_COUNT; i++) {
        module[i].pow2_half_dx = 0.25 * module[i].dx * module[i].dx;
        module[i].area = module[i].dx * module[i].dy;
    }
}

void print_detail(double score, module_enum_t* chromosome, size_t length, size_t cache_module_sum[MODULE_COUNT]) {
    printf("score=%lf\t", score);
    for (size_t i = 0; i < length; i++)
        printf("%d", chromosome[i]);
    printf(" : ");
    for (size_t i = 0; i < MODULE_COUNT; i++)
        printf("%lld,", cache_module_sum[i]);
    putchar('\n');
}

/**
 * @brief 计算一个结构因纬度导致变形后的y跨度，单位格
 *
 * @param pow2_half_dx 预计算参数，pow2_half_dx = (dx/2)^2
 * @param dy 结构在赤道时的y跨度
 * @param row_y_min 当前行y最小值点
 * @return double 结构变形后的y跨度
 */
double row_height_direct(double pow2_half_dx, double dy, double row_y_min) {
    // 计算当前位置距离地轴的半径长度
    double r_row_y_min = cos(row_y_min * (M_PI / 500.0)) * (500.0 / M_PI);
    // 计算y跨度补偿值
    // TODO 这里使用最保守的补偿方法，或许可以考虑进一步优化
    double y_corrected = r_row_y_min - sqrt(r_row_y_min * r_row_y_min - pow2_half_dx);
    return dy + y_corrected;
}

/**
 * @brief 估算在一个蓝图中，某种结构最多能放几个
 *
 * @param max_module_count 计算结果：每个模块最多放几个
 * @param module 模块的信息
 * @param need 每种模块的需求比例
 * @param max_x_span 允许的最大x跨度
 * @param max_y_span 允许的最大y跨度
 */
void compute_max_module_count(size_t* max_module_count, module_t* module, double* need, double max_x_span, double max_y_span) {
    double area_max = (max_x_span * (500.0 / M_PI)) /*扇面面积*/ * sin(max_y_span * (M_PI / 500.0)) /*低纬度占比*/;
    double area_module_sum = 0.0;
    for (module_enum_t i = 0; i < MODULE_COUNT; i++) {
        area_module_sum += module[i].area * need[i];
    }
    double k = area_max / area_module_sum;
    for (module_enum_t i = 0; i < MODULE_COUNT; i++) {
        max_module_count[i] = (size_t)ceil(k * need[i]);
    }
}

size_t compute_row_module_count(double row_y_max, double dx, double max_x_span) {
    double row_width = cos(row_y_max * (M_PI / 500.0)) * max_x_span;
    return (size_t)(row_width / dx);
}

double score(size_t cache_module_sum[MODULE_COUNT], double need[MODULE_COUNT], double row_y_max, double max_y_span) {
    if (row_y_max > max_y_span)
        return 0.0;
    double min_score = cache_module_sum[0] / need[0];
    for (module_enum_t module_enum = 1; module_enum < MODULE_COUNT; module_enum++) {
        double tmp_score = cache_module_sum[module_enum] / need[module_enum];
        if (tmp_score < min_score)
            min_score = tmp_score;
    }
    return min_score;
}

void search(
    double need[MODULE_COUNT],
    module_enum_t chromosome[CHROMOSOME_LENGTH],
    module_t module[MODULE_COUNT],
    size_t max_module_count[MODULE_COUNT],
    double cache_row_y_max[CHROMOSOME_LENGTH],
    size_t cache_module_sum[CHROMOSOME_LENGTH][MODULE_COUNT],
    int64_t index,
    double max_x_span,
    double max_y_span) {
    for (module_enum_t module_enum = 0; module_enum < MODULE_COUNT; module_enum++) {
        // 剪枝：检查某种建筑是否过多
        if (cache_module_sum[index - 1][module_enum] >= max_module_count[module_enum])
            continue;

        // 维护数据
        chromosome[index] = module_enum;
        cache_row_y_max[index] = cache_row_y_max[index - 1] + row_height_direct(module[module_enum].pow2_half_dx, module[module_enum].dy, cache_row_y_max[index - 1]);
        for (int i = 0; i < MODULE_COUNT; i++)
            cache_module_sum[index][i] = cache_module_sum[index - 1][i];
        cache_module_sum[index][module_enum] += compute_row_module_count(cache_row_y_max[index - 1], module[module_enum].dx, max_x_span);

        // 输出
        static double max_score = 0.0;
        double tmp_score = score(cache_module_sum[index], need, cache_row_y_max[index], max_y_span);
        if (tmp_score >= max_score) {
            print_detail(tmp_score, chromosome, index + 1, cache_module_sum[index]);
            max_score = tmp_score;
        }

        if (index + 1 < CHROMOSOME_LENGTH && cache_row_y_max[index] < max_y_span) {  // 剪枝：检查y跨度是否过大
            search(
                need,
                chromosome,
                module,
                max_module_count,
                cache_row_y_max,
                cache_module_sum,
                index + 1,
                max_x_span,
                max_y_span);  // 尾递归，gcc自己会优化
        }
    }
}

int main(void) {
    // TODO 输入参数处理
    double need[MODULE_COUNT] = {2.0, 1.0};
    double max_x_span = 100.0;
    double max_y_span = 200.0;

    // 读取模块
    module_t module[MODULE_COUNT];
    init_module(module);

    // 预计算数据
    size_t max_module_count[MODULE_COUNT];
    compute_max_module_count(max_module_count, module, need, max_x_span, max_y_span);
    printf("max_module_count:");
    for (int i = 0; i < MODULE_COUNT; i++) {
        printf("%lld,", max_module_count[i]);
    }
    putchar('\n');

    // 构造所有排列
    module_enum_t chromosome[CHROMOSOME_LENGTH] = {0};
    double cache_row_y_max[CHROMOSOME_LENGTH + 1] = {0.0};
    size_t cache_module_sum[CHROMOSOME_LENGTH + 1][MODULE_COUNT] = {0};
    search(
        need,
        chromosome,
        module,
        max_module_count,
        cache_row_y_max + 1,
        cache_module_sum + 1,
        0,
        max_x_span,
        max_y_span);
}