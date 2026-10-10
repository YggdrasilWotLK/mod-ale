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
    auto key = EntryKey<ItemEvents>(EVENT, ENTRY);\
    if ((!ItemEventBindings || !ItemEventBindings->HasBindingsFor(key)))\
        return;

#define START_HOOK_WITH_RETVAL(EVENT, ENTRY, RETVAL)\
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return RETVAL;\
    LOCK_YLA_STATE;\
    if (!YLA::IsInitialized())\
        return RETVAL;\
    auto key = EntryKey<ItemEvents>(EVENT, ENTRY);\
    if ((!ItemEventBindings || !ItemEventBindings->HasBindingsFor(key)))\
        return RETVAL;

void YLA::OnDummyEffect(WorldObject* pCaster, uint32 spellId, SpellEffIndex effIndex, Item* pTarget)
{
    START_HOOK(ITEM_EVENT_ON_DUMMY_EFFECT, pTarget->GetEntry());
    Push(pCaster);
    Push(spellId);
    Push(effIndex);
    Push(pTarget);
    CallAllFunctions(ItemEventBindings, key);
}

bool YLA::OnQuestAccept(Player* pPlayer, Item* pItem, Quest const* pQuest)
{
    START_HOOK_WITH_RETVAL(ITEM_EVENT_ON_QUEST_ACCEPT, pItem->GetEntry(), false);
    Push(pPlayer);
    Push(pItem);
    Push(pQuest);
    return CallAllFunctionsBool(ItemEventBindings, key);
}

bool YLA::OnUse(Player* pPlayer, Item* pItem, SpellCastTargets const& targets)
{
    ObjectGuid guid = pItem->GET_GUID();
    bool castSpell = true;

    if (!OnItemUse(pPlayer, pItem, targets))
        castSpell = false;

    pItem = pPlayer->GetItemByGuid(guid);
    if (pItem)
    {
        if (!OnItemGossip(pPlayer, pItem, targets))
            castSpell = false;
        pItem = pPlayer->GetItemByGuid(guid);
    }

    if (pItem && castSpell)
        return true;

    WorldPacket data(SMSG_INVENTORY_CHANGE_FAILURE, 18);
    data << uint8(59);
    data << guid;
    data << ObjectGuid(uint64(0));
    data << uint8(0);
    pPlayer->GetSession()->SendPacket(&data);
    return false;
}

bool YLA::OnItemUse(Player* pPlayer, Item* pItem, SpellCastTargets const& targets)
{
    START_HOOK_WITH_RETVAL(ITEM_EVENT_ON_USE, pItem->GetEntry(), true);
    Push(pPlayer);
    Push(pItem);

    if (GameObject* target = targets.GetGOTarget())
        Push(target);
    else if (Item* target = targets.GetItemTarget())
        Push(target);
    else if (Corpse* target = targets.GetCorpseTarget())
        Push(target);
    else if (Unit* target = targets.GetUnitTarget())
        Push(target);
    else if (WorldObject* target = targets.GetObjectTarget())
        Push(target);
    else
        Push();

    return CallAllFunctionsBool(ItemEventBindings, key, true);
}

bool YLA::OnExpire(Player* pPlayer, ItemTemplate const* pProto)
{
    START_HOOK_WITH_RETVAL(ITEM_EVENT_ON_EXPIRE, pProto->ItemId, false);
    Push(pPlayer);
    Push(pProto->ItemId);
    return CallAllFunctionsBool(ItemEventBindings, key);
}

bool YLA::OnRemove(Player* pPlayer, Item* pItem)
{
    START_HOOK_WITH_RETVAL(ITEM_EVENT_ON_REMOVE, pItem->GetEntry(), false);
    Push(pPlayer);
    Push(pItem);
    return CallAllFunctionsBool(ItemEventBindings, key);
}
