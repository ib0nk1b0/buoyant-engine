#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <Windows.h>

#define BUOYANT_ASSERT(x) assert(x)

#define UNIMPLEMENTED() BUOYANT_ASSERT(false)

#define internal static
#define global static
#define local_persist static

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float    f32;
typedef double   f64;

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

#include "win32_buoyant.h"

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

    HANDLE fileHandle = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    BUOYANT_ASSERT(fileHandle != INVALID_HANDLE_VALUE);

    LARGE_INTEGER fileSize;
    BUOYANT_ASSERT(GetFileSizeEx(fileHandle, &fileSize));

    BUOYANT_ASSERT(fileSize.QuadPart <= 0xFFFFFFFF);

    u32 fileSize32 = (u32)fileSize.QuadPart;

    result.data = ArenaPushArray(arena, char, fileSize32);
    result.size = fileSize32;

    DWORD bytesRead;
    BUOYANT_ASSERT(ReadFile(fileHandle, result.data, fileSize32, &bytesRead, 0) && fileSize32 == bytesRead);

    CloseHandle(fileHandle);

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

    ma_result result;
    ma_engine engine;

    BUOYANT_ASSERT(ma_engine_init(NULL, &engine) == MA_SUCCESS);

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

    // char versionBuf[256];
    // snprintf(versionBuf, 256, "Version:  %s\n", glGetString(GL_VERSION));
    // OutputDebugStringA(versionBuf);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // TODO: check for errors

    Renderer2D* renderer = Renderer2DInit(arena);

    Texture2D* testTexture = Texture2DCreateFromFile(arena, "resources\\textures\\test.png", true);
    // Texture2D* test2Texture = Texture2DCreateFromFile(arena, "resources\\textures\\test2.png", true);
    //
    // Texture2DBind(whiteTexture, 0);
    // Texture2DBind(testTexture, 1);
    // Texture2DBind(test2Texture, 2);

    Font* font = Renderer2DFontLoadFromFile(arena, "resources\\fonts\\opensans\\OpenSans-Medium.ttf");
    glCheckError(glClearColor(1.0f, 0.0f, 1.0f, 1.0f));

    f32 last_time = (f32)glfwGetTime();
    i32 width, height;
    i32 counter = 0;
    while(!WindowShouldClose())
    {
        glfwPollEvents();

        counter++;

        f32 time = (f32)glfwGetTime();
        f32 dt = time - last_time;
        last_time = time;

        glfwGetFramebufferSize(g_Window, &width, &height);

        glCheckError(glViewport(0, 0, width, height));
        glCheckError(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT));

        glm::mat4 camera, view, projection, viewProjection;

        camera = glm::mat4(1.0f);

        view = glm::inverse(camera);

        projection = glm::ortho(-16.0f, 16.0f, -9.0f, 9.0f, -1.0f, 1.0f);
        viewProjection = projection * view;

        Renderer2DBeginScene(renderer, viewProjection);

        local_persist f32 rotation = 0.0f;
        rotation += 100.0f * dt;
        glm::mat4 transform1 = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f,  3.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(5.0f));
        glm::mat4 transform2 = glm::translate(glm::mat4(1.0f), glm::vec3( 5.0f,  2.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(-rotation), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(5.0f));
        glm::mat4 transform3 = glm::translate(glm::mat4(1.0f), glm::vec3( 0.0f, -4.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(rotation*0.5f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(5.0f));

        if ((counter / 144) % 2 == 0)
        {
            Renderer2DDrawQuad(renderer, transform1, glm::vec4(1.0f));
            Renderer2DDrawTexturedQuad(renderer, testTexture, transform2, glm::vec4(1.0f));
        }
        else
        {
            Renderer2DDrawQuad(renderer, transform2, glm::vec4(1.0f));
            Renderer2DDrawTexturedQuad(renderer, testTexture, transform1, glm::vec4(1.0f));
        }

        glm::vec2 textureCoords[4] = {
            { 0.25f, 0.25f },
            { 0.25f, 0.75f },
            { 0.75f, 0.75f },
            { 0.75f, 0.25f },
        };

        Renderer2DDrawTexturedQuadUvs(renderer, testTexture, textureCoords, transform3, glm::vec4(1.0f, 0.5f, 0.2f, 1.0f));

        Renderer2DDrawString(renderer, "Hello, World", font, glm::mat4(1.0f), glm::vec4(1.0f));

        Renderer2DEndScene(renderer);

        glfwSwapBuffers(g_Window);
    }

    return 0;
}
