#include <maxminddb.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>

static const MMDB_entry_data_list_s* geoip2_encode(lua_State *L, const MMDB_entry_data_list_s *entry_data_list)
{
    switch (entry_data_list->entry_data.type)
    {
        case MMDB_DATA_TYPE_MAP:
        {
            lua_newtable(L);

            uint32_t size = entry_data_list->entry_data.data_size;

            for (entry_data_list = entry_data_list->next; size && entry_data_list; size--)
            {
                char key[(int)entry_data_list->entry_data.data_size + 1];
                key[entry_data_list->entry_data.data_size] = '\0';

                memcpy(key, entry_data_list->entry_data.utf8_string, entry_data_list->entry_data.data_size);

                entry_data_list = geoip2_encode(L, entry_data_list->next);

                lua_setfield(L, -2, key);
            }
        }
            break;
        case MMDB_DATA_TYPE_ARRAY:
        {
            lua_newtable(L);

            uint32_t size = entry_data_list->entry_data.data_size;
            lua_Integer index = 0;

            for (entry_data_list = entry_data_list->next; size && entry_data_list; size--)
            {
                entry_data_list = geoip2_encode(L, entry_data_list);
                lua_seti(L, -2, ++index);
            }
        }
            break;
        case MMDB_DATA_TYPE_UTF8_STRING:
            lua_pushlstring(L, entry_data_list->entry_data.utf8_string, entry_data_list->entry_data.data_size);
            entry_data_list = entry_data_list->next;
            break;
        case MMDB_DATA_TYPE_BYTES:
            lua_pushlstring(L, (const char *)entry_data_list->entry_data.bytes, entry_data_list->entry_data.data_size);
            entry_data_list = entry_data_list->next;
            break;
        case MMDB_DATA_TYPE_DOUBLE:
            lua_pushnumber(L, entry_data_list->entry_data.double_value);
            entry_data_list = entry_data_list->next;
            break;
        case MMDB_DATA_TYPE_FLOAT:
            lua_pushnumber(L, entry_data_list->entry_data.float_value);
            entry_data_list = entry_data_list->next;
            break;
        case MMDB_DATA_TYPE_UINT16:
            lua_pushinteger(L, entry_data_list->entry_data.uint16);
            entry_data_list = entry_data_list->next;
            break;
        case MMDB_DATA_TYPE_UINT32:
            lua_pushinteger(L, entry_data_list->entry_data.uint32);
            entry_data_list = entry_data_list->next;
            break;
        case MMDB_DATA_TYPE_BOOLEAN:
            lua_pushboolean(L, entry_data_list->entry_data.boolean ? 1 : 0);
            entry_data_list = entry_data_list->next;
            break;
        case MMDB_DATA_TYPE_UINT64:
            lua_pushinteger(L, (lua_Integer)entry_data_list->entry_data.uint64);  // may not support
            entry_data_list = entry_data_list->next;
            break;
        case MMDB_DATA_TYPE_INT32:
            lua_pushinteger(L, entry_data_list->entry_data.int32);
            entry_data_list = entry_data_list->next;
            break;
        default:
            lua_pushnil(L);  // we do not support uint128 and other types
            entry_data_list = entry_data_list->next;
            break;
    }

    return entry_data_list;
}

typedef struct {
    MMDB_s *mmdb;
} geoip2;

static int l_geoip2_create(lua_State *L)
{
    const char *file = luaL_checkstring(L, 1);
    MMDB_s *mmdb = malloc(sizeof(MMDB_s));
    int status = MMDB_open(file, MMDB_MODE_MMAP, mmdb);

    if (MMDB_SUCCESS != status)
    {
        free(mmdb);
        lua_pushnil(L);
        lua_pushstring(L, MMDB_strerror(status));
        return 2;
    }

    ((geoip2*)lua_newuserdata(L, sizeof(geoip2)))->mmdb = mmdb;
    luaL_getmetatable(L, "geoip2");
    lua_setmetatable(L, -2);

    return 1;
}

static int l_geoip2_destroy(lua_State *L)
{
    geoip2 *ptr = (geoip2*)luaL_checkudata(L, 1, "geoip2");
    MMDB_close(ptr->mmdb);
    free(ptr->mmdb);
    ptr->mmdb = NULL;
    return 0;
}

static int l_geoip2_lookup(lua_State *L)
{
    geoip2 *ptr = (geoip2*)luaL_checkudata(L, 1, "geoip2");
    const char *ip = luaL_checkstring(L, 2);

    int gai_error, mmdb_error;
    MMDB_lookup_result_s result = MMDB_lookup_string(ptr->mmdb, ip, &gai_error, &mmdb_error);

    if (0 != gai_error)
    {
        lua_pushnil(L);
        lua_pushstring(L, gai_strerror(gai_error));
        return 2;
    }

    if (MMDB_SUCCESS != mmdb_error)
    {
        lua_pushnil(L);
        lua_pushstring(L, MMDB_strerror(mmdb_error));
        return 2;
    }

    if (!result.found_entry)
    {
        lua_pushnil(L);
        lua_pushstring(L, "empty");
        return 2;
    }

    MMDB_entry_data_list_s *entry_data_list = NULL;
    int status = MMDB_get_entry_data_list(&result.entry, &entry_data_list);

    if (!entry_data_list)
    {
        lua_pushnil(L);
        lua_pushstring(L, "empty");
        return 2;
    }

    if (MMDB_SUCCESS != status)
    {
        MMDB_free_entry_data_list(entry_data_list);
        lua_pushnil(L);
        lua_pushstring(L, MMDB_strerror(mmdb_error));
        return 2;
    }

    geoip2_encode(L, entry_data_list);
    MMDB_free_entry_data_list(entry_data_list);

    return 1;
}

static const struct luaL_Reg l_methods[] = {
        {"lookup", l_geoip2_lookup},
        {"close", l_geoip2_destroy},
        {NULL, NULL}
};

static const struct luaL_Reg l_initial[] = {
        {"open", l_geoip2_create},
        {NULL, NULL}
};

int luaopen_geoip2(lua_State *L)
{
    luaL_checkversion(L);

    luaL_newmetatable(L, "geoip2");

    lua_newtable(L);
    luaL_setfuncs(L, l_methods, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, l_geoip2_destroy);
    lua_setfield(L, -2, "__gc");

    luaL_newlib(L, l_initial);
    return 1;
}