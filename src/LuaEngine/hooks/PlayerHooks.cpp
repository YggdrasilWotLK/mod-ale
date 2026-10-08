/*
 * Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
 * This program is free software licensed under GPL version 3
 * Please see the included DOCS/LICENSE.md for more information
 */

#include "Hooks.h"
#include "HookHelpers.h"
#include "LuaEngine.h"
#include "BindingMap.h"
#include "ALEIncludes.h"
#include "ALETemplate.h"

using namespace Hooks;

#define START_HOOK_WORLD(EVENT) \
    if (!ALEConfig::GetInstance().IsALEEnabled())\
        return;\
    auto key = EventKey<PlayerEvents>(EVENT);\
    if (!PlayerEventBindings->HasBindingsFor(key))\
        return;\
    LOCK_ALE

#define START_HOOK_WORLD_WITH_RETVAL(EVENT, RETVAL) \
    if (!ALEConfig::GetInstance().IsALEEnabled())\
        return RETVAL;\
    auto key = EventKey<PlayerEvents>(EVENT);\
    if (!PlayerEventBindings->HasBindingsFor(key))\
        return RETVAL;\
    LOCK_ALE

#define START_HOOK_MAP(EVENT) \
    if (!ALEConfig::GetInstance().IsALEEnabled())\
        return;\
    auto key = EventKey<PlayerEvents>(EVENT);\
    if (!PlayerEventBindings->HasBindingsFor(key))\
        return;\
    LOCK_ALE_STATE

#define START_HOOK_MAP_WITH_RETVAL(EVENT, RETVAL) \
    if (!ALEConfig::GetInstance().IsALEEnabled())\
        return RETVAL;\
    auto key = EventKey<PlayerEvents>(EVENT);\
    if (!PlayerEventBindings->HasBindingsFor(key))\
        return RETVAL;\
    LOCK_ALE_STATE

// MAP
void ALE::OnLearnTalents(Player* pPlayer, uint32 talentId, uint32 talentRank, uint32 spellid)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_LEARN_TALENTS);
    Push(pPlayer);
    Push(talentId);
    Push(talentRank);
    Push(spellid);
    CallAllFunctions(PlayerEventBindings, key);
}

// WORLD
bool ALE::OnCommand(ChatHandler& handler, const char* text)
{
    Player* player = handler.IsConsole() ? nullptr : handler.GetSession()->GetPlayer();
    if (!player || player->GetSession()->GetSecurity() >= SEC_ADMINISTRATOR)
    {
        std::string reload = text;
        std::transform(reload.begin(), reload.end(), reload.begin(), ::tolower);
        if (reload.find("reload ale") == 0)
        {
            ReloadALE();
            return false;
        }
    }

    START_HOOK_WORLD_WITH_RETVAL(PLAYER_EVENT_ON_COMMAND, true);
    Push(player);
    Push(text);
    Push(&handler);
    return CallAllFunctionsBool(PlayerEventBindings, key, true);
}

// MAP
void ALE::OnLootItem(Player* pPlayer, Item* pItem, uint32 count, ObjectGuid guid)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_LOOT_ITEM);
    Push(pPlayer);
    Push(pItem);
    Push(count);
    Push(guid);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnLootMoney(Player* pPlayer, uint32 amount)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_LOOT_MONEY);
    Push(pPlayer);
    Push(amount);
    CallAllFunctions(PlayerEventBindings, key);
}

// WORLD
void ALE::OnFirstLogin(Player* pPlayer)
{
    START_HOOK_WORLD(PLAYER_EVENT_ON_FIRST_LOGIN);
    Push(pPlayer);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnRepop(Player* pPlayer)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_REPOP);
    Push(pPlayer);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnResurrect(Player* pPlayer)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_RESURRECT);
    Push(pPlayer);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnQuestAbandon(Player* pPlayer, uint32 questId)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_QUEST_ABANDON);
    Push(pPlayer);
    Push(questId);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnEquip(Player* pPlayer, Item* pItem, uint8 bag, uint8 slot)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_EQUIP);
    Push(pPlayer);
    Push(pItem);
    Push(bag);
    Push(slot);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
InventoryResult ALE::OnCanUseItem(const Player* pPlayer, uint32 itemEntry)
{
    START_HOOK_MAP_WITH_RETVAL(PLAYER_EVENT_ON_CAN_USE_ITEM, EQUIP_ERR_OK);
    InventoryResult result = EQUIP_ERR_OK;
    Push(pPlayer);
    Push(itemEntry);
    int n = SetupStack(PlayerEventBindings, key, 2);

    while (n > 0)
    {
        int r = CallOneFunction(n--, 2, 1);

        if (lua_isnumber(L, r))
            result = (InventoryResult)CHECKVAL<uint32>(L, r);

        lua_pop(L, 1);
    }

    CleanUpStack(2);
    return result;
}

// MAP
void ALE::OnPlayerEnterCombat(Player* pPlayer, Unit* pEnemy)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_ENTER_COMBAT);
    Push(pPlayer);
    Push(pEnemy);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnPlayerLeaveCombat(Player* pPlayer)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_LEAVE_COMBAT);
    Push(pPlayer);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnPVPKill(Player* pKiller, Player* pKilled)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_KILL_PLAYER);
    Push(pKiller);
    Push(pKilled);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnCreatureKill(Player* pKiller, Creature* pKilled)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_KILL_CREATURE);
    Push(pKiller);
    Push(pKilled);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnPlayerKilledByCreature(Creature* pKiller, Player* pKilled)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_KILLED_BY_CREATURE);
    Push(pKiller);
    Push(pKilled);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnLevelChanged(Player* pPlayer, uint8 oldLevel)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_LEVEL_CHANGE);
    Push(pPlayer);
    Push(oldLevel);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnFreeTalentPointsChanged(Player* pPlayer, uint32 newPoints)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_TALENTS_CHANGE);
    Push(pPlayer);
    Push(newPoints);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnTalentsReset(Player* pPlayer, bool noCost)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_TALENTS_RESET);
    Push(pPlayer);
    Push(noCost);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnMoneyChanged(Player* pPlayer, int32& amount)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_MONEY_CHANGE);
    Push(pPlayer);
    Push(amount);
    int amountIndex = lua_gettop(L);
    int n = SetupStack(PlayerEventBindings, key, 2);

    while (n > 0)
    {
        int r = CallOneFunction(n--, 2, 1);

        if (lua_isnumber(L, r))
        {
            amount = CHECKVAL<int32>(L, r);
            ReplaceArgument(amount, amountIndex);
        }

        lua_pop(L, 1);
    }

    CleanUpStack(2);
}

// MAP
void ALE::OnGiveXP(Player* pPlayer, uint32& amount, Unit* pVictim, uint8 xpSource)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_GIVE_XP);
    Push(pPlayer);
    Push(amount);
    Push(pVictim);
    Push(xpSource);
    int amountIndex = lua_gettop(L) - 1;
    int n = SetupStack(PlayerEventBindings, key, 4);

    while (n > 0)
    {
        int r = CallOneFunction(n--, 4, 1);

        if (lua_isnumber(L, r))
        {
            amount = CHECKVAL<uint32>(L, r);
            ReplaceArgument(amount, amountIndex);
        }

        lua_pop(L, 1);
    }

    CleanUpStack(4);
}

// MAP
bool ALE::OnReputationChange(Player* pPlayer, uint32 factionID, int32& standing, bool incremental)
{
    START_HOOK_MAP_WITH_RETVAL(PLAYER_EVENT_ON_REPUTATION_CHANGE, true);
    bool result = true;
    Push(pPlayer);
    Push(factionID);
    Push(standing);
    Push(incremental);
    int standingIndex = lua_gettop(L) - 1;
    int n = SetupStack(PlayerEventBindings, key, 4);

    while (n > 0)
    {
        int r = CallOneFunction(n--, 4, 1);

        if (lua_isnumber(L, r))
        {
            standing = CHECKVAL<int32>(L, r);
            if (standing == -1)
                result = false;
            ReplaceArgument(standing, standingIndex);
        }

        lua_pop(L, 1);
    }

    CleanUpStack(4);
    return result;
}

// MAP
void ALE::OnDuelRequest(Player* pTarget, Player* pChallenger)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_DUEL_REQUEST);
    Push(pTarget);
    Push(pChallenger);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnDuelStart(Player* pStarter, Player* pChallenger)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_DUEL_START);
    Push(pStarter);
    Push(pChallenger);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnDuelEnd(Player* pWinner, Player* pLoser, DuelCompleteType type)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_DUEL_END);
    Push(pWinner);
    Push(pLoser);
    Push(type);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnEmote(Player* pPlayer, uint32 emote)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_EMOTE);
    Push(pPlayer);
    Push(emote);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnTextEmote(Player* pPlayer, uint32 textEmote, uint32 emoteNum, ObjectGuid guid)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_TEXT_EMOTE);
    Push(pPlayer);
    Push(textEmote);
    Push(emoteNum);
    Push(guid);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnPlayerSpellCast(Player* pPlayer, Spell* pSpell, bool skipCheck)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_SPELL_CAST);
    Push(pPlayer);
    Push(pSpell);
    Push(skipCheck);
    CallAllFunctions(PlayerEventBindings, key);
}

// WORLD
void ALE::OnLogin(Player* pPlayer)
{
    START_HOOK_WORLD(PLAYER_EVENT_ON_LOGIN);
    Push(pPlayer);
    CallAllFunctions(PlayerEventBindings, key);
}

// WORLD
void ALE::OnLogout(Player* pPlayer)
{
    START_HOOK_WORLD(PLAYER_EVENT_ON_LOGOUT);
    Push(pPlayer);
    CallAllFunctions(PlayerEventBindings, key);
}

// WORLD
void ALE::OnCreate(Player* pPlayer)
{
    START_HOOK_WORLD(PLAYER_EVENT_ON_CHARACTER_CREATE);
    Push(pPlayer);
    CallAllFunctions(PlayerEventBindings, key);
}

// WORLD
void ALE::OnDelete(uint32 guidlow)
{
    START_HOOK_WORLD(PLAYER_EVENT_ON_CHARACTER_DELETE);
    Push(guidlow);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnSave(Player* pPlayer)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_SAVE);
    Push(pPlayer);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnBindToInstance(Player* pPlayer, Difficulty difficulty, uint32 mapid, bool permanent)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_BIND_TO_INSTANCE);
    Push(pPlayer);
    Push(difficulty);
    Push(mapid);
    Push(permanent);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnUpdateArea(Player* pPlayer, uint32 oldArea, uint32 newArea)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_UPDATE_AREA);
    Push(pPlayer);
    Push(oldArea);
    Push(newArea);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnUpdateZone(Player* pPlayer, uint32 newZone, uint32 newArea)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_UPDATE_ZONE);
    Push(pPlayer);
    Push(newZone);
    Push(newArea);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnMapChanged(Player* player)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_MAP_CHANGE);
    Push(player);
    CallAllFunctions(PlayerEventBindings, key);
}

// WORLD
bool ALE::OnChat(Player* pPlayer, uint32 type, uint32 lang, std::string& msg)
{
    if (lang == LANG_ADDON)
        return OnAddonMessage(pPlayer, type, msg, NULL, NULL, NULL, NULL);

    START_HOOK_WORLD_WITH_RETVAL(PLAYER_EVENT_ON_CHAT, true);
    bool result = true;
    Push(pPlayer);
    Push(msg);
    Push(type);
    Push(lang);
    int n = SetupStack(PlayerEventBindings, key, 4);

    while (n > 0)
    {
        int r = CallOneFunction(n--, 4, 2);

        if (lua_isboolean(L, r + 0) && !lua_toboolean(L, r + 0))
            result = false;

        if (lua_isstring(L, r + 1))
            msg = std::string(lua_tostring(L, r + 1));

        lua_pop(L, 2);
    }

    CleanUpStack(4);
    return result;
}

// WORLD
bool ALE::OnChat(Player* pPlayer, uint32 type, uint32 lang, std::string& msg, Group* pGroup)
{
    if (lang == LANG_ADDON)
        return OnAddonMessage(pPlayer, type, msg, NULL, NULL, pGroup, NULL);

    START_HOOK_WORLD_WITH_RETVAL(PLAYER_EVENT_ON_GROUP_CHAT, true);
    bool result = true;
    Push(pPlayer);
    Push(msg);
    Push(type);
    Push(lang);
    Push(pGroup);
    int n = SetupStack(PlayerEventBindings, key, 5);

    while (n > 0)
    {
        int r = CallOneFunction(n--, 5, 2);

        if (lua_isboolean(L, r + 0) && !lua_toboolean(L, r + 0))
            result = false;

        if (lua_isstring(L, r + 1))
            msg = std::string(lua_tostring(L, r + 1));

        lua_pop(L, 2);
    }

    CleanUpStack(5);
    return result;
}

// WORLD
bool ALE::OnChat(Player* pPlayer, uint32 type, uint32 lang, std::string& msg, Guild* pGuild)
{
    if (lang == LANG_ADDON)
        return OnAddonMessage(pPlayer, type, msg, NULL, pGuild, NULL, NULL);

    START_HOOK_WORLD_WITH_RETVAL(PLAYER_EVENT_ON_GUILD_CHAT, true);
    bool result = true;
    Push(pPlayer);
    Push(msg);
    Push(type);
    Push(lang);
    Push(pGuild);
    int n = SetupStack(PlayerEventBindings, key, 5);

    while (n > 0)
    {
        int r = CallOneFunction(n--, 5, 2);

        if (lua_isboolean(L, r + 0) && !lua_toboolean(L, r + 0))
            result = false;

        if (lua_isstring(L, r + 1))
            msg = std::string(lua_tostring(L, r + 1));

        lua_pop(L, 2);
    }

    CleanUpStack(5);
    return result;
}

// WORLD
bool ALE::OnChat(Player* pPlayer, uint32 type, uint32 lang, std::string& msg, Channel* pChannel)
{
    if (lang == LANG_ADDON)
        return OnAddonMessage(pPlayer, type, msg, NULL, NULL, NULL, pChannel);

    START_HOOK_WORLD_WITH_RETVAL(PLAYER_EVENT_ON_CHANNEL_CHAT, true);
    bool result = true;
    Push(pPlayer);
    Push(msg);
    Push(type);
    Push(lang);
    Push(pChannel->IsConstant() ? static_cast<int32>(pChannel->GetChannelId()) : -static_cast<int32>(pChannel->GetChannelDBId()));
    int n = SetupStack(PlayerEventBindings, key, 5);

    while (n > 0)
    {
        int r = CallOneFunction(n--, 5, 2);

        if (lua_isboolean(L, r + 0) && !lua_toboolean(L, r + 0))
            result = false;

        if (lua_isstring(L, r + 1))
            msg = std::string(lua_tostring(L, r + 1));

        lua_pop(L, 2);
    }

    CleanUpStack(5);
    return result;
}

// WORLD
bool ALE::OnChat(Player* pPlayer, uint32 type, uint32 lang, std::string& msg, Player* pReceiver)
{
    if (lang == LANG_ADDON)
        return OnAddonMessage(pPlayer, type, msg, pReceiver, NULL, NULL, NULL);

    START_HOOK_WORLD_WITH_RETVAL(PLAYER_EVENT_ON_WHISPER, true);
    bool result = true;
    Push(pPlayer);
    Push(msg);
    Push(type);
    Push(lang);
    Push(pReceiver);
    int n = SetupStack(PlayerEventBindings, key, 5);

    while (n > 0)
    {
        int r = CallOneFunction(n--, 5, 2);

        if (lua_isboolean(L, r + 0) && !lua_toboolean(L, r + 0))
            result = false;

        if (lua_isstring(L, r + 1))
            msg = std::string(lua_tostring(L, r + 1));

        lua_pop(L, 2);
    }

    CleanUpStack(5);
    return result;
}

// MAP
void ALE::OnPetAddedToWorld(Player* player, Creature* pet)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_PET_ADDED_TO_WORLD);
    Push(player);
    Push(pet);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnLearnSpell(Player* player, uint32 spellId)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_LEARN_SPELL);
    Push(player);
    Push(spellId);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnAchiComplete(Player* player, AchievementEntry const* achievement)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_ACHIEVEMENT_COMPLETE);
    Push(player);
    Push(achievement);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnFfaPvpStateUpdate(Player* player, bool hasFfaPvp)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_FFAPVP_CHANGE);
    Push(player);
    Push(hasFfaPvp);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
bool ALE::OnCanInitTrade(Player* player, Player* target)
{
    START_HOOK_MAP_WITH_RETVAL(PLAYER_EVENT_ON_CAN_INIT_TRADE, true);
    Push(player);
    Push(target);
    return CallAllFunctionsBool(PlayerEventBindings, key);
}

// MAP
bool ALE::OnCanSendMail(Player* player, ObjectGuid receiverGuid, ObjectGuid mailbox, std::string& subject, std::string& body, uint32 money, uint32 cod, Item* item)
{
    START_HOOK_MAP_WITH_RETVAL(PLAYER_EVENT_ON_CAN_SEND_MAIL, true);
    Push(player);
    Push(receiverGuid);
    Push(mailbox);
    Push(subject);
    Push(body);
    Push(money);
    Push(cod);
    Push(item);
    return CallAllFunctionsBool(PlayerEventBindings, key);
}

// MAP
bool ALE::OnCanJoinLfg(Player* player, uint8 roles, lfg::LfgDungeonSet& dungeons, const std::string& comment)
{
    START_HOOK_MAP_WITH_RETVAL(PLAYER_EVENT_ON_CAN_JOIN_LFG, true);
    Push(player);
    Push(roles);

    lua_newtable(L);
    int table = lua_gettop(L);
    uint32 counter = 1;
    for (uint32 dungeon : dungeons)
    {
        ALE::Push(L, dungeon);
        lua_rawseti(L, table, counter);
        ++counter;
    }
    lua_settop(L, table);
    ++push_counter;

    Push(comment);
    return CallAllFunctionsBool(PlayerEventBindings, key);
}

// MAP
void ALE::OnQuestRewardItem(Player* player, Item* item, uint32 count)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_QUEST_REWARD_ITEM);
    Push(player);
    Push(item);
    Push(count);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnCreateItem(Player* player, Item* item, uint32 count)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_CREATE_ITEM);
    Push(player);
    Push(item);
    Push(count);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnStoreNewItem(Player* player, Item* item, uint32 count)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_STORE_NEW_ITEM);
    Push(player);
    Push(item);
    Push(count);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnPlayerCompleteQuest(Player* player, Quest const* quest)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_COMPLETE_QUEST);
    Push(player);
    Push(quest);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
bool ALE::OnCanGroupInvite(Player* player, std::string& memberName)
{
    START_HOOK_MAP_WITH_RETVAL(PLAYER_EVENT_ON_CAN_GROUP_INVITE, true);
    Push(player);
    Push(memberName);
    return CallAllFunctionsBool(PlayerEventBindings, key);
}

// MAP
void ALE::OnGroupRollRewardItem(Player* player, Item* item, uint32 count, RollVote voteType, Roll* roll)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_GROUP_ROLL_REWARD_ITEM);
    Push(player);
    Push(item);
    Push(count);
    Push(voteType);
    Push(roll);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnBattlegroundDesertion(Player* player, const BattlegroundDesertionType type)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_BG_DESERTION);
    Push(player);
    Push(type);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnCreatureKilledByPet(Player* player, Creature* killed)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_PET_KILL);
    Push(player);
    Push(killed);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
bool ALE::OnPlayerCanUpdateSkill(Player* player, uint32 skill_id)
{
    START_HOOK_MAP_WITH_RETVAL(PLAYER_EVENT_ON_CAN_UPDATE_SKILL, true);
    Push(player);
    Push(skill_id);
    return CallAllFunctionsBool(PlayerEventBindings, key);
}

// MAP
void ALE::OnPlayerBeforeUpdateSkill(Player* player, uint32 skill_id, uint32& value, uint32 max, uint32 step)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_BEFORE_UPDATE_SKILL);
    Push(player);
    Push(skill_id);
    Push(value);
    Push(max);
    Push(step);

    int valueIndex = lua_gettop(L) - 2;
    int n = SetupStack(PlayerEventBindings, key, 5);
    while (n > 0)
    {
        int r = CallOneFunction(n--, 5, 1);
        if (lua_isnumber(L, r))
        {
            value = CHECKVAL<uint32>(L, r);
            ReplaceArgument(value, valueIndex);
        }
        lua_pop(L, 1);
    }

    CleanUpStack(5);
}

// MAP
void ALE::OnPlayerUpdateSkill(Player* player, uint32 skill_id, uint32 value, uint32 max, uint32 step, uint32 new_value)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_UPDATE_SKILL);
    Push(player);
    Push(skill_id);
    Push(value);
    Push(max);
    Push(step);
    Push(new_value);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
bool ALE::CanPlayerResurrect(Player* player)
{
    START_HOOK_MAP_WITH_RETVAL(PLAYER_EVENT_ON_CAN_RESURRECT, true);
    Push(player);
    return CallAllFunctionsBool(PlayerEventBindings, key);
}

// MAP
void ALE::OnPlayerQuestAccept(Player* player, Quest const* quest)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_QUEST_ACCEPT);
    Push(player);
    Push(quest);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnPlayerAuraApply(Player* player, Aura* aura)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_AURA_APPLY);
    Push(player);
    Push(aura);
    CallAllFunctions(PlayerEventBindings, key);
}

// MAP
void ALE::OnPlayerHeal(Player* player, Unit* target, uint32& gain)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_HEAL);
    Push(player);
    Push(target);
    Push(gain);

    int gainIndex = lua_gettop(L);
    int n = SetupStack(PlayerEventBindings, key, 3);
    while (n > 0)
    {
        int r = CallOneFunction(n--, 3, 1);
        if (lua_isnumber(L, r))
        {
            gain = CHECKVAL<uint32>(L, r);
            ReplaceArgument(gain, gainIndex);
        }
        lua_pop(L, 1);
    }

    CleanUpStack(3);
}

// MAP
void ALE::OnPlayerDamage(Player* player, Unit* target, uint32& damage)
{
    START_HOOK_MAP(PLAYER_EVENT_ON_DAMAGE);
    Push(player);
    Push(target);
    Push(damage);

    int damageIndex = lua_gettop(L);
    int n = SetupStack(PlayerEventBindings, key, 3);
    while (n > 0)
    {
        int r = CallOneFunction(n--, 3, 1);
        if (lua_isnumber(L, r))
        {
            damage = CHECKVAL<uint32>(L, r);
            ReplaceArgument(damage, damageIndex);
        }
        lua_pop(L, 1);
    }

    CleanUpStack(3);
}
