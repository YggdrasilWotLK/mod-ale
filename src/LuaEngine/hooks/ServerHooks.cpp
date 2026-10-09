/*
 * Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
 * This program is free software licensed under GPL version 3
 * Please see the included DOCS/LICENSE.md for more information
 */

#include "Hooks.h"
#include "HookHelpers.h"
#include "LuaEngine.h"
#include "BindingMap.h"
#include "YLAEventMgr.h"
#include "YLAIncludes.h"
#include "YLATemplate.h"
#include "YlaDefer.h"

using namespace Hooks;

#define START_HOOK_WORLD(EVENT) \
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return;\
    auto key = EventKey<ServerEvents>(EVENT);\
    if (!ServerEventBindings->HasBindingsFor(key))\
        return;\
    LOCK_ALE

#define START_HOOK_WORLD_WITH_RETVAL(EVENT, RETVAL) \
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return RETVAL;\
    auto key = EventKey<ServerEvents>(EVENT);\
    if (!ServerEventBindings->HasBindingsFor(key))\
        return RETVAL;\
    LOCK_ALE

#define START_HOOK_MAP(EVENT) \
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return;\
    auto key = EventKey<ServerEvents>(EVENT);\
    if (!ServerEventBindings->HasBindingsFor(key))\
        return;\
    LOCK_YLA_STATE

#define START_HOOK_MAP_WITH_RETVAL(EVENT, RETVAL) \
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return RETVAL;\
    auto key = EventKey<ServerEvents>(EVENT);\
    if (!ServerEventBindings->HasBindingsFor(key))\
        return RETVAL;\
    LOCK_YLA_STATE

// WORLD
bool YLA::OnAddonMessage(Player* sender, uint32 type, std::string& msg, Player* receiver, Guild* guild, Group* group, Channel* channel)
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

bool YLA::OnTimedEvent(int funcRef, uint32 delay, uint32 calls, WorldObject* obj)
{
    LOCK_YLA_STATE;
    // A nested timed event would push its [function, args] on top of the
    // outer call's stack and invalidate the outer CallOneFunction indices.
    // Refuse (the event loop re-arms it for a later tick) instead of
    // mis-popping. Never assert here: asserts are fatal in this build.
    // event_level is logged to distinguish genuine nesting from a leak.
    if (event_level)
    {
        YLA_LOG_ERROR("[YLA]: Skipped nested timed event (funcRef {} delay {} calls {}) during active Lua execution (event_level {}).", funcRef, delay, calls, event_level);
        return false;
    }

    int top0 = lua_gettop(L);

    lua_rawgeti(L, LUA_REGISTRYINDEX, funcRef);
    if (!lua_isfunction(L, -1))
    {
        YLA_LOG_ERROR("[YLA]: Skipped timed event (funcRef {}): registry value is {}, not a function.", funcRef, luaL_typename(L, -1));
        lua_settop(L, top0);
        InvalidateObjects();
        return false;
    }

    Push(L, funcRef);
    Push(L, delay);
    Push(L, calls);
    Push(L, obj);

    ExecuteCall(4, 0);

    // The timed call must leave the stack exactly as found: any surplus or
    // deficit here would misalign every later op on L.
    if (lua_gettop(L) != top0)
    {
        YLA_LOG_ERROR("[YLA]: OnTimedEvent stack mismatch (funcRef {}): top-in {} top-out {}. Restoring.", funcRef, top0, lua_gettop(L));
        lua_settop(L, top0);
    }

    InvalidateObjects();
    return true;
}

// WORLD
void YLA::OnGameEventStart(uint32 eventid)
{
    START_HOOK_WORLD(GAME_EVENT_START);
    Push(eventid);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void YLA::OnGameEventStop(uint32 eventid)
{
    START_HOOK_WORLD(GAME_EVENT_STOP);
    Push(eventid);
    CallAllFunctions(ServerEventBindings, key);
}

void YLA::OnLuaStateClose()
{
    START_HOOK_WORLD(YLA_EVENT_ON_LUA_STATE_CLOSE);
    CallAllFunctions(ServerEventBindings, key);
}

void YLA::OnLuaStateOpen()
{
    START_HOOK_WORLD(YLA_EVENT_ON_LUA_STATE_OPEN);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
bool YLA::OnAreaTrigger(Player* pPlayer, AreaTriggerEntry const* pTrigger)
{
    START_HOOK_MAP_WITH_RETVAL(TRIGGER_EVENT_ON_TRIGGER, false);
    Push(pPlayer);
    Push(pTrigger->entry);
    return CallAllFunctionsBool(ServerEventBindings, key);
}

// WORLD
void YLA::OnChange(Weather* /*weather*/, uint32 zone, WeatherState state, float grade)
{
    START_HOOK_WORLD(WEATHER_EVENT_ON_CHANGE);
    Push(zone);
    Push(state);
    Push(grade);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void YLA::OnAdd(AuctionHouseObject* /*ah*/, AuctionEntry* entry)
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
void YLA::OnRemove(AuctionHouseObject* /*ah*/, AuctionEntry* entry)
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
void YLA::OnSuccessful(AuctionHouseObject* /*ah*/, AuctionEntry* entry)
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
void YLA::OnExpire(AuctionHouseObject* /*ah*/, AuctionEntry* entry)
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
void YLA::OnOpenStateChange(bool open)
{
    START_HOOK_WORLD(WORLD_EVENT_ON_OPEN_STATE_CHANGE);
    Push(open);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void YLA::OnConfigLoad(bool reload, bool isBefore)
{
    START_HOOK_WORLD(WORLD_EVENT_ON_CONFIG_LOAD);
    Push(reload);
    Push(isBefore);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void YLA::OnShutdownInitiate(ShutdownExitCode code, ShutdownMask mask)
{
    START_HOOK_WORLD(WORLD_EVENT_ON_SHUTDOWN_INIT);
    Push(code);
    Push(mask);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void YLA::OnShutdownCancel()
{
    START_HOOK_WORLD(WORLD_EVENT_ON_SHUTDOWN_CANCEL);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void YLA::OnWorldUpdate(uint32 diff)
{
    {
        LOCK_ALE;
        if (ShouldReload())
            _ReloadALE();
    }

    // Deferred far teleports/logouts (maps idle here). Deliberately without
    // LOCK_ALE: the queue has its own mutex, and holding global across the
    // hooks below would invert the lock order (global -> state here vs
    // state-first on Lua entry paths).
    YlaDefer::Drain();

    eventMgr->globalProcessor->Update(diff);
    {
        LOCK_ALE;
        std::lock_guard<std::recursive_mutex> qguard(queryMutex);
        httpManager.HandleHttpResponses(this, true);
        queryProcessor.ProcessReadyCallbacks();
    }

    {
        // Copy owning references and release g_states before touching
        // callbacks: callbacks take global -> state, so holding g_states
        // shared across them would invert the order (g_states -> global).
        std::vector<std::shared_ptr<YLA>> states;
        {
            std::shared_lock lock(g_states_mutex);
            for (auto& [mapId, state] : g_states)
                if (state)
                    states.push_back(state);
        }
        for (auto& state : states)
        {
            LOCK_ALE;
            Guard stateGuard(state->GetStateLock());
            std::lock_guard<std::recursive_mutex> qguard(state->queryMutex);
            state->httpManager.HandleHttpResponses(state.get(), false);
            state->queryProcessor.ProcessReadyCallbacks();
        }
    }

    START_HOOK_WORLD(WORLD_EVENT_ON_UPDATE);
    Push(diff);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void YLA::OnStartup()
{
    START_HOOK_WORLD(WORLD_EVENT_ON_STARTUP);
    CallAllFunctions(ServerEventBindings, key);
}

// WORLD
void YLA::OnShutdown()
{
    START_HOOK_WORLD(WORLD_EVENT_ON_SHUTDOWN);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
void YLA::OnCreate(Map* map)
{
    START_HOOK_MAP(MAP_EVENT_ON_CREATE);
    Push(map);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
void YLA::OnDestroy(Map* map)
{
    START_HOOK_MAP(MAP_EVENT_ON_DESTROY);
    Push(map);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
void YLA::OnPlayerEnter(Map* map, Player* player)
{
    START_HOOK_MAP(MAP_EVENT_ON_PLAYER_ENTER);
    Push(map);
    Push(player);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
void YLA::OnPlayerLeave(Map* map, Player* player)
{
    START_HOOK_MAP(MAP_EVENT_ON_PLAYER_LEAVE);
    Push(map);
    Push(player);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
void YLA::OnUpdate(Map* map, uint32 diff)
{
    START_HOOK_MAP(MAP_EVENT_ON_UPDATE);
    Push(map);
    Push(diff);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
void YLA::OnRemove(GameObject* gameobject)
{
    START_HOOK_MAP(WORLD_EVENT_ON_DELETE_GAMEOBJECT);
    Push(gameobject);
    CallAllFunctions(ServerEventBindings, key);
}

// MAP
void YLA::OnRemove(Creature* creature)
{
    START_HOOK_MAP(WORLD_EVENT_ON_DELETE_CREATURE);
    Push(creature);
    CallAllFunctions(ServerEventBindings, key);
}
