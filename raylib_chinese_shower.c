#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define UTF8_CODEPOINT_TOTAL_NUMBER  0x110000
#define UTF8_CODEPOINT_BITMAP_SIZE   (UTF8_CODEPOINT_TOTAL_NUMBER >> 3)

typedef struct {
    char* str;
    int32_t capacity;
    int32_t length;
} character_collector;

typedef struct {
    character_collector chars;
    uint8_t* codepoint_bitmap;
    int32_t codepoint_number;
} utf8_codepoint_collector;

typedef struct {
    utf8_codepoint_collector ucc;

    char font_file_type[32];
    unsigned char* font_file_data;
    int font_file_data_size;
} raylib_text_render;

/*
    buf size should be at least 5, result str would be ends with '\0'.
    
    if success, this function would return a positive number which represent the str's length, 
    else return -1.
*/
int32_t codepoint_to_utf8_str(uint32_t codepoint, char* buf) {
    /*
        the range is:

        1 byte:  00000000 - 0000007F
        2 bytes: 00000080 - 000007FF
        3 bytes: 00000800 - 0000FFFF
        4 bytes: 00010000 - 0010FFFF

        convert to:

        1 byte : 0xxx xxxx
        2 bytes: 110x xxxx 10xx xxxx
        3 bytes: 1110 xxxx 10xx xxxx 10xx xxxx
        4 bytes: 1111 0xxx 10xx xxxx 10xx xxxx 10xx xxxx
    */
    if (codepoint <= 0x7F) {
        buf[0] = (char)codepoint;
        buf[1] = '\0';
        return 1;
    }
    else if (codepoint <= 0x7FF) {
        buf[0] = (char)(0xC0 | (codepoint >> 6));
        buf[1] = (char)(0x80 | (codepoint & 0x3F));
        buf[2] = '\0';
        return 2;
    }
    else if (codepoint <= 0xFFFF) {
        buf[0] = (char)(0xE0 | (codepoint >> 12));
        buf[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buf[2] = (char)(0x80 | (codepoint & 0x3F));
        buf[3] = '\0';
        return 3;
    }
    else if (codepoint <= 0x10FFFF) {
        buf[0] = (char)(0xF0 | (codepoint >> 18));
        buf[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        buf[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buf[3] = (char)(0x80 | (codepoint & 0x3F));
        buf[4] = '\0';
        return 4;
    }
    else {
        return -1;
    }
}

/*
    this function assume that the given str is complete.

    if success, return the codepoint, and set the len to that codepoint's bytes number,
    else return -1.
*/
int32_t utf8_str_decode_next_codepoint(const unsigned char* str, int32_t* bytes_number) {
    int32_t code;

    if ((str[0] & 0x80) == 0) {
        code = str[0];
        *bytes_number = 1;
    }
    else if ((str[0] & 0xE0) == 0xC0) {
        code = ((str[0] & 0x1F) << 6) | (str[1] & 0x3F);
        *bytes_number = 2;
    }
    else if ((str[0] & 0xF0) == 0xE0) {
        code = ((str[0] & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
        *bytes_number = 3;
    }
    else if ((str[0] & 0xF8) == 0xF0) {
        code = ((str[0] & 0x07) << 18) | ((str[1] & 0x3F) << 12) |
               ((str[2] & 0x3F) << 6)  | (str[3] & 0x3F);
        *bytes_number = 4;
    }
    else {
        return -1;
    }

    return code;
}

static inline
uint8_t* utf8_codepoint_bitmap_create(void) {
    return (uint8_t*)calloc(UTF8_CODEPOINT_BITMAP_SIZE, sizeof(uint8_t));
}

static inline
void utf8_codepoint_bitmap_set(uint8_t* bitmap, int32_t i) {
    bitmap[i / 8] |= (1 << (i % 8));
}

static inline
void utf8_codepoint_bitmap_clear(uint8_t* bitmap, int32_t i) {
    bitmap[i / 8] &= ~(1 << (i % 8));
}

static inline
bool utf8_codepoint_bitmap_is_set(uint8_t* bitmap, int32_t i) {
    return bitmap[i / 8] >> (i % 8) & 0x1;
}

bool character_collector_init(character_collector* cc, int32_t capacity) {
    cc->capacity = capacity;
    cc->length = 0;
    cc->str = (char*)malloc(cc->capacity * sizeof(char));

    if (!cc->str) {
        return false;
    }
    else {
        cc->str[0] = '\0';
        return true;
    }
}

void character_collector_destroy(character_collector* cc) {
    free(cc->str);
}

bool character_collector_expand_capacity(character_collector* cc, int32_t new_capacity) {
    assert(new_capacity > cc->capacity);

    char* tmp = (char*)malloc(new_capacity * sizeof(char));
    if (!tmp) {
        return false;
    }

    memcpy(tmp, cc->str, cc->length * sizeof(char));
    free(cc->str);

    cc->str = tmp;
    cc->capacity = new_capacity;
    return true;
}

bool character_collector_append(character_collector* cc, const char* data, int32_t data_len) {
    int32_t wanted = cc->length + data_len + 1;

    if (wanted > cc->capacity) {
        if (!character_collector_expand_capacity(cc, wanted)) {
            return false;
        }
    }

    memcpy(cc->str + cc->length, data, data_len * sizeof(char));
    cc->length += data_len;
    cc->str[cc->length] = '\0';
    return true;
}

bool utf8_codepoint_collector_init(utf8_codepoint_collector* ucc) {
    ucc->codepoint_bitmap = utf8_codepoint_bitmap_create();
    if (!ucc->codepoint_bitmap) {
        return false;
    }

    if (!character_collector_init(&(ucc->chars), 256)) {
        free(ucc->codepoint_bitmap);
        return false;
    }

    ucc->codepoint_number = 0;
    return true;
}

void utf8_codepoint_collector_destroy(utf8_codepoint_collector* ucc) {
    free(ucc->codepoint_bitmap);
    character_collector_destroy(&(ucc->chars));
}

bool utf8_codepoint_collector_put_by_str(utf8_codepoint_collector* ucc, const char* utf8_str) {
    const unsigned char* ptr = (const unsigned char*)utf8_str;
    int32_t bytes_number = 0;

    while (*ptr) {
        int32_t codepoint = utf8_str_decode_next_codepoint(ptr, &bytes_number);

        if (!utf8_codepoint_bitmap_is_set(ucc->codepoint_bitmap, codepoint)) {
            if (!character_collector_append(&(ucc->chars), (const char*)ptr, bytes_number)) {
                return false;
            }

            utf8_codepoint_bitmap_set(ucc->codepoint_bitmap, codepoint);
            ucc->codepoint_number += 1;
        }

        ptr += bytes_number;
    }

    return true;
}

bool utf8_codepoint_collector_put_by_codepoint(utf8_codepoint_collector* ucc, uint32_t codepoint) {
    if (!utf8_codepoint_bitmap_is_set(ucc->codepoint_bitmap, codepoint)) {
        char buf[5];
        int32_t len = codepoint_to_utf8_str(codepoint, buf);

        if (len < 0) {
            return false;
        }

        if (!character_collector_append(&(ucc->chars), buf, len)) {
            return false;
        }

        utf8_codepoint_bitmap_set(ucc->codepoint_bitmap, codepoint);
        ucc->codepoint_number += 1;
        return true;
    }

    return false;
}

bool raylib_text_render_init(raylib_text_render* render, const char* font_file_type, const char* font_file_path) {
    snprintf(render->font_file_type, sizeof(render->font_file_type), "%s", font_file_type);
    
    render->font_file_data = LoadFileData(font_file_path, &(render->font_file_data_size));
    if (render->font_file_data == NULL) {
        return false;
    }

    if (!utf8_codepoint_collector_init(&(render->ucc))) {
        UnloadFileData(render->font_file_data);
        return false;
    }

    return true;
}

void raylib_text_render_destroy(raylib_text_render* render) {
    utf8_codepoint_collector_destroy(&(render->ucc));

    if (render->font_file_data) {
        UnloadFileData(render->font_file_data);
    }
}

bool raylib_text_render_add_characters(raylib_text_render* render, const char* utf8_str) {
    return utf8_codepoint_collector_put_by_str(&(render->ucc), utf8_str);
}

bool raylib_text_render_add_codepoint(raylib_text_render* render, uint32_t codepoint) {
    assert(codepoint > 0 && codepoint < UTF8_CODEPOINT_TOTAL_NUMBER);
    return utf8_codepoint_collector_put_by_codepoint(&(render->ucc), codepoint);
}

bool raylib_text_render_load_font(raylib_text_render* render, int font_size, Font* font) {
    int raylib_codepoints_count = 0;
    int* raylib_codepoints = LoadCodepoints(render->ucc.chars.str, &raylib_codepoints_count);

    if (!raylib_codepoints) {
        return false;
    }

    Font tmp = LoadFontFromMemory(render->font_file_type, render->font_file_data, render->font_file_data_size, font_size, raylib_codepoints, raylib_codepoints_count);

    if (!IsFontValid(tmp)) {
        UnloadCodepoints(raylib_codepoints);
        return false;
    }
    
    *font = tmp;
    UnloadCodepoints(raylib_codepoints);
    return true;
}

/*
    gcc raylib_chinese_shower.c -I D:\third-party\raylib-5.5_win64_mingw-w64\raylib-5.5_win64_mingw-w64\include -L D:\third-party\raylib-5.5_win64_mingw-w64\raylib-5.5_win64_mingw-w64\lib -l raylib -lopengl32 -lgdi32 -lwinmm
*/
int main(void) {
    InitWindow(800, 450, "测试中文显示");
    SetTargetFPS(60);

    /* these code must be called after InitWindow. */
    raylib_text_render rtr;
    raylib_text_render_init(&rtr, ".ttf", "simkai.ttf");

    raylib_text_render_add_characters(&rtr, "这是一段文本");
    raylib_text_render_add_characters(&rtr, "纯文本用于测试");
    raylib_text_render_add_characters(&rtr, "是的, 测试专用");

    Font font;
    raylib_text_render_load_font(&rtr, 32, &font);

    /* draw text. */
    while (!WindowShouldClose()) 
    {
        BeginDrawing();
        ClearBackground(WHITE);
        DrawTextEx(font, "这是一段文本, 专用于测试", (Vector2){ 50, 50 }, 32, 0, RED);
        EndDrawing();
    }

    UnloadFont(font);
    CloseWindow();
    raylib_text_render_destroy(&rtr);
    return 0;
}
