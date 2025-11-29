/*
    @author yuan
    @brief This is my solution to handle some Chinese characters shown problems for raylib, just a cache pool base on a hash map.
*/
#ifndef __RAYLIB_EXTEND_TEXT_BUFFER_POOL_H__
#define __RAYLIB_EXTEND_TEXT_BUFFER_POOL_H__

#include <stddef.h>
#include <raylib.h>

struct TextData {
    char* text;
    Font font;
    int fontSize;

    struct TextData* next;
};

struct TextBufferPool {
    struct TextData** bucket;
    unsigned int fileDataSize;
    unsigned char* fontFileData;
    size_t bucketSize;
    size_t len;
};

/*
    init the whole thing with your given font.
    you must make sure that the fontPath is valid, this function won't check that.

    if out of memory, this funtion return 0, else return 1.
*/
int text_buffer_pool_init(struct TextBufferPool* pool, int bucketSize, const char* fontPath);

/*
    destroy the whole thing.
*/
void text_buffer_pool_destroy(struct TextBufferPool* pool);

/*
    build the text data, and return it to you (no rehash would happen, so you can use the returned pointer safely).
    if the text has been built before, and the fontSize is not equal to the given one, this function would update it.

    if out of memory, this function return NULL, else return the built data.
*/
struct TextData* text_buffer_pool_put(struct TextBufferPool* pool, const char* text, int fontSize);

/*
    get data handle by the given text.
    if not found, return NULL.
*/
struct TextData* text_buffer_pool_get(struct TextBufferPool* pool, const char* text);

/*
    delete one text data by the given name.
*/
void text_buffer_pool_del(struct TextBufferPool* pool, const char* text);

/*
    see what we have already.
*/
void text_buffer_pool_seek_text(const struct TextBufferPool* pool);

#endif
