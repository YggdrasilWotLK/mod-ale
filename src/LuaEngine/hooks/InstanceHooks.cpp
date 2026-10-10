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
#include "YLAInstanceAI.h"

using namespace Hooks;

#define START_HOOK(EVENT, AI)\
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return;\
    LOCK_YLA_STATE;\
    if (!YLA::IsInitialized())\
        return;\
    auto mapKey = EntryKey<InstanceEvents>(EVENT, AI->instance->GetId());\
    auto instanceKey = EntryKey<InstanceEvents>(EVENT, AI->instance->GetInstanceId());\
    if ((!MapEventBindings || !MapEventBindings->HasBindingsFor(mapKey)) && (!InstanceEventBindings || !InstanceEventBindings->HasBindingsFor(instanceKey)))\
        return;\
    PushInstanceData(L, AI);\
    Push(AI->instance)

#define START_HOOK_WITH_RETVAL(EVENT, AI, RETVAL)\
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return RETVAL;\
    LOCK_YLA_STATE;\
    if (!YLA::IsInitialized())\
        return RETVAL;\
    auto mapKey = EntryKey<InstanceEvents>(EVENT, AI->instance->GetId());\
    auto instanceKey = EntryKey<InstanceEvents>(EVENT, AI->instance->GetInstanceId());\
    if ((!MapEventBindings || !MapEventBindings->HasBindingsFor(mapKey)) && (!InstanceEventBindings || !InstanceEventBindings->HasBindingsFor(instanceKey)))\
        return RETVAL;\
    PushInstanceData(L, AI);\
    Push(AI->instance)

void YLA::OnInitialize(YLAInstanceAI* ai)
{
    START_HOOK(INSTANCE_EVENT_ON_INITIALIZE, ai);
    CallAllFunctions(MapEventBindings, InstanceEventBindings, mapKey, instanceKey);
}

void YLA::OnLoad(YLAInstanceAI* ai)
{
    START_HOOK(INSTANCE_EVENT_ON_LOAD, ai);
    CallAllFunctions(MapEventBindings, InstanceEventBindings, mapKey, instanceKey);
}

void YLA::OnUpdateInstance(YLAInstanceAI* ai, uint32 diff)
{
    START_HOOK(INSTANCE_EVENT_ON_UPDATE, ai);
    Push(diff);
    CallAllFunctions(MapEventBindings, InstanceEventBindings, mapKey, instanceKey);
}

void YLA::OnPlayerEnterInstance(YLAInstanceAI* ai, Player* player)
{
    START_HOOK(INSTANCE_EVENT_ON_PLAYER_ENTER, ai);
    Push(player);
    CallAllFunctions(MapEventBindings, InstanceEventBindings, mapKey, instanceKey);
}

void YLA::OnCreatureCreate(YLAInstanceAI* ai, Creature* creature)
{
    START_HOOK(INSTANCE_EVENT_ON_CREATURE_CREATE, ai);
    Push(creature);
    CallAllFunctions(MapEventBindings, InstanceEventBindings, mapKey, instanceKey);
}

void YLA::OnGameObjectCreate(YLAInstanceAI* ai, GameObject* gameobject)
{
    START_HOOK(INSTANCE_EVENT_ON_GAMEOBJECT_CREATE, ai);
    Push(gameobject);
    CallAllFunctions(MapEventBindings, InstanceEventBindings, mapKey, instanceKey);
}

bool YLA::OnCheckEncounterInProgress(YLAInstanceAI* ai)
{
    START_HOOK_WITH_RETVAL(INSTANCE_EVENT_ON_CHECK_ENCOUNTER_IN_PROGRESS, ai, false);
    return CallAllFunctionsBool(MapEventBindings, InstanceEventBindings, mapKey, instanceKey);
}
