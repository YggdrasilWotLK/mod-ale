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
        uint32 key, value;
        while (iss >> key >> value)
            dataStore[key] = value;
    }
    sALE->OnLoad(this);
}

const char* ALEInstanceAI::Save() const
{
    std::ostringstream oss;
    for (auto const& [key, value] : dataStore)
        oss << key << " " << value << " ";
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