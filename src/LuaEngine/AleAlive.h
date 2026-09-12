/*
 * Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
 * This program is free software licensed under GPL version 3
 * Please see the included DOCS/LICENSE.md for more information
 */

#ifndef _ALE_ALIVE_H
#define _ALE_ALIVE_H

#include <mutex>
#include <unordered_set>

class WorldObject;

/*
 * Tracks live Players so Lua Player userdata can be validated without
 * dereferencing a possibly-freed pointer.
 *
 * Problem: ALE pushes raw `Player*` into Lua (e.g. `GetPlayersInWorld`
 * snapshots, player-bound timed events). A playerbot logout deletes the
 * C++ object (`LogoutPlayerBot` -> `WorldSession::LogoutPlayer` ->
 * `Map::DeleteFromWorld` -> `delete player`) while Lua still holds the
 * pointer. The old `callstackid` check only catches stale references across
 * event boundaries, not objects freed during / just before the current
 * event -> SIGSEGV in e.g. `Object::GetGuidValue`.
 *
 * Scope is deliberately players-only: only Player userdata is validated
 * against this set, so creature / gameobject behavior is unchanged.
 *
 * Protocol (see ALE_WorldObjectScript in ALE_SC.cpp):
 * - Insert on `OnWorldObjectSetMap`, filtered to `TYPEID_PLAYER`. SetMap
 *   cannot be used for type filtering in `OnWorldObjectCreate` (it fires in
 *   the `WorldObject` base ctor, before the type is set), but every in-world
 *   player passes through `Map::Add` -> `SetMap` before any Lua hook can see
 *   it (verified: `IsInWorld()` is already true at `OnPlayerLogin`), so no
 *   live player is ever missing.
 * - Erase on `OnWorldObjectDestroy`, filtered the same way. It runs inside
 *   `~WorldObject` while the memory is still allocated (`GetTypeId()` is
 *   still valid there), before `delete ALEEvents`.
 * - `Contains` never dereferences, so it is safe to call with a dangling
 *   pointer value; it just reports "not alive".
 * - Checks use short critical sections only. The mutex is deliberately never
 *   held across Lua calls: Lua errors use longjmp, which skips C++ stack
 *   unwinding and would permanently wedge the mutex.
 */
class AleAlive
{
public:
    using Guard = std::lock_guard<std::recursive_mutex>;

    static std::recursive_mutex& Mutex()
    {
        static std::recursive_mutex mutex;
        return mutex;
    }

    static void Insert(WorldObject* obj)
    {
        if (!obj)
            return;
        std::lock_guard<std::recursive_mutex> guard{ Mutex() };
        Live().insert(obj);
    }

    static void Erase(WorldObject* obj)
    {
        if (!obj)
            return;
        std::lock_guard<std::recursive_mutex> guard{ Mutex() };
        Live().erase(obj);
    }

    // Safe to call with a dangling pointer: performs no dereference.
    static bool Contains(WorldObject* obj)
    {
        if (!obj)
            return false;
        std::lock_guard<std::recursive_mutex> guard{ Mutex() };
        return Live().find(obj) != Live().end();
    }

    // Same as Contains but assumes the caller already holds Mutex().
    // Must not dereference; pure set membership test.
    static bool ContainsLocked(WorldObject* obj)
    {
        if (!obj)
            return false;
        return Live().find(obj) != Live().end();
    }

private:
    static std::unordered_set<WorldObject*>& Live()
    {
        static std::unordered_set<WorldObject*> live;
        return live;
    }
};

#endif
