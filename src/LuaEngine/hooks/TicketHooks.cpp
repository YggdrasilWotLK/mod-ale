/*
 * Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
 * This program is free software licensed under GPL version 3
 * Please see the included DOCS/LICENSE.md for more information
 */

#include "Hooks.h"
#include "HookHelpers.h"
#include "LuaEngine.h"
#include "BindingMap.h"
#include "YLAIncludes.h"
#include "YLATemplate.h"

using namespace Hooks;

#define START_HOOK(EVENT) \
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return;\
    auto key = EventKey<TicketEvents>(EVENT);\
    if (!TicketEventBindings->HasBindingsFor(key))\
        return;\
    LOCK_ALE;\
    /* WORLD dispatch runs Lua on this state: hold its lock too
       (global -> state order). In multistate the global lock alone
       does not serialize against map threads on the same L. */\
    YLA::Guard __yla_world_state_guard(this->GetStateLock());

void YLA::OnTicketCreate(GmTicket* ticket)
{
    START_HOOK(TICKET_EVENT_ON_CREATE);
    Push(ticket);
    CallAllFunctions(TicketEventBindings, key);
}

void YLA::OnTicketUpdateLastChange(GmTicket* ticket)
{
    START_HOOK(TICKET_EVENT_UPDATE_LAST_CHANGE);
    Push(ticket);
    CallAllFunctions(TicketEventBindings, key);
}

void YLA::OnTicketClose(GmTicket* ticket)
{
    START_HOOK(TICKET_EVENT_ON_CLOSE);
    Push(ticket);
    CallAllFunctions(TicketEventBindings, key);
}

void YLA::OnTicketResolve(GmTicket* ticket)
{
    START_HOOK(TICKET_EVENT_ON_RESOLVE);
    Push(ticket);
    CallAllFunctions(TicketEventBindings, key);
}
