/*
 * Copyright (C) 2010 - 2025 Eluna Lua Engine <https://elunaluaengine.github.io/>
 * This program is free software licensed under GPL version 3
 * Please see the included DOCS/LICENSE.md for more information
 */

#ifndef _HOOK_HELPERS_H
#define _HOOK_HELPERS_H

#include "LuaEngine.h"
#include "YLAUtility.h"

/*
 * Sets up the stack so that event handlers can be called.
 *
 * Returns the number of functions that were pushed onto the stack.
 */
template<typename K1, typename K2>
int YLA::SetupStack(BindingMap<K1>* bindings1, BindingMap<K2>* bindings2, const K1& key1, const K2& key2, int number_of_arguments)
{
    // Diff the caller's arg count against the pushes actually made: with a
    // polluted push_counter the lua_insert below would misplace event_id and
    // the func count derived from stack growth would be wrong.
    if (number_of_arguments != this->push_counter)
    {
        YLA_LOG_ERROR("[YLA]: SetupStack arg mismatch: caller {} pushed {}. Trusting pushed count.", number_of_arguments, (int)this->push_counter);
        number_of_arguments = this->push_counter;
    }
    if (number_of_arguments < 0)
        number_of_arguments = 0;
    ASSERT(key1.event_id == key2.event_id);
    // Stack: [arguments]

    Push(key1.event_id);
    this->push_counter = 0;
    ++number_of_arguments;
    // Stack: [arguments], event_id

    int arguments_top = lua_gettop(L);
    if (arguments_top < number_of_arguments)
    {
        YLA_LOG_ERROR("[YLA]: SetupStack underflow: need {} have {}. Popping event_id, calling nothing.", number_of_arguments, arguments_top);
        lua_pop(L, 1); // event_id just pushed above
        return 0;
    }
    int first_argument_index = arguments_top - number_of_arguments + 1;

    lua_insert(L, first_argument_index);
    // Stack: event_id, [arguments]

    bindings1->PushRefsFor(key1);
    if (bindings2)
        bindings2->PushRefsFor(key2);
    // Stack: event_id, [arguments], [functions]

    // Diff the pushed func count against actual stack growth: PushRefsFor
    // must have added exactly the [functions] slots.
    int number_of_functions = lua_gettop(L) - arguments_top;
    if (number_of_functions < 0)
    {
        YLA_LOG_ERROR("[YLA]: SetupStack func mismatch: arguments_top {} top-now {}. Restoring.", arguments_top, lua_gettop(L));
        lua_settop(L, arguments_top);
        return 0;
    }
    return number_of_functions;
}

/*
 * Replace one of the arguments pushed before `SetupStack` with a new value.
 */
template<typename T>
void YLA::ReplaceArgument(T value, uint8 index)
{
    // Stack: event_id, [arguments], [functions], [results]
    if (index == 0 || (int)index >= lua_gettop(L))
    {
        YLA_LOG_ERROR("[YLA]: ReplaceArgument refused: index {} top {}.", (int)index, lua_gettop(L));
        return;
    }

    YLA::Push(L, value);
    // Stack: event_id, [arguments], [functions], [results], value

    lua_replace(L, index + 1);
    // Stack: event_id, [arguments and value], [functions], [results]
}

/*
 * Call all event handlers registered to the event ID/entry combination and ignore any results.
 */
template<typename K1, typename K2>
void YLA::CallAllFunctions(BindingMap<K1>* bindings1, BindingMap<K2>* bindings2, const K1& key1, const K2& key2)
{
    int number_of_arguments = this->push_counter;
    // Stack: [arguments]

    int number_of_functions = SetupStack(bindings1, bindings2, key1, key2, number_of_arguments);
    // Stack: event_id, [arguments], [functions]

    while (number_of_functions > 0)
    {
        CallOneFunction(number_of_functions, number_of_arguments, 0);
        --number_of_functions;
        // Stack: event_id, [arguments], [functions - 1]
    }
    // Stack: event_id, [arguments]

    CleanUpStack(number_of_arguments);
    // Stack: (empty)
}

/*
 * Call all event handlers registered to the event ID/entry combination,
 *   and returns `default_value` if ALL event handlers returned `default_value`,
 *   otherwise returns the opposite of `default_value`.
 */
template<typename K1, typename K2>
bool YLA::CallAllFunctionsBool(BindingMap<K1>* bindings1, BindingMap<K2>* bindings2, const K1& key1, const K2& key2, bool default_value/* = false*/)
{
    bool result = default_value;
    // Note: number_of_arguments here does not count in eventID, which is pushed in SetupStack
    int number_of_arguments = this->push_counter;
    // Stack: [arguments]

    int number_of_functions = SetupStack(bindings1, bindings2, key1, key2, number_of_arguments);
    // Stack: event_id, [arguments], [functions]

    while (number_of_functions > 0)
    {
        int r = CallOneFunction(number_of_functions, number_of_arguments, 1);
        --number_of_functions;
        // Stack: event_id, [arguments], [functions - 1], result

        if (lua_isboolean(L, r) && (lua_toboolean(L, r) == 1) != default_value)
            result = !default_value;

        lua_pop(L, 1);
        // Stack: event_id, [arguments], [functions - 1]
    }
    // Stack: event_id, [arguments]

    CleanUpStack(number_of_arguments);
    // Stack: (empty)
    return result;
}

#endif // _HOOK_HELPERS_H
