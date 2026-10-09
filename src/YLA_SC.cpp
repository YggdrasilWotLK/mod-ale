/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Chat.h"
#include "YLAEventMgr.h"
#include "YlaAlive.h"
#include "Log.h"
#include "LuaEngine.h"
#include "Pet.h"
#include "Player.h"
#include "Vehicle.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "SpellAuras.h"

class YLA_AllCreatureScript : public AllCreatureScript
{
public:
    YLA_AllCreatureScript() : AllCreatureScript("YLA_AllCreatureScript") { }

    bool CanCreatureGossipHello(Player* player, Creature* creature) override
    {
        if (YLA::GetMapStateOrGlobal(creature->GetMapId(), creature->GetInstanceId())->OnGossipHello(player, creature))
            return true;
        return false;
    }

    bool CanCreatureGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (YLA::GetMapStateOrGlobal(creature->GetMapId(), creature->GetInstanceId())->OnGossipSelect(player, creature, sender, action))
            return true;
        return false;
    }

    bool CanCreatureGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, const char* code) override
    {
        if (YLA::GetMapStateOrGlobal(creature->GetMapId(), creature->GetInstanceId())->OnGossipSelectCode(player, creature, sender, action, code))
            return true;
        return false;
    }

    void OnCreatureAddWorld(Creature* creature) override
    {
        auto E = YLA::GetMapStateOrGlobal(creature->GetMapId(), creature->GetInstanceId());
        E->OnAddToWorld(creature);
        E->OnAllCreatureAddToWorld(creature);

        if (creature->IsGuardian() && creature->ToTempSummon() && creature->ToTempSummon()->GetSummonerGUID().IsPlayer())
            E->OnPetAddedToWorld(creature->ToTempSummon()->GetSummonerUnit()->ToPlayer(), creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        auto E = YLA::GetMapStateOrGlobal(creature->GetMapId(), creature->GetInstanceId());
        E->OnRemoveFromWorld(creature);
        E->OnAllCreatureRemoveFromWorld(creature);
    }

    bool CanCreatureQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        auto E = YLA::GetMapStateOrGlobal(creature->GetMapId(), creature->GetInstanceId());
        E->OnPlayerQuestAccept(player, quest);
        E->OnQuestAccept(player, creature, quest);
        return false;
    }

    bool CanCreatureQuestReward(Player* player, Creature* creature, Quest const* quest, uint32 opt) override
    {
        if (YLA::GetMapStateOrGlobal(creature->GetMapId(), creature->GetInstanceId())->OnQuestReward(player, creature, quest, opt))
        {
            ClearGossipMenuFor(player);
            return true;
        }
        return false;
    }

    CreatureAI* GetCreatureAI(Creature* creature) const override
    {
        if (CreatureAI* luaAI = YLA::GetMapStateOrGlobal(creature->GetMapId(), creature->GetInstanceId())->GetAI(creature))
            return luaAI;
        return nullptr;
    }

    void OnCreatureSelectLevel(const CreatureTemplate* cinfo, Creature* creature) override
    {
        YLA::GetMapStateOrGlobal(creature->GetMapId(), creature->GetInstanceId())->OnAllCreatureSelectLevel(cinfo, creature);
    }

    void OnBeforeCreatureSelectLevel(const CreatureTemplate* cinfo, Creature* creature, uint8& level) override
    {
        YLA::GetMapStateOrGlobal(creature->GetMapId(), creature->GetInstanceId())->OnAllCreatureBeforeSelectLevel(cinfo, creature, level);
    }
};

class YLA_AllGameObjectScript : public AllGameObjectScript
{
public:
    YLA_AllGameObjectScript() : AllGameObjectScript("YLA_AllGameObjectScript") { }

    void OnGameObjectAddWorld(GameObject* go) override
    {
        YLA::GetMapStateOrGlobal(go->GetMapId(), go->GetInstanceId())->OnAddToWorld(go);
    }

    void OnGameObjectRemoveWorld(GameObject* go) override
    {
        YLA::GetMapStateOrGlobal(go->GetMapId(), go->GetInstanceId())->OnRemoveFromWorld(go);
    }

    void OnGameObjectUpdate(GameObject* go, uint32 diff) override
    {
        YLA::GetMapStateOrGlobal(go->GetMapId(), go->GetInstanceId())->UpdateAI(go, diff);
    }

    bool CanGameObjectGossipHello(Player* player, GameObject* go) override
    {
        auto E = YLA::GetMapStateOrGlobal(go->GetMapId(), go->GetInstanceId());
        if (E->OnGossipHello(player, go))
            return true;
        if (E->OnGameObjectUse(player, go))
            return true;
        return false;
    }

    void OnGameObjectDamaged(GameObject* go, Player* player) override
    {
        YLA::GetMapStateOrGlobal(go->GetMapId(), go->GetInstanceId())->OnDamaged(go, player);
    }

    void OnGameObjectDestroyed(GameObject* go, Player* player) override
    {
        YLA::GetMapStateOrGlobal(go->GetMapId(), go->GetInstanceId())->OnDestroyed(go, player);
    }

    void OnGameObjectLootStateChanged(GameObject* go, uint32 state, Unit* /*unit*/) override
    {
        YLA::GetMapStateOrGlobal(go->GetMapId(), go->GetInstanceId())->OnLootStateChanged(go, state);
    }

    void OnGameObjectStateChanged(GameObject* go, uint32 state) override
    {
        YLA::GetMapStateOrGlobal(go->GetMapId(), go->GetInstanceId())->OnGameObjectStateChanged(go, state);
    }

    bool CanGameObjectQuestAccept(Player* player, GameObject* go, Quest const* quest) override
    {
        auto E = YLA::GetMapStateOrGlobal(go->GetMapId(), go->GetInstanceId());
        E->OnPlayerQuestAccept(player, quest);
        E->OnQuestAccept(player, go, quest);
        return false;
    }

    bool CanGameObjectGossipSelect(Player* player, GameObject* go, uint32 sender, uint32 action) override
    {
        if (YLA::GetMapStateOrGlobal(go->GetMapId(), go->GetInstanceId())->OnGossipSelect(player, go, sender, action))
            return true;
        return false;
    }

    bool CanGameObjectGossipSelectCode(Player* player, GameObject* go, uint32 sender, uint32 action, const char* code) override
    {
        if (YLA::GetMapStateOrGlobal(go->GetMapId(), go->GetInstanceId())->OnGossipSelectCode(player, go, sender, action, code))
            return true;
        return false;
    }

    bool CanGameObjectQuestReward(Player* player, GameObject* go, Quest const* quest, uint32 opt) override
    {
        auto E = YLA::GetMapStateOrGlobal(go->GetMapId(), go->GetInstanceId());
        if (E->OnQuestAccept(player, go, quest))
        {
            E->OnPlayerQuestAccept(player, quest);
            return false;
        }
        E->OnQuestReward(player, go, quest, opt);
        return false;
    }

    GameObjectAI* GetGameObjectAI(GameObject* go) const override
    {
        YLA::GetMapStateOrGlobal(go->GetMapId(), go->GetInstanceId())->OnSpawn(go);
        return nullptr;
    }
};

class YLA_AllItemScript : public AllItemScript
{
public:
    YLA_AllItemScript() : AllItemScript("YLA_AllItemScript") { }

    bool CanItemQuestAccept(Player* player, Item* item, Quest const* quest) override
    {
        auto E = YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId());
        if (E->OnQuestAccept(player, item, quest))
        {
            E->OnPlayerQuestAccept(player, quest);
            return false;
        }
        return true;
    }

    bool CanItemUse(Player* player, Item* item, SpellCastTargets const& targets) override
    {
        if (!YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnUse(player, item, targets))
            return true;
        return false;
    }

    bool CanItemExpire(Player* player, ItemTemplate const* proto) override
    {
        if (YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnExpire(player, proto))
            return false;
        return true;
    }

    bool CanItemRemove(Player* player, Item* item) override
    {
        if (YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnRemove(player, item))
            return false;
        return true;
    }

    void OnItemGossipSelect(Player* player, Item* item, uint32 sender, uint32 action) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->HandleGossipSelectOption(player, item, sender, action, "");
    }

    void OnItemGossipSelectCode(Player* player, Item* item, uint32 sender, uint32 action, const char* code) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->HandleGossipSelectOption(player, item, sender, action, code);
    }
};

class YLA_AllMapScript : public AllMapScript
{
public:
    YLA_AllMapScript() : AllMapScript("YLA_AllMapScript", {
        ALLMAPHOOK_ON_BEFORE_CREATE_INSTANCE_SCRIPT,
        ALLMAPHOOK_ON_DESTROY_INSTANCE,
        ALLMAPHOOK_ON_CREATE_MAP,
        ALLMAPHOOK_ON_DESTROY_MAP,
        ALLMAPHOOK_ON_PLAYER_ENTER_ALL,
        ALLMAPHOOK_ON_PLAYER_LEAVE_ALL,
        ALLMAPHOOK_ON_MAP_UPDATE
    }) { }

    void OnBeforeCreateInstanceScript(InstanceMap* instanceMap, InstanceScript** instanceData, bool /*load*/, std::string /*data*/, uint32 /*completedEncounterMask*/) override
    {
        if (!YLAConfig::GetInstance().IsCompatibilityModeEnabled())
        {
            if (!YLA::GetMapState(instanceMap->GetId(), instanceMap->GetInstanceId()))
                YLA::CreateMapState(instanceMap->GetId(), instanceMap->GetInstanceId());
        }
        if (instanceData)
            *instanceData = YLA::GetMapStateOrGlobal(instanceMap->GetId(), instanceMap->GetInstanceId())->GetInstanceData(instanceMap);
    }

    void OnDestroyInstance(MapInstanced* /*mapInstanced*/, Map* map) override
    {
        YLA::GetMapStateOrGlobal(map->GetId(), map->GetInstanceId())->FreeInstanceId(map->GetInstanceId());
    }

    void OnCreateMap(Map* map) override
    {
        if (!YLAConfig::GetInstance().IsCompatibilityModeEnabled())
        {
            if (!YLA::GetMapState(map->GetId(), map->GetInstanceId()))
                YLA::CreateMapState(map->GetId(), map->GetInstanceId());
        }
        YLA::GetMapStateOrGlobal(map->GetId(), map->GetInstanceId())->OnCreate(map);
    }

    void OnDestroyMap(Map* map) override
    {
        YLA::GetMapStateOrGlobal(map->GetId(), map->GetInstanceId())->OnDestroy(map);
        YLA::ClearMapData(map->GetId());
        YLA::ClearMapBox(map->GetId(), map->GetInstanceId());
        if (!YLAConfig::GetInstance().IsCompatibilityModeEnabled())
            YLA::DestroyMapState(map->GetId(), map->GetInstanceId());
    }

    void OnPlayerEnterAll(Map* map, Player* player) override
    {
        YLA::GetMapStateOrGlobal(map->GetId(), map->GetInstanceId())->OnPlayerEnter(map, player);
    }

    void OnPlayerLeaveAll(Map* map, Player* player) override
    {
        YLA::GetMapStateOrGlobal(map->GetId(), map->GetInstanceId())->OnPlayerLeave(map, player);
    }

    void OnMapUpdate(Map* map, uint32 diff) override
    {
        YLA::GetMapStateOrGlobal(map->GetId(), map->GetInstanceId())->OnUpdate(map, diff);
    }
};

class YLA_AuctionHouseScript : public AuctionHouseScript
{
public:
    YLA_AuctionHouseScript() : AuctionHouseScript("YLA_AuctionHouseScript", {
        AUCTIONHOUSEHOOK_ON_AUCTION_ADD,
        AUCTIONHOUSEHOOK_ON_AUCTION_REMOVE,
        AUCTIONHOUSEHOOK_ON_AUCTION_SUCCESSFUL,
        AUCTIONHOUSEHOOK_ON_AUCTION_EXPIRE
    }) { }

    void OnAuctionAdd(AuctionHouseObject* ah, AuctionEntry* entry) override
    {
        gALE->OnAdd(ah, entry);
    }

    void OnAuctionRemove(AuctionHouseObject* ah, AuctionEntry* entry) override
    {
        gALE->OnRemove(ah, entry);
    }

    void OnAuctionSuccessful(AuctionHouseObject* ah, AuctionEntry* entry) override
    {
        gALE->OnSuccessful(ah, entry);
    }

    void OnAuctionExpire(AuctionHouseObject* ah, AuctionEntry* entry) override
    {
        gALE->OnExpire(ah, entry);
    }
};

class YLA_BGScript : public BGScript
{
public:
    YLA_BGScript() : BGScript("YLA_BGScript", {
        ALLBATTLEGROUNDHOOK_ON_BATTLEGROUND_START,
        ALLBATTLEGROUNDHOOK_ON_BATTLEGROUND_END,
        ALLBATTLEGROUNDHOOK_ON_BATTLEGROUND_DESTROY,
        ALLBATTLEGROUNDHOOK_ON_BATTLEGROUND_CREATE
    }) { }

    void OnBattlegroundStart(Battleground* bg) override
    {
        YLA::GetMapStateOrGlobal(bg->GetMapId(), bg->GetInstanceID())->OnBGStart(bg, bg->GetBgTypeID(), bg->GetInstanceID());
    }

    void OnBattlegroundEnd(Battleground* bg, TeamId winnerTeam) override
    {
        YLA::GetMapStateOrGlobal(bg->GetMapId(), bg->GetInstanceID())->OnBGEnd(bg, bg->GetBgTypeID(), bg->GetInstanceID(), winnerTeam);
    }

    void OnBattlegroundDestroy(Battleground* bg) override
    {
        YLA::GetMapStateOrGlobal(bg->GetMapId(), bg->GetInstanceID())->OnBGDestroy(bg, bg->GetBgTypeID(), bg->GetInstanceID());
    }

    void OnBattlegroundCreate(Battleground* bg) override
    {
        YLA::GetMapStateOrGlobal(bg->GetMapId(), bg->GetInstanceID())->OnBGCreate(bg, bg->GetBgTypeID(), bg->GetInstanceID());
    }
};

class YLA_CommandSC : public CommandSC
{
public:
    YLA_CommandSC() : CommandSC("YLA_CommandSC", {
        ALLCOMMANDHOOK_ON_TRY_EXECUTE_COMMAND
    }) { }

    bool OnTryExecuteCommand(ChatHandler& handler, std::string_view cmdStr) override
    {
        if (!gALE->OnCommand(handler, std::string(cmdStr).c_str()))
            return false;
        return true;
    }
};

class YLA_YLAScript : public YLAScript
{
public:
    YLA_YLAScript() : YLAScript("YLA_YLAScript") { }

    void OnWeatherChange(Weather* weather, WeatherState state, float grade) override
    {
        gALE->OnChange(weather, weather->GetZone(), state, grade);
    }

    bool CanAreaTrigger(Player* player, AreaTrigger const* trigger) override
    {
        if (YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnAreaTrigger(player, trigger))
            return true;
        return false;
    }
};

class YLA_GameEventScript : public GameEventScript
{
public:
    YLA_GameEventScript() : GameEventScript("YLA_GameEventScript", {
        GAMEEVENTHOOK_ON_START,
        GAMEEVENTHOOK_ON_STOP
    }) { }

    void OnStart(uint16 eventID) override
    {
        gALE->OnGameEventStart(eventID);
    }

    void OnStop(uint16 eventID) override
    {
        gALE->OnGameEventStop(eventID);
    }
};

class YLA_GroupScript : public GroupScript
{
public:
    YLA_GroupScript() : GroupScript("YLA_GroupScript", {
        GROUPHOOK_ON_ADD_MEMBER,
        GROUPHOOK_ON_INVITE_MEMBER,
        GROUPHOOK_ON_REMOVE_MEMBER,
        GROUPHOOK_ON_CHANGE_LEADER,
        GROUPHOOK_ON_DISBAND,
        GROUPHOOK_ON_CREATE
    }) { }

    void OnAddMember(Group* group, ObjectGuid guid) override
    {
        gALE->OnAddMember(group, guid);
    }

    void OnInviteMember(Group* group, ObjectGuid guid) override
    {
        gALE->OnInviteMember(group, guid);
    }

    void OnRemoveMember(Group* group, ObjectGuid guid, RemoveMethod method, ObjectGuid /* kicker */, const char* /* reason */) override
    {
        gALE->OnRemoveMember(group, guid, method);
    }

    void OnChangeLeader(Group* group, ObjectGuid newLeaderGuid, ObjectGuid oldLeaderGuid) override
    {
        gALE->OnChangeLeader(group, newLeaderGuid, oldLeaderGuid);
    }

    void OnDisband(Group* group) override
    {
        gALE->OnDisband(group);
    }

    void OnCreate(Group* group, Player* leader) override
    {
        gALE->OnCreate(group, leader->GetGUID(), group->GetGroupType());
    }
};

class YLA_GuildScript : public GuildScript
{
public:
    YLA_GuildScript() : GuildScript("YLA_GuildScript", {
        GUILDHOOK_ON_ADD_MEMBER,
        GUILDHOOK_ON_REMOVE_MEMBER,
        GUILDHOOK_ON_MOTD_CHANGED,
        GUILDHOOK_ON_INFO_CHANGED,
        GUILDHOOK_ON_CREATE,
        GUILDHOOK_ON_DISBAND,
        GUILDHOOK_ON_MEMBER_WITDRAW_MONEY,
        GUILDHOOK_ON_MEMBER_DEPOSIT_MONEY,
        GUILDHOOK_ON_ITEM_MOVE,
        GUILDHOOK_ON_EVENT,
        GUILDHOOK_ON_BANK_EVENT
    }) { }

    void OnAddMember(Guild* guild, Player* player, uint8& plRank) override
    {
        gALE->OnAddMember(guild, player, plRank);
    }

    void OnRemoveMember(Guild* guild, Player* player, bool isDisbanding, bool /*isKicked*/) override
    {
        gALE->OnRemoveMember(guild, player, isDisbanding);
    }

    void OnMOTDChanged(Guild* guild, const std::string& newMotd) override
    {
        gALE->OnMOTDChanged(guild, newMotd);
    }

    void OnInfoChanged(Guild* guild, const std::string& newInfo) override
    {
        gALE->OnInfoChanged(guild, newInfo);
    }

    void OnCreate(Guild* guild, Player* leader, const std::string& name) override
    {
        gALE->OnCreate(guild, leader, name);
    }

    void OnDisband(Guild* guild) override
    {
        gALE->OnDisband(guild);
    }

    void OnMemberWitdrawMoney(Guild* guild, Player* player, uint32& amount, bool isRepair) override
    {
        gALE->OnMemberWitdrawMoney(guild, player, amount, isRepair);
    }

    void OnMemberDepositMoney(Guild* guild, Player* player, uint32& amount) override
    {
        gALE->OnMemberDepositMoney(guild, player, amount);
    }

    void OnItemMove(Guild* guild, Player* player, Item* pItem, bool isSrcBank, uint8 srcContainer, uint8 srcSlotId,
        bool isDestBank, uint8 destContainer, uint8 destSlotId) override
    {
        gALE->OnItemMove(guild, player, pItem, isSrcBank, srcContainer, srcSlotId, isDestBank, destContainer, destSlotId);
    }

    void OnEvent(Guild* guild, uint8 eventType, ObjectGuid::LowType playerGuid1, ObjectGuid::LowType playerGuid2, uint8 newRank) override
    {
        gALE->OnEvent(guild, eventType, playerGuid1, playerGuid2, newRank);
    }

    void OnBankEvent(Guild* guild, uint8 eventType, uint8 tabId, ObjectGuid::LowType playerGuid, uint32 itemOrMoney, uint16 itemStackCount, uint8 destTabId) override
    {
        gALE->OnBankEvent(guild, eventType, tabId, playerGuid, itemOrMoney, itemStackCount, destTabId);
    }
};

class YLA_LootScript : public LootScript
{
public:
    YLA_LootScript() : LootScript("YLA_LootScript", {
        LOOTHOOK_ON_LOOT_MONEY
    }) { }

    void OnLootMoney(Player* player, uint32 gold) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnLootMoney(player, gold);
    }
};

class YLA_MiscScript : public MiscScript
{
public:
    YLA_MiscScript() : MiscScript("YLA_MiscScript", {
        MISCHOOK_GET_DIALOG_STATUS
    }) { }

    void GetDialogStatus(Player* player, Object* questgiver) override
    {
        auto E = YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId());
        if (questgiver->GetTypeId() == TYPEID_GAMEOBJECT)
            E->GetDialogStatus(player, questgiver->ToGameObject());
        else if (questgiver->GetTypeId() == TYPEID_UNIT)
            E->GetDialogStatus(player, questgiver->ToCreature());
    }
};

class YLA_PetScript : public PetScript
{
public:
    YLA_PetScript() : PetScript("YLA_PetScript", {
        PETHOOK_ON_PET_ADD_TO_WORLD
    }) { }

    void OnPetAddToWorld(Pet* pet) override
    {
        YLA::GetMapStateOrGlobal(pet->GetMapId(), pet->GetInstanceId())->OnPetAddedToWorld(pet->GetOwner(), pet);
    }
};

class YLA_PlayerScript : public PlayerScript
{
public:
    YLA_PlayerScript() : PlayerScript("YLA_PlayerScript", {
        PLAYERHOOK_ON_PLAYER_RESURRECT,
        PLAYERHOOK_CAN_PLAYER_USE_CHAT,
        PLAYERHOOK_CAN_PLAYER_USE_PRIVATE_CHAT,
        PLAYERHOOK_CAN_PLAYER_USE_GROUP_CHAT,
        PLAYERHOOK_CAN_PLAYER_USE_GUILD_CHAT,
        PLAYERHOOK_CAN_PLAYER_USE_CHANNEL_CHAT,
        PLAYERHOOK_ON_LOOT_ITEM,
        PLAYERHOOK_ON_PLAYER_LEARN_TALENTS,
        PLAYERHOOK_CAN_USE_ITEM,
        PLAYERHOOK_ON_EQUIP,
        PLAYERHOOK_ON_PLAYER_ENTER_COMBAT,
        PLAYERHOOK_ON_PLAYER_LEAVE_COMBAT,
        PLAYERHOOK_CAN_REPOP_AT_GRAVEYARD,
        PLAYERHOOK_ON_QUEST_ABANDON,
        PLAYERHOOK_ON_MAP_CHANGED,
        PLAYERHOOK_ON_GOSSIP_SELECT,
        PLAYERHOOK_ON_GOSSIP_SELECT_CODE,
        PLAYERHOOK_ON_PVP_KILL,
        PLAYERHOOK_ON_CREATURE_KILL,
        PLAYERHOOK_ON_PLAYER_KILLED_BY_CREATURE,
        PLAYERHOOK_ON_LEVEL_CHANGED,
        PLAYERHOOK_ON_FREE_TALENT_POINTS_CHANGED,
        PLAYERHOOK_ON_TALENTS_RESET,
        PLAYERHOOK_ON_MONEY_CHANGED,
        PLAYERHOOK_ON_GIVE_EXP,
        PLAYERHOOK_ON_REPUTATION_CHANGE,
        PLAYERHOOK_ON_DUEL_REQUEST,
        PLAYERHOOK_ON_DUEL_START,
        PLAYERHOOK_ON_DUEL_END,
        PLAYERHOOK_ON_EMOTE,
        PLAYERHOOK_ON_TEXT_EMOTE,
        PLAYERHOOK_ON_SPELL_CAST,
        PLAYERHOOK_ON_LOGIN,
        PLAYERHOOK_ON_LOGOUT,
        PLAYERHOOK_ON_CREATE,
        PLAYERHOOK_ON_SAVE,
        PLAYERHOOK_ON_DELETE,
        PLAYERHOOK_ON_BIND_TO_INSTANCE,
        PLAYERHOOK_ON_UPDATE_AREA,
        PLAYERHOOK_ON_UPDATE_ZONE,
        PLAYERHOOK_ON_FIRST_LOGIN,
        PLAYERHOOK_ON_LEARN_SPELL,
        PLAYERHOOK_ON_ACHI_COMPLETE,
        PLAYERHOOK_ON_FFA_PVP_STATE_UPDATE,
        PLAYERHOOK_CAN_INIT_TRADE,
        PLAYERHOOK_CAN_SEND_MAIL,
        PLAYERHOOK_CAN_JOIN_LFG,
        PLAYERHOOK_ON_QUEST_REWARD_ITEM,
        PLAYERHOOK_ON_GROUP_ROLL_REWARD_ITEM,
        PLAYERHOOK_ON_CREATE_ITEM,
        PLAYERHOOK_ON_STORE_NEW_ITEM,
        PLAYERHOOK_ON_PLAYER_COMPLETE_QUEST,
        PLAYERHOOK_CAN_GROUP_INVITE,
        PLAYERHOOK_ON_BATTLEGROUND_DESERTION,
        PLAYERHOOK_ON_CREATURE_KILLED_BY_PET,
        PLAYERHOOK_ON_CAN_UPDATE_SKILL,
        PLAYERHOOK_ON_BEFORE_UPDATE_SKILL,
        PLAYERHOOK_ON_UPDATE_SKILL,
        PLAYERHOOK_CAN_RESURRECT
    }) { }

    // MAP
    void OnPlayerResurrect(Player* player, float /*restore_percent*/, bool /*applySickness*/) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnResurrect(player);
    }

    // WORLD
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 lang, std::string& msg) override
    {
        if (type != CHAT_MSG_SAY && type != CHAT_MSG_YELL && type != CHAT_MSG_EMOTE)
            return true;
        if (!gALE->OnChat(player, type, lang, msg))
            return false;
        return true;
    }

    // WORLD
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 lang, std::string& msg, Player* target) override
    {
        if (!gALE->OnChat(player, type, lang, msg, target))
            return false;
        return true;
    }

    // WORLD
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 lang, std::string& msg, Group* group) override
    {
        if (!gALE->OnChat(player, type, lang, msg, group))
            return false;
        return true;
    }

    // WORLD
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 lang, std::string& msg, Guild* guild) override
    {
        if (!gALE->OnChat(player, type, lang, msg, guild))
            return false;
        return true;
    }

    // WORLD
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 lang, std::string& msg, Channel* channel) override
    {
        if (!gALE->OnChat(player, type, lang, msg, channel))
            return false;
        return true;
    }

    // MAP
    void OnPlayerLootItem(Player* player, Item* item, uint32 count, ObjectGuid lootguid) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnLootItem(player, item, count, lootguid);
    }

    // MAP
    void OnPlayerLearnTalents(Player* player, uint32 talentId, uint32 talentRank, uint32 spellid) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnLearnTalents(player, talentId, talentRank, spellid);
    }

    // MAP
    bool OnPlayerCanUseItem(Player* player, ItemTemplate const* proto, InventoryResult& result) override
    {
        result = YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnCanUseItem(player, proto->ItemId);
        return result != EQUIP_ERR_OK ? false : true;
    }

    // MAP
    void OnPlayerEquip(Player* player, Item* it, uint8 bag, uint8 slot, bool /*update*/) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnEquip(player, it, bag, slot);
    }

    // MAP
    void OnPlayerEnterCombat(Player* player, Unit* enemy) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnPlayerEnterCombat(player, enemy);
    }

    // MAP
    void OnPlayerLeaveCombat(Player* player) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnPlayerLeaveCombat(player);
    }

    // MAP
    bool OnPlayerCanRepopAtGraveyard(Player* player) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnRepop(player);
        return true;
    }

    // MAP
    void OnPlayerQuestAbandon(Player* player, uint32 questId) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnQuestAbandon(player, questId);
    }

    // MAP
    void OnPlayerMapChanged(Player* player) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnMapChanged(player);
    }

    // MAP
    void OnPlayerGossipSelect(Player* player, uint32 menu_id, uint32 sender, uint32 action) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->HandleGossipSelectOption(player, menu_id, sender, action, "");
    }

    // MAP
    void OnPlayerGossipSelectCode(Player* player, uint32 menu_id, uint32 sender, uint32 action, const char* code) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->HandleGossipSelectOption(player, menu_id, sender, action, code);
    }

    // MAP
    void OnPlayerPVPKill(Player* killer, Player* killed) override
    {
        YLA::GetMapStateOrGlobal(killer->GetMapId(), killer->GetInstanceId())->OnPVPKill(killer, killed);
    }

    // MAP
    void OnPlayerCreatureKill(Player* killer, Creature* killed) override
    {
        YLA::GetMapStateOrGlobal(killer->GetMapId(), killer->GetInstanceId())->OnCreatureKill(killer, killed);
    }

    // MAP
    void OnPlayerKilledByCreature(Creature* killer, Player* killed) override
    {
        YLA::GetMapStateOrGlobal(killer->GetMapId(), killer->GetInstanceId())->OnPlayerKilledByCreature(killer, killed);
    }

    // MAP
    void OnPlayerLevelChanged(Player* player, uint8 oldLevel) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnLevelChanged(player, oldLevel);
    }

    // MAP
    void OnPlayerFreeTalentPointsChanged(Player* player, uint32 points) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnFreeTalentPointsChanged(player, points);
    }

    // MAP
    void OnPlayerTalentsReset(Player* player, bool noCost) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnTalentsReset(player, noCost);
    }

    // MAP
    void OnPlayerMoneyChanged(Player* player, int32& amount) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnMoneyChanged(player, amount);
    }

    // MAP
    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 xpSource) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnGiveXP(player, amount, victim, xpSource);
    }

    // MAP
    bool OnPlayerReputationChange(Player* player, uint32 factionID, int32& standing, bool incremental) override
    {
        return YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnReputationChange(player, factionID, standing, incremental);
    }

    // MAP
    void OnPlayerDuelRequest(Player* target, Player* challenger) override
    {
        YLA::GetMapStateOrGlobal(challenger->GetMapId(), challenger->GetInstanceId())->OnDuelRequest(target, challenger);
    }

    // MAP
    void OnPlayerDuelStart(Player* player1, Player* player2) override
    {
        YLA::GetMapStateOrGlobal(player1->GetMapId(), player1->GetInstanceId())->OnDuelStart(player1, player2);
    }

    // MAP
    void OnPlayerDuelEnd(Player* winner, Player* loser, DuelCompleteType type) override
    {
        YLA::GetMapStateOrGlobal(winner->GetMapId(), winner->GetInstanceId())->OnDuelEnd(winner, loser, type);
    }

    // MAP
    void OnPlayerEmote(Player* player, uint32 emote) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnEmote(player, emote);
    }

    // MAP
    void OnPlayerTextEmote(Player* player, uint32 textEmote, uint32 emoteNum, ObjectGuid guid) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnTextEmote(player, textEmote, emoteNum, guid);
    }

    // MAP
    void OnPlayerSpellCast(Player* player, Spell* spell, bool skipCheck) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnPlayerSpellCast(player, spell, skipCheck);
    }

    // WORLD
    void OnPlayerLogin(Player* player) override
    {
        if (player->YLAEvents)
            player->YLAEvents->CaptureGuid();
        gALE->OnLogin(player);
    }

    // WORLD
    void OnPlayerLogout(Player* player) override
    {
        gALE->OnLogout(player);
    }

    // WORLD
    void OnPlayerCreate(Player* player) override
    {
        gALE->OnCreate(player);
    }

    // WORLD
    void OnPlayerSave(Player* player) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnSave(player);
    }

    // WORLD
    void OnPlayerDelete(ObjectGuid guid, uint32 /*accountId*/) override
    {
        gALE->OnDelete(guid.GetCounter());
    }

    // MAP
    void OnPlayerBindToInstance(Player* player, Difficulty difficulty, uint32 mapid, bool permanent) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnBindToInstance(player, difficulty, mapid, permanent);
    }

    // MAP
    void OnPlayerUpdateArea(Player* player, uint32 oldArea, uint32 newArea) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnUpdateArea(player, oldArea, newArea);
    }

    // MAP
    void OnPlayerUpdateZone(Player* player, uint32 newZone, uint32 newArea) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnUpdateZone(player, newZone, newArea);
    }

    // WORLD
    void OnPlayerFirstLogin(Player* player) override
    {
        gALE->OnFirstLogin(player);
    }

    // MAP
    void OnPlayerLearnSpell(Player* player, uint32 spellId) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnLearnSpell(player, spellId);
    }

    // MAP
    void OnPlayerAchievementComplete(Player* player, AchievementEntry const* achievement) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnAchiComplete(player, achievement);
    }

    // MAP
    void OnPlayerFfaPvpStateUpdate(Player* player, bool IsFlaggedForFfaPvp) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnFfaPvpStateUpdate(player, IsFlaggedForFfaPvp);
    }

    // MAP
    bool OnPlayerCanInitTrade(Player* player, Player* target) override
    {
        return YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnCanInitTrade(player, target);
    }

    // MAP
    bool OnPlayerCanSendMail(Player* player, ObjectGuid receiverGuid, ObjectGuid mailbox, std::string& subject, std::string& body, uint32 money, uint32 cod, Item* item) override
    {
        return YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnCanSendMail(player, receiverGuid, mailbox, subject, body, money, cod, item);
    }

    // MAP
    bool OnPlayerCanJoinLfg(Player* player, uint8 roles, lfg::LfgDungeonSet& dungeons, const std::string& comment) override
    {
        return YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnCanJoinLfg(player, roles, dungeons, comment);
    }

    // MAP
    void OnPlayerQuestRewardItem(Player* player, Item* item, uint32 count) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnQuestRewardItem(player, item, count);
    }

    // MAP
    void OnPlayerGroupRollRewardItem(Player* player, Item* item, uint32 count, RollVote voteType, Roll* roll) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnGroupRollRewardItem(player, item, count, voteType, roll);
    }

    // MAP
    void OnPlayerCreateItem(Player* player, Item* item, uint32 count) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnCreateItem(player, item, count);
    }

    // MAP
    void OnPlayerStoreNewItem(Player* player, Item* item, uint32 count) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnStoreNewItem(player, item, count);
    }

    // MAP
    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnPlayerCompleteQuest(player, quest);
    }

    // MAP
    bool OnPlayerCanGroupInvite(Player* player, std::string& memberName) override
    {
        return YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnCanGroupInvite(player, memberName);
    }

    // MAP
    void OnPlayerBattlegroundDesertion(Player* player, const BattlegroundDesertionType type) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnBattlegroundDesertion(player, type);
    }

    // MAP
    void OnPlayerCreatureKilledByPet(Player* player, Creature* killed) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnCreatureKilledByPet(player, killed);
    }

    // MAP
    bool OnPlayerCanUpdateSkill(Player* player, uint32 skill_id) override
    {
        return YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnPlayerCanUpdateSkill(player, skill_id);
    }

    // MAP
    void OnPlayerBeforeUpdateSkill(Player* player, uint32 skill_id, uint32& value, uint32 max, uint32 step) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnPlayerBeforeUpdateSkill(player, skill_id, value, max, step);
    }

    // MAP
    void OnPlayerUpdateSkill(Player* player, uint32 skill_id, uint32 value, uint32 max, uint32 step, uint32 new_value) override
    {
        YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->OnPlayerUpdateSkill(player, skill_id, value, max, step, new_value);
    }

    // MAP
    bool OnPlayerCanResurrect(Player* player) override
    {
        return YLA::GetMapStateOrGlobal(player->GetMapId(), player->GetInstanceId())->CanPlayerResurrect(player);
    }
};

class YLA_ServerScript : public ServerScript
{
public:
    YLA_ServerScript() : ServerScript("YLA_ServerScript", {
        SERVERHOOK_CAN_PACKET_SEND,
        SERVERHOOK_CAN_PACKET_RECEIVE
    }) { }

    bool CanPacketSend(WorldSession* session, WorldPacket& packet) override
    {
        if (!gALE->OnPacketSend(session, packet))
            return false;
        return true;
    }

    bool CanPacketReceive(WorldSession* session, WorldPacket& packet) override
    {
        if (!gALE->OnPacketReceive(session, packet))
            return false;
        return true;
    }
};

class YLA_SpellSC : public SpellSC
{
public:
    YLA_SpellSC() : SpellSC("YLA_SpellSC", {
        ALLSPELLHOOK_ON_DUMMY_EFFECT_GAMEOBJECT,
        ALLSPELLHOOK_ON_DUMMY_EFFECT_CREATURE,
        ALLSPELLHOOK_ON_DUMMY_EFFECT_ITEM,
        ALLSPELLHOOK_ON_CAST_CANCEL,
        ALLSPELLHOOK_ON_CAST,
        ALLSPELLHOOK_ON_PREPARE
    }) { }

    void OnDummyEffect(WorldObject* caster, uint32 spellID, SpellEffIndex effIndex, GameObject* gameObjTarget) override
    {
        YLA::GetMapStateOrGlobal(caster->GetMapId(), caster->GetInstanceId())->OnDummyEffect(caster, spellID, effIndex, gameObjTarget);
    }

    void OnDummyEffect(WorldObject* caster, uint32 spellID, SpellEffIndex effIndex, Creature* creatureTarget) override
    {
        YLA::GetMapStateOrGlobal(caster->GetMapId(), caster->GetInstanceId())->OnDummyEffect(caster, spellID, effIndex, creatureTarget);
    }

    void OnDummyEffect(WorldObject* caster, uint32 spellID, SpellEffIndex effIndex, Item* itemTarget) override
    {
        YLA::GetMapStateOrGlobal(caster->GetMapId(), caster->GetInstanceId())->OnDummyEffect(caster, spellID, effIndex, itemTarget);
    }

    void OnSpellCastCancel(Spell* spell, Unit* caster, SpellInfo const* spellInfo, bool bySelf) override
    {
        YLA::GetMapStateOrGlobal(caster->GetMapId(), caster->GetInstanceId())->OnSpellCastCancel(caster, spell, spellInfo, bySelf);
    }

    void OnSpellCast(Spell* spell, Unit* caster, SpellInfo const* spellInfo, bool skipCheck) override
    {
        YLA::GetMapStateOrGlobal(caster->GetMapId(), caster->GetInstanceId())->OnSpellCast(caster, spell, spellInfo, skipCheck);
    }

    void OnSpellPrepare(Spell* spell, Unit* caster, SpellInfo const* spellInfo) override
    {
        YLA::GetMapStateOrGlobal(caster->GetMapId(), caster->GetInstanceId())->OnSpellPrepare(caster, spell, spellInfo);
    }
};

class YLA_VehicleScript : public VehicleScript
{
public:
    YLA_VehicleScript() : VehicleScript("YLA_VehicleScript") { }

    void OnInstall(Vehicle* veh) override
    {
        YLA::GetMapStateOrGlobal(veh->GetBase()->GetMapId(), veh->GetBase()->GetInstanceId())->OnInstall(veh);
    }

    void OnUninstall(Vehicle* veh) override
    {
        YLA::GetMapStateOrGlobal(veh->GetBase()->GetMapId(), veh->GetBase()->GetInstanceId())->OnUninstall(veh);
    }

    void OnInstallAccessory(Vehicle* veh, Creature* accessory) override
    {
        YLA::GetMapStateOrGlobal(veh->GetBase()->GetMapId(), veh->GetBase()->GetInstanceId())->OnInstallAccessory(veh, accessory);
    }

    void OnAddPassenger(Vehicle* veh, Unit* passenger, int8 seatId) override
    {
        YLA::GetMapStateOrGlobal(veh->GetBase()->GetMapId(), veh->GetBase()->GetInstanceId())->OnAddPassenger(veh, passenger, seatId);
    }

    void OnRemovePassenger(Vehicle* veh, Unit* passenger) override
    {
        YLA::GetMapStateOrGlobal(veh->GetBase()->GetMapId(), veh->GetBase()->GetInstanceId())->OnRemovePassenger(veh, passenger);
    }
};

class YLA_WorldObjectScript : public WorldObjectScript
{
public:
    YLA_WorldObjectScript() : WorldObjectScript("YLA_WorldObjectScript", {
        WORLDOBJECTHOOK_ON_WORLD_OBJECT_DESTROY,
        WORLDOBJECTHOOK_ON_WORLD_OBJECT_CREATE,
        WORLDOBJECTHOOK_ON_WORLD_OBJECT_SET_MAP,
        WORLDOBJECTHOOK_ON_WORLD_OBJECT_UPDATE
    }) { }

    void OnWorldObjectDestroy(WorldObject* object) override
    {
        YLA::ClearObjectData(object->GetGUID());
        // Players-only: GUID and type are still valid inside ~WorldObject.
        if (object->GetTypeId() == TYPEID_PLAYER)
            YlaAlive::Erase(object->GetGUID(), object);
        if (object->YLAEvents)
        {
            delete object->YLAEvents;
            object->YLAEvents = nullptr;
        }
    }
    
    void OnWorldObjectCreate(WorldObject* object) override
    {
        object->YLAEvents = nullptr;
    }

    void OnWorldObjectSetMap(WorldObject* object, Map* map) override
    {
        if (!object->YLAEvents)
        {
            if (!YLAConfig::GetInstance().IsCompatibilityModeEnabled())
            {
                if (auto state = YLA::GetMapState(map->GetId(), map->GetInstanceId()))
                    object->YLAEvents = new YLAEventProcessor(state->GetSelfRef(), state, object);
                else if (YLA::GALE)
                    object->YLAEvents = new YLAEventProcessor(YLA::GALE->GetSelfRef(), YLA::OwningRef(YLA::GALE), object);
                else
                    object->YLAEvents = new YLAEventProcessor(YlaStateRef(), nullptr, object);
            }
            else if (YLA::GALE)
            {
                object->YLAEvents = new YLAEventProcessor(YLA::GALE->GetSelfRef(), YLA::OwningRef(YLA::GALE), object);
            }
            else
            {
                object->YLAEvents = new YLAEventProcessor(YlaStateRef(), nullptr, object);
            }
        }
        // Players-only: OnWorldObjectCreate fires in the WorldObject base
        // ctor before the type is set, so filter here instead.
        if (object->GetTypeId() == TYPEID_PLAYER)
            YlaAlive::Insert(object->GetGUID(), object);
    }

    void OnWorldObjectUpdate(WorldObject* object, uint32 diff) override
    {
        if (object->YLAEvents)
            object->YLAEvents->Update(diff);
    }
};

class YLA_WorldScript : public WorldScript
{
public:
    YLA_WorldScript() : WorldScript("YLA_WorldScript", {
        WORLDHOOK_ON_OPEN_STATE_CHANGE,
        WORLDHOOK_ON_BEFORE_CONFIG_LOAD,
        WORLDHOOK_ON_AFTER_CONFIG_LOAD,
        WORLDHOOK_ON_SHUTDOWN_INITIATE,
        WORLDHOOK_ON_SHUTDOWN_CANCEL,
        WORLDHOOK_ON_UPDATE,
        WORLDHOOK_ON_STARTUP,
        WORLDHOOK_ON_SHUTDOWN,
        WORLDHOOK_ON_AFTER_UNLOAD_ALL_MAPS,
        WORLDHOOK_ON_BEFORE_WORLD_INITIALIZED
    }) { }

    void OnOpenStateChange(bool open) override
    {
        gALE->OnOpenStateChange(open);
    }

    void OnBeforeConfigLoad(bool reload) override
    {
        YLAConfig::GetInstance().Initialize(reload);
        if (!reload)
        {
            ///- Initialize Lua Engine
            LOG_INFO("YLA", "Initialize YLA Lua Engine...");
            YLA::Initialize();
        }

        gALE->OnConfigLoad(reload, true);
    }

    void OnAfterConfigLoad(bool reload) override
    {
        gALE->OnConfigLoad(reload, false);
    }

    void OnShutdownInitiate(ShutdownExitCode code, ShutdownMask mask) override
    {
        gALE->OnShutdownInitiate(code, mask);
    }

    void OnShutdownCancel() override
    {
        gALE->OnShutdownCancel();
    }

    void OnUpdate(uint32 diff) override
    {
        gALE->OnWorldUpdate(diff);
    }

    void OnStartup() override
    {
        gALE->OnStartup();
    }

    void OnShutdown() override
    {
        gALE->OnShutdown();
    }

    void OnAfterUnloadAllMaps() override
    {
        YLA::Uninitialize();
    }

    void OnBeforeWorldInitialized() override
    {
        ///- Run YLA scripts.
        // in multithread foreach: run scripts
        gALE->RunScripts();
        YLA::RunScriptsOnAllMapStates();
        gALE->OnConfigLoad(false, false); // Must be done after YLA is initialized and scripts have run.
    }
};

class YLA_TicketScript : public TicketScript
{
public:
    YLA_TicketScript() : TicketScript("YLA_TicketScript", {
        TICKETHOOK_ON_TICKET_CREATE,
        TICKETHOOK_ON_TICKET_UPDATE_LAST_CHANGE,
        TICKETHOOK_ON_TICKET_CLOSE,
        TICKETHOOK_ON_TICKET_RESOLVE
    }) { }

    void OnTicketCreate(GmTicket* ticket) override
    {
        gALE->OnTicketCreate(ticket);
    }

    void OnTicketUpdateLastChange(GmTicket* ticket) override
    {
        gALE->OnTicketUpdateLastChange(ticket);
    }

    void OnTicketClose(GmTicket* ticket) override
    {
        gALE->OnTicketClose(ticket);
    }

    void OnTicketResolve(GmTicket* ticket) override
    {
        gALE->OnTicketResolve(ticket);
    }
};

class YLA_UnitScript : public UnitScript
{
public:
    YLA_UnitScript() : UnitScript("YLA_UnitScript") { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!unit || !aura) return;
        auto E = YLA::GetMapStateOrGlobal(unit->GetMapId(), unit->GetInstanceId());
        if (unit->IsPlayer())
            E->OnPlayerAuraApply(unit->ToPlayer(), aura);
        if (unit->IsCreature())
            E->OnCreatureAuraApply(unit->ToCreature(), aura);
        E->OnSpellAuraApply(unit, aura);
    }

    void OnAuraRemove(Unit* unit, AuraApplication* aurApp, AuraRemoveMode mode) override
    {
        if (!unit || !aurApp || !aurApp->GetBase()) return;
        auto E = YLA::GetMapStateOrGlobal(unit->GetMapId(), unit->GetInstanceId());
        E->OnSpellAuraRemove(unit, aurApp->GetBase(), static_cast<uint8>(mode));
    }

    void OnHeal(Unit* healer, Unit* receiver, uint32& gain) override
    {
        if (!receiver || !healer) return;
        auto E = YLA::GetMapStateOrGlobal(healer->GetMapId(), healer->GetInstanceId());
        if (healer->IsPlayer())
            E->OnPlayerHeal(healer->ToPlayer(), receiver, gain);
        if (healer->IsCreature())
            E->OnCreatureHeal(healer->ToCreature(), receiver, gain);
    }

    void OnDamage(Unit* attacker, Unit* receiver, uint32& damage) override
    {
        if (!attacker || !receiver) return;
        auto E = YLA::GetMapStateOrGlobal(attacker->GetMapId(), attacker->GetInstanceId());
        if (attacker->IsPlayer())
            E->OnPlayerDamage(attacker->ToPlayer(), receiver, damage);
        if (attacker->IsCreature())
            E->OnCreatureDamage(attacker->ToCreature(), receiver, damage);
    }
};

void AddSC_ALE()
{
    new YLA_AllCreatureScript();
    new YLA_AllGameObjectScript();
    new YLA_AllItemScript();
    new YLA_AllMapScript();
    new YLA_AuctionHouseScript();
    new YLA_BGScript();
    new YLA_CommandSC();
    new YLA_YLAScript();
    new YLA_GameEventScript();
    new YLA_GroupScript();
    new YLA_GuildScript();
    new YLA_LootScript();
    new YLA_MiscScript();
    new YLA_PetScript();
    new YLA_PlayerScript();
    new YLA_ServerScript();
    new YLA_SpellSC();
    new YLA_TicketScript();
    new YLA_VehicleScript();
    new YLA_WorldObjectScript();
    new YLA_WorldScript();
    new YLA_UnitScript();
}