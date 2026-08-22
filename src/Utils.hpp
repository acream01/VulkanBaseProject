#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>


// ate: Start reading at the end of the file
    // binary: Read the file as binary file(avoid text transformations)
inline std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    // read from the end of the file to allocate precise buffer size
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    // rewind, read all bytes into buffer
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;

}