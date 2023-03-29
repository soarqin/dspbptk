#include "libdspbptk.h"

i8_t read_i8(void* bin) {
    return *((i8_t*)bin);
}

i16_t read_i16(void* bin) {
    return *((i16_t*)bin);
}

i32_t read_i32(void* bin) {
    return *((i32_t*)bin);
}

i64_t read_i64(void* bin) {
    return *((i64_t*)bin);
}

f32_t read_f32(void* bin) {
    return *((f32_t*)bin);
}

f64_t read_f64(void* bin) {
    return *((f64_t*)bin);
}

////////////////////////////////////////////////////////////////////////////////
// 这些函数用于解耦dspbptk与底层库依赖，如果需要更换底层库时只要换掉这几个函数里就行
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 通用的base64编码。用于解耦dspbptk与更底层的base64库。假定out有足够的空间。
 *
 * @param in 编码前的二进制流
 * @param inlen 编码前的二进制流长度
 * @param out 编码后的二进制流
 * @return size_t 编码后的二进制流长度
 */
size_t base64_enc(const void* in, size_t inlen, char* out) {
    return tb64v256enc((unsigned char*)in, inlen, (unsigned char*)out);
}

/**
 * @brief 通用的base64解码。用于解耦dspbptk与更底层的base64库。假定out有足够的空间。
 *
 * @param in 解码前的二进制流
 * @param inlen 解码前的二进制流长度
 * @param out 解码后的二进制流
 * @return size_t 当返回值<=0时表示解码错误；当返回值>=1时，表示成功并返回解码后的二进制流长度
 */
size_t base64_dec(const char* in, size_t inlen, void* out) {
    return tb64v256dec((unsigned char*)in, inlen, (unsigned char*)out);
}

/**
 * @brief 返回base64解码后的准确长度。用于解耦dspbptk与更底层的base64库
 */
size_t base64_declen(const char* base64, size_t base64_length) {
    return tb64declen((unsigned char*)base64, base64_length);
}

/**
 * @brief 通用的gzip压缩。用于解耦dspbptk与更底层的gzip库
 *
 * @param in 压缩前的二进制流
 * @param in_nbytes 压缩前的二进制流长度
 * @param out 压缩后的二进制流
 * @return size_t 压缩后的二进制流长度
 */
size_t gzip_enc(const unsigned char* in, size_t in_nbytes, unsigned char* out) {
    struct libdeflate_compressor* p_compressor = libdeflate_alloc_compressor(12);
    size_t gzip_length = libdeflate_gzip_compress(
        p_compressor, in, in_nbytes, out, BLUEPRINT_MAX_LENGTH);
    libdeflate_free_compressor(p_compressor);
    return gzip_length;
}

/**
 * @brief 通用的gzip解压。用于解耦dspbptk与更底层的gzip库。假定out有足够的空间。
 *
 * @param in 解压前的二进制流
 * @param in_nbytes 解压前的二进制流的长度
 * @param out 解压后的二进制流
 * @return size_t 当返回值<=3时，表示解压错误；当返回值>=4时，表示解压成功并返回解压后的二进制流长度
 */
size_t gzip_dec(const unsigned char* in, size_t in_nbytes, unsigned char* out) {
    size_t actual_out_nbytes_ret;
    struct libdeflate_decompressor* p_decompressor = libdeflate_alloc_decompressor();
    enum libdeflate_result result = libdeflate_gzip_decompress(
        p_decompressor, in, in_nbytes, out, BLUEPRINT_MAX_LENGTH, &actual_out_nbytes_ret);
    libdeflate_free_decompressor(p_decompressor);
    if(result != LIBDEFLATE_SUCCESS)
        return (size_t)result;
    else
        return actual_out_nbytes_ret;
}

/**
 * @brief 返回gzip解压后的准确长度。
 *
 * @param in 解压前的二进制流
 * @param in_nbytes 解压前的二进制流的长度
 * @return size_t 解压后的二进制流长度
 */
size_t gzip_declen(const unsigned char* in, size_t in_nbytes) {
    return (size_t) * ((uint32_t*)(in + in_nbytes - 4));
}



////////////////////////////////////////////////////////////////////////////////
// dspbptk decode
////////////////////////////////////////////////////////////////////////////////

dspbptk_error_t blueprint_decode(blueprint_t* blueprint, const char* string) {
    const size_t string_length = strlen(string);

    // 不是蓝图就可以直接返回了
#ifndef DSPBPTK_NO_ERROR
    if(string_length < 10)
        return not_blueprint;
    if(memcmp(string, "BLUEPRINT:", 10) != 0)
        return not_blueprint;
#endif
    MSG("is blueprint");

    // 分割字符串
    const char* head = string;
    const char* base64 = strchr(string, (int)'\"') + 1;
    const char* md5f = string + string_length - MD5F_LENGTH;

#ifndef DSPBPTK_NO_ERROR
    if(*(md5f - 1) != '\"')
        return blueprint_md5f_broken;
#endif

    const size_t head_length = (size_t)(base64 - head - 1);
    const size_t base64_length = (size_t)(md5f - base64 - 1);
    MSG("split string");
    DBG(base64_length);

    // 解析md5f(并校验)
#ifndef DSPBPTK_NO_WARNING
    char md5f_check[MD5F_LENGTH + 1] = "\0";
    md5f_str(md5f_check, string, head_length + 1 + base64_length);
    if(memcmp(md5f, md5f_check, MD5F_LENGTH) != 0)
        fprintf(stderr, "Warning: MD5 abnormal.\n");
#endif
    blueprint->md5f = (char*)calloc(32 + 1, sizeof(char));
    memcpy(blueprint->md5f, md5f, 32);
    MSG("md5f check");

    // 解析head
    blueprint->shortDesc = (char*)calloc(SHORTDESC_MAX_LENGTH + 1, sizeof(char));
    int argument_count = sscanf(string, "BLUEPRINT:0,%"PRId64",%"PRId64",%"PRId64",%"PRId64",%"PRId64",%"PRId64",0,%"PRId64",%"PRId64".%"PRId64".%"PRId64".%"PRId64",%[^\"]",
        &blueprint->layout,
        &blueprint->icons[0],
        &blueprint->icons[1],
        &blueprint->icons[2],
        &blueprint->icons[3],
        &blueprint->icons[4],
        &blueprint->time,
        &blueprint->gameVersion[0],
        &blueprint->gameVersion[1],
        &blueprint->gameVersion[2],
        &blueprint->gameVersion[3],
        blueprint->shortDesc
    );
#ifndef DSPBPTK_NO_ERROR
    if(argument_count != 12)
        return blueprint_head_broken;
#endif
    MSG("head parsing");

    // 解析base64
    {
        size_t gzip_length = base64_declen(base64, base64_length);
        DBG(gzip_length);
        void* gzip = calloc(gzip_length, 1);
    #ifndef DSPBPTK_NO_ERROR
        if(gzip == NULL)
            return out_of_memory;
    #endif
        gzip_length = base64_dec(base64, base64_length, gzip);
        DBG(gzip_length);
    #ifndef DSPBPTK_NO_ERROR
        if(gzip_length <= 0)
            return blueprint_base64_broken;
    #endif
        MSG("base64 dec");

        size_t bin_length = gzip_declen(gzip, gzip_length);
        DBG(bin_length);
        void* bin = calloc(bin_length, 1);
    #ifndef DSPBPTK_NO_ERROR
        if(bin == NULL)
            return out_of_memory;
    #endif
        bin_length = gzip_dec(gzip, gzip_length, bin);
        DBG(bin_length);
    #ifndef DSPBP_NO_CHECK
        if(bin_length <= 3)
            return blueprint_gzip_broken;
    #endif
        MSG("gzip dec");

        // 解析二进制流
        {
            void* p = bin;

            blueprint->version = (i64_t)read_i32(p + bin_offset_version);
            blueprint->cursorOffset_x = (i64_t)read_i32(p + bin_offset_cursorOffset_x);
            blueprint->cursorOffset_y = (i64_t)read_i32(p + bin_offset_cursorOffset_y);
            blueprint->cursorTargetArea = (i64_t)read_i32(p + bin_offset_cursorTargetArea);
            blueprint->dragBoxSize_x = (i64_t)read_i32(p + bin_offset_dragBoxSize_x);
            blueprint->dragBoxSize_y = (i64_t)read_i32(p + bin_offset_dragBoxSize_y);
            blueprint->primaryAreaIdx = (i64_t)read_i32(p + bin_offset_primaryAreaIdx);

            MSG("bin parsing");

            const size_t AREA_NUM = (size_t)read_i8(p + BIN_OFFSET_AREA_NUM);
            blueprint->AREA_NUM = AREA_NUM;
            blueprint->area = (area_t*)calloc(AREA_NUM, sizeof(area_t));
        #ifndef DSPBP_NO_CHECK
            if(blueprint->area == NULL)
                return out_of_memory;
        #endif

            DBG(AREA_NUM);

            p += BIN_OFFSET_AREA_ARRAY;

            for(size_t i = 0; i < AREA_NUM; i++) {
                blueprint->area[i].index = (i64_t)read_i8(p + area_offset_index);
                blueprint->area[i].parentIndex = (i64_t)read_i8(p + area_offset_parentIndex);
                blueprint->area[i].tropicAnchor = (i64_t)read_i16(p + area_offset_tropicAnchor);
                blueprint->area[i].areaSegments = (i64_t)read_i16(p + area_offset_areaSegments);
                blueprint->area[i].anchorLocalOffsetX = (i64_t)read_i16(p + area_offset_anchorLocalOffsetX);
                blueprint->area[i].anchorLocalOffsetY = (i64_t)read_i16(p + area_offset_anchorLocalOffsetY);
                blueprint->area[i].width = (i64_t)read_i16(p + area_offset_width);
                blueprint->area[i].height = (i64_t)read_i16(p + area_offset_height);

                p += AREA_OFFSET_AREA_NEXT;
            }

            MSG("area parsing");

            const size_t BUILDING_NUM = (size_t)read_i32(p);
            blueprint->BUILDING_NUM = BUILDING_NUM;
            blueprint->building = (building_t*)calloc(BUILDING_NUM, sizeof(building_t));
        #ifndef DSPBP_NO_CHECK
            if(blueprint->building == NULL)
                return out_of_memory;
        #endif

            DBG(BUILDING_NUM);

            p += sizeof(int32_t);

            for(size_t i = 0; i < BUILDING_NUM; i++) {
                blueprint->building[i].index = (i64_t)read_i32(p + building_offset_index);
                blueprint->building[i].areaIndex = (i64_t)read_i8(p + building_offset_areaIndex);
                blueprint->building[i].localOffset.x = (f64_t)read_f32(p + building_offset_localOffset_x);
                blueprint->building[i].localOffset.y = (f64_t)read_f32(p + building_offset_localOffset_y);
                blueprint->building[i].localOffset.z = (f64_t)read_f32(p + building_offset_localOffset_z);
                blueprint->building[i].localOffset.w = (f64_t)1.0;
                blueprint->building[i].localOffset2.x = (f64_t)read_f32(p + building_offset_localOffset_x2);
                blueprint->building[i].localOffset2.y = (f64_t)read_f32(p + building_offset_localOffset_y2);
                blueprint->building[i].localOffset2.z = (f64_t)read_f32(p + building_offset_localOffset_z2);
                blueprint->building[i].localOffset2.w = (f64_t)1.0;
                blueprint->building[i].yaw = (f64_t)read_f32(p + building_offset_yaw);
                blueprint->building[i].yaw2 = (f64_t)read_f32(p + building_offset_yaw2);
                blueprint->building[i].itemId = (i64_t)read_i16(p + building_offset_itemId);
                blueprint->building[i].modelIndex = (i64_t)read_i16(p + building_offset_modelIndex);
                blueprint->building[i].tempOutputObjIdx = (i64_t)read_i32(p + building_offset_tempOutputObjIdx);
                blueprint->building[i].tempInputObjIdx = (i64_t)read_i32(p + building_offset_tempInputObjIdx);
                blueprint->building[i].outputToSlot = (i64_t)read_i8(p + building_offset_tempInputObjIdx);
                blueprint->building[i].inputFromSlot = (i64_t)read_i8(p + building_offset_inputFromSlot);
                blueprint->building[i].outputFromSlot = (i64_t)read_i8(p + building_offset_outputFromSlot);
                blueprint->building[i].inputToSlot = (i64_t)read_i8(p + building_offset_inputToSlot);
                blueprint->building[i].outputOffset = (i64_t)read_i8(p + building_offset_outputOffset);
                blueprint->building[i].inputOffset = (i64_t)read_i8(p + building_offset_inputOffset);
                blueprint->building[i].recipeId = (i64_t)read_i16(p + building_offset_recipeId);
                blueprint->building[i].filterId = (i64_t)read_i16(p + building_offset_filterId);

                // DBG(blueprint->building[i].itemId);

                const size_t PARAMETERS_NUM = (i64_t)read_i16(p + building_offset_num);
                blueprint->building[i].PARAMETERS_NUM = PARAMETERS_NUM;
                if(PARAMETERS_NUM > 0) {
                    blueprint->building[i].parameters = (i64_t*)calloc(PARAMETERS_NUM, sizeof(i64_t));
                #ifndef DSPBP_NO_CHECK
                    if(blueprint->building[i].parameters == NULL)
                        return out_of_memory;
                #endif
                }
                else {
                    blueprint->building[i].parameters = NULL;
                }

                p += building_offset_parameters;

                for(size_t j = 0; j < PARAMETERS_NUM; j++) {
                    blueprint->building[i].parameters[j] = (i64_t)read_i32(p + j * sizeof(i32_t));
                }

                p += PARAMETERS_NUM * sizeof(i32_t);
            }
            MSG("building parsing");

        }

        free(gzip);
        free(bin);
    }

    return no_error;
}



////////////////////////////////////////////////////////////////////////////////
// dspbptk encode
////////////////////////////////////////////////////////////////////////////////

typedef struct {
    int32_t id;
    int32_t index;
}id_t;

// int cmp_building(const void* p_a, const void* p_b) {
//     void* a = *((void**)p_a);
//     void* b = *((void**)p_b);
//     return get_building_itemID(a) - get_building_itemID(b);
// }

int cmp_id(const void* p_a, const void* p_b) {
    id_t* a = ((id_t*)p_a);
    id_t* b = ((id_t*)p_b);
    return a->id - b->id;
}

int cmp_index(const void* p_a, const void* p_b) {
    id_t* a = ((id_t*)p_a);
    id_t* b = ((id_t*)p_b);
    return a->index - b->index;
}

void re_index(int32_t* ObjIdx, id_t* id_lut, size_t building_num) {
    if(*ObjIdx == OBJ_NULL)
        return;
    id_t* p_id = bsearch(ObjIdx, id_lut, building_num, sizeof(id_t), cmp_id);
    if(p_id == NULL) {
    #ifndef DSPBPTK_NO_WARNING
        fprintf(stderr, "Warning: index %d no found! Reindex index to OBJ_NULL(-1).\n", *ObjIdx);
    #endif
        * ObjIdx = OBJ_NULL;
    }
    else {
        *ObjIdx = p_id->index;
    }
}

dspbptk_error_t blueprint_encode(const blueprint_t* blueprint, char* string) {
    MSG("////////////////////////////////////////////////////////////////////");
    void* bin = calloc(BLUEPRINT_MAX_LENGTH, 1);
    void* gzip = calloc(BLUEPRINT_MAX_LENGTH, 1);

    char* ptr_str = string;
    void* ptr_bin = bin;

    // 输出head
    sprintf(string, "BLUEPRINT:0,%"PRId64",%"PRId64",%"PRId64",%"PRId64",%"PRId64",%"PRId64",0,%"PRId64",%"PRId64".%"PRId64".%"PRId64".%"PRId64",%s\"",
        blueprint->layout,
        blueprint->icons[0],
        blueprint->icons[1],
        blueprint->icons[2],
        blueprint->icons[3],
        blueprint->icons[4],
        blueprint->time,
        blueprint->gameVersion[0],
        blueprint->gameVersion[1],
        blueprint->gameVersion[2],
        blueprint->gameVersion[3],
        blueprint->shortDesc
    );

    size_t head_length = strlen(string); // 这个长度已经带了前一个引号
    ptr_str += head_length;

    MSG("print head");
    DBG(head_length);

    // 编码bin head
    *((i32_t*)(ptr_bin + bin_offset_version)) = (i32_t)blueprint->version;
    *((i32_t*)(ptr_bin + bin_offset_cursorOffset_x)) = (i32_t)blueprint->cursorOffset_x;
    *((i32_t*)(ptr_bin + bin_offset_cursorOffset_y)) = (i32_t)blueprint->cursorOffset_y;
    *((i32_t*)(ptr_bin + bin_offset_cursorTargetArea)) = (i32_t)blueprint->cursorTargetArea;
    *((i32_t*)(ptr_bin + bin_offset_dragBoxSize_x)) = (i32_t)blueprint->dragBoxSize_x;
    *((i32_t*)(ptr_bin + bin_offset_dragBoxSize_y)) = (i32_t)blueprint->dragBoxSize_y;
    *((i32_t*)(ptr_bin + bin_offset_primaryAreaIdx)) = (i32_t)blueprint->primaryAreaIdx;
    MSG("cpy bin head");

    // 编码区域总数
    *((i8_t*)(ptr_bin + BIN_OFFSET_AREA_NUM)) = (i8_t)blueprint->AREA_NUM;
    DBG(*((i8_t*)(ptr_bin + BIN_OFFSET_AREA_NUM)));

    // 编码区域数组
    ptr_bin += BIN_OFFSET_AREA_ARRAY;
    for(size_t i = 0; i < blueprint->AREA_NUM; i++) {
        *((i8_t*)(ptr_bin + area_offset_index)) = (i8_t)blueprint->area[i].index;
        *((i8_t*)(ptr_bin + area_offset_parentIndex)) = (i8_t)blueprint->area[i].parentIndex;
        *((i16_t*)(ptr_bin + area_offset_tropicAnchor)) = (i16_t)blueprint->area[i].tropicAnchor;
        *((i16_t*)(ptr_bin + area_offset_areaSegments)) = (i16_t)blueprint->area[i].areaSegments;
        *((i16_t*)(ptr_bin + area_offset_anchorLocalOffsetX)) = (i16_t)blueprint->area[i].anchorLocalOffsetX;
        *((i16_t*)(ptr_bin + area_offset_anchorLocalOffsetY)) = (i16_t)blueprint->area[i].anchorLocalOffsetY;
        *((i16_t*)(ptr_bin + area_offset_width)) = (i16_t)blueprint->area[i].width;
        *((i16_t*)(ptr_bin + area_offset_height)) = (i16_t)blueprint->area[i].height;

        ptr_bin += AREA_OFFSET_AREA_NEXT;
    }

    // 编码建筑总数
    *((i32_t*)(ptr_bin)) = (i32_t)blueprint->BUILDING_NUM;
    DBG(*((i32_t*)(ptr_bin)));

    // 编码建筑数组
    ptr_bin += sizeof(i32_t);
    for(size_t i = 0; i < blueprint->BUILDING_NUM; i++) {

    #define MACRO_ENCODE(name, type)\
    {*((type*)(ptr_bin + building_offset_##name)) = (type)blueprint->building[i].name;}

        MACRO_ENCODE(index, i32_t);
        MACRO_ENCODE(areaIndex, i8_t);

        // TODO 处理齐次坐标
        *((f32_t*)(ptr_bin + building_offset_localOffset_x)) = (f32_t)(blueprint->building[i].localOffset.x);
        *((f32_t*)(ptr_bin + building_offset_localOffset_y)) = (f32_t)(blueprint->building[i].localOffset.y);
        *((f32_t*)(ptr_bin + building_offset_localOffset_z)) = (f32_t)(blueprint->building[i].localOffset.z);

        *((f32_t*)(ptr_bin + building_offset_localOffset_x2)) = (f32_t)(blueprint->building[i].localOffset2.x);
        *((f32_t*)(ptr_bin + building_offset_localOffset_y2)) = (f32_t)(blueprint->building[i].localOffset2.y);
        *((f32_t*)(ptr_bin + building_offset_localOffset_z2)) = (f32_t)(blueprint->building[i].localOffset2.z);

        *((f32_t*)(ptr_bin + building_offset_yaw)) = (f32_t)(blueprint->building[i].yaw);
        *((f32_t*)(ptr_bin + building_offset_yaw2)) = (f32_t)(blueprint->building[i].yaw2);

        MACRO_ENCODE(itemId, i16_t);
        MACRO_ENCODE(modelIndex, i16_t);
        MACRO_ENCODE(tempOutputObjIdx, i32_t);
        MACRO_ENCODE(tempInputObjIdx, i32_t);
        MACRO_ENCODE(outputToSlot, i8_t);
        MACRO_ENCODE(inputFromSlot, i8_t);
        MACRO_ENCODE(outputFromSlot, i8_t);
        MACRO_ENCODE(inputToSlot, i8_t);
        MACRO_ENCODE(outputOffset, i8_t);
        MACRO_ENCODE(inputOffset, i8_t);
        MACRO_ENCODE(recipeId, i16_t);
        MACRO_ENCODE(filterId, i16_t);

        // TODO 统一命名
        *((i32_t*)(ptr_bin + building_offset_num)) = (i32_t)blueprint->building[i].PARAMETERS_NUM;

        ptr_bin += building_offset_parameters;
        for(size_t j = 0; j < blueprint->building[i].PARAMETERS_NUM; j++) {
            *((i32_t*)(ptr_bin + sizeof(i32_t) * j)) = (i32_t)blueprint->building[i].parameters[j];
        }
        ptr_bin += sizeof(i32_t) * blueprint->building[i].PARAMETERS_NUM;
    }

    // 计算二进制流长度
    size_t bin_length = (size_t)(ptr_bin - bin);

    size_t gzip_length = gzip_enc(bin, bin_length, gzip);

    size_t base64_length = base64_enc(gzip, gzip_length, ptr_str);


    // 计算md5f
    char md5f_hex[MD5F_LENGTH + 1] = "\0";
    md5f_str(md5f_hex, string, head_length + base64_length);

    ptr_str += base64_length;
    sprintf(ptr_str, "\"%s", md5f_hex);

    free(bin);
    free(gzip);
    return no_error;
}

void free_blueprint(blueprint_t* blueprint) {
    free(blueprint->shortDesc);
    free(blueprint->md5f);
    free(blueprint->area);
    for(size_t i = 0; i < blueprint->BUILDING_NUM; i++) {
        if(blueprint->building[i].PARAMETERS_NUM > 0)
            free(blueprint->building[i].parameters);
    }
    free(blueprint->building);
}