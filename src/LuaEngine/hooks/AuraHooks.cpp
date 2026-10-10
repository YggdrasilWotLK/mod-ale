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

#define START_HOOK(EVENT, ENTRY)\
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return;\
    LOCK_YLA_STATE;\
    if (!YLA::IsInitialized())\
        return;\
    auto key = EntryKey<AuraEvents>(EVENT, ENTRY);\
    if ((!AuraEventBindings || !AuraEventBindings->HasBindingsFor(key)))\
        return;

void YLA::OnAuraEventApply(Unit* unit, Aura* aura)
{
    uint32 spellId = aura->GetId();
    START_HOOK(AURA_EVENT_ON_APPLY, spellId);
    Push(unit);
    Push(aura);
    CallAllFunctions(AuraEventBindings, key);
}

void YLA::OnAuraEventRemove(Unit* unit, Aura* aura, uint8 mode)
{
    uint32 spellId = aura->GetId();
    START_HOOK(AURA_EVENT_ON_REMOVE, spellId);
    Push(unit);
    Push(aura);
    Push(mode);
    CallAllFunctions(AuraEventBindings, key);
}
