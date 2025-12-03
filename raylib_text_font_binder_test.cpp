#include "raylib.h"
#include "raylib_text_font_binder.hpp"

using namespace raylib_extend;

// g++ raylib_text_font_binder_test.cpp -I D:\third-party\raylib-5.5_win64_mingw-w64\raylib-5.5_win64_mingw-w64\include -L D:\third-party\raylib-5.5_win64_mingw-w64\raylib-5.5_win64_mingw-w64\lib -l raylib -lopengl32 -lgdi32 -lwinmm
int main(void) 
{
    FontFileData ffd{ "simkai.ttf" };

    InitWindow(800, 450, "测试中文显示");
    SetTargetFPS(60);

    TextData data{ "这是一段测试文本", ffd, 64 };  /* render our text. */

    while (!WindowShouldClose()) 
    {
        BeginDrawing();
        ClearBackground(WHITE);
        DrawTextEx(*(data.font()), data.text().c_str(), (Vector2){ 50, 50 }, data.font_size(), 20, RED);  /* draw it. */
        EndDrawing();
    }

    return 0;
}
