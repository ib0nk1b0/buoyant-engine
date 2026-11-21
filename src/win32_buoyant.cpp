#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#undef INFINITE
#include <msdf-atlas-gen.h>

#define BUOYANT_ASSERT(x) assert(x)
#define UNIMPLEMENTED() BUOYANT_ASSERT(false)

#include "base.h"

char* PlatformReadEntireFile(Arena* arena, const char* filepath, u32* outSize)
{
    HANDLE fileHandle = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    BUOYANT_ASSERT(fileHandle != INVALID_HANDLE_VALUE);

    LARGE_INTEGER fileSize;
    BUOYANT_ASSERT(GetFileSizeEx(fileHandle, &fileSize));

    BUOYANT_ASSERT(fileSize.QuadPart <= 0xFFFFFFFF);

    u32 fileSize32 = (u32)fileSize.QuadPart;

    char* result = ArenaPushArray(arena, char, fileSize32);
    if (outSize) *outSize = fileSize32;

    DWORD bytesRead;
    BUOYANT_ASSERT(ReadFile(fileHandle, result, fileSize32, &bytesRead, 0) && fileSize32 == bytesRead);

    CloseHandle(fileHandle);
    
    return result;
}

Arena* ArenaCreate(u64 size)
{
    size_t totalSize = size + sizeof(Arena);
    Arena* arena = (Arena*)VirtualAlloc(0, totalSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    arena->size = size;

    return arena;
}

void ArenaRelease(Arena* arena)
{
    if (arena)
    {
        VirtualFree(arena, 0, MEM_RELEASE);
    }
}

// TODO: page aligned pushes?
void* ArenaPush(Arena* arena, u64 size)
{
    BUOYANT_ASSERT(arena->pos + size < arena->size);

    u8* out = (u8*)arena + sizeof(Arena) + arena->pos;
    arena->pos += size;

    return out;
}

void* ArenaPop(Arena* arena, u64 size)
{
    BUOYANT_ASSERT(arena->pos >= size);

    arena->pos -= size; 
    u8* out = (u8*)arena + sizeof(Arena) + arena->pos;

    // TODO: should this be done on every pop?
    memset(out, 0, size);

    return out;
}

char* SvMakeCstr(Arena* arena, StringView sv)
{
    char* cstr = ArenaPushArray(arena, char, sv.size + 1);
    memcpy(cstr, sv.data, sv.size);
    cstr[sv.size] = 0;
    return cstr;
}

StringView SvReadEntireFile(Arena* arena, const char* filepath)
{
    StringView result = {};

    u32 fileSize32 = 0;
    result.data = PlatformReadEntireFile(arena, filepath, &fileSize32);
    result.size = fileSize32;

    return result;
}

StringView SvChopByDelim(StringView* sv, char delim)
{
    u64 i = 0;
    while (i < sv->size && sv->data[i] != delim)
    {
        i += 1;
    }

    StringView lhs = {};
    lhs.data = sv->data;
    lhs.size = i;

    if (i < sv->size)
    {
        sv->data += i + 1;
        sv->size -= (i + 1);
    }
    else
    {
        sv->data += i;
        sv->size -= i;
    }

    return lhs;
}

StringView SvTrimLeft(StringView sv)
{
    while (sv.size > 0 && isspace(sv.data[0]))
    {
        sv.data += 1;
        sv.size -= 1;
    }

    return sv;
}

StringView SvTrimRight(StringView sv)
{
    while(sv.size > 0 && isspace(sv.data[sv.size - 1]))
    {
        sv.size -= 1;
    }

    return sv;
}

StringView SvTrim(StringView sv)
{
    return SvTrimLeft(SvTrimRight(sv));
}


// TODO: globals
global GLFWwindow* g_Window = nullptr;
global bool g_WindowShouldClose = false;

//
// GLFW callbacks
//

internal void GLFWErrorCallback(i32 error, const char* desc)
{
    // TODO: error handling
    OutputDebugStringA(desc);
}

internal void GLFWKeyCallback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods)
{
    // TODO: key callbacks
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) g_WindowShouldClose = true;
}

#include "opengl_renderer.cpp"
#include "buoyant.cpp"

inline bool WindowShouldClose()
{
    return g_WindowShouldClose || glfwWindowShouldClose(g_Window);
}

i32 WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, i32 nShowCmd)
{
    //
    // Arena
    //

    Arena* arena = ArenaCreate(Megabytes(64));
    Arena* transientArena = ArenaCreate(Megabytes(64));

    //
    // GLFW
    //

    BUOYANT_ASSERT(glfwInit());

    g_Window = glfwCreateWindow(1280, 720, "Buoyant-Engine", NULL, NULL);

    BUOYANT_ASSERT(g_Window != NULL);

    glfwSetErrorCallback(GLFWErrorCallback);
    glfwSetKeyCallback(g_Window, GLFWKeyCallback);

    glfwMakeContextCurrent(g_Window);
    gladLoadGL(glfwGetProcAddress);

    // TODO: command line argument + configurable
    bool vSync = true;
    glfwSwapInterval(vSync);

    //
    // Miniaudio
    //

    // TODO: pull this out
    ma_result result;
    ma_engine engine;

    BUOYANT_ASSERT(ma_engine_init(NULL, &engine) == MA_SUCCESS);

    ma_engine_set_volume(&engine, 0.2f);

    // The simplest and least flexible way to play a sound is like so:
    // sound object is managed by miniaudio internally
    // ma_engine_play_sound(&engine, "resources\\sounds\\synth.flac", NULL);
    ma_sound sound;

    result = ma_sound_init_from_file(&engine, "resources\\audio\\sound.wav", 0, NULL, NULL, &sound);

    BUOYANT_ASSERT(result == MA_SUCCESS);

    ma_sound_set_looping(&sound, MA_TRUE);

    ma_sound_start(&sound);

    //
    // Renderer setup
    //

    char versionBuf[256];
    snprintf(versionBuf, 256, "Version:  %s\n", glGetString(GL_VERSION));
    OutputDebugStringA(versionBuf);

    // TODO: 
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    GameMemory* gameMemory = ArenaPushStruct(arena, GameMemory);
    gameMemory->permenantArena = arena;
    gameMemory->transientArena = transientArena;

    glCheckError(glClearColor(1.0f, 0.0f, 1.0f, 1.0f));

    f32 last_time = (f32)glfwGetTime();
    i32 width, height;
    while(!WindowShouldClose())
    {
        glfwPollEvents();

        f32 time = (f32)glfwGetTime();
        f32 dt = time - last_time;
        last_time = time;

        glfwGetFramebufferSize(g_Window, &width, &height);

        glCheckError(glViewport(0, 0, width, height)); // TODO: should be done on resize only
 
        // TODO: should this be in renderer?
        glCheckError(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));

        BuoyantUpdate(gameMemory, dt);

        glfwSwapBuffers(g_Window);
    }

    return 0;
}
