/*
 * Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
 * This program is free software licensed under GPL version 3
 * Please see the included DOCS/LICENSE.md for more information
 */

#ifndef _ALE_ALIVE_H
#define _ALE_ALIVE_H

#include "ObjectGuid.h"
#include <mutex>
#include <unordered_map>

class WorldObject;

// Live players keyed by GUID. Identity is the GUID, never the address,
// so address reuse across logout/login cannot validate stale userdata.
class AleAlive
{
public:
    using Guard = std::lock_guard<std::recursive_mutex>;

    static std::recursive_mutex& Mutex()
    {
        static std::recursive_mutex mutex;
        return mutex;
    }

    static void Insert(ObjectGuid guid, WorldObject* obj)
    {
        if (guid.IsEmpty() || !obj)
            return;
        std::lock_guard<std::recursive_mutex> guard{ Mutex() };
        Live()[guid] = obj;
    }

    static void Erase(ObjectGuid guid, WorldObject* obj)
    {
        if (guid.IsEmpty() || !obj)
            return;
        std::lock_guard<std::recursive_mutex> guard{ Mutex() };
        auto it = Live().find(guid);
        if (it != Live().end() && it->second == obj)
            Live().erase(it);
    }

    // True only if guid is still mapped to this exact pointer.
    // Compares values only, never dereferences.
    static bool MatchesLocked(ObjectGuid guid, WorldObject* raw)
    {
        if (guid.IsEmpty() || !raw)
            return false;
        auto it = Live().find(guid);
        return it != Live().end() && it->second == raw;
    }

private:
    static std::unordered_map<ObjectGuid, WorldObject*>& Live()
    {
        static std::unordered_map<ObjectGuid, WorldObject*> live;
        return live;
    }
};

#endif
