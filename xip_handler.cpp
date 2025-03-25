/*
* Based off: https://github.com/JayFoxRox/UnXiP/blob/master/unxip.cpp
*/

#include "xip_handler.h"
#include <cstring>
#include <filesystem>

XIPHandler::XIPHandler() : xipFile(nullptr), isModified(false) {
    memset(&header, 0, sizeof(XIPHDR));
}

XIPHandler::~XIPHandler() {
    Close();
}

bool XIPHandler::OpenXIP(const std::string& filepath) {
    Close();

    xipFile = fopen(filepath.c_str(), "rb+");
    if (!xipFile) {
        return false;
    }

    return ReadHeader() && ReadFileEntries();
}

bool XIPHandler::CreateXIP(const std::string& filepath) {
    Close();

    xipFile = fopen(filepath.c_str(), "wb+");
    if (!xipFile) {
        return false;
    }

    // Initialize header
    memcpy(header.magic, "XIP0", 4);
    header.numFiles = 0;
    header.numNames = 0;
    header.dataOffset = sizeof(XIPHDR);
    header.dataSize = 0;

    return WriteHeader();
}

bool XIPHandler::ReadHeader() {
    if (!xipFile) return false;

    fseek(xipFile, 0, SEEK_SET);
    if (fread(&header, 1, sizeof(XIPHDR), xipFile) != sizeof(XIPHDR)) {
        return false;
    }

    // Verify magic number
    if (strncmp(header.magic, "XIP0", 4) != 0) {
        return false;
    }

    return true;
}

bool XIPHandler::ReadFileEntries() {
    if (!xipFile) return false;

    // Read file data entries
    fileData.resize(header.numFiles);
    if (fread(fileData.data(), sizeof(FILEDATA), header.numFiles, xipFile) != header.numFiles) {
        return false;
    }

    // Read filename entries
    fileNames.resize(header.numFiles);
    if (fread(fileNames.data(), sizeof(FILENAME), header.numFiles, xipFile) != header.numFiles) {
        return false;
    }

    // Read filename block
    size_t filenameBlockOffset = sizeof(XIPHDR) + 
                                (header.numFiles * sizeof(FILEDATA)) + 
                                (header.numNames * sizeof(FILENAME));
    size_t filenameBlockSize = header.dataOffset - filenameBlockOffset;
    
    filenameBlock.resize(filenameBlockSize);
    if (fread(filenameBlock.data(), 1, filenameBlockSize, xipFile) != filenameBlockSize) {
        return false;
    }

    return true;
}

bool XIPHandler::ExtractFile(const std::string& filename, const std::string& outputPath) {
    if (!xipFile) return false;

    // Find file entry
    for (size_t i = 0; i < header.numFiles; i++) {
        const char* entryName = GetFilenameAt(i);
        if (entryName && filename == entryName) {
            const FILEDATA& entry = fileData[i];
            
            // Skip directories
            if (entry.type == 4) {
                return false;
            }

            // Create output file
            FILE* outFile = fopen(outputPath.c_str(), "wb");
            if (!outFile) {
                return false;
            }

            // Copy file data
            std::vector<uint8_t> buffer(4096);
            size_t remaining = entry.size;
            fseek(xipFile, header.dataOffset + entry.offset, SEEK_SET);

            while (remaining > 0) {
                size_t toRead = std::min(remaining, buffer.size());
                if (fread(buffer.data(), 1, toRead, xipFile) != toRead) {
                    fclose(outFile);
                    return false;
                }
                if (fwrite(buffer.data(), 1, toRead, outFile) != toRead) {
                    fclose(outFile);
                    return false;
                }
                remaining -= toRead;
            }

            fclose(outFile);
            return true;
        }
    }

    return false;
}

const char* XIPHandler::GetFilenameAt(size_t index) const {
    if (index >= header.numFiles) return nullptr;
    return filenameBlock.data() + fileNames[index].nameOffset;
}

std::vector<std::string> XIPHandler::ListFiles() {
    std::vector<std::string> files;
    for (size_t i = 0; i < header.numFiles; i++) {
        const char* filename = GetFilenameAt(i);
        if (filename) {
            files.push_back(filename);
        }
    }
    return files;
}

void XIPHandler::Close() {
    if (xipFile) {
        if (isModified) {
            WriteHeader();
            WriteFileEntries();
        }
        fclose(xipFile);
        xipFile = nullptr;
    }
    fileData.clear();
    fileNames.clear();
    filenameBlock.clear();
    isModified = false;
}

bool XIPHandler::WriteHeader() {
    if (!xipFile) return false;
    
    fseek(xipFile, 0, SEEK_SET);
    return fwrite(&header, 1, sizeof(XIPHDR), xipFile) == sizeof(XIPHDR);
}

bool XIPHandler::WriteFileEntries() {
    if (!xipFile) return false;

    // Write file data entries
    if (fwrite(fileData.data(), sizeof(FILEDATA), header.numFiles, xipFile) != header.numFiles) {
        return false;
    }

    // Write filename entries
    if (fwrite(fileNames.data(), sizeof(FILENAME), header.numFiles, xipFile) != header.numFiles) {
        return false;
    }

    // Write filename block
    return fwrite(filenameBlock.data(), 1, filenameBlock.size(), xipFile) == filenameBlock.size();
} 