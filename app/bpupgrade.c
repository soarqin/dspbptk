#include <stdio.h>
#include <stdlib.h>

#include "libdspbptk.h"

int main(int argc, char* argv[]) {
    // dspbptk的错误值
    dspbptk_error_t errorlevel;

    // 检查用户是否输入了文件名
    if(argc <= 1) {
        fprintf(stderr, "Usage: bpupgrade <filename>\n");
        errorlevel = -1;
        goto error;
    }

    // 打开蓝图文件
    FILE* fpi = fopen(argv[1], "rb");
    if(fpi == NULL) {
        fprintf(stderr, "Error: Cannot read file:\"%s\".\n", argv[1]);
        errorlevel = -1;
        goto error;
    }

    // 分配字符串内存空间
    char* str_i = (char*)calloc(BLUEPRINT_MAX_LENGTH, sizeof(char));

    // 读取字符串
    fscanf(fpi, "%s", str_i);
    fclose(fpi);

    // 分配并初始化蓝图数据和蓝图编解码器
    blueprint_t bp;
    dspbptk_coder_t coder;
    dspbptk_init_coder(&coder);

    // 蓝图解码
    errorlevel = blueprint_decode(&coder, &bp, str_i);
    if(errorlevel) {
        goto error;
    }

    for (size_t i = 0; i < bp.numBuildings; i++) {
        building_t *building = &bp.buildings[i];
        switch (building->itemId) {
        case 2104: {
            fprintf(stdout, "Checking index %lld [itemId: %lld]:\n", building->index, building->itemId);
            int j;
            int used_slots[5] = {0, 0, 0, 0, 0};
            int has_warper = 0;
            for (j = 0; j < 12; j++) {
                if (building->parameters[192 + j * 4] == 0) continue;
                i64_t slot = building->parameters[192 + j * 4 + 1];
                if (slot > 0) {
                    if (slot < 6) {
                        used_slots[slot - 1] = 1;
                    } else {
                        has_warper = 1;
                    }
                }
            }
            if (has_warper) {
                fprintf(stdout, "  Has warper output, skip!\n");
                break;
            }
            for (j = 0; j < 5; j++) {
                if (building->parameters[j * 6] > 0 && (building->parameters[j * 6 + 1] ^ building->parameters[j * 6 + 2]) == 3)
                    used_slots[j] = 1;
            }
            int mapping[5] = {0, 1, 2, 3, 4};
            int rev_mapping[5] = {-1, -1, -1, -1, -1};
            for (j = 0; j < 5;) {
                if (!used_slots[mapping[j]]) {
                    int k;
                    for (k = j; k < 5; k++) {
                        mapping[k]++;
                    }
                } else {
                    j++;
                }
            }
            for (j = 0; j < 5; j++) {
                if (mapping[j] >= 5) break;
                rev_mapping[mapping[j]] = j;
                if (mapping[j] != j) {
                    fprintf(stdout, "  Slot %d->%d\n", mapping[j], j);
                    memcpy(&building->parameters[6 * j], &building->parameters[6 * mapping[j]], 6 * sizeof(i64_t));
                }
            }
            if (j >= 5) {
                fprintf(stdout, "  No free slots, skip!\n");
                break;
            }
            memset(&building->parameters[6 * j], 0, (5 - j) * 6 * sizeof(i64_t));
            for (j = 0; j < 12; j++) {
                i64_t slot = building->parameters[192 + j * 4 + 1];
                if (slot > 0 && slot < 6) {
                    i64_t new_slot = rev_mapping[slot - 1] + 1;
                    if (new_slot == slot) continue;
                    building->parameters[192 + j * 4 + 1] = new_slot;
                    fprintf(stdout, "  Connect index [%d]: %lld->%lld\n", j, slot, new_slot);
                }
            }
            building->itemId--;
            building->modelIndex--;
            break;
        }
        }
    }

    // 蓝图编码
    char* str_o = (char*)calloc(BLUEPRINT_MAX_LENGTH, sizeof(char));
    errorlevel = blueprint_encode(&coder, &bp, str_o);
    if(errorlevel) {
        goto error;
    }

    FILE *fpo = fopen(argv[2], "wb");
    fprintf(fpo, "%s", str_o);
    fclose(fpo);

    // 释放蓝图数据和蓝图编解码器
    dspbptk_free_blueprint(&bp);
    dspbptk_free_coder(&coder);
    // 释放字符串内存空间
    free(str_o);
    free(str_i);

    // 退出程序
    printf("Finish.\n");
    return 0;

    error:
    fprintf(stderr, "errorlevel = %d\n", errorlevel);
    return errorlevel;
}
