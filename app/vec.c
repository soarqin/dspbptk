#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "libdspbptk.h"

uint64_t get_timestamp(void) {
    struct timespec t;
    clock_gettime(0, &t);
    return (uint64_t)t.tv_sec * 1000000000 + (uint64_t)t.tv_nsec;
}

double d_t(uint64_t t1, uint64_t t0) {
    return (double)(t1 - t0) / 1000000.0;
}

void print_help_doc() {
    printf(
        "Usage:   vec options...\n"
        "Example: vec -i unit.txt -o final.txt -v 125,0,0,8\n"
        "\n"
        "Options:\n"
        "h             Show this help doc.\n"
        "i<file>       Input file(blueprint).\n"
        "v[x,y,z,N]    Array blueprint N times with vector [x,y,z].\n"
        "o<file>       Output file(blueprint).\n"
        "\n");
}

int main(int argc, char* argv[]) {
    // dspbptk的错误值
    dspbptk_error_t errorlevel;

    // 声明启动参数
    char* bp_path_i = NULL;
    char* bp_path_o = NULL;

    i64_t aN = 1;
    f64_t ax = 0.0;
    f64_t ay = 0.0;
    f64_t az = 0.0;

    // 处理启动参数
    const char* options = "i:o:v:";

    char arg;

    while ((arg = getopt(argc, argv, options)) != -1) {
        switch (arg) {
            case 'i': {
                bp_path_i = optarg;
                break;
            }

            case 'o': {
                bp_path_o = optarg;
                break;
            }

            case 'v': {
                int tmp_err = sscanf(optarg, "%lf,%lf,%lf,%lld", &ax, &ay, &az, &aN);
                if (tmp_err != 4) {
                    errorlevel = -1;
                    goto error;
                } else if (aN < 1) {
                    fprintf(stderr, "非法参数N=%lld", aN);
                    errorlevel = -1;
                    goto error;
                } else {
                    break;
                }
            }
        }
    }

    // 打开蓝图文件
    FILE* fpi = fopen(bp_path_i, "r");
    if (fpi == NULL) {
        fprintf(stderr, "Error: Cannot read file:\"%s\".\n", bp_path_i);
        errorlevel = -1;
        goto error;
    }

    // 分配字符串内存空间
    char* str_i = (char*)calloc(BLUEPRINT_MAX_LENGTH, sizeof(char));
    char* str_o = (char*)calloc(BLUEPRINT_MAX_LENGTH, sizeof(char));

    // 读取字符串
    fscanf(fpi, "%s", str_i);
    fclose(fpi);

    // 分配并初始化蓝图数据和蓝图编解码器
    blueprint_t bp;
    dspbptk_coder_t coder;
    dspbptk_init_coder(&coder);

    // 蓝图解码
    uint64_t t_dec_0 = get_timestamp();
    errorlevel = blueprint_decode(&coder, &bp, str_i);
    uint64_t t_dec_1 = get_timestamp();
    fprintf(stderr, "dec time = %.3lf ms\n", d_t(t_dec_1, t_dec_0));
    if (errorlevel) {
        goto error;
    }

    uint64_t t_edt_0 = get_timestamp();

    // 调整蓝图大小
    i64_t old_numBuildings = bp.numBuildings;
    dspbptk_resize(&bp, bp.numBuildings * aN);

    // 蓝图处理
    for (i64_t i = 1; i < aN; i++) {
        i64_t index_base = i * old_numBuildings;
        dspbptk_building_copy(&bp.buildings[index_base], &bp.buildings[0], old_numBuildings, index_base);
    }

    for (i64_t i = 0; i < aN; i++) {
        i64_t index_base = i * old_numBuildings;
        f64x4_t xy = {i * ax, i * ay, i * az, 0.0};
        for (int j = index_base; j < index_base + old_numBuildings; j++) {
            dspbptk_building_localOffset_add(&bp.buildings[j], xy);
        }
    }

    uint64_t t_edt_1 = get_timestamp();
    fprintf(stderr, "edt time = %.3lf ms\n", d_t(t_edt_1, t_edt_0));

    // 蓝图编码
    uint64_t t_enc_0 = get_timestamp();
    errorlevel = blueprint_encode(&coder, &bp, str_o);
    uint64_t t_enc_1 = get_timestamp();
    fprintf(stderr, "enc time = %.3lf ms\n", d_t(t_enc_1, t_enc_0));
    if (errorlevel) {
        goto error;
    }

    // 比较处理前后的蓝图变化
    size_t strlen_i = strlen(str_i);
    size_t strlen_o = strlen(str_o);
    fprintf(stderr, "strlen_i = %zu\nstrlen_o = %zu (%.3lf%%)\n",
            strlen_i, strlen_o, ((double)strlen_o / (double)strlen_i - 1.0) * 100.0);
    FILE* fpo = fopen(bp_path_o, "w");
    if (fpo == NULL) {
        fprintf(stderr, "Error: Cannot overwrite file:\"%s\".\n", bp_path_i);
        errorlevel = -1;
        goto error;
    }
    fprintf(fpo, "%s", str_o);
    fclose(fpo);

    // 释放蓝图数据和蓝图编解码器
    dspbptk_free_blueprint(&bp);
    dspbptk_free_coder(&coder);
    // 释放字符串内存空间
    free(str_o);
    free(str_i);

    // 退出程序
    fprintf(stderr, "Finish.\n");
    return 0;

error:
    print_help_doc();
    fprintf(stderr, "errorlevel = %d\n", errorlevel);
    return errorlevel;
}
