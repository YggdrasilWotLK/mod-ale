/*
* Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
* This program is free software licensed under GPL version 3
* Please see the included DOCS/LICENSE.md for more information
*/

#include "YLAFileWatcher.h"
#include "LuaEngine.h"
#include "YLAUtility.h"
#include <boost/filesystem.hpp>

YLAFileWatcher::YLAFileWatcher() : running(false), checkInterval(1)
{
}

YLAFileWatcher::~YLAFileWatcher()
{
    StopWatching();
}

void YLAFileWatcher::StartWatching(const std::string& scriptPath, uint32 intervalSeconds)
{
    if (running.load())
    {
        YLA_LOG_DEBUG("[YLAFileWatcher]: Already watching files");
        return;
    }

    if (scriptPath.empty())
    {
        YLA_LOG_ERROR("[YLAFileWatcher]: Cannot start watching - script path is empty");
        return;
    }

    watchPath = scriptPath;
    checkInterval = intervalSeconds;
    running.store(true);

    ScanDirectory(watchPath);

    watcherThread = std::thread(&YLAFileWatcher::WatchLoop, this);
    
    YLA_LOG_INFO("[YLAFileWatcher]: Started watching '{}' (interval: {}s)", watchPath, checkInterval);
}

void YLAFileWatcher::StopWatching()
{
    if (!running.load())
        return;

    running.store(false);

    if (watcherThread.joinable())
        watcherThread.join();

    fileTimestamps.clear();
    
    YLA_LOG_INFO("[YLAFileWatcher]: Stopped watching files");
}

void YLAFileWatcher::WatchLoop()
{
    while (running.load())
    {
        try
        {
            CheckForChanges();
        }
        catch (const std::exception& e)
        {
            YLA_LOG_ERROR("[YLAFileWatcher]: Error during file watching: {}", e.what());
        }

        std::this_thread::sleep_for(std::chrono::seconds(checkInterval));
    }
}

bool YLAFileWatcher::IsWatchedFileType(const std::string& filename) {
    return (filename.length() >= 4 && filename.substr(filename.length() - 4) == ".lua") ||
        (filename.length() >= 4 && filename.substr(filename.length() - 4) == ".ext") ||
        (filename.length() >= 5 && filename.substr(filename.length() - 5) == ".moon");
}

void YLAFileWatcher::ScanDirectory(const std::string& path)
{
    try
    {
        boost::filesystem::path dir(path);
        
        if (!boost::filesystem::exists(dir) || !boost::filesystem::is_directory(dir))
            return;

        boost::filesystem::directory_iterator end_iter;
        
        for (boost::filesystem::directory_iterator dir_iter(dir); dir_iter != end_iter; ++dir_iter)
        {
            std::string fullpath = dir_iter->path().generic_string();
            
            if (boost::filesystem::is_directory(dir_iter->status()))
            {
                ScanDirectory(fullpath);
            }
            else if (boost::filesystem::is_regular_file(dir_iter->status()))
            {
                std::string filename = dir_iter->path().filename().generic_string();
                
                if (IsWatchedFileType(filename))
                {
                    fileTimestamps[fullpath] = boost::filesystem::last_write_time(dir_iter->path());
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        YLA_LOG_ERROR("[YLAFileWatcher]: Error scanning directory '{}': {}", path, e.what());
    }
}

void YLAFileWatcher::CheckForChanges()
{
    bool hasChanges = false;
    
    try
    {
        boost::filesystem::path dir(watchPath);
        
        if (!boost::filesystem::exists(dir) || !boost::filesystem::is_directory(dir))
            return;

        boost::filesystem::directory_iterator end_iter;
        
        for (boost::filesystem::directory_iterator dir_iter(dir); dir_iter != end_iter; ++dir_iter)
        {
            if (ShouldReloadFile(dir_iter->path().generic_string()))
                hasChanges = true;
        }
        
        for (auto it = fileTimestamps.begin(); it != fileTimestamps.end();)
        {
            if (!boost::filesystem::exists(it->first))
            {
                YLA_LOG_DEBUG("[YLAFileWatcher]: File deleted: {}", it->first);
                it = fileTimestamps.erase(it);
                hasChanges = true;
            }
            else
            {
                ++it;
            }
        }
    }
    catch (const std::exception& e)
    {
        YLA_LOG_ERROR("[YLAFileWatcher]: Error checking for changes: {}", e.what());
        return;
    }

    if (hasChanges)
    {
        YLA_LOG_INFO("[YLAFileWatcher]: Lua script changes detected - triggering reload");
        ALE::ReloadALE();
        
        ScanDirectory(watchPath);
    }
}

bool YLAFileWatcher::ShouldReloadFile(const std::string& filepath)
{
    try
    {
        boost::filesystem::path file(filepath);
        
        if (boost::filesystem::is_directory(file))
        {
            boost::filesystem::directory_iterator end_iter;
            
            for (boost::filesystem::directory_iterator dir_iter(file); dir_iter != end_iter; ++dir_iter)
            {
                if (ShouldReloadFile(dir_iter->path().generic_string()))
                    return true;
            }
            return false;
        }
        
        if (!boost::filesystem::is_regular_file(file))
            return false;
            
        std::string filename = file.filename().generic_string();

        if (!IsWatchedFileType(filename)) return false;
            
        auto currentTime = boost::filesystem::last_write_time(file);
        auto it = fileTimestamps.find(filepath);
        
        if (it == fileTimestamps.end())
        {
            YLA_LOG_DEBUG("[YLAFileWatcher]: New file detected: {}", filepath);
            fileTimestamps[filepath] = currentTime;
            return true;
        }
        
        if (it->second != currentTime)
        {
            YLA_LOG_DEBUG("[YLAFileWatcher]: File modified: {}", filepath);
            it->second = currentTime;
            return true;
        }
    }
    catch (const std::exception& e)
    {
        YLA_LOG_ERROR("[YLAFileWatcher]: Error checking file '{}': {}", filepath, e.what());
    }
    
    return false;
}
