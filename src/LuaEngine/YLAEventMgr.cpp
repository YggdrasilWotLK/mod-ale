/*
* Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
* This program is free software licensed under GPL version 3
* Please see the included DOCS/LICENSE.md for more information
*/

#include "YLAEventMgr.h"
#include "LuaEngine.h"
#include "YlaAlive.h"
#include "Object.h"
#include "ObjectAccessor.h"
#include <vector>

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
};

YLAEventProcessor::YLAEventProcessor(const YlaStateRef& _owner, std::shared_ptr<YLA> ownerLock, WorldObject* _obj) : m_time(0), obj(_obj), guidCaptured(false), owner(_owner)
{
    // can be called from multiple threads
    if (obj && ownerLock && ownerLock->eventMgr)
    {
        EventMgr::Guard guard(ownerLock->eventMgr->GetLock());
        ownerLock->eventMgr->processors.insert(this);
    }
}

YLAEventProcessor::~YLAEventProcessor()
{
    // can be called from multiple threads
    {
        Guard guard(mutex);
        dead = true;
    }
    // can be called from multiple threads
    if (auto state = YLA::LockStateRef(owner))
    {
        YLA::Guard guard(state->GetStateLock());
        RemoveEvents_internal();
    }
    else
    {
        RemoveEvents_internal();
    }

    if (obj && YLA::IsInitialized())
    {
        if (auto state = YLA::LockStateRef(owner))
        {
            if (state->eventMgr)
            {
                EventMgr::Guard guard(state->eventMgr->GetLock());
                state->eventMgr->processors.erase(this);
            }
        }
    }
}

void YLAEventProcessor::CaptureGuid()
{
    Guard guard(mutex);
    if (guidCaptured || !obj)
        return;

    objGuid = obj->GET_GUID();
    guidCaptured = true;
}

void YLAEventProcessor::Update(uint32 diff)
{
    struct DueCall
    {
        LuaEvent* luaEvent;
        uint32 delay;
        uint32 repeatsArg;
        bool remove;
        WorldObject* liveObj;
        bool deadTarget;
        YlaStateRef owner;
        uint64 sweep;
    };

    // Due events leave the containers here and are sole-owned by the
    // locals until fired/deleted below, so the lock is never held across
    // Lua. Repeating events are re-added only after firing, when still
    // RUN and the processor is not being destroyed: destroy can therefore
    // never observe (and double-free) an event the firing loop owns.
    std::vector<DueCall> due;
    std::vector<LuaEvent*> dropped;
    {
        Guard guard(mutex);
        m_time += diff;
        while (!eventList.empty() && eventList.begin()->first <= m_time)
        {
            EventList::iterator it = eventList.begin();
            LuaEvent* luaEvent = it->second;
            eventList.erase(it);

            if (luaEvent->state != LUAEVENT_STATE_ERASE)
                eventMap.erase(luaEvent->funcRef);

            if (luaEvent->state != LUAEVENT_STATE_RUN)
            {
                dropped.push_back(luaEvent);
                continue;
            }

            uint32 delay = luaEvent->delay;
            bool remove = luaEvent->repeats == 1;
            uint32 repeatsArg = luaEvent->repeats;
            if (!remove && luaEvent->repeats)
                luaEvent->repeats--;

            // GUID-checked liveness; destroyed/relogged players resolve to
            // nullptr instead of a dangling pointer (and the call is
            // skipped below). Pre-capture players pass through (normal
            // during login hooks).
            WorldObject* liveObj = nullptr;
            if (guidCaptured && !objGuid.IsEmpty())
            {
                Player* found = ObjectAccessor::FindPlayer(objGuid);
                if (found)
                {
                    YlaAlive::Guard aliveGuard{ YlaAlive::Mutex() };
                    WorldObject* wo = static_cast<WorldObject*>(found);
                    if (YlaAlive::MatchesLocked(objGuid, wo) && wo->IsInWorld() &&
                        !found->IsDuringRemoveFromWorld())
                        liveObj = wo;
                }
                else
                {
                    YlaAlive::Guard aliveGuard{ YlaAlive::Mutex() };
                    if (YlaAlive::MatchesLocked(objGuid, obj))
                        liveObj = obj;
                }
            }
            else if (!guidCaptured)
                liveObj = obj;

            due.push_back(DueCall{ luaEvent, delay, repeatsArg, remove, liveObj, guidCaptured && !objGuid.IsEmpty() && !liveObj, luaEvent->owner, massSweep.load(std::memory_order_acquire) });
        }
    }

    for (DueCall& call : due)
    {
        bool fire = true;
        {
            Guard guard(mutex);
            // Another thread may have aborted it after it became due, or
            // the processor may have started destruction. Dead targets
            // stay skipped; unrepeatable leftovers are deleted below.
            if (dead || call.luaEvent->state != LUAEVENT_STATE_RUN || call.deadTarget)
            {
                fire = false;
                dropped.push_back(call.luaEvent);
            }
        }
        if (!fire)
            continue;

        // Publish the in-flight event so targeted SetState from Lua
        // (self-removal) reaches it: it was popped from the containers.
        {
            Guard guard(mutex);
            firing = call.luaEvent;
        }

        // Resolve the owning state into a shared reference that keeps it
        // alive for the whole call. Destroyed/recreated states resolve to
        // null and are skipped: no raw slot is ever dereferenced. Locking
        // mirrors LOCK_YLA_STATE for the resolved state (global -> state
        // in compat, state-only in multistate).
        if (auto state = YLA::LockStateRef(call.owner))
        {
            YLA::Guard globalGuard(YLAConfig::GetInstance().IsCompatibilityModeEnabled() ? YLA::GetLock() : YLA::GetNoopLock());
            YLA::Guard stateGuard(state->GetStateLock());
            if (state->HasLuaState())
                state->OnTimedEvent(call.luaEvent->funcRef, call.delay, call.repeatsArg, call.liveObj);
        }

        {
            Guard guard(mutex);
            // Re-add only when still scheduled to run, nobody tore the
            // processor down meanwhile, and no mass removal swept during
            // the call (sole ownership returns to the containers exactly
            // once). Anything else is deleted below.
            firing = nullptr;
            if (!call.remove && !dead && call.luaEvent->state == LUAEVENT_STATE_RUN &&
                massSweep.load(std::memory_order_acquire) == call.sweep)
                AddEvent(call.luaEvent);
            else
                dropped.push_back(call.luaEvent);
        }
    }

    for (LuaEvent* luaEvent : dropped)
        RemoveEvent(luaEvent);
}

void YLAEventProcessor::SetStates(LuaEventState state)
{
    Guard guard(mutex);
    ++massSweep;
    for (EventList::iterator it = eventList.begin(); it != eventList.end(); ++it)
        it->second->SetState(state);
    if (state == LUAEVENT_STATE_ERASE)
        eventMap.clear();
}

void YLAEventProcessor::RemoveEvents_internal()
{
    std::vector<LuaEvent*> doomed;
    {
        Guard guard(mutex);
        ++massSweep;
        for (EventList::iterator it = eventList.begin(); it != eventList.end(); ++it)
            doomed.push_back(it->second);

        eventList.clear();
        eventMap.clear();
    }
    for (LuaEvent* luaEvent : doomed)
        RemoveEvent(luaEvent);
}

void YLAEventProcessor::SetState(int eventId, LuaEventState state)
{
    Guard guard(mutex);
    if (eventMap.find(eventId) != eventMap.end())
        eventMap[eventId]->SetState(state);
    else if (firing && firing->funcRef == eventId)
        firing->SetState(state);
    if (state == LUAEVENT_STATE_ERASE)
        eventMap.erase(eventId);
}

void YLAEventProcessor::AddEvent(LuaEvent* luaEvent)
{
    Guard guard(mutex);
    luaEvent->GenerateDelay();
    eventList.insert(std::pair<uint64, LuaEvent*>(m_time + luaEvent->delay, luaEvent));
    eventMap[luaEvent->funcRef] = luaEvent;
}

void YLAEventProcessor::AddEvent(int funcRef, uint32 min, uint32 max, uint32 repeats, const YlaStateRef& owner)
{
    AddEvent(new LuaEvent(funcRef, min, max, repeats, owner));
}

void YLAEventProcessor::RemoveEvent(LuaEvent* luaEvent)
{
    // Decide here; the unref runs without the processor lock so we never
    // hold proc -> state (Lua entry paths take state -> proc). Caller must
    // have removed the event from the containers already. The owner is
    // resolved into a shared reference: dead/recreated states (or a dead
    // registry after CloseLua, whose refs lua_close reclaimed) skip the
    // unref instead of touching freed memory.
    YlaStateRef owner;
    int funcRef = 0;
    bool erase = false;
    {
        Guard guard(mutex);
        owner = luaEvent->owner;
        funcRef = luaEvent->funcRef;
        erase = (luaEvent->state == LUAEVENT_STATE_ERASE);
    }
    delete luaEvent;
    if (!erase && YLA::IsInitialized())
    {
        if (auto state = YLA::LockStateRef(owner))
        {
            // Unreference using the event's own state, mirroring
            // LOCK_YLA_STATE (global -> state in compat, state-only in
            // multistate) so the unref cannot race Lua execution on L.
            YLA::Guard globalGuard(YLAConfig::GetInstance().IsCompatibilityModeEnabled() ? YLA::GetLock() : YLA::GetNoopLock());
            YLA::Guard stateGuard(state->GetStateLock());
            if (state->HasLuaState())
                luaL_unref(state->L, LUA_REGISTRYINDEX, funcRef);
        }
    }
}

EventMgr::EventMgr(const YlaStateRef& _owner) : globalProcessor(new YLAEventProcessor(_owner, nullptr, NULL)), owner(_owner)
{
}

EventMgr::~EventMgr()
{
    {
        Guard guard(GetLock());
        if (!processors.empty())
            for (ProcessorSet::const_iterator it = processors.begin(); it != processors.end(); ++it)
                (*it)->RemoveEvents_internal();
        globalProcessor->RemoveEvents_internal();
    }
    delete globalProcessor;
    globalProcessor = NULL;
}

void EventMgr::SetStates(LuaEventState state)
{
    Guard guard(GetLock());
    if (!processors.empty())
        for (ProcessorSet::const_iterator it = processors.begin(); it != processors.end(); ++it)
            (*it)->SetStates(state);
    globalProcessor->SetStates(state);
}

void EventMgr::SetState(int eventId, LuaEventState state)
{
    Guard guard(GetLock());
    if (!processors.empty())
        for (ProcessorSet::const_iterator it = processors.begin(); it != processors.end(); ++it)
            (*it)->SetState(eventId, state);
    globalProcessor->SetState(eventId, state);
}
