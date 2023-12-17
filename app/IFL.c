#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include "libdspbptk.h"

uint64_t get_timestamp(void) {
    struct timespec t;
    clock_gettime(0, &t);
    return (uint64_t)t.tv_sec * 1000000000 + (uint64_t)t.tv_nsec;
}

double d_t(uint64_t t1, uint64_t t0) {
    return (double)(t1 - t0) / 1000000.0;
}

void xyz_to_xy(f64x4_t* xyz, f64x4_t* xy) {
    xy->y = asin(xyz->z) * (250.0 / M_PI_2);

    xy->x = acos(xyz->y / sqrt(1.0 - xyz->z * xyz->z)) * (250.0 / M_PI_2);
    if(xyz->x < 0.0)
        xy->x = -xy->x;
}

void print_help_doc() {
    printf(
        "Usage:   IFL options...\n"
        "Example: IFL -i unit.txt -o final.txt -l list.txt -N 130\n"
        "\n"
        "Options:\n"
        "h             Show this help doc.\n"
        "i<file>       Input file(blueprint).\n"
        "l<file>       Coordinate List File.\n"
        "N[num]        Num of Coordinate.\n"
        "o<file>       Output file(blueprint).\n"
        "\n"
    );
}

int main(int argc, char* argv[]) {
    // dspbptk的错误值
    dspbptk_error_t errorlevel;

    if(argc <= 1) {
        errorlevel = -1;
        goto error;
    }

    // 启动参数
    i64_t num_list = 0;
    char* bp_path_i = NULL;
    char* bp_path_o = NULL;
    char* list_path = NULL;

    // 处理启动参数
    const char* options = "hi:l:N:o:";

    char arg;

    while((arg = getopt(argc, argv, options)) != -1) {
        switch(arg) {
        case 'h': {
            errorlevel = 0;
            goto error;
        }

        case 'i': {
            bp_path_i = optarg;
            break;
        }

        case 'l': {
            list_path = optarg;
            break;
        }

        case 'N': {
            sscanf(optarg, "%lld", &num_list);
            if(num_list < 1) {
                fprintf(stderr, "非法参数N=%lld", num_list);
                errorlevel = -1;
                goto error;
            }
            break;
        }
        case 'o': {
            bp_path_o = optarg;
            break;
        }
        }
    }

    // 读取坐标列表
    FILE* fpl = fopen(list_path, "r");
    if(fpl == NULL) {
        fprintf(stderr, "Error: Cannot read file:\"%s\".\n", list_path);
        errorlevel = -1;
        goto error;
    }
    f64x4_t* pos_list = calloc(num_list, sizeof(f64x4_t));
    for(i64_t i = 0; i < num_list; i++) {
        fscanf(fpl, "%lf", &pos_list[i].x);
        fscanf(fpl, "%lf", &pos_list[i].y);
        fscanf(fpl, "%lf", &pos_list[i].z);
    }
    fclose(fpl);

    // 打开蓝图文件
    FILE* fpi = fopen(bp_path_i, "r");
    if(fpi == NULL) {
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
    if(errorlevel) {
        goto error;
    }

    uint64_t t_edt_0 = get_timestamp();

    // 检查是不是单建筑蓝图，如果是把坐标归零
    if(bp.numBuildings == 1) {
        bp.buildings[0].localOffset.x = 0.0;
        bp.buildings[0].localOffset.y = 0.0;
        bp.buildings[0].localOffset.z = 0.0;
        bp.buildings[0].localOffset2.x = 0.0;
        bp.buildings[0].localOffset2.y = 0.0;
        bp.buildings[0].localOffset2.z = 0.0;
        fprintf(stderr, "检测到单建筑蓝图，坐标已归正至原点\n");
    }

    // 调整蓝图大小
    i64_t old_numBuildings = bp.numBuildings;
    bp.numBuildings *= num_list;
    bp.buildings = realloc(bp.buildings, sizeof(building_t) * bp.numBuildings);
    DBG(old_numBuildings);
    DBG(bp.numBuildings);

    // 蓝图处理
    // TODO 兼容拓展标准的蓝图，这个过程可能需要重新编号
    // TODO 检查double free
    for(i64_t i = 1; i < num_list; i++) {
        i64_t index_base = i * old_numBuildings;

    // 复制蓝图
        memcpy(&bp.buildings[index_base], &bp.buildings[0], sizeof(building_t) * old_numBuildings);
    }

    for(i64_t i = 0; i < num_list; i++) {
        f64x4_t xy;
        xyz_to_xy(&pos_list[i], &xy);
        DBG(pos_list[i].x);
        DBG(pos_list[i].y);
        DBG(pos_list[i].z);
        DBG(xy.x);
        DBG(xy.y);

        i64_t index_base = i * old_numBuildings;

        for(int j = 0; j < old_numBuildings; j++) {
            bp.buildings[j].index += index_base;
            if(bp.buildings[j].tempOutputObjIdx != OBJ_NULL)
                bp.buildings[j].tempOutputObjIdx += index_base;
            if(bp.buildings[j].tempInputObjIdx != OBJ_NULL)
                bp.buildings[j].tempInputObjIdx += index_base;

            bp.buildings[index_base + j].localOffset.x += xy.x;
            bp.buildings[index_base + j].localOffset.y += xy.y;

            i64_t* old_parameters = bp.buildings[j].parameters;
            bp.buildings[j].parameters = calloc(bp.buildings[j].numParameters, sizeof(i64_t));
            memcpy(bp.buildings[j].parameters, old_parameters, bp.buildings[j].numParameters * sizeof(i64_t));
        }
    }

    uint64_t t_edt_1 = get_timestamp();
    fprintf(stderr, "edt time = %.3lf ms\n", d_t(t_edt_1, t_edt_0));

    // 蓝图编码
    uint64_t t_enc_0 = get_timestamp();
    errorlevel = blueprint_encode(&coder, &bp, str_o);
    uint64_t t_enc_1 = get_timestamp();
    fprintf(stderr, "enc time = %.3lf ms\n", d_t(t_enc_1, t_enc_0));
    if(errorlevel) {
        goto error;
    }

    // 比较处理前后的蓝图变化
    size_t strlen_i = strlen(str_i);
    size_t strlen_o = strlen(str_o);
    fprintf(stderr, "strlen_i = %zu\nstrlen_o = %zu (%.3lf%%)\n",
        strlen_i, strlen_o, ((double)strlen_o / (double)strlen_i - 1.0) * 100.0);
    FILE* fpo = fopen(bp_path_o, "w");
    if(fpo == NULL) {
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
    // 释放坐标列表
    free(pos_list);

    // 退出程序
    fprintf(stderr, "Finish.\n");
    return 0;

error:
    print_help_doc();
    fprintf(stderr, "errorlevel = %d\n", errorlevel);
    return errorlevel;
}
