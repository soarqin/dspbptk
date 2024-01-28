#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
#define CHROMOSOME_LENGTH 64

typedef struct {
    double score;
    uint64_t score2;
    module_enum_t* chromosome;
    uint64_t rand;
} individual_t;

// 缓存评价函数中一行结构的相关数据
typedef struct {
    double row_y_max;
    size_t buildings_count;
} row_data_t[CHROMOSOME_LENGTH];

double row_r_scale_direct(double y) {
    return cos(y * (M_PI / 500.0));
}

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
        return row_height_direct(module_list[module_enum].half_dx_pow2, module_list[module_enum].dy, row_y_min);
}

double score(double x_span_equator, double need_y_max, const need_t need, const module_enum_t* chromosome, const module_t* module_list, row_data_t row_data) {
    // 计算y轴跨度
    row_data[0].row_y_max = row_height(chromosome[0], module_list, 0.0);

    for (int i = 1; i < CHROMOSOME_LENGTH; i++) {
        row_data[i].row_y_max = row_data[i - 1].row_y_max + row_height(chromosome[i], module_list, row_data[i - 1].row_y_max);
        // y跨度超过约束条件的直接给零分
        if (row_data[i].row_y_max > need_y_max)
            return 0.0;
    }

    // 模块总数
    size_t total_module[MODULE_NUM] = {0};

    // 计算每层的模块数量
    for (int i = i; i < CHROMOSOME_LENGTH; i++) {
        if (chromosome[i] == none) {
            row_data[i].buildings_count = 0;
        } else {
            module_enum_t module_type = chromosome[i];
            double x_span_here = x_span_equator * row_r_scale_direct(row_data[i].row_y_max);
            row_data[i].buildings_count = x_span_here / module_list[module_type].dx;
            total_module[module_type] += row_data[i].buildings_count;
        }
    }

    // 找出瓶颈的模块，作为最终评分
    double score_min = total_module[0] / need[0];
    for (module_enum_t i = 1; i < MODULE_NUM; i++) {
        double score_tmp = total_module[i] / need[i];
        if (score_tmp < score_min) {
            score_min = score_tmp;
        }
    }

    return score_min;
}

uint64_t score2(const module_enum_t* chromosome) {
    module_enum_t last_module = none;
    uint64_t cnt = 0;
    for (int i = 0; i < CHROMOSOME_LENGTH; i++) {
        if (chromosome[i] != none) {
            if (chromosome[i] != last_module)
                cnt++;
            last_module = chromosome[i];
        }
    }
    return cnt;
}

void init_module(module_t* module) {
    // TODO 自动读取
    module[smelter].dx = (2.31118 + 0.0001);
    module[smelter].dy = (5.31118 + 0.0001);
    module[assembler].dx = (3.0416 + 0.0001);
    module[assembler].dy = (6.7416 + 0.0001);
    // TODO 自动读取end

    // 预处理数据，打表
    for (int i = 0; i < MODULE_NUM; i++) {
        module[i].half_dx_pow2 = module[i].dx / 2.0;
        module[i].half_dx_pow2 *= module[i].half_dx_pow2;
    }
}

int cmp_individual(const void* a, const void* b) {
    double sa = ((individual_t*)a)->score;
    double sb = ((individual_t*)b)->score;
    if (sa != sb)
        return sa > sb ? 1 : -1;  // 优先考虑密度
    else
        return ((individual_t*)b)->score2 - ((individual_t*)a)->score2;  // 密度相同时，建筑排列越简单越好
}

uint64_t individual_rand(individual_t* individual) {
    individual->rand = random_next(individual->rand);
    return individual->rand;
}

int main(void) {
    module_t module[MODULE_NUM];
    init_module(module);

    // 需求
    double need_x_span_equator = 100.0;  // 允许最大的在赤道的x跨度
    double need_y_span = 200.0;          // 允许最大的y跨度
    need_t need = {2.0, 1.0};            // 需要的建筑的比例

    // 遗传算法参数
    const size_t group_size = 65536;    // 种群数量
    const double mutation_rate = 0.02;  // 变异概率
    const size_t iteration = 500;       // 迭代次数

    // 创建种群
    individual_t* individual = calloc(group_size, sizeof(individual_t));
    time_t now = time(NULL) * CLOCKS_PER_SEC + clock();
    for (size_t i = 0; i < group_size; i++) {
        individual[i].rand = now + i;
        individual[i].chromosome = calloc(CHROMOSOME_LENGTH, sizeof(module_enum_t));
        for (size_t j = 0; j < CHROMOSOME_LENGTH; j++) {
            individual[i].chromosome[j] = (individual_rand(&individual[i]) % (MODULE_NUM + 1)) - 1;
        }
        row_data_t row_data;
        individual[i].score = score(need_x_span_equator, need_y_span, need, individual[i].chromosome, module, row_data);
        individual[i].score2 = score2(individual[i].chromosome);
    }
    qsort(individual, group_size, sizeof(individual_t), cmp_individual);
    fprintf(stderr, "%lld种群创建完成\n", group_size);

    // 进化迭代
    for (size_t i = 1; i <= iteration; i++) {
        // 统计数据
        double score_sum = 0.0;
        size_t best_index = 0;
        for (int j = 0; j < group_size; j++) {
            score_sum += individual[j].score;
            if (cmp_individual(&individual[j], &individual[best_index]) > 0)
                best_index = j;
        }
        fprintf(stderr, "\n第%lld轮迭代:\t平均分=%.7lf,best_index=%lld", i, score_sum / group_size, best_index);

        // 交叉(单点交叉,二元锦标赛选择算子)
#pragma omp parallel for
        for (int j = 0; j < best_index; j++) {
            size_t cross_point = individual_rand(&individual[j]) % (CHROMOSOME_LENGTH - 1) + 1;
            size_t father1 = individual_rand(&individual[j]) % group_size;
            size_t father2 = individual_rand(&individual[j]) % group_size;
            size_t mother1 = individual_rand(&individual[j]) % group_size;
            size_t mother2 = individual_rand(&individual[j]) % group_size;
            size_t father = cmp_individual(&individual[father1], &individual[father2]) > 0 ? father1 : father2;
            size_t mother = cmp_individual(&individual[mother1], &individual[mother2]) > 0 ? mother1 : mother2;

            for (int k = 0; k < cross_point; k++)
                individual[j].chromosome[k] = individual[father].chromosome[k];
            for (int k = cross_point; k < CHROMOSOME_LENGTH; k++)
                individual[j].chromosome[k] = individual[mother].chromosome[k];
        }
        fprintf(stderr, "交叉完成,");

        // 变异(随机变异/交换突变)
#pragma omp parallel for
        for (int j = 0; j < group_size; j++) {
            while (to_double(individual_rand(&individual[j])) < mutation_rate) {
                size_t mutation_point_a = individual_rand(&individual[j]) % CHROMOSOME_LENGTH;
                individual[j].chromosome[mutation_point_a] = (individual_rand(&individual[j]) % (MODULE_NUM + 1)) - 1;
            }
            while (to_double(individual_rand(&individual[j])) < mutation_rate) {
                size_t mutation_point_b = individual_rand(&individual[j]) % (CHROMOSOME_LENGTH - 1);
                size_t mutation_point_c = individual_rand(&individual[j]) % (CHROMOSOME_LENGTH - 1);
                module_enum_t tmp = individual[j].chromosome[mutation_point_b];
                individual[j].chromosome[mutation_point_b] = individual[j].chromosome[mutation_point_c];
                individual[j].chromosome[mutation_point_c] = tmp;
            }
        }
        fprintf(stderr, "变异完成,");

        // 更新评分
#pragma omp parallel for
        for (int j = 0; j < group_size; j++) {
            row_data_t row_data;
            individual[j].score = score(need_x_span_equator, need_y_span, need, individual[j].chromosome, module, row_data);
            individual[j].score2 = score2(individual[j].chromosome);
        }

        qsort(individual, group_size, sizeof(individual_t), cmp_individual);
        fprintf(stderr, "最优解: score=%.7lf score2=%lld\t", individual[group_size - 1].score, individual[group_size - 1].score2);
        for (int j = 0; j < CHROMOSOME_LENGTH; j++) {
            if (individual[group_size - 1].chromosome[j] != none)
                printf("%d", individual[group_size - 1].chromosome[j]);
        }
    }

    // 输出结果

    // 释放所有内存
    for (size_t i = 0; i < group_size; i++)
        free(individual[i].chromosome);
    free(individual);
}
