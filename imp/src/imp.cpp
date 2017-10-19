// Extension lib defines
#define LIB_NAME "Imp"
#define MODULE_NAME "imp"

#define DLIB_LOG_DOMAIN "Imp"

#include <dmsdk/sdk.h>

static void printStack(lua_State* L) {
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++)  {
        int luaType = lua_type(L, i);
        dmLogInfo("STACK #%d: %s [%s]", i, lua_tostring(L, i), lua_typename(L, luaType));
    }
}


static double double_from_table(lua_State* L, int table_index, int table_key) {
    lua_pushnumber(L, table_key);
    lua_gettable(L, table_index);
    double n = luaL_checknumber(L, lua_gettop(L));
    lua_pop(L, 1);
    return n;
}


static int xytoi(int x, int y, int w, int h, int bpp) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= w) x = w - 1;
    if (y >= h) y = h - 1;
    return (y * w * bpp) + (x * bpp);
}



static int apply_filter(int x, int y, int w, int h, int bpp, int offset, double* filter, uint8_t* pixels) {
    double sum = 0;
    sum += pixels[xytoi(x - 1, y - 1, w, h, bpp) + offset] * filter[0];
    sum += pixels[xytoi(x - 0, y - 1, w, h, bpp) + offset] * filter[1];
    sum += pixels[xytoi(x + 1, y - 1, w, h, bpp) + offset] * filter[2];
    sum += pixels[xytoi(x - 1, y - 0, w, h, bpp) + offset] * filter[3];
    sum += pixels[xytoi(x - 0, y - 0, w, h, bpp) + offset] * filter[4];
    sum += pixels[xytoi(x + 1, y - 0, w, h, bpp) + offset] * filter[5];
    sum += pixels[xytoi(x - 1, y + 1, w, h, bpp) + offset] * filter[6];
    sum += pixels[xytoi(x - 0, y + 1, w, h, bpp) + offset] * filter[7];
    sum += pixels[xytoi(x + 1, y + 1, w, h, bpp) + offset] * filter[8];
    if (sum < 0) sum = 0;
    if (sum > 255) sum = 255;
    return sum;
}


static int Apply(lua_State* L) {
    int top = lua_gettop(L);

    dmScript::LuaHBuffer *lua_src_buffer = dmScript::CheckBuffer(L, 1);
    uint32_t width = luaL_checknumber(L, 2);
    uint32_t height = luaL_checknumber(L, 3);
    luaL_checktype(L, 4, LUA_TTABLE);

    double filter[9];
    filter[0] = double_from_table(L, 4, 1);
    filter[1] = double_from_table(L, 4, 2);
    filter[2] = double_from_table(L, 4, 3);
    filter[3] = double_from_table(L, 4, 4);
    filter[4] = double_from_table(L, 4, 5);
    filter[5] = double_from_table(L, 4, 6);
    filter[6] = double_from_table(L, 4, 7);
    filter[7] = double_from_table(L, 4, 8);
    filter[8] = double_from_table(L, 4, 9);
    // for(int i=0; i < 9; i++) {
    //     dmLogError("filter at %d is %f", i, filter[i]);
    // }

    dmBuffer::HBuffer src_buffer = lua_src_buffer->m_Buffer;
    dmBuffer::Result r;

    // get stream name, type and count
    dmhash_t stream_name = 0;
    r = dmBuffer::GetStreamName(src_buffer, 0, &stream_name);
    if (r != dmBuffer::RESULT_OK) {
        dmLogError("Unable to get stream name");
        return 0;
    }
    dmBuffer::ValueType stream_type;
    uint32_t stream_type_count = 0;
    r = dmBuffer::GetStreamType(src_buffer, stream_name, &stream_type, &stream_type_count);
    if (r != dmBuffer::RESULT_OK) {
        dmLogError("Unable to get stream type and count");
        return 0;
    }
    if (stream_type != dmBuffer::VALUE_TYPE_UINT8) {
        dmLogError("Expected buffer to be of type dmBuffer::VALUE_TYPE_UINT8");
        return 0;
    }


    // get source buffer bytes
    uint8_t* src_bytes = 0;
    uint32_t src_size = 0;
    r = dmBuffer::GetBytes(src_buffer, (void**)&src_bytes, &src_size);
    if (r != dmBuffer::RESULT_OK) {
        dmLogError("Unable to get bytes from source buffer");
        return 0;
    }


    // create destination buffer
    dmBuffer::HBuffer dst_buffer;
    dmBuffer::StreamDeclaration dstStreamsDecl[] = {
    {
        stream_name, dmBuffer::VALUE_TYPE_UINT8, (uint8_t)stream_type_count }
    };
    r = dmBuffer::Create(src_size / stream_type_count, dstStreamsDecl, 1, &dst_buffer);
    if (r != dmBuffer::RESULT_OK) {
        dmLogError("Unable to create target buffer");
        return 0;
    }


    // get destination buffer bytes
    uint8_t* dst_bytes = 0;
    uint32_t dst_size = 0;
    r = dmBuffer::GetBytes(dst_buffer, (void**)&dst_bytes, &dst_size);
    if (r != dmBuffer::RESULT_OK) {
        dmLogError("Unable to get bytes from destination buffer");
        return 0;
    }


    // apply filter
    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            int i = xytoi(x, y, width, height, stream_type_count);
            dst_bytes[i + 0] = apply_filter(x, y, width, height, stream_type_count, 0, filter, src_bytes);
            dst_bytes[i + 1] = apply_filter(x, y, width, height, stream_type_count, 1, filter, src_bytes);
            dst_bytes[i + 2] = apply_filter(x, y, width, height, stream_type_count, 2, filter, src_bytes);
            if(stream_type_count == 4) {
                dst_bytes[i + 3] = apply_filter(x, y, width, height, stream_type_count, 3, filter, src_bytes);
            }
        }
    }

    // return destination buffer
    dmScript::LuaHBuffer lua_dst_buffer = { dst_buffer, true };
    dmScript::PushBuffer(L, lua_dst_buffer);

    assert(top + 1 == lua_gettop(L));
    return 1;
}


// Functions exposed to Lua
static const luaL_reg Module_methods[] = {
    {"apply", Apply},
    {0, 0}
};

static void LuaInit(lua_State* L) {
    int top = lua_gettop(L);

    // Register lua names
    luaL_register(L, MODULE_NAME, Module_methods);

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

dmExtension::Result AppInitializeImpExtension(dmExtension::AppParams* params) {
    return dmExtension::RESULT_OK;
}

dmExtension::Result InitializeImpExtension(dmExtension::Params* params) {
    // Init Lua
    LuaInit(params->m_L);
    printf("Registered %s Extension\n", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizeImpExtension(dmExtension::AppParams* params) {
    return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeImpExtension(dmExtension::Params* params) {
    return dmExtension::RESULT_OK;
}


// Defold SDK uses a macro for setting up extension entry points:
//
// DM_DECLARE_EXTENSION(symbol, name, app_init, app_final, init, update, on_event, final)

DM_DECLARE_EXTENSION(Imp, LIB_NAME, AppInitializeImpExtension, AppFinalizeImpExtension, InitializeImpExtension, 0, 0, FinalizeImpExtension)
