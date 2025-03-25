/*
* Based off: https://github.com/JayFoxRox/UnXiP/blob/master/unxip.cpp
*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

// XIP file header structure
struct XIPHDR {
    char magic[4];        // "XIP0"
    uint32_t dataOffset;  // Offset to file data
    uint16_t numFiles;    // Number of files in archive
    uint16_t numNames;    // Number of filenames
    uint32_t dataSize;    // Total size of file data
};

// File entry data structure
struct FILEDATA {
    uint32_t offset;      // Offset from data section start
    uint32_t size;        // Size of file
    uint32_t type;        // File type (4 = directory)
    uint32_t timestamp;   // File timestamp
};

// Filename entry structure
struct FILENAME {
    uint16_t dataIndex;   // Index into file data array
    uint16_t nameOffset;  // Offset into filename block
};

class XIPHandler {
public:
    XIPHandler();
    ~XIPHandler();

    bool OpenXIP(const std::string& filepath);
    bool CreateXIP(const std::string& filepath);
    bool ExtractFile(const std::string& filename, const std::string& outputPath);
    bool AddFile(const std::string& filepath, const std::string& entryName);
    std::vector<std::string> ListFiles();
    void Close();

private:
    bool ReadHeader();
    bool ReadFileEntries();
    bool WriteHeader();
    bool WriteFileEntries();
    const char* GetFilenameAt(size_t index) const;

    FILE* xipFile;
    XIPHDR header;
    std::vector<FILEDATA> fileData;
    std::vector<FILENAME> fileNames;
    std::vector<char> filenameBlock;
    bool isModified;
}; 