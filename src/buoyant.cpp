#include "buoyant.h"

void BuoyantUpdate(GameMemory* memory, f32 dt)
{
    Arena* arena = memory->permenantArena;

    if (!memory->initialized)
    {
        memory->initialized = true;

        memory->renderer = Renderer2DInit(arena);

        memory->textures = ArenaPushArray(arena, Texture2D*, 32);

        memory->textures[0] = Texture2DCreateFromFile(arena, "resources\\textures\\test.png", true);
        memory->textures[1] = Texture2DCreateFromFile(arena, "resources\\textures\\test2.png", true);

        memory->font = Renderer2DFontLoadFromFile(arena, "resources\\fonts\\opensans\\OpenSans-Medium.ttf");
    }

    Renderer2D* renderer = memory->renderer;

    local_persist u32 counter = 0;
    counter++;


    glm::mat4 camera, view, projection, viewProjection;

    camera = glm::mat4(1.0f);

    view = glm::inverse(camera);

    projection = glm::ortho(-16.0f, 16.0f, -9.0f, 9.0f, -1.0f, 1.0f);
    viewProjection = projection * view;

    Renderer2DBeginScene(renderer, viewProjection);

    local_persist f32 rotation = 0.0f;
    rotation += 100.0f * dt;
    // rotation = 0.0f;
    glm::mat4 transform1 = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f,  3.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(5.0f));
    glm::mat4 transform2 = glm::translate(glm::mat4(1.0f), glm::vec3( 5.0f,  2.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(-rotation), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(5.0f));
    glm::mat4 transform3 = glm::translate(glm::mat4(1.0f), glm::vec3( 0.0f, -4.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(rotation*0.5f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(5.0f));

    if ((counter / 144) % 2 == 0)
    {
        Renderer2DDrawQuad(renderer, transform1, glm::vec4(1.0f));
        Renderer2DDrawTexturedQuad(renderer, memory->textures[0], transform2, glm::vec4(1.0f));
    }
    else
    {
        Renderer2DDrawQuad(renderer, transform2, glm::vec4(1.0f));
        Renderer2DDrawTexturedQuad(renderer, memory->textures[0], transform1, glm::vec4(1.0f));
    }

    Renderer2DDrawTexturedQuad(renderer, memory->textures[1], glm::mat4(1.0f), glm::vec4(0.3f, 0.2f, 0.8f, 1.0f));

    glm::vec2 textureCoords[4] = {
        { 0.25f, 0.25f },
        { 0.25f, 0.75f },
        { 0.75f, 0.75f },
        { 0.75f, 0.25f },
    };

    Renderer2DDrawTexturedQuadUvs(renderer, memory->textures[0], textureCoords, transform3, glm::vec4(1.0f, 0.5f, 0.2f, 1.0f));

    Renderer2DDrawString(renderer, "Hello, World", memory->font, glm::mat4(1.0f), glm::vec4(1.0f));

    Renderer2DEndScene(renderer);
}

