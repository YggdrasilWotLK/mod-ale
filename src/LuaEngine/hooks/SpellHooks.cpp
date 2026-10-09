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

#define START_HOOK(EVENT, ENTRY) \
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return;\
    auto key = EntryKey<SpellEvents>(EVENT, ENTRY);\
    if (!SpellEventBindings->HasBindingsFor(key))\
        return;\
    LOCK_YLA_STATE

#define START_HOOK_WITH_RETVAL(EVENT, ENTRY, RETVAL) \
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return RETVAL;\
    auto key = EntryKey<SpellEvents>(EVENT, ENTRY);\
    if (!SpellEventBindings->HasBindingsFor(key))\
        return RETVAL;\
    LOCK_YLA_STATE

void YLA::OnSpellCastCancel(Unit* caster, Spell* spell, SpellInfo const* spellInfo, bool bySelf)
{
    START_HOOK(SPELL_EVENT_ON_CAST_CANCEL, spellInfo->Id);
    Push(caster);
    Push(spell);
    Push(bySelf);
    CallAllFunctions(SpellEventBindings, key);
}

void YLA::OnSpellCast(Unit* caster, Spell* spell, SpellInfo const* spellInfo, bool skipCheck)
{
    START_HOOK(SPELL_EVENT_ON_CAST, spellInfo->Id);
    Push(caster);
    Push(spell);
    Push(skipCheck);
    CallAllFunctions(SpellEventBindings, key);
}

void YLA::OnSpellPrepare(Unit* caster, Spell* spell, SpellInfo const* spellInfo)
{
    START_HOOK(SPELL_EVENT_ON_PREPARE, spellInfo->Id);
    Push(caster);
    Push(spell);
    CallAllFunctions(SpellEventBindings, key);
}

void YLA::OnSpellAuraApply(Unit* unit, Aura* aura)
{
    uint32 spellId = aura->GetSpellInfo()->GetSpellId();
    START_HOOK(SPELL_EVENT_ON_AURA_APPLY, spellId);
    Push(unit);
    Push(aura);
    CallAllFunctions(SpellEventBindings, key);
}

void YLA::OnSpellAuraRemove(Unit* unit, Aura* aura, uint8 mode)
{
    uint32 spellId = aura->GetSpellInfo()->GetSpellId();
    START_HOOK(SPELL_EVENT_ON_AURA_REMOVE, spellId);
    Push(unit);
    Push(aura);
    Push(mode);
    CallAllFunctions(SpellEventBindings, key);
}
