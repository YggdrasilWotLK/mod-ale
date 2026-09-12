/*
* Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
* This program is free software licensed under GPL version 3
* Please see the included DOCS/LICENSE.md for more information
*/

#include "ALEEventMgr.h"
#include "LuaEngine.h"
#include "AleAlive.h"
#include "Object.h"
#include "ObjectAccessor.h"

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
};

ALEEventProcessor::ALEEventProcessor(ALE** _E, WorldObject* _obj) : m_time(0), obj(_obj), guidCaptured(false), E(_E)
{
    // can be called from multiple threads
    if (obj)
    {
        EventMgr::Guard guard((*E)->eventMgr->GetLock());
        (*E)->eventMgr->processors.insert(this);
    }
}

ALEEventProcessor::~ALEEventProcessor()
{
    // can be called from multiple threads
    {
        LOCK_ALE;
        RemoveEvents_internal();
    }

    if (obj && ALE::IsInitialized())
    {
        EventMgr::Guard guard((*E)->eventMgr->GetLock());
        (*E)->eventMgr->processors.erase(this);
    }
}

void ALEEventProcessor::CaptureGuid()
{
    if (guidCaptured || !obj)
        return;

    objGuid = obj->GET_GUID();
    guidCaptured = true;
}

void ALEEventProcessor::Update(uint32 diff)
{
    isUpdating = true;

    m_time += diff;
    while (!eventList.empty() && eventList.begin()->first <= m_time)
    {
        auto it = eventList.begin();
        LuaEvent* luaEvent = it->second;
        eventList.erase(it);

        if (luaEvent->state != LUAEVENT_STATE_ERASE)
            eventMap.erase(luaEvent->funcRef);

        if (luaEvent->state == LUAEVENT_STATE_RUN)
        {
            uint32 delay = luaEvent->delay;
            bool remove = luaEvent->repeats == 1;
            if (!remove)
                AddEvent(luaEvent); // Reschedule before calling incase RemoveEvents used

            WorldObject* liveObj = nullptr;
            if (guidCaptured && !objGuid.IsEmpty())
            {
                Player* found = ObjectAccessor::FindPlayer(objGuid);
                // FindPlayer only guarantees IsInWorld() at lookup time under
                // its own lock; re-validate against the alive registry (which
                // never dereferences, so it is safe even if the player was
                // freed by a bot logout right after the lookup) before
                // handing the pointer to Lua.
                if (found)
                {
                    AleAlive::Guard aliveGuard{ AleAlive::Mutex() };
                    WorldObject* wo = static_cast<WorldObject*>(found);
                    if (AleAlive::ContainsLocked(wo) && wo->IsInWorld() &&
                        !found->IsDuringRemoveFromWorld())
                        liveObj = wo;
                }
            }
            else if (!guidCaptured)
                liveObj = obj;

            // Call the timed event
            (*E)->OnTimedEvent(luaEvent->funcRef, delay, luaEvent->repeats ? luaEvent->repeats-- : luaEvent->repeats, liveObj);

            if (!remove)
                continue;
        }

        // Event should be deleted (executed last time or set to be aborted)
        RemoveEvent(luaEvent);
    }

    isUpdating = false;
    ProcessDeferredOps();
}

void ALEEventProcessor::SetStates(LuaEventState state)
{
    if (isUpdating)
    {
        QueueDeferredOp(DeferredOpType::SetStates, nullptr, 0, state);
        return;
    }

    for (EventList::iterator it = eventList.begin(); it != eventList.end(); ++it)
        it->second->SetState(state);
    if (state == LUAEVENT_STATE_ERASE)
        eventMap.clear();
}

void ALEEventProcessor::RemoveEvents_internal()
{
    if (isUpdating)
    {
        QueueDeferredOp(DeferredOpType::ClearAll);
        return;
    }

    //if (!final)
    //{
    //    for (EventList::iterator it = eventList.begin(); it != eventList.end(); ++it)
    //        it->second->to_Abort = true;
    //    return;
    //}

    for (EventList::iterator it = eventList.begin(); it != eventList.end(); ++it)
        RemoveEvent(it->second);

    deferredOps.clear();
    eventList.clear();
    eventMap.clear();
}

void ALEEventProcessor::SetState(int eventId, LuaEventState state)
{
    if (isUpdating)
    {
        QueueDeferredOp(DeferredOpType::SetState, nullptr, eventId, state);
        return;
    }

    if (eventMap.find(eventId) != eventMap.end())
        eventMap[eventId]->SetState(state);
    if (state == LUAEVENT_STATE_ERASE)
        eventMap.erase(eventId);
}

void ALEEventProcessor::AddEvent(LuaEvent* luaEvent)
{
    if (isUpdating)
    {
        QueueDeferredOp(DeferredOpType::AddEvent, luaEvent);
        return;
    }

    luaEvent->GenerateDelay();
    eventList.insert(std::pair<uint64, LuaEvent*>(m_time + luaEvent->delay, luaEvent));
    eventMap[luaEvent->funcRef] = luaEvent;
}

void ALEEventProcessor::AddEvent(int funcRef, uint32 min, uint32 max, uint32 repeats)
{
    AddEvent(new LuaEvent(funcRef, min, max, repeats));
}

void ALEEventProcessor::RemoveEvent(LuaEvent* luaEvent)
{
    // Unreference if should and if ALE was not yet uninitialized and if the lua state still exists
    if (luaEvent->state != LUAEVENT_STATE_ERASE && ALE::IsInitialized() && (*E)->HasLuaState())
    {
        // Free lua function ref
        luaL_unref((*E)->L, LUA_REGISTRYINDEX, luaEvent->funcRef);
    }
    delete luaEvent;
}

void ALEEventProcessor::QueueDeferredOp(DeferredOpType type, LuaEvent* event, int eventId, LuaEventState state)
{
    DeferredOp op;
    op.type = type;
    op.event = event;
    op.eventId = eventId;
    op.state = state;
    deferredOps.push_back(op);
}

void ALEEventProcessor::ProcessDeferredOps()
{
    if (deferredOps.empty())
        return;

    std::vector<DeferredOp> ops;
    ops.swap(deferredOps);

    for (DeferredOp& op : ops)
    {
        switch (op.type)
        {
        case DeferredOpType::AddEvent:
            AddEvent(op.event);
            break;

        case DeferredOpType::SetState:
            SetState(op.eventId, op.state);
            break;

        case DeferredOpType::SetStates:
            SetStates(op.state);
            break;

        case DeferredOpType::ClearAll:
            RemoveEvents_internal();
            break;
        }
    }
}

EventMgr::EventMgr(ALE** _E) : globalProcessor(new ALEEventProcessor(_E, NULL)), E(_E)
{
}

EventMgr::~EventMgr()
{
    {
        Guard guard(GetLock());
        if (!processors.empty())
            for (ProcessorSet::const_iterator it = processors.begin(); it != processors.end(); ++it) // loop processors
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
        for (ProcessorSet::const_iterator it = processors.begin(); it != processors.end(); ++it) // loop processors
            (*it)->SetStates(state);
    globalProcessor->SetStates(state);
}

void EventMgr::SetState(int eventId, LuaEventState state)
{
    Guard guard(GetLock());
    if (!processors.empty())
        for (ProcessorSet::const_iterator it = processors.begin(); it != processors.end(); ++it) // loop processors
            (*it)->SetState(eventId, state);
    globalProcessor->SetState(eventId, state);
}
