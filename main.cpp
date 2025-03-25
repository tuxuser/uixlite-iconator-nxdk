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
    std::vector<std::string> vecDrives;
    std::vector<GameInfo> games;

    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);

    // Mount E:
    BOOL ret = nxMountDrive('E', "\\Device\\Harddisk0\\Partition1\\");
    if (!ret) {
        debugPrint("Failed to mount E: drive!\n");
        Sleep(5000);
        return 1;
    }
    vecDrives.push_back("E");

    ret = nxMountDrive('F', "\\Device\\Harddisk0\\Partition6\\");
    if (!ret) {
        debugPrint("Failed to mount F: drive!\n");
    } else {
        vecDrives.push_back("F");
    }

    ret = nxMountDrive('G', "\\Device\\Harddisk0\\Partition7\\");
    if (!ret) {
        debugPrint("Failed to mount G: drive!\n");
    } else {
        vecDrives.push_back("G");
    }

    debugPrint("Enumerated drives:\n");
    for (auto& driveLetter : vecDrives) {
        debugPrint("%s:\\\n", driveLetter.c_str());
    }

    // Search for games in each drive
    for (auto& driveLetter : vecDrives) {
        std::string games_path = driveLetter + ":\\Games";
        FindDefaultXBE(games_path, games);
    }

    debugPrint("\nFound %zu games:\n", games.size());
    for (const auto& game : games) {
        debugPrint("Path: %s\nTitle: %s\nTitle ID: %s\n", 
                  game.xbe_path.c_str(),
                  game.title.c_str(),
                  game.title_id.c_str());
        debugPrint("Title image: %zu bytes\n\n",
                  game.title_image.size());
    }

    while (1) {
        Sleep(2000);
    }

    return 0;
}
