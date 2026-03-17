/*
* Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
* This program is free software licensed under GPL version 3
* Please see the included DOCS/LICENSE.md for more information
*/

#ifndef _ALE_INSTANCE_DATA_H
#define _ALE_INSTANCE_DATA_H

#include "LuaEngine.h"
#include "InstanceScript.h"

class ALEInstanceAI : public InstanceData
{
private:
    std::string lastSaveData;

public:
    ALEInstanceAI(Map* map) : InstanceData(map)
    {
    }

    ALE* GetE() const
    {
        return ALE::GetMapStateOrGlobal(instance->GetId());
    }

    void Initialize() override;
    void Load(const char* data) override;
    std::string GetSaveData() override { return Save(); }
    const char* Save() const;

    void Reload() { Load(NULL); }

    uint32 GetData(uint32 key) const override;
    void SetData(uint32 key, uint32 value) override;
    uint64 GetData64(uint32 key) const override;
    void SetData64(uint32 key, uint64 value) override;

    void Update(uint32 diff) override
    {
        ALE* E = GetE();
        if (!E->HasInstanceData(instance))
            Reload();
        E->OnUpdateInstance(this, diff);
    }

    bool IsEncounterInProgress() const override
    {
        return GetE()->OnCheckEncounterInProgress(const_cast<ALEInstanceAI*>(this));
    }

    void OnPlayerEnter(Player* player) override
    {
        GetE()->OnPlayerEnterInstance(this, player);
    }

    void OnGameObjectCreate(GameObject* gameobject) override
    {
        GetE()->OnGameObjectCreate(this, gameobject);
    }

    void OnCreatureCreate(Creature* creature) override
    {
        GetE()->OnCreatureCreate(this, creature);
    }
};

#endif
