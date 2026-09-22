/*
 * Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
 * This program is free software licensed under GPL version 3
 * Please see the included DOCS/LICENSE.md for more information
 */

#ifndef _ALE_DEFER_H
#define _ALE_DEFER_H

#include "MapMgr.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "Player.h"
#include <mutex>
#include <vector>

// Far teleports and logouts from Lua run in OnWorldUpdate (maps idle).
// Running them inline would unlink the player from a map mid-iteration.
// Same-map teleports only relocate and stay inline.
class AleDefer
{
public:
    // False = bad destination. Same-map runs inline, cross-map is queued.
    static bool Teleport(Player* player, uint32 mapId, float x, float y, float z, float o)
    {
        if (!player)
            return false;
        if (player->GetMapId() == mapId && !player->IsBeingTeleportedFar())
            return player->TeleportTo(mapId, x, y, z, o);
        if (!MapMgr::IsValidMapCoord(mapId, x, y, z, o))
            return false;
        {
            std::lock_guard<std::mutex> guard(Mutex());
            Teleports().push_back({ player->GetGUID(), mapId, x, y, z, o });
        }
        return true;
    }

    static void Logout(Player* player, bool save)
    {
        if (!player)
            return;
        std::lock_guard<std::mutex> guard(Mutex());
        Logouts().push_back({ player->GetGUID(), save });
    }

    static void Drain()
    {
        std::vector<TeleportOp> teleports;
        std::vector<LogoutOp> logouts;
        {
            std::lock_guard<std::mutex> guard(Mutex());
            teleports.swap(Teleports());
            logouts.swap(Logouts());
        }
        for (TeleportOp const& op : teleports)
        {
            Player* player = ObjectAccessor::FindPlayer(op.playerGuid);
            if (!player || !player->GetSession())
                continue;
            if (player->IsInFlight())
            {
                player->GetMotionMaster()->MovementExpired();
                player->m_taxi.ClearTaxiDestinations();
            }
            player->TeleportTo(op.mapId, op.x, op.y, op.z, op.o);
        }
        for (LogoutOp const& op : logouts)
        {
            Player* player = ObjectAccessor::FindPlayer(op.playerGuid);
            if (!player || !player->GetSession())
                continue;
            player->GetSession()->LogoutPlayer(op.save);
        }
    }

private:
    struct TeleportOp
    {
        ObjectGuid playerGuid;
        uint32 mapId = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f, o = 0.0f;
    };
    struct LogoutOp
    {
        ObjectGuid playerGuid;
        bool save = true;
    };

    static std::mutex& Mutex()
    {
        static std::mutex mutex;
        return mutex;
    }
    static std::vector<TeleportOp>& Teleports()
    {
        static std::vector<TeleportOp> queue;
        return queue;
    }
    static std::vector<LogoutOp>& Logouts()
    {
        static std::vector<LogoutOp> queue;
        return queue;
    }
};

#endif
