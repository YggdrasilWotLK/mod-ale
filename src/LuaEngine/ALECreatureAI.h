/*
 * Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
 * This program is free software licensed under GPL version 3
 * Please see the included DOCS/LICENSE.md for more information
 */

#ifndef _ALE_CREATURE_AI_H
#define _ALE_CREATURE_AI_H

#include "LuaEngine.h"

struct ScriptedAI;

struct ALECreatureAI : ScriptedAI
{
    bool justSpawned;
    std::vector<std::pair<uint32, uint32>> movepoints;
    ALE* E;

    ALECreatureAI(Creature* creature) : ScriptedAI(creature), justSpawned(true),
        E(ALE::GetMapStateOrGlobal(creature->GetMapId()))
    {
    }

    ~ALECreatureAI() { }

    void UpdateAI(uint32 diff) override
    {
        if (justSpawned)
        {
            justSpawned = false;
            JustRespawned();
        }

        if (!movepoints.empty())
        {
            for (auto& point : movepoints)
            {
                if (!E->MovementInform(me, point.first, point.second))
                    ScriptedAI::MovementInform(point.first, point.second);
            }
            movepoints.clear();
        }

        if (!E->UpdateAI(me, diff))
        {
            if (!me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC))
                ScriptedAI::UpdateAI(diff);
        }
    }

    void JustEngagedWith(Unit* target) override
    {
        if (!E->EnterCombat(me, target))
            ScriptedAI::JustEngagedWith(target);
    }

    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType damagetype, SpellSchoolMask damageSchoolMask) override
    {
        if (!E->DamageTaken(me, attacker, damage))
            ScriptedAI::DamageTaken(attacker, damage, damagetype, damageSchoolMask);
    }

    void JustDied(Unit* killer) override
    {
        if (!E->JustDied(me, killer))
            ScriptedAI::JustDied(killer);
    }

    void KilledUnit(Unit* victim) override
    {
        if (!E->KilledUnit(me, victim))
            ScriptedAI::KilledUnit(victim);
    }

    void JustSummoned(Creature* summon) override
    {
        if (!E->JustSummoned(me, summon))
            ScriptedAI::JustSummoned(summon);
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        if (!E->SummonedCreatureDespawn(me, summon))
            ScriptedAI::SummonedCreatureDespawn(summon);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        movepoints.push_back(std::make_pair(type, id));
    }

    void AttackStart(Unit* target) override
    {
        if (!E->AttackStart(me, target))
            ScriptedAI::AttackStart(target);
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        if (!E->EnterEvadeMode(me))
            ScriptedAI::EnterEvadeMode();
    }

    void JustRespawned() override
    {
        if (!E->JustRespawned(me))
            ScriptedAI::JustRespawned();
    }

    void JustReachedHome() override
    {
        if (!E->JustReachedHome(me))
            ScriptedAI::JustReachedHome();
    }

    void ReceiveEmote(Player* player, uint32 emoteId) override
    {
        if (!E->ReceiveEmote(me, player, emoteId))
            ScriptedAI::ReceiveEmote(player, emoteId);
    }

    void CorpseRemoved(uint32& respawnDelay) override
    {
        if (!E->CorpseRemoved(me, respawnDelay))
            ScriptedAI::CorpseRemoved(respawnDelay);
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (!E->MoveInLineOfSight(me, who))
            ScriptedAI::MoveInLineOfSight(who);
    }

    void SpellHit(Unit* caster, SpellInfo const* spell) override
    {
        if (!E->SpellHit(me, caster, spell))
            ScriptedAI::SpellHit(caster, spell);
    }

    void SpellHitTarget(Unit* target, SpellInfo const* spell) override
    {
        if (!E->SpellHitTarget(me, target, spell))
            ScriptedAI::SpellHitTarget(target, spell);
    }

    void IsSummonedBy(WorldObject* summoner) override
    {
        if (!summoner->ToUnit() || !E->OnSummoned(me, summoner->ToUnit()))
            ScriptedAI::IsSummonedBy(summoner);
    }

    void SummonedCreatureDies(Creature* summon, Unit* killer) override
    {
        if (!E->SummonedCreatureDies(me, summon, killer))
            ScriptedAI::SummonedCreatureDies(summon, killer);
    }

    void OwnerAttackedBy(Unit* attacker) override
    {
        if (!E->OwnerAttackedBy(me, attacker))
            ScriptedAI::OwnerAttackedBy(attacker);
    }

    void OwnerAttacked(Unit* target) override
    {
        if (!E->OwnerAttacked(me, target))
            ScriptedAI::OwnerAttacked(target);
    }
};

#endif