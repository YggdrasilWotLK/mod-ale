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

#define START_HOOK(EVENT)\
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return;\
    LOCK_YLA_STATE;\
    if (!YLA::IsInitialized())\
        return;\
    auto key = EventKey<AllCreatureEvents>(EVENT);\
    if ((!AllCreatureEventBindings || !AllCreatureEventBindings->HasBindingsFor(key)))\
        return;

#define START_HOOK_WITH_RETVAL(EVENT, RETVAL)\
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return RETVAL;\
    LOCK_YLA_STATE;\
    if (!YLA::IsInitialized())\
        return RETVAL;\
    auto key = EventKey<AllCreatureEvents>(EVENT);\
    if ((!AllCreatureEventBindings || !AllCreatureEventBindings->HasBindingsFor(key)))\
        return RETVAL;

void YLA::OnAllCreatureAddToWorld(Creature* creature)
{
    START_HOOK(ALL_CREATURE_EVENT_ON_ADD);
    Push(creature);
    CallAllFunctions(AllCreatureEventBindings, key);
}

void YLA::OnAllCreatureRemoveFromWorld(Creature* creature)
{
    START_HOOK(ALL_CREATURE_EVENT_ON_REMOVE);
    Push(creature);
    CallAllFunctions(AllCreatureEventBindings, key);
}

void YLA::OnAllCreatureSelectLevel(const CreatureTemplate* cinfo, Creature* creature)
{
    START_HOOK(ALL_CREATURE_EVENT_ON_SELECT_LEVEL);
    Push(cinfo);
    Push(creature);
    CallAllFunctions(AllCreatureEventBindings, key);
}

void YLA::OnAllCreatureBeforeSelectLevel(const CreatureTemplate* cinfo, Creature* creature, uint8& level)
{
    START_HOOK(ALL_CREATURE_EVENT_ON_BEFORE_SELECT_LEVEL);
    Push(cinfo);
    Push(creature);
    Push(level);
    int levelIndex = lua_gettop(L);
    int n = SetupStack(AllCreatureEventBindings, key, 3);

    while (n > 0)
    {
        int r = CallOneFunction(n--, 3, 1);

        if (lua_isnumber(L, r))
        {
            level = CHECKVAL<uint8>(L, r);
            ReplaceArgument(level, levelIndex);
        }

        lua_pop(L, 1);
    }

    CleanUpStack(3);
}
