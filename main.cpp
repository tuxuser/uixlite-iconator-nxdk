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
                    parser.ExtractTitle(game.title) &&
                    parser.ExtractTitleImage(game.title_image)) {
                    games.push_back(game);
                    debugPrint("Found game: %s (Title ID: %s)\n", 
                             game.title.c_str(), 
                             game.title_id.c_str());
                    debugPrint("Title image: %zu bytes\n",
                             game.title_image.size());
                }
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
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
        debugPrint("Path: %s\nTitle: %s\nTitle ID: %s\n", 
                  title.xbe_path.c_str(),
                  title.title.c_str(),
                  title.title_id.c_str());
        debugPrint("Title image: %zu bytes\n\n",
                  title.title_image.size());
    }

    while (1) {
        Sleep(2000);
    }

    return 0;
}
