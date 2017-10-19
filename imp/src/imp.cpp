// Extension lib defines
#define LIB_NAME "Imp"
#define MODULE_NAME "imp"

#define DLIB_LOG_DOMAIN "Imp"

#include <dmsdk/sdk.h>


struct Kernel {
    double values[9];
};

struct BufferData {
    dmhash_t name;
    dmBuffer::ValueType type;
    uint32_t count;
};

static void printStack(lua_State* L) {
    int n = lua_gettop(L);
    for (int i = 1; i <= n; i++)  {
        int luaType = lua_type(L, i);
        dmLogInfo("STACK #%d: %s [%s]", i, lua_tostring(L, i), lua_typename(L, luaType));
    }
}


static double double_from_table(lua_State* L, int table_index, int table_key) {
    lua_rawgeti(L, table_index, table_key);
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


static int apply_kernel(int x, int y, int w, int h, int bpp, int offset, Kernel kernel, uint8_t* pixels) {
    double sum = 0;
    sum += pixels[xytoi(x - 1, y - 1, w, h, bpp) + offset] * kernel.values[0];
    sum += pixels[xytoi(x - 0, y - 1, w, h, bpp) + offset] * kernel.values[1];
    sum += pixels[xytoi(x + 1, y - 1, w, h, bpp) + offset] * kernel.values[2];
    sum += pixels[xytoi(x - 1, y - 0, w, h, bpp) + offset] * kernel.values[3];
    sum += pixels[xytoi(x - 0, y - 0, w, h, bpp) + offset] * kernel.values[4];
    sum += pixels[xytoi(x + 1, y - 0, w, h, bpp) + offset] * kernel.values[5];
    sum += pixels[xytoi(x - 1, y + 1, w, h, bpp) + offset] * kernel.values[6];
    sum += pixels[xytoi(x - 0, y + 1, w, h, bpp) + offset] * kernel.values[7];
    sum += pixels[xytoi(x + 1, y + 1, w, h, bpp) + offset] * kernel.values[8];
    if (sum < 0) sum = 0;
    if (sum > 255) sum = 255;
    return sum;
}

static Kernel check_kernel(lua_State* L, int index) {
    luaL_checktype(L, 5, LUA_TTABLE);
    Kernel kernel;
    kernel.values[0] = double_from_table(L, 5, 1);
    kernel.values[1] = double_from_table(L, 5, 2);
    kernel.values[2] = double_from_table(L, 5, 3);
    kernel.values[3] = double_from_table(L, 5, 4);
    kernel.values[4] = double_from_table(L, 5, 5);
    kernel.values[5] = double_from_table(L, 5, 6);
    kernel.values[6] = double_from_table(L, 5, 7);
    kernel.values[7] = double_from_table(L, 5, 8);
    kernel.values[8] = double_from_table(L, 5, 9);
    return kernel;
}

static BufferData get_buffer_data(dmBuffer::HBuffer buffer) {
    BufferData data;
    dmBuffer::GetStreamName(buffer, 0, &data.name);
    dmBuffer::GetStreamType(buffer, data.name, &data.type, &data.count);
    return data;
}


static dmBuffer::HBuffer check_and_validate_buffer(lua_State* L, int index) {
    dmScript::LuaHBuffer *lua_buffer = dmScript::CheckBuffer(L, index);
    dmBuffer::HBuffer buffer = lua_buffer->m_Buffer;

    if (!dmBuffer::IsBufferValid(buffer)) {
        luaL_error(L, "Buffer is invalid");
    }
    BufferData data = get_buffer_data(buffer);
    if (data.type != dmBuffer::VALUE_TYPE_UINT8) {
        luaL_error(L, "Buffer is not of type UINT8");
    }
    return buffer;
}

static bool compare_buffers(dmBuffer::HBuffer a, dmBuffer::HBuffer b) {
    BufferData a_data = get_buffer_data(a);
    BufferData b_data = get_buffer_data(b);
    return (a_data.count == b_data.count);
}

static int Convolution(lua_State* L) {
    int top = lua_gettop(L);

    dmBuffer::HBuffer src_buffer = check_and_validate_buffer(L, 1);
    dmBuffer::HBuffer dst_buffer = check_and_validate_buffer(L, 2);
    uint32_t width = luaL_checknumber(L, 3);
    uint32_t height = luaL_checknumber(L, 4);
    Kernel kernel = check_kernel(L, 5);

    if(!compare_buffers(src_buffer, dst_buffer)) {
        luaL_error(L, "Buffers are not of the same length");
    }

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

    // get destination buffer bytes
    uint8_t* dst_bytes = 0;
    uint32_t dst_size = 0;
    r = dmBuffer::GetBytes(dst_buffer, (void**)&dst_bytes, &dst_size);
    if (r != dmBuffer::RESULT_OK) {
        dmLogError("Unable to get bytes from destination buffer");
        return 0;
    }

    // apply kernel
    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            int i = xytoi(x, y, width, height, stream_type_count);
            dst_bytes[i + 0] = apply_kernel(x, y, width, height, stream_type_count, 0, kernel, src_bytes);
            dst_bytes[i + 1] = apply_kernel(x, y, width, height, stream_type_count, 1, kernel, src_bytes);
            dst_bytes[i + 2] = apply_kernel(x, y, width, height, stream_type_count, 2, kernel, src_bytes);
            if(stream_type_count == 4) {
                dst_bytes[i + 3] = apply_kernel(x, y, width, height, stream_type_count, 3, kernel, src_bytes);
            }
        }
    }

    assert(top == lua_gettop(L));
    return 0;
}


// Functions exposed to Lua
static const luaL_reg Module_methods[] = {
    {"convolution", Convolution},
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
