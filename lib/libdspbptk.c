#include "libdspbptk.h"
#include "enum_offset.h"

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
    return chromium_base64_encode(out, in, inlen);
    // return _tb64e((unsigned char*)in, inlen, (unsigned char*)out);
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
    return chromium_base64_decode(out, in, inlen);
}

/**
 * @brief 返回base64解码后的准确长度。用于解耦dspbptk与更底层的base64库
 */
size_t base64_declen(const char* base64, size_t base64_length) {
    return chromium_base64_decode_len(base64_length);
}

/**
 * @brief 通用的gzip压缩。用于解耦dspbptk与更底层的gzip库
 *
 * @param in 压缩前的二进制流
 * @param in_nbytes 压缩前的二进制流长度
 * @param out 压缩后的二进制流
 * @return size_t 压缩后的二进制流长度
 */
size_t gzip_enc(dspbptk_coder_t* coder, const unsigned char* in, size_t in_nbytes, unsigned char* out) {
    size_t gzip_length = libdeflate_gzip_compress(
        coder->p_compressor, in, in_nbytes, out, 0xffffffff);
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
size_t gzip_dec(dspbptk_coder_t* coder, const unsigned char* in, size_t in_nbytes, unsigned char* out) {
    size_t actual_out_nbytes_ret;
    enum libdeflate_result result = libdeflate_gzip_decompress(
        coder->p_decompressor, in, in_nbytes, out, 0xffffffff, &actual_out_nbytes_ret);
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

void dspbptk_calloc_parameters(building_t* building, size_t N) {
    if(N > 0) {
        building->numParameters = N;
        building->parameters = (i64_t*)calloc(N, sizeof(i64_t));
    }
    else {
        // TODO 这里应该静默吗？
        building->numParameters = 0;
        building->parameters = NULL;
    }
}

dspbptk_error_t blueprint_decode(dspbptk_coder_t* coder, blueprint_t* blueprint, const char* string) {
    // 初始化结构体，置零
    memset(blueprint, 0, sizeof(blueprint_t));

    // 获取输入的字符串的长度
    const size_t string_length = strlen(string);

    // 检查是不是蓝图
#ifndef DSPBPTK_NO_ERROR
    if(string_length < 10)
        return not_blueprint;
    if(memcmp(string, "BLUEPRINT:", 10) != 0)
        return not_blueprint;
#endif

    // 根据双引号标记字符串
    const char* head = string;
    const char* base64 = strchr(string, (int)'\"') + 1;
    const char* md5f = string + string_length - MD5F_LENGTH;
#ifndef DSPBPTK_NO_ERROR
    if(*(md5f - 1) != '\"')
        return blueprint_md5f_broken;
#endif
    const size_t head_length = (size_t)(base64 - head - 1);
    const size_t base64_length = (size_t)(md5f - base64 - 1);

    // 解析md5f(并校验)
#ifndef DSPBPTK_NO_WARNING
    char md5f_check[MD5F_LENGTH + 1] = "\0";
    md5f_str(md5f_check, coder->buffer1, string, head_length + 1 + base64_length);
    if(memcmp(md5f, md5f_check, MD5F_LENGTH) != 0)
        fprintf(stderr, "Warning: MD5 abnormal!\nthis:\t%s\nactual:\t%s\n", md5f, md5f_check);
#endif
    blueprint->md5f = (char*)calloc(MD5F_LENGTH + 1, sizeof(char));
    memcpy(blueprint->md5f, md5f, MD5F_LENGTH);

    // 解析head
    blueprint->shortDesc = (char*)calloc(SHORTDESC_MAX_LENGTH + 1, sizeof(char));
    blueprint->desc = (char*)calloc(SHORTDESC_MAX_LENGTH + 1, sizeof(char));
    int argument_count = sscanf(string, "BLUEPRINT:0,%"PRId64",%"PRId64",%"PRId64",%"PRId64",%"PRId64",%"PRId64",0,%"PRId64",%"PRId64".%"PRId64".%"PRId64".%"PRId64",%[^,],%[^\"]",
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
        blueprint->shortDesc,
        blueprint->desc
    );
#ifndef DSPBPTK_NO_ERROR
    // FIXME 有时会报错
    if(argument_count < 12)
        return blueprint_head_broken;
#endif

    // 解析base64
    {
        // base64解码
        size_t gzip_length = base64_declen(base64, base64_length);
        void* gzip = coder->buffer0;
        gzip_length = base64_dec(base64, base64_length, gzip);
    #ifndef DSPBPTK_NO_ERROR
        if(gzip_length <= 0)
            return blueprint_base64_broken;
    #endif

        // gzip解压
        size_t bin_length = gzip_declen(gzip, gzip_length);
        void* bin = coder->buffer1;
        bin_length = gzip_dec(coder, gzip, gzip_length, bin);
    #ifndef DSPBPTK_NO_ERROR
        if(bin_length <= 3)
            return blueprint_gzip_broken;
    #endif

        // 解析二进制流
        {
            // 用于操作二进制流的指针
            void* ptr_bin = bin;

            // 解析二进制流的头
            blueprint->version = *((i32_t*)(ptr_bin + bin_offset_version));
            blueprint->cursorOffsetX = *((i32_t*)(ptr_bin + bin_offset_cursorOffsetX));
            blueprint->cursorOffsetY = *((i32_t*)(ptr_bin + bin_offset_cursorOffsetY));
            blueprint->cursorTargetArea = *((i32_t*)(ptr_bin + bin_offset_cursorTargetArea));
            blueprint->dragBoxSizeX = *((i32_t*)(ptr_bin + bin_offset_dragBoxSizeX));
            blueprint->dragBoxSizeY = *((i32_t*)(ptr_bin + bin_offset_dragBoxSizeY));
            blueprint->primaryAreaIdx = *((i32_t*)(ptr_bin + bin_offset_primaryAreaIdx));

            // 解析区域数量
            const size_t AREA_NUM = (size_t) * ((i8_t*)(ptr_bin + bin_offset_numAreas));
            blueprint->numAreas = AREA_NUM;
            blueprint->areas = (area_t*)calloc(AREA_NUM, sizeof(area_t));

            // 解析区域数组
            ptr_bin += bin_offset_areas;
            for(size_t i = 0; i < AREA_NUM; i++) {
                blueprint->areas[i].index = *((i8_t*)(ptr_bin + area_offset_index));
                blueprint->areas[i].parentIndex = *((i8_t*)(ptr_bin + area_offset_parentIndex));
                blueprint->areas[i].tropicAnchor = *((i16_t*)(ptr_bin + area_offset_tropicAnchor));
                blueprint->areas[i].areaSegments = *((i16_t*)(ptr_bin + area_offset_areaSegments));
                blueprint->areas[i].anchorLocalOffsetX = *((i16_t*)(ptr_bin + area_offset_anchorLocalOffsetX));
                blueprint->areas[i].anchorLocalOffsetY = *((i16_t*)(ptr_bin + area_offset_anchorLocalOffsetY));
                blueprint->areas[i].width = *((i16_t*)(ptr_bin + area_offset_width));
                blueprint->areas[i].height = *((i16_t*)(ptr_bin + area_offset_height));
                ptr_bin += area_offset_next;
            }

            // 解析建筑数量
            const size_t BUILDING_NUM = (size_t) * ((i32_t*)(ptr_bin));
            blueprint->numBuildings = BUILDING_NUM;
            blueprint->buildings = (building_t*)calloc(BUILDING_NUM, sizeof(building_t));

            // 解析建筑数组
            ptr_bin += sizeof(int32_t);
            for(size_t i = 0; i < BUILDING_NUM; i++) {
                blueprint->buildings[i].index = *((i32_t*)(ptr_bin + building_offset_index));
                blueprint->buildings[i].areaIndex = *((i8_t*)(ptr_bin + building_offset_areaIndex));
                blueprint->buildings[i].localOffset.x = *((f32_t*)(ptr_bin + building_offset_localOffset_x));
                blueprint->buildings[i].localOffset.y = *((f32_t*)(ptr_bin + building_offset_localOffset_y));
                blueprint->buildings[i].localOffset.z = *((f32_t*)(ptr_bin + building_offset_localOffset_z));
                blueprint->buildings[i].localOffset.w = 1.0;
                blueprint->buildings[i].localOffset2.x = *((f32_t*)(ptr_bin + building_offset_localOffset_x2));
                blueprint->buildings[i].localOffset2.y = *((f32_t*)(ptr_bin + building_offset_localOffset_y2));
                blueprint->buildings[i].localOffset2.z = *((f32_t*)(ptr_bin + building_offset_localOffset_z2));
                blueprint->buildings[i].localOffset2.w = 1.0;
                blueprint->buildings[i].yaw = *((f32_t*)(ptr_bin + building_offset_yaw));
                blueprint->buildings[i].yaw2 = *((f32_t*)(ptr_bin + building_offset_yaw2));
                blueprint->buildings[i].itemId = *((i16_t*)(ptr_bin + building_offset_itemId));
                blueprint->buildings[i].modelIndex = *((i16_t*)(ptr_bin + building_offset_modelIndex));
                blueprint->buildings[i].tempOutputObjIdx = *((i32_t*)(ptr_bin + building_offset_tempOutputObjIdx));
                blueprint->buildings[i].tempInputObjIdx = *((i32_t*)(ptr_bin + building_offset_tempInputObjIdx));
                blueprint->buildings[i].outputToSlot = *((i8_t*)(ptr_bin + building_offset_outputToSlot));
                blueprint->buildings[i].inputFromSlot = *((i8_t*)(ptr_bin + building_offset_inputFromSlot));
                blueprint->buildings[i].outputFromSlot = *((i8_t*)(ptr_bin + building_offset_outputFromSlot));
                blueprint->buildings[i].inputToSlot = *((i8_t*)(ptr_bin + building_offset_inputToSlot));
                blueprint->buildings[i].outputOffset = *((i8_t*)(ptr_bin + building_offset_outputOffset));
                blueprint->buildings[i].inputOffset = *((i8_t*)(ptr_bin + building_offset_inputOffset));
                blueprint->buildings[i].recipeId = *((i16_t*)(ptr_bin + building_offset_recipeId));
                blueprint->buildings[i].filterId = *((i16_t*)(ptr_bin + building_offset_filterId));

                // 解析建筑的参数列表
                const size_t PARAMETERS_NUM = *((i16_t*)(ptr_bin + building_offset_numParameters));
                dspbptk_calloc_parameters(&blueprint->buildings[i], PARAMETERS_NUM);

                // 解析建筑的参数列表
                ptr_bin += building_offset_parameters;
                for(size_t j = 0; j < PARAMETERS_NUM; j++)
                    blueprint->buildings[i].parameters[j] = *((i32_t*)(ptr_bin + j * sizeof(i32_t)));
                ptr_bin += PARAMETERS_NUM * sizeof(i32_t);
            }
        }
    }

    return no_error;
}



////////////////////////////////////////////////////////////////////////////////
// dspbptk encode
////////////////////////////////////////////////////////////////////////////////

int cmp_id(const void* p_a, const void* p_b) {
    index_t* a = ((index_t*)p_a);
    index_t* b = ((index_t*)p_b);
    return a->id - b->id;
}

void generate_lut(const blueprint_t* blueprint, index_t* id_lut) {
    for(size_t i = 0; i < blueprint->numBuildings; i++) {
        id_lut[i].id = blueprint->buildings[i].index;
        id_lut[i].index = i;
    }
    qsort(id_lut, blueprint->numBuildings, sizeof(index_t), cmp_id);
}

i32_t get_idx(i64_t* ObjIdx, index_t* id_lut, size_t numBuildings) {
    if(*ObjIdx == OBJ_NULL)
        return OBJ_NULL;
    index_t* p_id = bsearch(ObjIdx, id_lut, numBuildings, sizeof(index_t), cmp_id);
    if(p_id == NULL) {
    #ifndef DSPBPTK_NO_WARNING
        fprintf(stderr, "Warning: index %"PRId64" no found! Reindex index to OBJ_NULL(-1).\n", *ObjIdx);
    #endif
        return OBJ_NULL;
    }
    else {
        return p_id->index;
    }
}

dspbptk_error_t blueprint_encode(dspbptk_coder_t* coder, const blueprint_t* blueprint, char* string) {

    // 初始化用于操作的几个指针
    void* bin = coder->buffer0;
    char* ptr_str = string;
    void* ptr_bin = bin;

    // 输出head
    sprintf(string, "BLUEPRINT:0,%"PRId64",%"PRId64",%"PRId64",%"PRId64",%"PRId64",%"PRId64",0,%"PRId64",%"PRId64".%"PRId64".%"PRId64".%"PRId64",%s,%s\"",
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
        blueprint->shortDesc == NULL ? "\0" : blueprint->shortDesc,
        blueprint->desc == NULL ? "\0" : blueprint->desc
    );
    size_t head_length = strlen(string) - 1;
    ptr_str += head_length + 1;

    // 编码bin head
    *((i32_t*)(ptr_bin + bin_offset_version)) = (i32_t)blueprint->version;
    *((i32_t*)(ptr_bin + bin_offset_cursorOffsetX)) = (i32_t)blueprint->cursorOffsetX;
    *((i32_t*)(ptr_bin + bin_offset_cursorOffsetY)) = (i32_t)blueprint->cursorOffsetY;
    *((i32_t*)(ptr_bin + bin_offset_cursorTargetArea)) = (i32_t)blueprint->cursorTargetArea;
    *((i32_t*)(ptr_bin + bin_offset_dragBoxSizeX)) = (i32_t)blueprint->dragBoxSizeX;
    *((i32_t*)(ptr_bin + bin_offset_dragBoxSizeY)) = (i32_t)blueprint->dragBoxSizeY;
    *((i32_t*)(ptr_bin + bin_offset_primaryAreaIdx)) = (i32_t)blueprint->primaryAreaIdx;


    // 编码区域总数
    *((i8_t*)(ptr_bin + bin_offset_numAreas)) = (i8_t)blueprint->numAreas;

    // 编码区域数组
    ptr_bin += bin_offset_areas;
    for(size_t i = 0; i < blueprint->numAreas; i++) {
        *((i8_t*)(ptr_bin + area_offset_index)) = (i8_t)blueprint->areas[i].index;
        *((i8_t*)(ptr_bin + area_offset_parentIndex)) = (i8_t)blueprint->areas[i].parentIndex;
        *((i16_t*)(ptr_bin + area_offset_tropicAnchor)) = (i16_t)blueprint->areas[i].tropicAnchor;
        *((i16_t*)(ptr_bin + area_offset_areaSegments)) = (i16_t)blueprint->areas[i].areaSegments;
        *((i16_t*)(ptr_bin + area_offset_anchorLocalOffsetX)) = (i16_t)blueprint->areas[i].anchorLocalOffsetX;
        *((i16_t*)(ptr_bin + area_offset_anchorLocalOffsetY)) = (i16_t)blueprint->areas[i].anchorLocalOffsetY;
        *((i16_t*)(ptr_bin + area_offset_width)) = (i16_t)blueprint->areas[i].width;
        *((i16_t*)(ptr_bin + area_offset_height)) = (i16_t)blueprint->areas[i].height;
        ptr_bin += area_offset_next;
    }

    // 编码建筑总数
    *((i32_t*)(ptr_bin)) = (i32_t)blueprint->numBuildings;

    // 重新生成index
    index_t* id_lut = (index_t*)coder->buffer1;
    generate_lut(blueprint, id_lut);
    // re_index(blueprint, coder->buffer1);

    // 编码建筑数组
    ptr_bin += sizeof(i32_t);
    for(size_t i = 0; i < blueprint->numBuildings; i++) {
        *((i32_t*)(ptr_bin + building_offset_index)) = get_idx(&blueprint->buildings[i].index, id_lut, blueprint->numBuildings);
        *((i8_t*)(ptr_bin + building_offset_areaIndex)) = (i8_t)blueprint->buildings[i].areaIndex;
        *((f32_t*)(ptr_bin + building_offset_localOffset_x)) = (f32_t)(blueprint->buildings[i].localOffset.x / blueprint->buildings[i].localOffset.w);
        *((f32_t*)(ptr_bin + building_offset_localOffset_y)) = (f32_t)(blueprint->buildings[i].localOffset.y / blueprint->buildings[i].localOffset.w);
        *((f32_t*)(ptr_bin + building_offset_localOffset_z)) = (f32_t)(blueprint->buildings[i].localOffset.z / blueprint->buildings[i].localOffset.w);
        *((f32_t*)(ptr_bin + building_offset_localOffset_x2)) = (f32_t)(blueprint->buildings[i].localOffset2.x / blueprint->buildings[i].localOffset2.w);
        *((f32_t*)(ptr_bin + building_offset_localOffset_y2)) = (f32_t)(blueprint->buildings[i].localOffset2.y / blueprint->buildings[i].localOffset2.w);
        *((f32_t*)(ptr_bin + building_offset_localOffset_z2)) = (f32_t)(blueprint->buildings[i].localOffset2.z / blueprint->buildings[i].localOffset2.w);
        *((f32_t*)(ptr_bin + building_offset_yaw)) = (f32_t)blueprint->buildings[i].yaw;
        *((f32_t*)(ptr_bin + building_offset_yaw2)) = (f32_t)blueprint->buildings[i].yaw2;
        *((i16_t*)(ptr_bin + building_offset_itemId)) = (i16_t)blueprint->buildings[i].itemId;
        *((i16_t*)(ptr_bin + building_offset_modelIndex)) = (i16_t)blueprint->buildings[i].modelIndex;
        *((i32_t*)(ptr_bin + building_offset_tempOutputObjIdx)) = get_idx(&blueprint->buildings[i].tempOutputObjIdx, id_lut, blueprint->numBuildings);
        *((i32_t*)(ptr_bin + building_offset_tempInputObjIdx)) = get_idx(&blueprint->buildings[i].tempInputObjIdx, id_lut, blueprint->numBuildings);
        *((i8_t*)(ptr_bin + building_offset_outputToSlot)) = (i8_t)blueprint->buildings[i].outputToSlot;
        *((i8_t*)(ptr_bin + building_offset_inputFromSlot)) = (i8_t)blueprint->buildings[i].inputFromSlot;
        *((i8_t*)(ptr_bin + building_offset_outputFromSlot)) = (i8_t)blueprint->buildings[i].outputFromSlot;
        *((i8_t*)(ptr_bin + building_offset_inputToSlot)) = (i8_t)blueprint->buildings[i].inputToSlot;
        *((i8_t*)(ptr_bin + building_offset_outputOffset)) = (i8_t)blueprint->buildings[i].outputOffset;
        *((i8_t*)(ptr_bin + building_offset_inputOffset)) = (i8_t)blueprint->buildings[i].inputOffset;
        *((i16_t*)(ptr_bin + building_offset_recipeId)) = (i16_t)blueprint->buildings[i].recipeId;
        *((i16_t*)(ptr_bin + building_offset_filterId)) = (i16_t)blueprint->buildings[i].filterId;

       // 编码建筑的参数列表长度
        *((i16_t*)(ptr_bin + building_offset_numParameters)) = (i16_t)blueprint->buildings[i].numParameters;

        // 编码建筑的参数列表
        ptr_bin += building_offset_parameters;
        for(size_t j = 0; j < blueprint->buildings[i].numParameters; j++) {
            *((i32_t*)(ptr_bin + sizeof(i32_t) * j)) = (i32_t)blueprint->buildings[i].parameters[j];
        }
        ptr_bin += sizeof(i32_t) * blueprint->buildings[i].numParameters;
    }

    // 计算二进制流长度
    size_t bin_length = (size_t)(ptr_bin - bin);
    void* gzip = coder->buffer1;
    size_t gzip_length = gzip_enc(coder, bin, bin_length, gzip);
    size_t base64_length = base64_enc(gzip, gzip_length, ptr_str);

    // 计算md5f
    char md5f_hex[MD5F_LENGTH + 1] = "\0";
    md5f_str(md5f_hex, coder->buffer1, string, head_length + 1 + base64_length);
    ptr_str += base64_length;
    sprintf(ptr_str, "\"%s", md5f_hex);

    return no_error;
}

////////////////////////////////////////////////////////////////////////////////
// dspbptk free blueprint
////////////////////////////////////////////////////////////////////////////////

void dspbptk_free_blueprint(blueprint_t* blueprint) {
    free(blueprint->shortDesc);
    free(blueprint->desc);
    free(blueprint->md5f);
    free(blueprint->areas);
    for(size_t i = 0; i < blueprint->numBuildings; i++) {
        if(blueprint->buildings[i].numParameters > 0)
            free(blueprint->buildings[i].parameters);
    }
    free(blueprint->buildings);
}



////////////////////////////////////////////////////////////////////////////////
// dspbptk alloc coder
////////////////////////////////////////////////////////////////////////////////

void dspbptk_init_coder(dspbptk_coder_t* coder) {
    coder->buffer0 = calloc(BLUEPRINT_MAX_LENGTH, 1);
    coder->buffer1 = calloc(BLUEPRINT_MAX_LENGTH, 1);
    coder->p_compressor = libdeflate_alloc_compressor(12);
    coder->p_decompressor = libdeflate_alloc_decompressor();
}

////////////////////////////////////////////////////////////////////////////////
// dspbptk free coder
////////////////////////////////////////////////////////////////////////////////

void dspbptk_free_coder(dspbptk_coder_t* coder) {
    free(coder->buffer0);
    free(coder->buffer1);
    libdeflate_free_compressor(coder->p_compressor);
    libdeflate_free_decompressor(coder->p_decompressor);
}



////////////////////////////////////////////////////////////////////////////////
// dspbptk api
////////////////////////////////////////////////////////////////////////////////

void dspbptk_resize(blueprint_t* blueprint, size_t N) {
    blueprint->numBuildings = N;
    blueprint->buildings = realloc(blueprint->buildings, sizeof(building_t) * N);
}