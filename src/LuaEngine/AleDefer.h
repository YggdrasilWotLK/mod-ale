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

// Deferred map-unlinking player ops for the single-map-thread configuration.
//
// Problem: Player::TeleportTo across maps calls
// oldmap->RemovePlayerFromMap(this) INLINE, and WorldSession::LogoutPlayer
// runs the full synchronous teardown (map unlink included). When Lua invokes
// either from inside a map update (creature UpdateAI, combat hooks, timed
// events, MAP_EVENT_ON_UPDATE), that unlink mutates the player/grid lists of
// the map currently being iterated -> iterator invalidation, same-thread
// reentrancy crash. Same-map teleports only Relocate (no unlink, same as
// core blink/knockback spells) and stay inline.
//
// Fix: validate now, execute later. Far teleports and logouts requested from
// Lua are queued here and drained in ALE::OnWorldUpdate, which runs on the
// world thread after MapMgr::Update has finished (and waited for the map
// worker): no map iteration is active, so the unlink is exactly as safe as
// the core's own session-phase teleport/logout paths. Sub-tick delay, same
// tick in practice.
//
// Threading: every Schedule* caller runs inside Lua under LOCK_ALE, and
// Drain() runs under LOCK_ALE from OnWorldUpdate, so the queue itself needs
// no ordering guarantees beyond a short mutex held only for the swap — never
// held across Lua/core calls (ops scheduled from inside the drain run next
// tick). KickPlayer only closes the socket (real removal happens later in
// session update) and stays inline with a null-session guard.
class AleDefer
{
public:
    // Returns false when the destination is invalid (nothing scheduled).
    // Same-map destinations execute inline via the normal TeleportTo path
    // and return its result.
    static bool Teleport(Player* player, uint32 mapId, float x, float y, float z, float o)
    {
        if (!player)
            return false;
        // Same map: no unlink, just Relocate (same risk profile as core
        // blink/knockback spells issued mid-update) — stay inline so core's
        // own MustDelayTeleport handling applies during player updates.
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

    // Runs on the world thread in OnWorldUpdate (maps idle). Players are
    // re-resolved by GUID at drain time; gone players are skipped.
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
            // Full core validation (map bounds, entry rights) runs again
            // inside TeleportTo, as on any other caller path.
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
