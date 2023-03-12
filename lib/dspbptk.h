
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "libdeflate/libdeflate.h"
#include "Turbo-Base64/conf.h"
#include "Turbo-Base64/turbob64_.h"
#include "Turbo-Base64/turbob64.h"

#ifndef DSPBPTK
#define DSPBPTK

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#define debug_reset() {last_time = 0;}
#define debug(x) {\
    long long int now = get_timestamp();\
    fprintf(stderr, "DEBUG: "x ": %.6lfms\n", last_time == 0 ? 0.0 : (double)(now - last_time) / 1000000.0);\
    last_time = now;}
#else
#define debug_reset()
#define debug(x)
#endif

#define BP_LEN 268435456 // 256mb

    typedef struct {
        int64_t layout;             // layout，作用未知
        int64_t icons[5];           // 蓝图图标
        int64_t time;               // 时间戳
        int64_t game_version[4];    // 创建蓝图的游戏版本
        size_t raw_len;             // 蓝图数据的长度
        void* raw;                  // 指向蓝图数据
        char* md5;                  // 指向蓝图md5f
    } bp_data_t;

    // 内部函数

    /**
     * @brief 从文件读取蓝图，不检查蓝图正确性。会给blueprint分配内存，别忘了free(blueprint);
     *
     * @param file_name 待读取的文件名
     * @param p_blueprint 指向蓝图字符串的指针
     * @return size_t 蓝图的尺寸
     */
    size_t file_to_blueprint(const char* file_name, char** p_blueprint);

    /**
     * @brief 将蓝图写入文件，不检查蓝图正确性
     *
     * @param file_name 待写入的文件名
     * @param blueprint 蓝图字符串
     * @return int 写入是否成功
     */
    int blueprint_to_file(const char* file_name, const char* blueprint);

    /**
     * @brief 从蓝图字符串解析其中的数据到bp_data。会给bp_data分配内存，别忘了free_bp_data(&bp_data);
     *
     * @param p_bp_data 指向bp_data的指针
     * @param blueprint 蓝图字符串
     * @return int 解析是否成功
     */
    int blueprint_to_data(bp_data_t* p_bp_data, const char* blueprint);

    /**
     * @brief 释放bp_data的内存
     *
     * @param p_bp_data 指向bp_data的指针
     */
    void free_bp_data(bp_data_t* p_bp_data);

#ifdef __cplusplus
}
#endif

#endif
