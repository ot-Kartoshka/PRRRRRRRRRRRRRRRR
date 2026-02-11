#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class Base64Exception : public std::runtime_error {
public:
    Base64Exception(const std::string& msg, size_t line, size_t pos = 0)
        : std::runtime_error(msg), line_(line), pos_(pos) {}

    size_t getLine() const { return line_; }
    size_t getPos() const { return pos_; }
private:
    size_t line_;
    size_t pos_;
};

class Base64 {
public:
    static constexpr size_t LINE_LENGTH = 76;
    static constexpr char PAD_CHAR = '=';
    static constexpr char COMMENT_CHAR = '-';
    static const std::string METADATA_PREFIX; 

    static void encodeFile(const fs::path& inputPath, const fs::path& outputPath);
    static void decodeFile(const fs::path& inputPath, const fs::path& outputPath);

    static std::string getMetadataFilename(const fs::path& inputPath);

private:
    static int getCharIndex(char c);
};