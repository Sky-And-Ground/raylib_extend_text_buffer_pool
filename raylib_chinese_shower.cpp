#include <raylib.h>
#include <string>
#include <memory>
#include <bitset>
#include <cstdint>

class utf8_codepoint_collector {
    static constexpr int32_t utf8_codepoint_total_number = 0x110000;
    static constexpr int32_t bitmap_size = utf8_codepoint_total_number / 8;

    std::string characters;
    std::unique_ptr<std::bitset<bitmap_size>> bitmap;

    /*
        buf size should be at least 5, result str would be ends with '\0'.
        
        if success, this function would return a positive number which represent the str's length, 
        else return -1.
    */
    int32_t codepoint_to_utf8_str(uint32_t codepoint, char* buf) noexcept {
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
    int32_t utf8_str_decode_next_codepoint(const unsigned char* str, int32_t* bytes_number) noexcept {
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
public:
    utf8_codepoint_collector() : characters{}, bitmap{ std::make_unique<std::bitset<bitmap_size>>() } {}

    void put_by_str(const std::string& utf8_str) noexcept {
        const unsigned char* ptr = (const unsigned char*)utf8_str.c_str();
        int32_t bytes_number = 0;

        while (*ptr) {
            int32_t codepoint = utf8_str_decode_next_codepoint(ptr, &bytes_number);

            if (!bitmap->test(codepoint)) {
                characters.append((const char*)ptr, bytes_number);
                bitmap->set(codepoint, true);
            }

            ptr += bytes_number;
        }
    }

    void put_by_codepoint(uint32_t codepoint) noexcept {
        if (!bitmap->test(codepoint)) {
            char buf[5];
            int32_t len = codepoint_to_utf8_str(codepoint, buf);

            if (len < 0) {
                return;
            }

            characters.append(buf, len);
            bitmap->set(codepoint, true);
        }
    }

    const std::string& get_characters() const noexcept {
        return characters;
    }
};

struct FontFileDataDeleter {
    void operator()(unsigned char* data) const noexcept {
        if (data) {
            UnloadFileData(data);
        }
    }
};

class raylib_text_render {
    utf8_codepoint_collector ucc;

    std::unique_ptr<unsigned char[], FontFileDataDeleter> font_file_data;
    std::string font_file_type;
    int font_file_data_size;
public:
    raylib_text_render() = default;

    bool load_font_file(const std::string& font_file_type, const std::string& font_file_path) noexcept {
        this->font_file_type = font_file_type;
        unsigned char* tmp = LoadFileData(font_file_path.c_str(), &font_file_data_size);

        if (!tmp) {
            return false;
        }
        else {
            font_file_data.reset(tmp);
            return true;
        }
    }

    void add_characters(const std::string& utf8_str) noexcept {
        ucc.put_by_str(utf8_str);
    }

    void add_codepoint(uint32_t codepoint) noexcept {
        ucc.put_by_codepoint(codepoint);
    }

    bool render_font(int font_size, Font& font) noexcept {
        int raylib_codepoints_count = 0;
        int* raylib_codepoints = LoadCodepoints(ucc.get_characters().c_str(), &raylib_codepoints_count);

        if (!raylib_codepoints) {
            return false;
        }

        Font tmp = LoadFontFromMemory(font_file_type.c_str(), font_file_data.get(), font_file_data_size, font_size, raylib_codepoints, raylib_codepoints_count);

        if (!IsFontValid(tmp)) {
            UnloadCodepoints(raylib_codepoints);
            return false;
        }
        
        font = tmp;
        UnloadCodepoints(raylib_codepoints);
        return true;
    }
};

/*
    g++ raylib_chinese_shower.cpp -I D:\third-party\raylib-5.5_win64_mingw-w64\raylib-5.5_win64_mingw-w64\include -L D:\third-party\raylib-5.5_win64_mingw-w64\raylib-5.5_win64_mingw-w64\lib -l raylib -lopengl32 -lgdi32 -lwinmm
*/
int main() {
    InitWindow(800, 450, "测试中文显示");
    SetTargetFPS(60);

    // these code must be called after InitWindow.
    raylib_text_render rtr;

    if (!rtr.load_font_file(".ttf", "simkai.ttf")) {
        TraceLog(LOG_ERROR, "load font file failed\n");
        CloseWindow();
        return -1;
    }

    for (uint32_t i = 32; i < 128; ++i) {
        rtr.add_codepoint(i);
    }

    for (uint32_t i = 0x4E00; i <= 0x9FFF; ++i) {
        rtr.add_codepoint(i);
    }

    Font font;
    if (!rtr.render_font(16, font)) {
        TraceLog(LOG_ERROR, "render font failed\n");
        CloseWindow();
        return -1;
    }

    // draw text.
    while (!WindowShouldClose()) 
    {
        BeginDrawing();
        ClearBackground(WHITE);
        DrawTextEx(font, "这是一段文本, 专用于测试", (Vector2){ 50, 50 }, 32, 0, RED);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
