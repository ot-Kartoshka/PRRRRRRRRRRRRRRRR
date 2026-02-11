#include "Base64.hpp"
#include <iostream>
#include <string>


bool askUser(const std::string& filename) {
    std::cout << "Output filename not specified. Use '" << filename << "'? [y/n]: ";
    char response;
    std::cin >> response;
    std::cin.ignore(1000, '\n');
    return (response == 'y' || response == 'Y');
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage:\n  Encode: " << argv[0] << " -e <input> [output]\n";
        std::cout << "  Decode: " << argv[0] << " -d <input> [output]\n";
        return 1;
    }

    std::string mode = argv[1];
    fs::path inputFile = argv[2];
    fs::path outputFile;

    try {
        if (!fs::exists(inputFile)) {
            std::cerr << "Error: Input file does not exist.\n";
            return 1;
        }

        if (mode == "-e") {
            if (argc >= 4) {
                outputFile = argv[3];
            }
            else {
                outputFile = inputFile.string() + ".base64";
                std::cout << "Output file not provided. Creating: " << outputFile << "\n";
            }

            Base64::encodeFile(inputFile, outputFile);
            std::cout << "Encoding complete.\n";

        }
        else if (mode == "-d") {
            if (argc >= 4) {
                outputFile = argv[3];
            }
            else {
                std::string suggestedName = Base64::getMetadataFilename(inputFile);

                if (suggestedName.empty() && inputFile.extension() == ".base64") {
                    suggestedName = inputFile.stem().string();
                }

                if (!suggestedName.empty()) {
                    if (askUser(suggestedName)) {
                        outputFile = suggestedName;
                    }
                    else {
                        std::cout << "Please enter output filename: ";
                        std::string userFilename;
                        std::cin >> userFilename;
                        outputFile = userFilename;
                    }
                }
                else {
                    std::cerr << "Error: Output filename required.\n";
                    return 1;
                }
            }

            if (fs::exists(outputFile)) {
                std::cout << "Warning: File " << outputFile << " exists. Overwrite? [y/n]: ";
                char resp;
                std::cin >> resp;
                if (resp != 'y' && resp != 'Y') {
                    std::cout << "Operation cancelled.\n";
                    return 0;
                }
            }

            Base64::decodeFile(inputFile, outputFile);
            std::cout << "Decoding complete.\n";
        }
    }
    catch (const Base64Exception& e) {
        std::cerr << "Error at line " << e.getLine();
        if (e.getPos() > 0) std::cerr << ", pos " << e.getPos();
        std::cerr << ": " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}