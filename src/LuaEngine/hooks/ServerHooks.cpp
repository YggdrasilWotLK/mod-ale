/*
 * Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
 * This program is free software licensed under GPL version 3
 * Please see the included DOCS/LICENSE.md for more information
 */

#include "Hooks.h"
#include "HookHelpers.h"
#include "LuaEngine.h"
#include "BindingMap.h"
#include "ALEEventMgr.h"
#include "ALEIncludes.h"
#include "ALETemplate.h"

using namespace Hooks;

#define START_HOOK_WORLD(EVENT) \
    if (!ALEConfig::GetInstance().IsALEEnabled())\
        return;\
    auto key = EventKey<ServerEvents>(EVENT);\
    if (!ServerEventBindings->HasBindingsFor(key))\
        return;\
    LOCK_ALE

#define START_HOOK_WORLD_WITH_RETVAL(EVENT, RETVAL) \
    if (!ALEConfig::GetInstance().IsALEEnabled())\
        return RETVAL;\
    auto key = EventKey<ServerEvents>(EVENT);\
    if (!ServerEventBindings->HasBindingsFor(key))\
        return RETVAL;\
    LOCK_ALE

#define START_HOOK_MAP(EVENT) \
    if (!ALEConfig::GetInstance().IsALEEnabled())\
        return;\
    auto key = EventKey<ServerEvents>(EVENT);\
    if (!ServerEventBindings->HasBindingsFor(key))\
        return;\
    LOCK_ALE_STATE

#define START_HOOK_MAP_WITH_RETVAL(EVENT, RETVAL) \
    if (!ALEConfig::GetInstance().IsALEEnabled())\
        return RETVAL;\
    auto key = EventKey<ServerEvents>(EVENT);\
    if (!ServerEventBindings->HasBindingsFor(key))\
        return RETVAL;\
    LOCK_ALE_STATE

// WORLD
bool ALE::OnAddonMessage(Player* sender, uint32 type, std::string& msg, Player* receiver, Guild* guild, Group* group, Channel* channel)
{
    START_HOOK_WORLD_WITH_RETVAL(ADDON_EVENT_ON_MESSAGE, true);
    Push(sender);
    Push(type);

    auto delimeter_position = msg.find('\t');
    if (delimeter_position == std::string::npos)
    {
        Push(msg);
        Push();
    }
    else
    {
        std::string prefix = msg.substr(0, delimeter_position);
        std::string content = msg.substr(delimeter_position + 1, std::string::npos);
        Push(prefix);
        Push(content);
    }

    if (receiver)
        Push(receiver);
    else if (guild)
        Push(guild);
    else if (group)
        Push(group);
    else if (channel)
        Push(channel->GetChannelId());
    else
        Push();

    return CallAllFunctionsBool(ServerEventBindings, key, true);
}

void ALE::OnTimedEvent(int funcRef, uint32 delay, uint32 calls, WorldObject* obj)
{
    LOCK_ALE_STATE;
    ASSERT(!event_level);

    lua_rawgeti(L, LUA_REGISTRYINDEX, funcRef);

    Push(L, funcRef);
    Push(L, delay);
    Push(L, calls);
    Push(L, obj);

    ExecuteCall(4, 0);

    ASSERT(!event_level);
    InvalidateObjects();
}

// WORLD
void ALE::OnGameEventStart(uint32 eventid)
{
    START_HOOK_WORLD(GAME_EVENT_START);
    Push(eventid);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void ALE::OnGameEventStop(uint32 eventid)
{
    START_HOOK_WORLD(GAME_EVENT_STOP);
    Push(eventid);
    CallAllFunctions(ServerEventBindings, key);
}

void ALE::OnLuaStateClose()
{
    START_HOOK_WORLD(ALE_EVENT_ON_LUA_STATE_CLOSE);
    CallAllFunctions(ServerEventBindings, key);
}

void ALE::OnLuaStateOpen()
{
    START_HOOK_WORLD(ALE_EVENT_ON_LUA_STATE_OPEN);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
bool ALE::OnAreaTrigger(Player* pPlayer, AreaTriggerEntry const* pTrigger)
{
    START_HOOK_MAP_WITH_RETVAL(TRIGGER_EVENT_ON_TRIGGER, false);
    Push(pPlayer);
    Push(pTrigger->entry);
    return CallAllFunctionsBool(ServerEventBindings, key);
}

// WORLD
void ALE::OnChange(Weather* /*weather*/, uint32 zone, WeatherState state, float grade)
{
    START_HOOK_WORLD(WEATHER_EVENT_ON_CHANGE);
    Push(zone);
    Push(state);
    Push(grade);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void ALE::OnAdd(AuctionHouseObject* /*ah*/, AuctionEntry* entry)
{
    Player* owner = eObjectAccessor()FindPlayer(entry->owner);
    Item* item = eAuctionMgr->GetAItem(entry->item_guid);
    uint32 expiretime = entry->expire_time;

    if (!owner || !item)
        return;

    START_HOOK_WORLD(AUCTION_EVENT_ON_ADD);
    Push(entry->Id);
    Push(owner);
    Push(item);
    Push(expiretime);
    Push(entry->buyout);
    Push(entry->startbid);
    Push(entry->bid);
    Push(entry->bidder);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void ALE::OnRemove(AuctionHouseObject* /*ah*/, AuctionEntry* entry)
{
    Player* owner = eObjectAccessor()FindPlayer(entry->owner);
    Item* item = eAuctionMgr->GetAItem(entry->item_guid);
    uint32 expiretime = entry->expire_time;

    if (!owner || !item)
        return;

    START_HOOK_WORLD(AUCTION_EVENT_ON_REMOVE);
    Push(entry->Id);
    Push(owner);
    Push(item);
    Push(expiretime);
    Push(entry->buyout);
    Push(entry->startbid);
    Push(entry->bid);
    Push(entry->bidder);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void ALE::OnSuccessful(AuctionHouseObject* /*ah*/, AuctionEntry* entry)
{
    Player* owner = eObjectAccessor()FindPlayer(entry->owner);
    Item* item = eAuctionMgr->GetAItem(entry->item_guid);
    uint32 expiretime = entry->expire_time;

    if (!owner || !item)
        return;

    START_HOOK_WORLD(AUCTION_EVENT_ON_SUCCESSFUL);
    Push(entry->Id);
    Push(owner);
    Push(item);
    Push(expiretime);
    Push(entry->buyout);
    Push(entry->startbid);
    Push(entry->bid);
    Push(entry->bidder);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void ALE::OnExpire(AuctionHouseObject* /*ah*/, AuctionEntry* entry)
{
    Player* owner = eObjectAccessor()FindPlayer(entry->owner);
    Item* item = eAuctionMgr->GetAItem(entry->item_guid);
    uint32 expiretime = entry->expire_time;

    if (!owner || !item)
        return;

    START_HOOK_WORLD(AUCTION_EVENT_ON_EXPIRE);
    Push(entry->Id);
    Push(owner);
    Push(item);
    Push(expiretime);
    Push(entry->buyout);
    Push(entry->startbid);
    Push(entry->bid);
    Push(entry->bidder);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void ALE::OnOpenStateChange(bool open)
{
    START_HOOK_WORLD(WORLD_EVENT_ON_OPEN_STATE_CHANGE);
    Push(open);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void ALE::OnConfigLoad(bool reload, bool isBefore)
{
    START_HOOK_WORLD(WORLD_EVENT_ON_CONFIG_LOAD);
    Push(reload);
    Push(isBefore);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void ALE::OnShutdownInitiate(ShutdownExitCode code, ShutdownMask mask)
{
    START_HOOK_WORLD(WORLD_EVENT_ON_SHUTDOWN_INIT);
    Push(code);
    Push(mask);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void ALE::OnShutdownCancel()
{
    START_HOOK_WORLD(WORLD_EVENT_ON_SHUTDOWN_CANCEL);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void ALE::OnWorldUpdate(uint32 diff)
{
    {
        LOCK_ALE;
        if (ShouldReload())
            _ReloadALE();
    }

    eventMgr->globalProcessor->Update(diff);
    httpManager.HandleHttpResponses();
    queryProcessor.ProcessReadyCallbacks();

    START_HOOK_WORLD(WORLD_EVENT_ON_UPDATE);
    Push(diff);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void ALE::OnStartup()
{
    START_HOOK_WORLD(WORLD_EVENT_ON_STARTUP);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void ALE::OnShutdown()
{
    START_HOOK_WORLD(WORLD_EVENT_ON_SHUTDOWN);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
void ALE::OnCreate(Map* map)
{
    START_HOOK_MAP(MAP_EVENT_ON_CREATE);
    Push(map);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
void ALE::OnDestroy(Map* map)
{
    START_HOOK_MAP(MAP_EVENT_ON_DESTROY);
    Push(map);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
void ALE::OnPlayerEnter(Map* map, Player* player)
{
    START_HOOK_MAP(MAP_EVENT_ON_PLAYER_ENTER);
    Push(map);
    Push(player);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
void ALE::OnPlayerLeave(Map* map, Player* player)
{
    START_HOOK_MAP(MAP_EVENT_ON_PLAYER_LEAVE);
    Push(map);
    Push(player);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
void ALE::OnUpdate(Map* map, uint32 diff)
{
    START_HOOK_MAP(MAP_EVENT_ON_UPDATE);
    Push(map);
    Push(diff);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
void ALE::OnRemove(GameObject* gameobject)
{
    START_HOOK_MAP(WORLD_EVENT_ON_DELETE_GAMEOBJECT);
    Push(gameobject);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
void ALE::OnRemove(Creature* creature)
{
    START_HOOK_MAP(WORLD_EVENT_ON_DELETE_CREATURE);
    Push(creature);
    CallAllFunctions(ServerEventBindings, key);
}
