#include "raylib.h"
#include "raylib_extend_text_buffer_pool.h"

int main(void) 
{
    struct TextBufferPool pool;
    struct TextData* text;

    /* load the simfang.ttf, then we would render text with it. */
    text_buffer_pool_init(&pool, 256, "simfang.ttf");
    InitWindow(800, 450, "测试中文显示");
    
    /* so here, we create our needed text. */
    text = text_buffer_pool_put(&pool, "这是一段测试文本", 32);
    SetTargetFPS(60);

    while (!WindowShouldClose()) 
    {
        BeginDrawing();
        ClearBackground(WHITE);

        /* just render the text. */
        DrawTextEx(text->font, text->text, (Vector2){ 50,50 }, text->fontSize, 5, RED);
        
        EndDrawing();
    }

    text_buffer_pool_destroy(&pool);
    return 0;
}
