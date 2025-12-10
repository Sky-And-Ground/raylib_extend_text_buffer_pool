#include "raylib.h"
#include "raylib_text_font_binder.hpp"

using namespace raylib_extend;

int main(void) 
{
    FontFileData ffd{ "simkai.ttf" };

    InitWindow(800, 450, "测试中文显示");
    SetTargetFPS(60);

    // render our needed characters, using wide string.
    // and be cautions here, we also include a space and a comma.
    TextData data;
    data.add_if_not_exists("这是一段文本", ffd);
    data.add_if_not_exists("纯文本用于测试", ffd);
    data.add_if_not_exists("是的, 测试专用", ffd);

    while (!WindowShouldClose()) 
    {
        BeginDrawing();
        ClearBackground(WHITE);
        DrawTextEx(*(data.font()), "这是一段文本, 专用于测试", (Vector2){ 50, 50 }, 32, 0, RED);  // draw it.
        EndDrawing();
    }

    return 0;
}
