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

// Set of live Players. Insert on SetMap, erase on destroy. Contains never
// dereferences, so it is safe to call with a dangling pointer.
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

    static bool Contains(WorldObject* obj)
    {
        if (!obj)
            return false;
        std::lock_guard<std::recursive_mutex> guard{ Mutex() };
        return Live().find(obj) != Live().end();
    }

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
