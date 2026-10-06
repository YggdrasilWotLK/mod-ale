/*
* Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
* This program is free software licensed under GPL version 3
* Please see the included DOCS/LICENSE.md for more information
*/

#ifndef _ALE_EVENT_MGR_H
#define _ALE_EVENT_MGR_H

#include "ALEUtility.h"
#include "Common.h"
#include "Util.h"
#include <atomic>
#include <map>
#include <memory>
#include <mutex>

#include "Define.h"
#include "ObjectGuid.h"

class ALE;
class EventMgr;
class ALEEventProcessor;
class WorldObject;

// Value-type identity of a Lua state for timer/HTTP/DB ownership.
// Never a raw pointer: resolved via ALE::LockStateRef into a shared_ptr
// that keeps the state alive for the duration of use. Recreating a map
// state yields a new seq, so stale refs resolve to null instead of a new
// state. (Global states resolve via the GALE holder.)
struct AleStateRef
{
    bool global = true;
    uint32 mapId = (uint32)(-1);
    uint32 instanceId = 0;
    uint64 seq = 0;
};

enum LuaEventState
{
    LUAEVENT_STATE_RUN,    // On next call run the function normally
    LUAEVENT_STATE_ABORT,  // On next call unregisters reffed function and erases the data
    LUAEVENT_STATE_ERASE,  // On next call just erases the data
};

struct LuaEvent
{
    LuaEvent(int _funcRef, uint32 _min, uint32 _max, uint32 _repeats, const AleStateRef& _owner) :
        min(_min), max(_max), delay(0), repeats(_repeats), funcRef(_funcRef), state(LUAEVENT_STATE_RUN), owner(_owner)
    {
    }

    void SetState(LuaEventState _state)
    {
        if (state != LUAEVENT_STATE_ERASE)
            state = _state;
    }

    void GenerateDelay()
    {
        delay = urand(min, max);
    }

    uint32 min;
    uint32 max;
    uint32 delay;
    uint32 repeats;
    int funcRef;
    LuaEventState state;
    // Owning-state identity (never a raw slot): resolved via
    // ALE::LockStateRef at fire/unref time, so destroy/reload can never
    // leave a dangling reference behind, including for due-local events.
    AleStateRef owner;
};

class ALEEventProcessor
{
    friend class EventMgr;

public:
    typedef std::multimap<uint64, LuaEvent*> EventList;
    typedef std::unordered_map<int, LuaEvent*> EventMap;

    // ownerLock keeps the owning state alive for registry insert/erase;
    // owner is the long-term identity used to resolve at fire/unref time.
    ALEEventProcessor(const AleStateRef& owner, std::shared_ptr<ALE> ownerLock, WorldObject* _obj);
    ~ALEEventProcessor();

    void CaptureGuid();
    void Update(uint32 diff);
    // removes all timed events on next tick or at tick end
    void SetStates(LuaEventState state);
    // set the event to be removed when executing
    void SetState(int eventId, LuaEventState state);
    void AddEvent(int funcRef, uint32 min, uint32 max, uint32 repeats, const AleStateRef& owner);
    EventMap eventMap;

private:
    using Guard = std::lock_guard<std::recursive_mutex>;
    // Serializes container access across map/world threads. Leaf-only:
    // never held across OnTimedEvent or registry unref (those take state
    // locks, and Lua entry paths take state -> proc). Lock order is
    // state/eventMgr -> proc, never the reverse.
    // Due events are sole-owned by the firing loop (never dual-owned in
    // the containers), and re-added only after firing when still RUN and
    // the processor is not being destroyed, so destroy can never
    // double-free a repeating event.
    std::recursive_mutex mutex;

    void RemoveEvents_internal();
    void AddEvent(LuaEvent* luaEvent);
    void RemoveEvent(LuaEvent* luaEvent);
    EventList eventList;
    uint64 m_time;
    WorldObject* obj;
    ObjectGuid objGuid;
    bool guidCaptured;
    AleStateRef owner;
    // Set under proc lock at destruction start; Update consults it before
    // re-adding a repeating event instead of resurrecting it.
    std::atomic<bool> dead{false};
    // Bumped by mass removals (SetStates/RemoveEvents_internal). A
    // repeating event snapshotted before firing that observes a bump
    // afterwards was mass-removed by its own Lua callback mid-fire (the
    // containers no longer hold it, so the sweep could not mark it) and
    // must not be re-added. Targeted SetState bumps nothing.
    std::atomic<uint64> massSweep{0};
};

class EventMgr : public ALEUtil::Lockable
{
public:
    typedef std::unordered_set<ALEEventProcessor*> ProcessorSet;
    ProcessorSet processors;
    ALEEventProcessor* globalProcessor;
    AleStateRef owner;

    EventMgr(const AleStateRef& owner);
    ~EventMgr();

    // Set the state of all timed events
    // Execute only in safe env
    void SetStates(LuaEventState state);

    // Sets the eventId's state in all processors
    // Execute only in safe env
    void SetState(int eventId, LuaEventState state);
};

#endif
