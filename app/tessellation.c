#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "libdspbptk.h"

#define SIZE_CACHE_CHROMOSOME 4096
#define CHROMOSOME_LENGTH 64

// 枚举所有结构，按建筑的itemId顺序
typedef enum {
    assembler,
    // refine,
    // collider,
    smelter,
    // chemical,
    // lab,
    MODULE_COUNT
} module_enum_t;

const char* MODULE_PATH[MODULE_COUNT] = {
    ".\\module\\dense_unit_assembler.txt",
    // "module\\dense_unit_refine.txt",
    // "module\\dense_unit_collider.txt",
    ".\\module\\dense_unit_smelter.txt",
    // "module\\dense_unit_chemical.txt",
    // "module\\dense_unit_lab.txt",
};

typedef struct {
    double dx;
    double dy;
    double area;
    double pow2_half_dx;
    blueprint_t blueprint;
} module_t;

size_t read_dspbptk_module(module_t* module) {
    size_t argc = sscanf(module->blueprint.desc, "dspbptkmodule_-x_%lf_-y_%lf", &module->dx, &module->dy);
    return argc;
}

// TODO 异常处理
void init_module(module_t* module, dspbptk_coder_t* coder) {
    for (module_enum_t i = 0; i < MODULE_COUNT; i++) {
        // 读取蓝图
        FILE* fp = fopen(MODULE_PATH[i], "r");
        dspbptk_error_t errorlevel = blueprint_decode_file(coder, &module[i].blueprint, fp);
        fclose(fp);
        // 从简介读取模块信息
        size_t argc = read_dspbptk_module(&module[i]);
        if (argc == 2) {
            fprintf(stderr, "Registered dspbptk module: %s\n", MODULE_PATH[i]);
        } else {
            fprintf(stderr, "Error in registering dspbptk module \"%s\": %s\n", MODULE_PATH[i], module[i].blueprint.desc);
        }
    }

    for (module_enum_t i = 0; i < MODULE_COUNT; i++) {
        module[i].pow2_half_dx = 0.25 * module[i].dx * module[i].dx;
        module[i].area = module[i].dx * module[i].dy;
    }
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

/**
 * @brief 估算在特定纬度的行最多能放下多少特定结构
 *
 * @param row_y_max 当前行y坐标最大处的值
 * @param dx 模块尺寸x
 * @param max_x_span 整个蓝图允许的赤道处最大x跨度
 * @return size_t 最多能放下多少特定结构
 */
size_t compute_row_module_count(double row_y_max, double dx, double max_x_span) {
    return (size_t)(cos(row_y_max * (M_PI / 500.0)) * (max_x_span / dx));
}

/**
 * @brief 客观评分，只考虑密度
 *
 * @param cache_module_sum 缓存当前染色体表达出排列的各种模块总数，这个列表在search()中被维护
 * @param need 对不同模块的需求量
 * @param row_y_max 当前行y坐标最大处的值
 * @param max_y_span 整个蓝图允许的最大y跨度
 * @return double 评分，正比于理论最大产能，越高越好
 */
double objective_score(size_t cache_module_sum[MODULE_COUNT], const double need[MODULE_COUNT], double row_y_max, double max_y_span) {
    // y超标的直接判零分，拍不下去==产能为零
    if (row_y_max > max_y_span)
        return 0.0;
    double min_score = cache_module_sum[0] / need[0];
    // 找出最缺的建筑作为分数
    for (module_enum_t module_enum = 1; module_enum < MODULE_COUNT; module_enum++) {
        double tmp_objective_score = cache_module_sum[module_enum] / need[module_enum];
        if (tmp_objective_score < min_score)
            min_score = tmp_objective_score;
    }
    return min_score;
}

/**
 * @brief 主观评分，这个随便写都行不影响搜索
 *
 * @param chromosome 染色体，可以表达一个唯一的建筑排列（遗传算法那个）
 * @param length 染色体的有效长度
 * @return size_t 主观评分，正比于排列的复杂度，越低越简洁（主观上越好看）
 */
size_t subjective_score(const module_enum_t chromosome[CHROMOSOME_LENGTH], size_t length) {
    size_t score = 0;
    for (size_t i = 1; i < length; i++) {
        if (chromosome[i - 1] != chromosome[i])
            score++;
    }
    return score;
}

void print_detail(double score, size_t score2, module_enum_t* chromosome, size_t length, size_t cache_module_sum[MODULE_COUNT]) {
    printf("score = {%lf, %lld}\tchromosome = ", score, score2);
    for (size_t i = 0; i < length; i++)
        printf("%d", chromosome[i]);
    printf("\tmodule_sum = ");
    for (size_t i = 0; i < MODULE_COUNT; i++)
        printf("%lld,", cache_module_sum[i]);
    putchar('\n');
}

void output_blueprint(dspbptk_coder_t* coder, const module_t module[MODULE_COUNT], const module_enum_t* chromosome, size_t length, size_t max_x_span, double score, size_t score2) {
    // 声明变量，申请内存，或者一些统计数据
    blueprint_t blueprint;
    dspbptk_blueprint_init(&blueprint);

    double _row_max_y[CHROMOSOME_LENGTH + 1] = {0.0};
    double* row_max_y = _row_max_y + 1;
    size_t _row_module_num[CHROMOSOME_LENGTH + 1] = {0};
    size_t* row_module_num = _row_module_num + 1;
    size_t module_sum[MODULE_COUNT] = {0};

    // 计算每一行有多少模块
    for (int i = 0; i < length; i++) {
        // 计算当前行的y最大处的值
        row_max_y[i] = row_max_y[i - 1] + row_height_direct(module[chromosome[i]].pow2_half_dx, module[chromosome[i]].dy, row_max_y[i - 1]);
        // 计算当前行模块数量
        row_module_num[i] = compute_row_module_count(row_max_y[i], module[chromosome[i]].dx, max_x_span);
        module_sum[chromosome[i]] += row_module_num[i];
    }

    // 计算这些模块等价多少建筑
    size_t building_count = 0;
    for (module_enum_t module_enum = 0; module_enum < MODULE_COUNT; module_enum++) {
        building_count += module[module_enum].blueprint.numBuildings * module_sum[module_enum];
    }

    // 生成蓝图
    dspbptk_resize(&blueprint, building_count);

    size_t index = 0;
    for (int i = 0; i < length; i++) {  // 每一行
        double module_pos_y = row_max_y[i] - 0.5 * module[chromosome[i]].dy;
        double x_spacing = max_x_span / row_module_num[i];
        for (int j = 0; j < row_module_num[i]; j++) {  // 每个模块
            dspbptk_building_copy(&blueprint.buildings[index], module[chromosome[i]].blueprint.buildings, module[chromosome[i]].blueprint.numBuildings, index);
            double module_pos_x = (j + 0.5) * x_spacing;
            vec4 sph = {module_pos_x, module_pos_y};
            vec4 rct;
            sph_to_rct(sph, rct);
            mat4x4 rot;
            set_rot_mat(rct, rot);
            for (int k = index; k < index + module[chromosome[i]].blueprint.numBuildings; k++) {  // 每个建筑
                dspbptk_building_localOffset_rotation(&blueprint.buildings[k], rot);
            }
            index += module[chromosome[i]].blueprint.numBuildings;
        }
    }

    // 自动生成文件名
    char file_name[256] = {0};
    char* ptr_file_name = file_name;
    ptr_file_name += sprintf(ptr_file_name, "{%lf,%lld}", score, score2);
    for (size_t i = 0; i < length; i++)
        ptr_file_name += sprintf(ptr_file_name, "%d", chromosome[i]);
    sprintf(ptr_file_name, ".txt");

    // 输出蓝图
    printf("Output blueprint to \"%s\"\n", file_name);
    FILE* fp = fopen(file_name, "w");
    blueprint_encode_file(coder, &blueprint, fp);
    fclose(fp);

    // 释放内存
    dspbptk_free_blueprint(&blueprint);
}

typedef struct {
    double objective_score;
    size_t subjective_score;
    size_t length;
    module_enum_t chromosome[CHROMOSOME_LENGTH];
} best_chromosome_t;

/**
 * @brief 穷举所有可能的排列，然后计算当前排列的理论最大产能。注意这是一个递归的函数
 *
 * @param coder 用于输出计算结果的蓝图编码器
 * @param need 对不同模块的需求比例，由用户输入
 * @param max_x_span 允许的赤道处最大x跨度，由用户输入
 * @param max_y_span 允许的最大y跨度，由用户输入
 * @param module 模块的数据，程序启动时自动读取生成
 * @param chromosome 当前排列的编码(染色体)
 * @param max_module_count 当前输入参数的约束下，每种模块的数量上界，搜索环境初始化时自动计算
 * @param cache_row_y_max 缓存某一行的y最大值，在此函数中被自动维护
 * @param cache_module_sum 缓存当前排列的模块总数，在此函数中被自动维护
 * @param index 搜索深度，也就是当前回溯到了哪一行
 */
void search(dspbptk_coder_t* coder, const double need[MODULE_COUNT], double max_x_span, double max_y_span, const module_t module[MODULE_COUNT], const size_t max_module_count[MODULE_COUNT], module_enum_t chromosome[CHROMOSOME_LENGTH], double cache_row_y_max[CHROMOSOME_LENGTH], size_t cache_module_sum[CHROMOSOME_LENGTH][MODULE_COUNT], int64_t index, size_t* best_chromosome_count, best_chromosome_t best_chromosome[SIZE_CACHE_CHROMOSOME]) {  // 2024/01/30 喜欢一行300+字符吗，我故意的 :)
    for (module_enum_t module_enum = 0; module_enum < MODULE_COUNT; module_enum++) {
        // 剪枝1：检查某种建筑是否过多
        if (cache_module_sum[index - 1][module_enum] >= max_module_count[module_enum])
            continue;

        chromosome[index] = module_enum;
        // 缓存当前行的y坐标，避免继续向高纬度拓展建筑时重复计算坐标，并且可以剪枝
        cache_row_y_max[index] = cache_row_y_max[index - 1] + row_height_direct(module[module_enum].pow2_half_dx, module[module_enum].dy, cache_row_y_max[index - 1]);
        // 缓存模块总数统计，避免在评价函数中反复计算同一行的建筑数量，并且可以剪枝
        memcpy(cache_module_sum[index], cache_module_sum[index - 1], sizeof(size_t) * MODULE_COUNT);
        cache_module_sum[index][module_enum] += compute_row_module_count(cache_row_y_max[index - 1], module[module_enum].dx, max_x_span);

        // 评分并记录
        static double max_objective_score = 0.0;
        double tmp_objective_score = objective_score(cache_module_sum[index], need, cache_row_y_max[index], max_y_span);
        if (tmp_objective_score >= max_objective_score) {
            size_t tmp_subjective_score = subjective_score(chromosome, index + 1);
            print_detail(tmp_objective_score, tmp_subjective_score, chromosome, index + 1, cache_module_sum[index]);
            if (tmp_objective_score > max_objective_score) {
                max_objective_score = tmp_objective_score;
                (*best_chromosome_count) = 0;
            }
            if (*best_chromosome_count < SIZE_CACHE_CHROMOSOME) {
                best_chromosome[*best_chromosome_count].objective_score = tmp_objective_score;
                best_chromosome[*best_chromosome_count].subjective_score = tmp_subjective_score;
                best_chromosome[*best_chromosome_count].length = index;
                memcpy(best_chromosome[*best_chromosome_count].chromosome, chromosome, CHROMOSOME_LENGTH * sizeof(module_enum_t));
                (*best_chromosome_count)++;
            } else {
                fprintf(stderr, "Warning: SIZE_CACHE_CHROMOSOME is no enough to record all results.\n");
            }
        }

        // 剪枝2：检查y跨度是否过大
        if (cache_row_y_max[index] < max_y_span && index + 1 < CHROMOSOME_LENGTH) {
            search(coder, need, max_x_span, max_y_span, module, max_module_count, chromosome, cache_row_y_max, cache_module_sum, index + 1, best_chromosome_count, best_chromosome);  // 尾递归，gcc -O2或更高的优化等级可以优化成循环
        }
    }
}

int main(void) {
    // TODO 输入参数处理
    double need[MODULE_COUNT] = {2.0, 1.0};
    double max_x_span = 100.0;
    double max_y_span = 200.0;

    // 初始化蓝图编码器
    dspbptk_coder_t coder;
    dspbptk_init_coder(&coder);

    // 读取模块
    module_t module[MODULE_COUNT];
    init_module(module, &coder);

    // 预计算数据
    size_t max_module_count[MODULE_COUNT];
    compute_max_module_count(max_module_count, module, need, max_x_span, max_y_span);
    printf("max_module_count:");
    for (int i = 0; i < MODULE_COUNT; i++)
        printf("%lld,", max_module_count[i]);
    putchar('\n');

    // 构造所有排列
    size_t best_chromosome_count = 0;
    best_chromosome_t best_chromosome[SIZE_CACHE_CHROMOSOME];

    module_enum_t chromosome[CHROMOSOME_LENGTH] = {0};
    // 注意这里的length+1是了为了读-1的下标，不能去掉
    double cache_row_y_max[CHROMOSOME_LENGTH + 1] = {0.0};
    size_t cache_module_sum[CHROMOSOME_LENGTH + 1][MODULE_COUNT] = {0};
    search(&coder, need, max_x_span, max_y_span, module, max_module_count, chromosome, cache_row_y_max + 1, cache_module_sum + 1, 0, &best_chromosome_count, best_chromosome);

    // 输出蓝图
    for (int i = 0; i < best_chromosome_count; i++) {
        output_blueprint(&coder, module, best_chromosome[i].chromosome, best_chromosome[i].length, max_x_span, best_chromosome[i].objective_score, best_chromosome[i].subjective_score);
    }

    // 销毁蓝图编码器
    dspbptk_free_coder(&coder);

    return 0;
}