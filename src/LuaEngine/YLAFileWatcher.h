/*
* Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
* This program is free software licensed under GPL version 3
* Please see the included DOCS/LICENSE.md for more information
*/

#ifndef YLA_FILE_WATCHER_H
#define YLA_FILE_WATCHER_H

#include <thread>
#include <atomic>
#include <map>
#include <string>
#include <chrono>
#include <boost/filesystem.hpp>
#include "Common.h"

class YLAFileWatcher
{
public:
    YLAFileWatcher();
    ~YLAFileWatcher();

    void StartWatching(const std::string& scriptPath, uint32 intervalSeconds = 1);
    void StopWatching();
    bool IsWatching() const { return running.load(); }

private:
    void WatchLoop();
    void ScanDirectory(const std::string& path);
    void CheckForChanges();
    bool ShouldReloadFile(const std::string& filepath);
    bool IsWatchedFileType(const std::string& filename);

    std::thread watcherThread;
    std::atomic<bool> running;
    std::string watchPath;
    uint32 checkInterval;
    
    std::map<std::string, std::time_t> fileTimestamps;
};

#endif
