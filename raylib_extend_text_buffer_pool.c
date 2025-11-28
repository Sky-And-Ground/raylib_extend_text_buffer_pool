#include "raylib_extend_text_buffer_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int text_data_init(struct TextBufferPool* pool, struct TextData* data, const char* text, int rebuildText, int fontSize) {
    int codepointsCount;
    int* codepoints;

    if (rebuildText) {
        data->text = strdup(text);
        if (data->text == NULL) {
            return 0;
        }
    }

    codepoints = LoadCodepoints(text, &codepointsCount);
    data->font = LoadFontFromMemory(".ttf", pool->fontFileData, pool->fileDataSize, fontSize, codepoints, codepointsCount);
    UnloadCodepoints(codepoints);

    data->fontSize = fontSize;
    return 1;
}

static void text_data_destroy(struct TextData* data) {
    UnloadFont(data->font);
    free(data->text);
    free(data);
}

static unsigned int text_buffer_pool_hash(struct TextBufferPool* pool, const char* key) {
    unsigned int hashval = 0;

    while (*key) {
        hashval = 131 * hashval + (unsigned int)(*key - '0');
        ++key;
    }

    return hashval % pool->bucketSize;
}

int text_buffer_pool_init(struct TextBufferPool* pool, int bucketSize, const char* fontPath) {
    int i;

    pool->bucket = (struct TextData**)malloc(bucketSize * sizeof(struct TextData*));
    if (pool->bucket == NULL) {
        return 0;
    }

    for (i = 0; i < bucketSize; ++i) {
        pool->bucket[i] = NULL;
    }

    pool->fontFileData = LoadFileData(fontPath, &(pool->fileDataSize));
    pool->bucketSize = bucketSize;
    pool->len = 0;
    return 1;
}

void text_buffer_pool_destroy(struct TextBufferPool* pool) {
    int i;
    for (i = 0; i < pool->bucketSize; ++i) {
        struct TextData* cursor = pool->bucket[i];
        struct TextData* tmp;

        while (cursor) {
            tmp = cursor->next;
            text_data_destroy(cursor);
            cursor = tmp;
        }
    }

    UnloadFileData(pool->fontFileData);
}

struct TextData* text_buffer_pool_get(struct TextBufferPool* pool, const char* text) {
    unsigned int hashval = text_buffer_pool_hash(pool, text);
    struct TextData* cursor = pool->bucket[hashval];
    
    while (cursor) {
        if (strcmp(text, cursor->text) == 0) {
            return cursor;
        }

        cursor = cursor->next;
    }
    
    return NULL;
}

struct TextData* text_buffer_pool_put(struct TextBufferPool* pool, const char* text, int fontSize) {
    struct TextData* node = text_buffer_pool_get(pool, text);

    /* if exists, then update it. */
    if (node) {
        UnloadFont(node->font);
        text_data_init(pool, node, text, 0, fontSize);
        return node;
    }
    else {
        node = (struct TextData*)malloc(sizeof(struct TextData));

        if (node == NULL) {
            return NULL;
        }
        else {
            unsigned int hashval = text_buffer_pool_hash(pool, text);
            
            if (!text_data_init(pool, node, text, 1, fontSize)) {
                free(node);
                return NULL;
            }
            
            if (pool->bucket[hashval] == NULL) {
                pool->bucket[hashval] = node;
            }
            else {
                node->next = pool->bucket[hashval];
                pool->bucket[hashval] = node;
            }
            
            pool->len += 1;
            return node;
        }
    }
}

void text_buffer_pool_del(struct TextBufferPool* pool, const char* text) {
    unsigned int hashval = text_buffer_pool_hash(pool, text);
    struct TextData* cursor = pool->bucket[hashval];
    struct TextData* parent = NULL;
    
    while (cursor) {
        if (strcmp(text, cursor->text) == 0) {
            if (cursor == pool->bucket[hashval]) {
                struct TextData* tmp = cursor->next;
                text_data_destroy(cursor);
                pool->bucket[hashval] = tmp;
            }
            else {
                parent->next = cursor->next;
                free(cursor);
            }

            pool->len -= 1;
            return;
        }

        parent = cursor;
        cursor = cursor->next;
    }
}

void text_buffer_pool_seek_text(const struct TextBufferPool* pool) {
    int i;
    struct TextData* cursor;

    for (i = 0; i < pool->bucketSize; ++i) {
        cursor = pool->bucket[i];

        while (cursor) {
            printf("%s\n", cursor->text);
            cursor = cursor->next;
        }
    }
}
