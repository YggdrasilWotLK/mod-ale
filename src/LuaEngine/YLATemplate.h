/*
* Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
* This program is free software licensed under GPL version 3
* Please see the included DOCS/LICENSE.md for more information
*/

#ifndef _YLA_TEMPLATE_H
#define _YLA_TEMPLATE_H

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
};
#include "LuaEngine.h"
#include "YLACompat.h"
#include "YLAUtility.h"
#include "YlaAlive.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"

enum MethodRegisterState
{
    METHOD_REG_MAP   = 0,
    METHOD_REG_WORLD = 1,
    METHOD_REG_ALL   = 2
};

struct YLAGlobalRegister
{
    const char* name;
    int(*func)(lua_State*);
    MethodRegisterState regState;

    YLAGlobalRegister(const char* name, int(*f)(lua_State*), MethodRegisterState state = METHOD_REG_ALL)
        : name(name), func(f), regState(state) {}

    YLAGlobalRegister(const char* name, MethodRegisterState state = METHOD_REG_ALL)
        : name(name), func(nullptr), regState(state) {}
};

class YLAGlobal
{
public:
    static int thunk(lua_State* L)
    {
        YLAGlobalRegister* l = static_cast<YLAGlobalRegister*>(lua_touserdata(L, lua_upvalueindex(1)));
        int top = lua_gettop(L);
        int expected = l->func(L);
        int args = lua_gettop(L) - top;
        if (args < 0 || args > expected)
        {
            YLA_LOG_ERROR("[YLA]: {} returned unexpected amount of arguments {} out of {}. Report to devs", l->name, args, expected);
            ASSERT(false);
        }
        lua_settop(L, top + expected);
        return expected;
    }

    static int MethodWrongState(lua_State* L)
    {
        luaL_error(L, "attempt to call method '%s' that is not available in this state", lua_tostring(L, lua_upvalueindex(1)));
        return 0;
    }

    static void SetMethods(YLA* E, YLAGlobalRegister* methodTable)
    {
        ASSERT(E);
        ASSERT(methodTable);

        lua_pushglobaltable(E->L);

        for (; methodTable && methodTable->name; ++methodTable)
        {
            if (methodTable->regState != METHOD_REG_ALL)
            {
                bool isMapState = (E->GetStateMapId() != YLA_GLOBAL_STATE);
                if ((!isMapState && methodTable->regState == METHOD_REG_MAP) ||
                    (isMapState && methodTable->regState == METHOD_REG_WORLD))
                {
                    lua_pushstring(E->L, methodTable->name);
                    lua_pushstring(E->L, methodTable->name);
                    lua_pushcclosure(E->L, MethodWrongState, 1);
                    lua_rawset(E->L, -3);
                    continue;
                }
            }

            if (!methodTable->func)
                continue;

            lua_pushstring(E->L, methodTable->name);
            lua_pushlightuserdata(E->L, (void*)methodTable);
            lua_pushcclosure(E->L, thunk, 1);
            lua_rawset(E->L, -3);
        }

        lua_remove(E->L, -1);
    }
};

class YLAObject
{
public:
    template<typename T>
    YLAObject(T * obj, bool manageMemory, uint64 snapId);
    YLAObject(Player* obj, bool manageMemory, uint64 snapId);

    ~YLAObject()
    {
    }

    // Get wrapped object pointer
    void* GetObj() const { return object; }
    // Returns whether the object is valid or not for the given state
    // incarnation id. The snapshot is taken from the creating state, so
    // every map state invalidates exactly its own userdata (never GYLA's).
    bool IsValid(uint64 currentId) const { return !callstackid || callstackid == currentId; }
    // Returns whether the object can be invalidated or not
    bool CanInvalidate() const { return _invalidate; }
    // Returns pointer to the wrapped object's type name
    const char* GetTypeName() const { return type_name; }
    ObjectGuid GetPlayerGuid() const { return playerGuid; }

    // Sets the object pointer that is wrapped
    void SetObj(void* obj, uint64 snapId)
    {
        ASSERT(obj);
        object = obj;
        SetValid(true, snapId);
    }
    // Sets the object pointer to valid or invalid
    void SetValid(bool valid, uint64 snapId)
    {
        ASSERT(!valid || (valid && object));
        if (valid)
            if (CanInvalidate())
                callstackid = snapId;
            else
                callstackid = 0;
        else
            callstackid = 1;
    }
    // Sets whether the pointer will be invalidated at end of calls
    void SetValidation(bool invalidate)
    {
        _invalidate = invalidate;
    }
    // Invalidates the pointer if it should be invalidated
    void Invalidate()
    {
        if (CanInvalidate())
            callstackid = 1;
    }

private:
    uint64 callstackid;
    bool _invalidate;
    void* object;
    const char* type_name;
    ObjectGuid playerGuid = ObjectGuid::Empty;
};

template<typename T>
struct YLARegister
{
    const char* name;
    int(*mfunc)(lua_State*, T*);
    MethodRegisterState regState;

    YLARegister(const char* name, int(*func)(lua_State*, T*), MethodRegisterState state = METHOD_REG_ALL)
        : name(name), mfunc(func), regState(state) {}

    YLARegister(const char* name, MethodRegisterState state = METHOD_REG_ALL)
        : name(name), mfunc(nullptr), regState(state) {}
};

template<typename T>
class YLATemplate
{
public:
    static const char* tname;
    static bool manageMemory;

    // name will be used as type name
    // If gc is true, lua will handle the memory management for object pushed
    // gc should be used if pushing for example WorldPacket,
    // that will only be needed on lua side and will not be managed by TC/mangos/<core>
    static void Register(YLA* E, const char* name, bool gc = false)
    {
        ASSERT(E);
        ASSERT(name);

        // check that metatable isn't already there
        lua_getglobal(E->L, name);
        ASSERT(lua_isnoneornil(E->L, -1));

        // pop nil
        lua_pop(E->L, 1);

        tname = name;
        manageMemory = gc;

        // create metatable for userdata of this type
        luaL_newmetatable(E->L, tname);
        int metatable  = lua_gettop(E->L);

        // push methodtable to stack to be accessed and modified by users
        lua_pushvalue(E->L, metatable);
        lua_setglobal(E->L, tname);

        // tostring
        lua_pushcfunction(E->L, ToString);
        lua_setfield(E->L, metatable, "__tostring");

        // garbage collecting
        lua_pushcfunction(E->L, CollectGarbage);
        lua_setfield(E->L, metatable, "__gc");

        // make methods accessible through metatable
        lua_pushvalue(E->L, metatable);
        lua_setfield(E->L, metatable, "__index");

        // make new indexes saved to methods
        lua_pushcfunction(E->L, Add);
        lua_setfield(E->L, metatable, "__add");

        // make new indexes saved to methods
        lua_pushcfunction(E->L, Substract);
        lua_setfield(E->L, metatable, "__sub");

        // make new indexes saved to methods
        lua_pushcfunction(E->L, Multiply);
        lua_setfield(E->L, metatable, "__mul");

        // make new indexes saved to methods
        lua_pushcfunction(E->L, Divide);
        lua_setfield(E->L, metatable, "__div");

        // make new indexes saved to methods
        lua_pushcfunction(E->L, Mod);
        lua_setfield(E->L, metatable, "__mod");

        // make new indexes saved to methods
        lua_pushcfunction(E->L, Pow);
        lua_setfield(E->L, metatable, "__pow");

        // make new indexes saved to methods
        lua_pushcfunction(E->L, UnaryMinus);
        lua_setfield(E->L, metatable, "__unm");

        // make new indexes saved to methods
        lua_pushcfunction(E->L, Concat);
        lua_setfield(E->L, metatable, "__concat");

        // make new indexes saved to methods
        lua_pushcfunction(E->L, Length);
        lua_setfield(E->L, metatable, "__len");

        // make new indexes saved to methods
        lua_pushcfunction(E->L, Equal);
        lua_setfield(E->L, metatable, "__eq");

        // make new indexes saved to methods
        lua_pushcfunction(E->L, Less);
        lua_setfield(E->L, metatable, "__lt");

        // make new indexes saved to methods
        lua_pushcfunction(E->L, LessOrEqual);
        lua_setfield(E->L, metatable, "__le");

        // make new indexes saved to methods
        lua_pushcfunction(E->L, Call);
        lua_setfield(E->L, metatable, "__call");

        // special method to get the object type
        lua_pushcfunction(E->L, GetType);
        lua_setfield(E->L, metatable, "GetObjectType");

        // special method to decide object invalidation at end of call
        lua_pushcfunction(E->L, SetInvalidation);
        lua_setfield(E->L, metatable, "SetInvalidation");

        // pop metatable
        lua_pop(E->L, 1);
    }

    template<typename C>
    static void SetMethods(YLA* E, YLARegister<C>* methodTable)
    {
        ASSERT(E);
        ASSERT(tname);
        ASSERT(methodTable);

        // get metatable
        lua_pushstring(E->L, tname);
        lua_rawget(E->L, LUA_REGISTRYINDEX);
        ASSERT(lua_istable(E->L, -1));

        for (; methodTable && methodTable->name; ++methodTable)
        {
            if (methodTable->regState != METHOD_REG_ALL)
            {
                bool isMapState = (E->GetStateMapId() != YLA_GLOBAL_STATE);
                if ((!isMapState && methodTable->regState == METHOD_REG_MAP) ||
                    (isMapState && methodTable->regState == METHOD_REG_WORLD))
                {
                    lua_pushstring(E->L, methodTable->name);
                    lua_pushstring(E->L, methodTable->name);
                    lua_pushcclosure(E->L, MethodWrongState, 1);
                    lua_rawset(E->L, -3);
                    continue;
                }
            }

            if (!methodTable->mfunc)
                continue;

            lua_pushstring(E->L, methodTable->name);
            lua_pushlightuserdata(E->L, (void*)methodTable);
            lua_pushcclosure(E->L, CallMethod, 1);
            lua_rawset(E->L, -3);
        }

        lua_pop(E->L, 1);
    }

    static int Push(lua_State* L, T const* obj)
    {
        if (!obj)
        {
            lua_pushnil(L);
            return 1;
        }

        // Create new userdata
        YLAObject** ptrHold = static_cast<YLAObject**>(lua_newuserdata(L, sizeof(YLAObject*)));
        if (!ptrHold)
        {
            YLA_LOG_ERROR("{} could not create new userdata", tname);
            lua_pushnil(L);
            return 1;
        }
        *ptrHold = new YLAObject(const_cast<T*>(obj), manageMemory, YLA::GetYLA(L)->GetCallstackId());

        // Set metatable for it
        lua_pushstring(L, tname);
        lua_rawget(L, LUA_REGISTRYINDEX);
        if (!lua_istable(L, -1))
        {
            YLA_LOG_ERROR("{} missing metatable", tname);
            lua_pop(L, 2);
            lua_pushnil(L);
            return 1;
        }
        lua_setmetatable(L, -2);
        return 1;
    }

    // Snapshot of the creating state's incarnation id. Stored per Push,
    // so validity is always judged against the state whose Lua owns this
    // userdata (multistate-safe, unlike a single global counter).
    static uint64 YlaSnapId(lua_State* L) { return YLA::GetYLA(L)->GetCallstackId(); }

    // GUID-checked at Push time; destroyed/relogged players are Lua errors.
    // Not-in-world players pass through (normal during login hooks).
    static Player* YlaResolvePlayer(lua_State* L, int narg, YLAObject* ALEObj, Player* raw, bool error)
    {
        auto fail = [&](const char* reason) -> Player*
        {
            char buff[256];
            snprintf(buff, 256, "%s expected, got %s (%s). Check your code.", tname, reason, luaL_typename(L, narg));
            if (error)
            {
                luaL_argerror(L, narg, buff);
            }
            else
            {
                YLA_LOG_ERROR("{}", buff);
            }
            return nullptr;
        };

        if (!raw)
            return fail("null player reference");
        ObjectGuid guid = ALEObj->GetPlayerGuid();
        if (guid.IsEmpty())
            return fail("pointer to destroyed (logged out) object");
        Player* live = ObjectAccessor::FindPlayer(guid);
        if (live)
        {
            if (live != raw)
                return fail("pointer to stale (relogged) object");
            return live;
        }
        YlaAlive::Guard guard{ YlaAlive::Mutex() };
        if (!YlaAlive::MatchesLocked(guid, static_cast<WorldObject*>(raw)))
            return fail("pointer to destroyed (logged out) object");
        return raw;
    }

    template<typename U>
    static U* YlaResolvePlayer(lua_State*, int, YLAObject*, U* raw, bool) { return raw; }

    static T* Check(lua_State* L, int narg, bool error = true)
    {
        YLAObject* ALEObj = YLA::CHECKTYPE(L, narg, tname, error);
        if (!ALEObj)
            return NULL;

        if (!ALEObj->IsValid(YlaSnapId(L)))
        {
            char buff[256];
            snprintf(buff, 256, "%s expected, got pointer to nonexisting (invalidated) object (%s). Check your code.", tname, luaL_typename(L, narg));
            if (error)
            {
                luaL_argerror(L, narg, buff);
            }
            else
            {
                YLA_LOG_ERROR("{}", buff);
            }
            return NULL;
        }
        T* raw = static_cast<T*>(ALEObj->GetObj());
        return YlaResolvePlayer(L, narg, ALEObj, raw, error);
    }

    static int GetType(lua_State* L)
    {
        lua_pushstring(L, tname);
        return 1;
    }

    static int SetInvalidation(lua_State* L)
    {
        YLAObject* ALEObj = YLA::CHECKOBJ<YLAObject>(L, 1);
        bool invalidate = YLA::CHECKVAL<bool>(L, 2);

        ALEObj->SetValidation(invalidate);
        return 0;
    }

    static int MethodWrongState(lua_State* L)
    {
        luaL_error(L, "attempt to call method '%s' that is not available in this state", lua_tostring(L, lua_upvalueindex(1)));
        return 0;
    }

    static int CallMethod(lua_State* L)
    {
        // NOTE: no YlaAlive lock is held across CHECKOBJ/mfunc here on
        // purpose (Lua errors longjmp past C++ destructors). Liveness is
        // enforced by GUID resolution inside Check, so a destroyed object
        // becomes a Lua error, not a SIGSEGV.
        T* obj = YLA::CHECKOBJ<T>(L, 1); // get self
        if (!obj)
            return 0;
        YLARegister<T>* l = static_cast<YLARegister<T>*>(lua_touserdata(L, lua_upvalueindex(1)));
        int top = lua_gettop(L);
        int expected = l->mfunc(L, obj);
        int args = lua_gettop(L) - top;
        if (args < 0 || args > expected)
        {
            YLA_LOG_ERROR("[YLA]: {} returned unexpected amount of arguments {} out of {}. Report to devs", l->name, args, expected);
            ASSERT(false);
        }
        lua_settop(L, top + expected);
        return expected;
    }

    // Metamethods ("virtual")

    // Remember special cases like YLATemplate<Vehicle>::CollectGarbage
    static int CollectGarbage(lua_State* L)
    {
        // Get object pointer (and check type, no error)
        YLAObject* obj = YLA::CHECKOBJ<YLAObject>(L, 1, false);
        if (obj && manageMemory)
            delete static_cast<T*>(obj->GetObj());
        delete obj;
        return 0;
    }

    static int ToString(lua_State* L)
    {
        T* obj = YLA::CHECKOBJ<T>(L, 1, true); // get self
        lua_pushfstring(L, "%s: %p", tname, obj);
        return 1;
    }

    static int ArithmeticError(lua_State* L) { return luaL_error(L, "attempt to perform arithmetic on a %s value", tname); }
    static int CompareError(lua_State* L) { return luaL_error(L, "attempt to compare %s", tname); }
    static int Add(lua_State* L) { return ArithmeticError(L); }
    static int Substract(lua_State* L) { return ArithmeticError(L); }
    static int Multiply(lua_State* L) { return ArithmeticError(L); }
    static int Divide(lua_State* L) { return ArithmeticError(L); }
    static int Mod(lua_State* L) { return ArithmeticError(L); }
    static int Pow(lua_State* L) { return ArithmeticError(L); }
    static int UnaryMinus(lua_State* L) { return ArithmeticError(L); }
    static int Concat(lua_State* L) { return luaL_error(L, "attempt to concatenate a %s value", tname); }
    static int Length(lua_State* L) { return luaL_error(L, "attempt to get length of a %s value", tname); }
    static int Equal(lua_State* L) { YLA::Push(L, YLA::CHECKOBJ<T>(L, 1) == YLA::CHECKOBJ<T>(L, 2)); return 1; }
    static int Less(lua_State* L) { return CompareError(L); }
    static int LessOrEqual(lua_State* L) { return CompareError(L); }
    static int Call(lua_State* L) { return luaL_error(L, "attempt to call a %s value", tname); }
};

template<typename T>
YLAObject::YLAObject(T * obj, bool manageMemory, uint64 snapId) : callstackid(1), _invalidate(!manageMemory), object(obj), type_name(YLATemplate<T>::tname)
{
    SetValid(true, snapId);
}

inline YLAObject::YLAObject(Player* obj, bool manageMemory, uint64 snapId) : callstackid(1), _invalidate(!manageMemory), object(obj), type_name(YLATemplate<Player>::tname), playerGuid(obj ? obj->GetGUID() : ObjectGuid::Empty)
{
    SetValid(true, snapId);
}

template<typename T> const char* YLATemplate<T>::tname = NULL;
template<typename T> bool YLATemplate<T>::manageMemory = false;

#endif
