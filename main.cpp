#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <nxdk/mount.h>
#include <hal/debug.h>
#include <hal/video.h>
#include <hal/xbox.h>
#include "xbe_parser.h"
#include <string>
#include <vector>
#include <fstream>

struct DriveMapping {
    const char* devicePath;
    const char driveLetter;
    bool fatal;
};

// Map Xbox device paths to drive letters
static const DriveMapping DRIVE_MAPPINGS[] = {
    // HDD0
    {"\\Device\\Harddisk0\\Partition1", 'E', true},
    {"\\Device\\Harddisk0\\Partition6", 'F', false},
    {"\\Device\\Harddisk0\\Partition7", 'G', false},
    // HDD1
    {"\\Device\\Harddisk1\\Partition1", 'H', false},
    {"\\Device\\Harddisk1\\Partition6", 'I', false},
    {"\\Device\\Harddisk1\\Partition7", 'J', false},
};

static const std::string PATHS[] = {
    "Apps",
    "Dashboards"
    "Games",
    "Emulators",
    "Homebrew"
};

void FindDefaultXBE(const std::string& path, std::vector<GameInfo>& games) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;
    std::string search_path = path + "\\*\\*.*";
    
    hFind = FindFirstFile(search_path.c_str(), &findFileData);
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
                    debugPrint("Found game: %s (Title ID: %08X)\n", 
                             game.title.c_str(), 
                             game.title_id);
                    debugPrint("Title image: %zu bytes\n",
                             game.title_image.size());
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
    std::ofstream f(path);
    if (!f) return;
    
    f << "[default]\n";
    for (const auto& game : games) {
        std::string dirName = GetDirectoryName(game.xbe_path);
        if (!dirName.empty()) {
            f << dirName << "=" << std::hex << std::uppercase 
              << std::setfill('0') << std::setw(8) 
              << game.title_id << "\n";
        }
    }
}

void SaveTitleNamesIni(const std::vector<GameInfo>& games, std::string path) {
    std::ofstream f(path);
    if (!f) return;
    
    f << "[default]\n";
    for (const auto& game : games) {
        std::string dirName = GetDirectoryName(game.xbe_path);
        if (!dirName.empty()) {
            f << dirName << "=" << game.title << "\n";
        }
    }
}

void SaveTitleMeta(const std::vector<GameInfo>& games) {
    for (const auto& game : games) {
        if (game.title.empty()) continue;
        
        // Create directory path in format "E:\UDATA\XXXXXXXX\"
        char dirPath[MAX_PATH];
        snprintf(dirPath, sizeof(dirPath), "E:\\UDATA\\%08X", game.title_id);
        
        // Create the directory
        CreateDirectory(dirPath, NULL);
        
        char metaFilePath[MAX_PATH];
        snprintf(metaFilePath, sizeof(metaFilePath), "%s\\TitleMeta.xbx", dirPath);

        // Check if files already exist
        std::ifstream fMetaExists(metaFilePath);
        if (fMetaExists && fMetaExists.good()) {
            if (fMetaExists)
                fMetaExists.close();

            debugPrint("Title metadata already exists for %s, skipping...\n", game.title.c_str());
            continue;
        }

        // Write the title metadatadata
        std::ofstream f(metaFilePath);
        if (f) {
            f << "TitleName=" << game.title << "\n";
            debugPrint("Saved title meta for %s to %s\n", 
                    game.title.c_str(), metaFilePath);
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

        // Check if files already exist
        std::ifstream fImageExists(imageFilePath);
        if (fImageExists && fImageExists.good()) {
            if (fImageExists)
                fImageExists.close();

            debugPrint("Title image/icon already exist for %s, skipping...\n", game.title.c_str());
            continue;
        }

        // Write the title image data
        std::ofstream f(imageFilePath, std::ios::binary);
        if (f) {
            f.write(reinterpret_cast<const char*>(game.title_image.data()), 
                game.title_image.size());
            debugPrint("Saved title image for %s to %s\n", 
                    game.title.c_str(), imageFilePath);
        }
    }
}

int main(void)
{
    std::vector<char> vecDrives;
    std::vector<GameInfo> titles;
    bool success = false;

    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);

    for (const auto& mountPoint : DRIVE_MAPPINGS) {
        success = nxMountDrive(mountPoint.driveLetter, mountPoint.devicePath);
        if (!success && mountPoint.fatal) {
            debugPrint("Failed to mount %c from drive '%s'!\n", mountPoint.driveLetter, mountPoint.devicePath);
            Sleep(5000);
            return 1;
        } else if (success) {
            vecDrives.push_back(mountPoint.driveLetter);
        }
    }

    debugPrint("Enumerated drives:\n");
    for (auto& driveLetter : vecDrives) {
        debugPrint("%c:\\\n", driveLetter);
    }

    // Search for games in each drive
    for (auto& driveLetter : vecDrives) {
        for (auto& path : PATHS) {
            std::string scanPath = std::string(1, driveLetter) + ":\\" + path;
            FindDefaultXBE(scanPath, titles);
        }
    }

    debugPrint("\nFound %zu titles:\n", titles.size());
    for (const auto& title : titles) {
        debugPrint("Path: %s\nTitle: %s\nTitle ID: %08X\n", 
                  title.xbe_path.c_str(),
                  title.title.c_str(),
                  title.title_id);
        debugPrint("Title image: %zu bytes\n\n",
                  title.title_image.size());
    }

    debugPrint("Copying title images...\n");
    CopyTitleImages(titles);
    debugPrint("Saving title metadata...\n");
    SaveTitleMeta(titles);
    debugPrint("Saving Icons.ini ...\n");
    SaveIconsIni(titles, "E:\\Icons.ini");
    debugPrint("Saving TitleNames.ini ...\n");
    SaveTitleNamesIni(titles, "E:\\TitleNames.ini");

    while (1) {
        Sleep(2000);
    }

    return 0;
}
