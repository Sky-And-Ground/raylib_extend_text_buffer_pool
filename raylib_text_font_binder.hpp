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
#include <cstdint>
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
                    UnloadFont(*font);
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
        std::wstring _characters;
        std::unique_ptr<Font, detail::FontDeleter> _font;
        int _fontSize;

        std::string conv_unicode_to_utf8(const std::wstring& wstr) {
            std::string result;

            for (wchar_t c : wstr) {
                auto i = static_cast<uint32_t>(c);   // u32string could also work.

                if (i < 0x80) {
                    result += static_cast<char>(i);
                }
                else if (i < 0x800) {
                    result += static_cast<char>(0xc0 | (i >> 6));
                    result += static_cast<char>(0x80 | (i & 0x3f));
                }
                else if (i < 0x10000) {
                    result += static_cast<char>(0xe0 | (i >> 12));
                    result += static_cast<char>(0x80 | ((i >> 6) & 0x3f));
                    result += static_cast<char>(0x80 | (i & 0x3f));
                }
                else if (i < 0x200000) {
                    result += static_cast<char>(0xf0 | (i >> 18));
                    result += static_cast<char>(0x80 | ((i >> 12) & 0x3f));
                    result += static_cast<char>(0x80 | ((i >> 6) & 0x3f));
                    result += static_cast<char>(0x80 | (i & 0x3f));
                }
                else {
                    result += static_cast<char>(0xf8 | (i >> 24));
                    result += static_cast<char>(0x80 | ((i >> 18) & 0x3f));
                    result += static_cast<char>(0x80 | ((i >> 12) & 0x3f));
                    result += static_cast<char>(0x80 | ((i >> 6) & 0x3f));
                    result += static_cast<char>(0x80 | (i & 0x3f));
                }
            }

            return result;
        }

        std::wstring conv_utf8_to_unicode(const std::string& str) {
            std::wstring wstr;

            for (size_t i = 0; i < str.size() ;) {
                uint32_t codepoint = 0;
                uint8_t byte = static_cast<uint8_t>(str[i]);

                if ((byte & 0x80) == 0) {
                    codepoint = byte;
                    i += 1;
                }
                else if ((byte & 0xe0) == 0xc0) {
                    codepoint = ((byte & 0x1f) << 6) | (static_cast<uint8_t>(str[i + 1]) & 0x3f);
                    i += 2;
                }
                else if ((byte & 0xf0) == 0xe0) {
                    codepoint = ((byte & 0x0f) << 12) |
                                ((static_cast<uint8_t>(str[i + 1]) & 0x3f) << 6) |
                                (static_cast<uint8_t>(str[i + 2]) & 0x3f);
                    i += 3;
                }
                else if ((byte & 0xf8) == 0xf0) {
                    codepoint = ((byte & 0x07) << 18) |
                                ((static_cast<uint8_t>(str[i + 1]) & 0x3F) << 12) |
                                ((static_cast<uint8_t>(str[i + 2]) & 0x3F) << 6) |
                                (static_cast<uint8_t>(str[i + 3]) & 0x3F);
                    i += 4;
                }
                else {   // invalid, just break.
                    break;
                }

                if (codepoint <= 0xFFFF) {
                    wstr += static_cast<wchar_t>(codepoint);
                } else {
                    codepoint -= 0x10000;
                    wstr += static_cast<wchar_t>((codepoint >> 10) + 0xD800);
                    wstr += static_cast<wchar_t>((codepoint & 0x3FF) + 0xDC00);
                }
            }

            return wstr;
        }

        void reset(const std::wstring& characters, const FontFileData& ffd) {
            _characters = characters;

            CodePoints cp{ conv_unicode_to_utf8(_characters) };
            Font tmpFont = LoadFontFromMemory(".ttf", ffd.get_data(), ffd.get_data_size(), _fontSize, cp.get_data(), cp.get_count());
            _font.reset(new Font{ tmpFont });
        }
    public:
        TextData() 
            : _characters{ L"" }, _font{ nullptr }, _fontSize{ 32 }
        {}

        TextData(const std::string& characters, const FontFileData& ffd) 
            : _characters{ conv_utf8_to_unicode(characters) }, _font{ nullptr }, _fontSize{ 32 }
        {
            reset(characters, ffd);
        }

        void reset(const std::string& characters, const FontFileData& ffd) {
            _characters = conv_utf8_to_unicode(characters);

            CodePoints cp{ characters };
            Font tmpFont = LoadFontFromMemory(".ttf", ffd.get_data(), ffd.get_data_size(), _fontSize, cp.get_data(), cp.get_count());
            _font.reset(new Font{ tmpFont });
        }

        void add_if_not_exists(const std::string& others, const FontFileData& ffd) {
            bool added = false;

            std::wstring tmpWstr = conv_utf8_to_unicode(others);

            for (wchar_t c : tmpWstr) {
                auto iter = _characters.find(c);
                if (iter == std::wstring::npos) {
                    _characters += c;
                    added = true;
                }
            }

            if (added) {
                reset(_characters, ffd);
            }
        }

        const std::wstring& characters() const noexcept {
            return _characters;
        }

        const Font* font() const noexcept {
            return _font.get();
        }
    };
}
