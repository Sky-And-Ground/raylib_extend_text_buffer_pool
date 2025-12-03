/*
    @author yuan
    @brief a raylib extend tool, to solve the Chinese characters shown problems, written in C++17, only one header file.
*/
#pragma once

#include <string>
#include <stdexcept>
#include <filesystem>
#include <memory>
#include <utility>
#include <raylib.h>

namespace raylib_extend {
    class FontNotFoundException : public std::runtime_error {
    public:
        FontNotFoundException(const std::string& fontPath)
            : std::runtime_error{ "load font file failed: " + fontPath }
        {}
    };

    namespace detail {
        struct FontFileDataDeleter {
            void operator()(unsigned char* data) const noexcept {
                if (data) {
                    UnloadFileData(data);
                }
            }
        };

        struct CodepointsDeleter {
            void operator()(int* codepoints) const noexcept {
                if (codepoints) {
                    UnloadCodepoints(codepoints);
                }
            }
        };

        struct FontDeleter {
            void operator()(Font* font) const noexcept {
                if (font) {
                    if (font->texture.id != 0) {
                        UnloadFont(*font);
                    } 

                    delete font;
                }
            }
        };
    }

    // a wrapper for raylib font file data.
    class FontFileData {
        std::unique_ptr<unsigned char[], detail::FontFileDataDeleter> data;
        int dataSize;
    public:
        FontFileData(const std::string& fontPath) : data{ nullptr }, dataSize{ 0 } 
        {
            reset(fontPath);
        }

        void reset(const std::string& fontPath) {
            if (!std::filesystem::exists(fontPath)) {
                throw FontNotFoundException{ fontPath };
            }

            unsigned char* newData = LoadFileData(fontPath.c_str(), &dataSize);
            data.reset(newData);
        }

        const unsigned char* get_data() const noexcept { 
            return data.get();
        }

        int get_data_size() const noexcept { 
            return dataSize; 
        }
    };

    // a wrapper for raylib codepints.
    class CodePoints {
        std::unique_ptr<int[], detail::CodepointsDeleter> cp;
        int cpCount;
    public:
        CodePoints(const std::string& text) : cp{ nullptr }, cpCount{ 0 }
        {
            int* tmpCp = LoadCodepoints(text.data(), &cpCount);
            cp.reset(tmpCp);
        }

        int* get_data() const noexcept {
            return cp.get();
        }

        int get_count() const noexcept {
            return cpCount;
        }
    };

    class TextData {
        std::string _text;
        std::unique_ptr<Font, detail::FontDeleter> _font;
        int _fontSize;
    public:
        TextData(const std::string& text, const FontFileData& ffd, int fontSize) 
            : _text{ "" }, _font{ nullptr }, _fontSize{ 0 }
        {
            reset(text, ffd, fontSize);
        }

        void reset(const std::string& text, const FontFileData& ffd, int fontSize) {
            _text = text;
            _fontSize = fontSize;

            CodePoints cp{ _text };
            Font tmpFont = LoadFontFromMemory(".ttf", ffd.get_data(), ffd.get_data_size(), _fontSize, cp.get_data(), cp.get_count());
            _font.reset(new Font{ tmpFont });
        }

        const std::string& text() const noexcept {
            return _text;
        }

        const Font* font() const noexcept {
            return _font.get();
        }

        int font_size() const noexcept {
            return _fontSize;
        }
    };
}
