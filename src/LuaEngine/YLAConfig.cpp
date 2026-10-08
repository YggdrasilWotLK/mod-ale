#include "YLAConfig.h"
#include "YLAUtility.h"
#include <sstream>
#include <algorithm>
#include <cctype>

YLAConfig& YLAConfig::GetInstance()
{
    static YLAConfig instance;
    return instance;
}

YLAConfig::YLAConfig() : ConfigValueCache<ALEConfigValues>(ALEConfigValues::CONFIG_VALUE_COUNT)
{
}

void YLAConfig::Initialize(bool reload)
{
    ConfigValueCache<ALEConfigValues>::Initialize(reload);
    TokenizeAllowedMaps();
}

void YLAConfig::BuildConfigCache()
{
    SetConfigValue<bool>(ALEConfigValues::ENABLED,                    "YLA.Enabled",            "false");
    SetConfigValue<bool>(ALEConfigValues::TRACEBACK_ENABLED,          "YLA.TraceBack",          "false");
    SetConfigValue<bool>(ALEConfigValues::AUTORELOAD_ENABLED,         "YLA.AutoReload",         "false");
    SetConfigValue<bool>(ALEConfigValues::BYTECODE_CACHE_ENABLED,     "YLA.BytecodeCache",      "false");
    SetConfigValue<bool>(ALEConfigValues::COMPATIBILITY_MODE,         "YLA.CompatibilityMode",  "true");

    SetConfigValue<std::string>(ALEConfigValues::SCRIPT_PATH,         "YLA.ScriptPath",         "lua_scripts");
    SetConfigValue<std::string>(ALEConfigValues::REQUIRE_PATH,        "YLA.RequirePaths",       "");
    SetConfigValue<std::string>(ALEConfigValues::REQUIRE_CPATH,       "YLA.RequireCPaths",      "");
    SetConfigValue<std::string>(ALEConfigValues::ONLY_ON_MAPS,        "YLA.OnlyOnMaps",         "");

    SetConfigValue<uint32>(ALEConfigValues::AUTORELOAD_INTERVAL,      "YLA.AutoReloadInterval", 1);
}

bool YLAConfig::ShouldMapLoadALE(uint32 mapId) const
{
    if (m_allowedMaps.empty())
        return true;
    return m_allowedMaps.find(mapId) != m_allowedMaps.end();
}

bool YLAConfig::ShouldMapLoadALEByFolderName(const std::string& folderName, uint32 mapId) const
{
    std::string digits;
    for (char c : folderName)
    {
        if (std::isdigit(static_cast<unsigned char>(c)))
            digits += c;
        else
            break;
    }

    if (digits.empty())
        return true;

    try
    {
        uint32 folderMapId = std::stoul(digits);
        return folderMapId == mapId;
    }
    catch (std::exception&)
    {
        return true;
    }
}

void YLAConfig::TokenizeAllowedMaps()
{
    m_allowedMaps.clear();

    std::istringstream maps(static_cast<std::string>(GetConfigValue(ALEConfigValues::ONLY_ON_MAPS)));
    std::string mapIdStr;
    while (std::getline(maps, mapIdStr, ','))
    {
        mapIdStr.erase(std::remove_if(mapIdStr.begin(), mapIdStr.end(), [](char c) {
            return std::isspace(static_cast<unsigned char>(c));
        }), mapIdStr.end());

        if (mapIdStr.empty())
            continue;

        try
        {
            uint32 mapId = std::stoul(mapIdStr);
            m_allowedMaps.emplace(mapId);
        }
        catch (std::exception&)
        {
            YLA_LOG_ERROR("[YLAConfig]: Invalid map ID in YLA.OnlyOnMaps: '{}'", mapIdStr);
        }
    }
}
