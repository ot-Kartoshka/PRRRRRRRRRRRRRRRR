#include "Base64.hpp"
#include <iostream>
#include <vector>
#include <algorithm>

static const std::string_view B64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const std::string Base64::METADATA_PREFIX = "- Original filename: ";

int Base64::getCharIndex(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

std::string Base64::getMetadataFilename(const fs::path& inputPath) {
    std::ifstream inFile(inputPath);
    if (!inFile) return "";
    std::string line;
    if (std::getline(inFile, line)) {
        if (line.rfind(METADATA_PREFIX, 0) == 0) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            return line.substr(METADATA_PREFIX.length());
        }
    }
    return "";
}

void Base64::encodeFile(const fs::path& inputPath, const fs::path& outputPath) {
    std::ifstream inFile(inputPath, std::ios::binary);
    if (!inFile) throw std::runtime_error("Cannot open input file");

    std::ofstream outFile(outputPath);
    if (!outFile) throw std::runtime_error("Cannot open output file");

    outFile << METADATA_PREFIX << inputPath.filename().string() << "\n";

    std::vector<uint8_t> buffer(3);
    std::string lineBuffer;

    while (inFile.read(reinterpret_cast<char*>(buffer.data()), 3) || inFile.gcount() > 0) {
        size_t count = inFile.gcount();
        if (count < 3) std::fill(buffer.begin() + count, buffer.end(), 0);

        uint32_t val = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];

        std::string encodedBlock(4, ' ');
        encodedBlock[0] = B64_CHARS[(val >> 18) & 0x3F];
        encodedBlock[1] = B64_CHARS[(val >> 12) & 0x3F];
        encodedBlock[2] = (count > 1) ? B64_CHARS[(val >> 6) & 0x3F] : PAD_CHAR;
        encodedBlock[3] = (count > 2) ? B64_CHARS[val & 0x3F] : PAD_CHAR;

        lineBuffer += encodedBlock;

            if (lineBuffer.length() >= LINE_LENGTH) {
                outFile << lineBuffer.substr(0, LINE_LENGTH) << "\n";
                lineBuffer = lineBuffer.substr(LINE_LENGTH);
            }
        std::fill(buffer.begin(), buffer.end(), 0);
    }
    if (!lineBuffer.empty()) outFile << lineBuffer << "\n";
}

void Base64::decodeFile(const fs::path& inputPath, const fs::path& outputPath) {
    std::ifstream inFile(inputPath);
    if (!inFile) throw std::runtime_error("Cannot open encoded file");

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) throw std::runtime_error("Cannot open output file");


    std::string line;
    size_t lineCounter = 0;  
    size_t currentLineNum = 0;  

    std::string currentLine;
    bool hasCurrent = false;
    bool finished = false;

    auto processLine = [&](const std::string& str, bool isLastLine, size_t actualLineNum) {
        if (!isLastLine && str.length() != LINE_LENGTH) {
            throw Base64Exception("Некоректна довжина рядку (" + std::to_string(str.length()) + ")", actualLineNum);
        }

        if (str.length() % 4 != 0) {
            throw Base64Exception("Некоректна довжина блоку (не кратна 4)", actualLineNum);
        }

        size_t commentPos = str.find(COMMENT_CHAR);
        if (commentPos != std::string::npos) {
            throw Base64Exception("Некоректний вхідний символ (" + std::string(1, COMMENT_CHAR) + ")", actualLineNum, commentPos);
        }

        for (size_t i = 0; i < str.length(); i += 4) {
            if (finished) {
                std::cerr << "Warning: Data found after end of message at line " << actualLineNum << ". Ignored.\n";
                return;
            }

            std::string chunk = str.substr(i, 4);
            int vals[4];

            for (int k = 0; k < 4; ++k) {
                if (chunk[k] == PAD_CHAR) {
                    vals[k] = -1;

                        if (k < 2) {
                            throw Base64Exception("Неправильне використання паддінгу", actualLineNum, i + k);
                        }

                    if (k < 3 && chunk[k + 1] != PAD_CHAR) {
                        throw Base64Exception("Неправильне використання паддінгу (розрив)", actualLineNum, i + k);
                    }

                }
                else {
                    vals[k] = getCharIndex(chunk[k]);

                        if (vals[k] == -1) {
                            throw Base64Exception("Некоректний вхідний символ (" + std::string(1, chunk[k]) + ")", actualLineNum, i + k);
                        }
                }
            }

            uint32_t val = 0;
            int validBytes = 0;

            if (vals[2] == -1) { 
                if (vals[3] != -1) throw Base64Exception("Неправильне використання паддінгу", actualLineNum, i + 3);
                val = (vals[0] << 18) | (vals[1] << 12);
                validBytes = 1;
                finished = true;
            }
            else if (vals[3] == -1) { 
                val = (vals[0] << 18) | (vals[1] << 12) | (vals[2] << 6);
                validBytes = 2;
                finished = true;
            }
            else {
                val = (vals[0] << 18) | (vals[1] << 12) | (vals[2] << 6) | vals[3];
                validBytes = 3;
            }

            outFile.put((val >> 16) & 0xFF);
            if (validBytes > 1) outFile.put((val >> 8) & 0xFF);
            if (validBytes > 2) outFile.put(val & 0xFF);
        }
        };

    while (std::getline(inFile, line)) {
        lineCounter++; 
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line.empty() || line[0] == COMMENT_CHAR) continue;

        if (hasCurrent) {
            processLine(currentLine, false, currentLineNum);
        }

        currentLine = line;
        currentLineNum = lineCounter;
        hasCurrent = true;
    }

    if (hasCurrent) {
        processLine(currentLine, true, currentLineNum);
    }
}