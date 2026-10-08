/*
* Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
* This program is free software licensed under GPL version 3
* Please see the included DOCS/LICENSE.md for more information
*/

#include "ALEInstanceAI.h"
#include <sstream>

void ALEInstanceAI::Initialize()
{
    sALE->OnInitialize(this);
}

void ALEInstanceAI::Load(const char* data)
{
    if (data)
    {
        lastSaveData = data;
        std::istringstream iss(lastSaveData);
        std::string token;
        std::string section;
        while (iss >> token)
        {
            if (token == "d32" || token == "d64")
            {
                section = token;
                continue;
            }
            uint32 key = std::stoul(token);
            if (section == "d32")
            {
                uint32 value;
                iss >> value;
                dataStore[key] = value;
            }
            else if (section == "d64")
            {
                uint64 value;
                iss >> value;
                dataStore64[key] = value;
            }
        }
    }
    sALE->OnLoad(this);
}

const char* ALEInstanceAI::Save() const
{
    std::ostringstream oss;
    if (!dataStore.empty())
    {
        oss << "d32 ";
        for (auto const& [key, value] : dataStore)
            oss << key << " " << value << " ";
    }
    if (!dataStore64.empty())
    {
        oss << "d64 ";
        for (auto const& [key, value] : dataStore64)
            oss << key << " " << value << " ";
    }
    lastSaveData = oss.str();
    return lastSaveData.c_str();
}

uint32 ALEInstanceAI::GetData(uint32 key) const
{
    auto it = dataStore.find(key);
    return it != dataStore.end() ? it->second : 0;
}

void ALEInstanceAI::SetData(uint32 key, uint32 value)
{
    dataStore[key] = value;
}

uint64 ALEInstanceAI::GetData64(uint32 key) const
{
    auto it = dataStore64.find(key);
    return it != dataStore64.end() ? it->second : 0;
}

void ALEInstanceAI::SetData64(uint32 key, uint64 value)
{
    dataStore64[key] = value;
}