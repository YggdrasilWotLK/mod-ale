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
#include "YLAEventMgr.h"
#include "YLATemplate.h"

using namespace Hooks;

#define START_HOOK(EVENT, ENTRY) \
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return;\
    auto key = EntryKey<GameObjectEvents>(EVENT, ENTRY);\
    if (!GameObjectEventBindings->HasBindingsFor(key))\
        return;\
    LOCK_YLA_STATE

#define START_HOOK_WITH_RETVAL(EVENT, ENTRY, RETVAL) \
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return RETVAL;\
    auto key = EntryKey<GameObjectEvents>(EVENT, ENTRY);\
    if (!GameObjectEventBindings->HasBindingsFor(key))\
        return RETVAL;\
    LOCK_YLA_STATE

void YLA::OnDummyEffect(WorldObject* pCaster, uint32 spellId, SpellEffIndex effIndex, GameObject* pTarget)
{
    START_HOOK(GAMEOBJECT_EVENT_ON_DUMMY_EFFECT, pTarget->GetEntry());
    Push(pCaster);
    Push(spellId);
    Push(effIndex);
    Push(pTarget);
    CallAllFunctions(GameObjectEventBindings, key);
}

void YLA::UpdateAI(GameObject* pGameObject, uint32 diff)
{
    pGameObject->YLAEvents->Update(diff);
    START_HOOK(GAMEOBJECT_EVENT_ON_AIUPDATE, pGameObject->GetEntry());
    Push(pGameObject);
    Push(diff);
    CallAllFunctions(GameObjectEventBindings, key);
}

bool YLA::OnQuestAccept(Player* pPlayer, GameObject* pGameObject, Quest const* pQuest)
{
    START_HOOK_WITH_RETVAL(GAMEOBJECT_EVENT_ON_QUEST_ACCEPT, pGameObject->GetEntry(), false);
    Push(pPlayer);
    Push(pGameObject);
    Push(pQuest);
    return CallAllFunctionsBool(GameObjectEventBindings, key);
}

bool YLA::OnQuestReward(Player* pPlayer, GameObject* pGameObject, Quest const* pQuest, uint32 opt)
{
    START_HOOK_WITH_RETVAL(GAMEOBJECT_EVENT_ON_QUEST_REWARD, pGameObject->GetEntry(), false);
    Push(pPlayer);
    Push(pGameObject);
    Push(pQuest);
    Push(opt);
    return CallAllFunctionsBool(GameObjectEventBindings, key);
}

void YLA::GetDialogStatus(const Player* pPlayer, const GameObject* pGameObject)
{
    START_HOOK(GAMEOBJECT_EVENT_ON_DIALOG_STATUS, pGameObject->GetEntry());
    Push(pPlayer);
    Push(pGameObject);
    CallAllFunctions(GameObjectEventBindings, key);
}

void YLA::OnDestroyed(GameObject* pGameObject, WorldObject* attacker)
{
    START_HOOK(GAMEOBJECT_EVENT_ON_DESTROYED, pGameObject->GetEntry());
    Push(pGameObject);
    Push(attacker);
    CallAllFunctions(GameObjectEventBindings, key);
}

void YLA::OnDamaged(GameObject* pGameObject, WorldObject* attacker)
{
    START_HOOK(GAMEOBJECT_EVENT_ON_DAMAGED, pGameObject->GetEntry());
    Push(pGameObject);
    Push(attacker);
    CallAllFunctions(GameObjectEventBindings, key);
}

void YLA::OnLootStateChanged(GameObject* pGameObject, uint32 state)
{
    START_HOOK(GAMEOBJECT_EVENT_ON_LOOT_STATE_CHANGE, pGameObject->GetEntry());
    Push(pGameObject);
    Push(state);
    CallAllFunctions(GameObjectEventBindings, key);
}

void YLA::OnGameObjectStateChanged(GameObject* pGameObject, uint32 state)
{
    START_HOOK(GAMEOBJECT_EVENT_ON_GO_STATE_CHANGED, pGameObject->GetEntry());
    Push(pGameObject);
    Push(state);
    CallAllFunctions(GameObjectEventBindings, key);
}

void YLA::OnSpawn(GameObject* pGameObject)
{
    START_HOOK(GAMEOBJECT_EVENT_ON_SPAWN, pGameObject->GetEntry());
    Push(pGameObject);
    CallAllFunctions(GameObjectEventBindings, key);
}

void YLA::OnAddToWorld(GameObject* pGameObject)
{
    START_HOOK(GAMEOBJECT_EVENT_ON_ADD, pGameObject->GetEntry());
    Push(pGameObject);
    CallAllFunctions(GameObjectEventBindings, key);
}

void YLA::OnRemoveFromWorld(GameObject* pGameObject)
{
    START_HOOK(GAMEOBJECT_EVENT_ON_REMOVE, pGameObject->GetEntry());
    Push(pGameObject);
    CallAllFunctions(GameObjectEventBindings, key);
}

bool YLA::OnGameObjectUse(Player* pPlayer, GameObject* pGameObject)
{
    START_HOOK_WITH_RETVAL(GAMEOBJECT_EVENT_ON_USE, pGameObject->GetEntry(), false);
    Push(pGameObject);
    Push(pPlayer);
    return CallAllFunctionsBool(GameObjectEventBindings, key);
}
