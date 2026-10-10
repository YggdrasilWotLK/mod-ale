/*
 * Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
 * This program is free software licensed under GPL version 3
 * Please see the included DOCS/LICENSE.md for more information
 */

#include "Hooks.h"
#include "HookHelpers.h"
#include "LuaEngine.h"
#include "BindingMap.h"
#include "YLATemplate.h"

using namespace Hooks;

#define START_HOOK(EVENT)\
    if (!YLAConfig::GetInstance().IsALEEnabled())\
        return;\
    LOCK_YLA;\
    /* WORLD dispatch runs Lua on this state: hold its lock too. */\
    YLA::Guard __yla_world_state_guard(this->GetStateLock());\
    if (!YLA::IsInitialized())\
        return;\
    auto key = EventKey<GroupEvents>(EVENT);\
    if ((!GroupEventBindings || !GroupEventBindings->HasBindingsFor(key)))\
        return;

void YLA::OnAddMember(Group* group, ObjectGuid guid)
{
    START_HOOK(GROUP_EVENT_ON_MEMBER_ADD);
    Push(group);
    Push(guid);
    CallAllFunctions(GroupEventBindings, key);
}

void YLA::OnInviteMember(Group* group, ObjectGuid guid)
{
    START_HOOK(GROUP_EVENT_ON_MEMBER_INVITE);
    Push(group);
    Push(guid);
    CallAllFunctions(GroupEventBindings, key);
}

void YLA::OnRemoveMember(Group* group, ObjectGuid guid, uint8 method)
{
    START_HOOK(GROUP_EVENT_ON_MEMBER_REMOVE);
    Push(group);
    Push(guid);
    Push(method);
    CallAllFunctions(GroupEventBindings, key);
}

void YLA::OnChangeLeader(Group* group, ObjectGuid newLeaderGuid, ObjectGuid oldLeaderGuid)
{
    START_HOOK(GROUP_EVENT_ON_LEADER_CHANGE);
    Push(group);
    Push(newLeaderGuid);
    Push(oldLeaderGuid);
    CallAllFunctions(GroupEventBindings, key);
}

void YLA::OnDisband(Group* group)
{
    START_HOOK(GROUP_EVENT_ON_DISBAND);
    Push(group);
    CallAllFunctions(GroupEventBindings, key);
}

void YLA::OnCreate(Group* group, ObjectGuid leaderGuid, GroupType groupType)
{
    START_HOOK(GROUP_EVENT_ON_CREATE);
    Push(group);
    Push(leaderGuid);
    Push(groupType);
    CallAllFunctions(GroupEventBindings, key);
}
