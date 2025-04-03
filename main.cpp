#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <nxdk/mount.h>
#include <nxdk/path.h>
#include <hal/debug.h>
#include <hal/video.h>
#include <hal/xbox.h>
#include "xbe_parser.h"
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

struct DriveMapping {
    const char* devicePath;
    const char driveLetter;
    bool failFatal;
};

// Map Xbox device paths to drive letters
static const DriveMapping DRIVE_MAPPINGS[] = {
    // HDD0
    {"\\Device\\Harddisk0\\Partition2", 'C', true},
    {"\\Device\\Harddisk0\\Partition1", 'E', true},
    {"\\Device\\Harddisk0\\Partition6", 'F', false},
    {"\\Device\\Harddisk0\\Partition7", 'G', false},
    // HDD1
    {"\\Device\\Harddisk1\\Partition1", 'H', false},
    {"\\Device\\Harddisk1\\Partition6", 'I', false},
    {"\\Device\\Harddisk1\\Partition7", 'J', false},
};

std::string Trim(const std::string& str) {
    const std::string whitespace = " \t\r\n";
    const auto start = str.find_first_not_of(whitespace);

    if (start == std::string::npos) {
        return ""; // String is all whitespace
    }

    const auto end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::vector<std::string> SplitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);

    while (std::getline(tokenStream, token, delimiter)) {
        // Trim whitespace
        token = Trim(token);

        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

void FindDefaultXBE(const std::string& path, std::vector<GameInfo>& games) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;

    std::string searchPath = path + "\\*.*";

    hFind = FindFirstFile(searchPath.c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        std::string current_path = path + "\\" + findFileData.cFileName;
        
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Skip "." and ".."
            if (strcmp(findFileData.cFileName, ".") != 0 && 
                strcmp(findFileData.cFileName, "..") != 0) {
                FindDefaultXBE(current_path, games);
            }
        } else if (strcmp(findFileData.cFileName, "default.xbe") == 0) {
            XBEParser parser;
            if (parser.LoadXBE(current_path)) {
                GameInfo game;
                game.xbe_path = current_path;
                
                if (parser.ExtractTitleID(game.title_id) && 
                    parser.ExtractTitle(game.title)) {
                    // If Title image extraction fails, its not fatal
                    parser.ExtractTitleImage(game.title_image);

                    games.push_back(game);
                    /*
                    debugPrint("Found game: %s (Title ID: %08X)\n", 
                             game.title.c_str(), 
                             game.title_id);
                    debugPrint("Title image: %zu bytes\n",
                             game.title_image.size());
                    */
                }
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

std::string GetDirectoryName(const std::string& fullPath) {
    // Find the last backslash
    size_t lastSlash = fullPath.find_last_of('\\');
    if (lastSlash == std::string::npos) return "";
    
    // Find the second-to-last backslash
    size_t secondLastSlash = fullPath.find_last_of('\\', lastSlash - 1);
    if (secondLastSlash == std::string::npos) return "";
    
    // Extract the directory name between the two last backslashes
    return fullPath.substr(secondLastSlash + 1, lastSlash - secondLastSlash - 1);
}

void SaveIconsIni(const std::vector<GameInfo>& games, std::string path) {
    std::ofstream f(path, std::ios::trunc);
    if (!f.is_open()) {
        debugPrint("Failed to open %s for writing!\n", path.c_str());
        return;
    }
    
    f << "[default]" << std::endl;
    for (const auto& game : games) {
        std::string dirName = GetDirectoryName(game.xbe_path);
        if (!dirName.empty()) {
            f << dirName << "=" << std::hex
              << std::setfill('0') << std::setw(8) 
              << game.title_id << std::endl;
        }
    }
}

void SaveTitleNamesIni(const std::vector<GameInfo>& games, std::string path) {
    std::map<std::string, std::string> titlecache;

    std::ifstream fin(path);
    std::string linebuf;
    if (fin.is_open() && fin.good()) {
        // We already got a TitleNames.ini, so lets read it's entries

        // Discard the first line "[default]"
        std::getline(fin, linebuf);
        if (linebuf.compare(0, strlen("[default]"), "[default]") != 0) {
            debugPrint("ERR: Invalid start of TitleNames.ini");
        }

        // Read the rest of the ini file and store values in a map
        while (std::getline(fin, linebuf)) {
            if (linebuf.empty())
                continue;

            auto split = SplitString(linebuf, '=');
            // Store key-value-pair in map
            titlecache[Trim(split[0])] = Trim(split[1]);
        }
    }

    // Add new entries to map
    for (const auto& game : games) {
        std::string dirName = GetDirectoryName(game.xbe_path);
        if (!dirName.empty() && titlecache.find(dirName) == titlecache.end()) {
            // Key is not known, add the entry
            titlecache[dirName] = game.title;
        }
    }
    
    std::ofstream f(path, std::ios::trunc);
    if (!f.is_open()) {
        debugPrint("Failed to open %s for writing!\n", path.c_str());
        return;
    }

    // Write out the final file
    f << "[default]" << std::endl;
    for (const auto& title : titlecache) {
        f << title.first << "=" << title.second << std::endl;
    }
}

void SaveTitleMeta(const std::vector<GameInfo>& games) {
    for (const auto& game : games) {
        if (game.title.empty()) continue;
        
        // Create directory path in format "E:\UDATA\XXXXXXXX\"
        char dirPath[MAX_PATH];
        snprintf(dirPath, sizeof(dirPath), "E:\\UDATA\\%08x", game.title_id);
        
        // Create the directory
        CreateDirectory(dirPath, NULL);
        
        char metaFilePath[MAX_PATH];
        snprintf(metaFilePath, sizeof(metaFilePath), "%s\\TitleMeta.xbx", dirPath);

        /*
        // Check if files already exist
        std::ifstream fMetaExists(metaFilePath);
        if (fMetaExists.is_open() && fMetaExists.good()) {
            if (fMetaExists)
                fMetaExists.close();

            debugPrint("Title metadata already exists for %s, skipping...\n", game.title.c_str());
            continue;
        }
        */

        // Write the title metadatadata
        std::ofstream f(metaFilePath, std::ios::trunc);
        if (f.is_open()) {
            f << "TitleName=" << game.title << std::endl;
            // debugPrint("Saved title meta for %s to %s\n", game.title.c_str(), metaFilePath);
        } else {
            debugPrint("Failed opening %s for writing!\n", metaFilePath);
        }
    }
}

void CopyTitleImages(const std::vector<GameInfo>& games) {
    for (const auto& game : games) {
        if (game.title_image.empty()) continue;
        
        // Create directory path in format "E:\UDATA\XXXXXXXX\"
        char dirPath[MAX_PATH];
        snprintf(dirPath, sizeof(dirPath), "E:\\UDATA\\%08X", game.title_id);
        
        // Create the directory
        CreateDirectory(dirPath, NULL);
        
        // Create full file paths
        char imageFilePath[MAX_PATH];
        snprintf(imageFilePath, sizeof(imageFilePath), "%s\\TitleImage.xbx", dirPath);

        /*
        std::ifstream fImageExists(imageFilePath);
        if (fImageExists.is_open() && fImageExists.good()) {
            fImageExists.close();

            debugPrint("Title image/icon already exist for %s, skipping...\n", game.title.c_str());
            continue;
        }
        */

        std::vector<uint8_t> titleImageData; // = game.title_image;
        //if (titleImageData.empty()) {
            // Read from local file instead
            char sourceIconPath[MAX_PATH];
            snprintf(dirPath, sizeof(dirPath), "Q:\\Icons\\%08x.xbx", game.title_id);

            std::ifstream sourceIcon(sourceIconPath, std::ios::binary | std::ios::ate);
            if (!sourceIcon.is_open()) {
                debugPrint("Failed opening icon xbx from %s\n", sourceIconPath);
                continue;
            }

            auto iconFileSize = sourceIcon.tellg();
            sourceIcon.seekg(0, std::ios::beg);

            // Read from local icon file into vec
            titleImageData.reserve(iconFileSize);
            titleImageData.insert(titleImageData.begin(),
               std::istream_iterator<uint8_t>(sourceIcon),
               std::istream_iterator<uint8_t>());
        //}

        // Write the title image data
        std::ofstream f(imageFilePath, std::ios::binary | std::ios::trunc);
        if (f.is_open()) {
            f.write(reinterpret_cast<const char*>(titleImageData.data()), 
                titleImageData.size());
            // debugPrint("Saved title image for %s to %s\n", game.title.c_str(), imageFilePath);
        } else {
            debugPrint("Failed opening %s for writing!\n", imageFilePath);
        }
    }
}

bool MountHome()
{
    // Slightly modified nxdk\automount_d.c
    char targetPath[MAX_PATH];
    nxGetCurrentXbeNtPath(targetPath);

    // Cut off the XBE file name by inserting a null-terminator
    char *filenameStr;
    filenameStr = strrchr(targetPath, '\\');
    assert(filenameStr != NULL);
    *(filenameStr + 1) = '\0';

    return nxMountDrive('Q', targetPath);
}

std::vector<std::string> LoadPathsFromConfig(const std::string& configPath) {
    std::vector<std::string> paths;
    std::ifstream file(configPath);
    
    if (!file.is_open()) {
        debugPrint("Failed to open config file: %s\n", configPath.c_str());
        return paths;
    }

    std::string line;
    std::string currentSection;
    int maxItems = 8; // Default value
    
    while (std::getline(file, line)) {
        // Trim whitespace
        line = Trim(line);
        
        if (line.empty() || line[0] == ';') continue; // Skip empty lines and comments
        
        // Check for section
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }
        
        // Only process lines in [LauncherMenu] section
        if (currentSection != "LauncherMenu") continue;
        
        // Split line into key and value
        size_t equalPos = line.find('=');
        if (equalPos == std::string::npos) continue;
        
        std::string key = Trim(line.substr(0, equalPos));
        std::string value = Trim(line.substr(equalPos + 1));
        
        if (key == "MaxLauncherMenuItems") {
            maxItems = std::stoi(value);
        } else if (strncmp(key.c_str(), "Path", strlen("Path")) == 0 && value.length() > 0) {
            // Extract path index and verify it's within maxItems
            int pathIndex = std::stoi(key.substr(4));
            if (pathIndex >= 0 && pathIndex < maxItems) {
                auto pathList = SplitString(value, ';');
                for (const auto& path : pathList) {
                    // Check for duplicates (case-insensitive)
                    bool isDuplicate = false;
                    for (const auto& existingPath : paths) {
                        if (_stricmp(path.c_str(), existingPath.c_str()) == 0) {
                            isDuplicate = true;
                        }
                    }

                    if (!isDuplicate) {
                        paths.push_back(path);
                    }
                }
            }
        }
    }
    
    return paths;
}

int main(void) {
    std::vector<char> vecDrives;
    std::vector<GameInfo> titles;
    bool success = false;

    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);

    debugPrint(".: Iconator 4 UIX-Lite :.\n\n");

    if (!MountHome()) {
        debugPrint("Failed to mount home drive Q:\n");
        Sleep(5000);
        return 1;
    }

    for (const auto& mountPoint : DRIVE_MAPPINGS) {
        success = nxMountDrive(mountPoint.driveLetter, mountPoint.devicePath);
        if (!success && mountPoint.failFatal) {
            debugPrint("Failed to mount %c from drive '%s'!\n", mountPoint.driveLetter, mountPoint.devicePath);
            Sleep(5000);
            return 2;
        } else if (success) {
            vecDrives.push_back(mountPoint.driveLetter);
        }
    }

    auto paths = LoadPathsFromConfig("C:\\UIX Configs\\config.ini");
    if (paths.empty()) {
        debugPrint("Couldn't enumerate paths from config.ini!\n");
        Sleep(5000);
        return 3;
    }

    debugPrint("Enumerated paths:\n");
    for (auto& path : paths) {
        debugPrint("%s\n", path.c_str());
    }

    debugPrint("Enumerated drives:\n");
    for (auto& driveLetter : vecDrives) {
        debugPrint("%c:\\\n", driveLetter);
    }

    // Search for games in each drive
    for (auto& driveLetter : vecDrives) {
        debugPrint("Searching on drive %c...\n", driveLetter);
        for (auto& path : paths) {
            std::string scanPath = std::string(1, driveLetter) + ":\\" + path;
            // debugPrint("Searching in %s\n", scanPath.c_str());
            FindDefaultXBE(scanPath, titles);
        }
    }

    debugPrint("\nFound %zu titles\n", titles.size());

    /*
    for (const auto& title : titles) {
        debugPrint("Path: %s\nTitle: %s\nTitle ID: %08X\n", 
                  title.xbe_path.c_str(),
                  title.title.c_str(),
                  title.title_id);
    }
    */

    debugPrint("Copying title images...\n");
    CopyTitleImages(titles);
    debugPrint("Saving title metadata...\n");
    SaveTitleMeta(titles);
    debugPrint("Saving Icons.ini ...\n");
    SaveIconsIni(titles, "C:\\UIX Configs\\Icons.ini");
    debugPrint("Saving TitleNames.ini ...\n");
    SaveTitleNamesIni(titles, "C:\\UIX Configs\\TitleNames.ini");

    debugPrint("Exiting in 10 seconds...");
    Sleep(10000);

    return 0;
}
