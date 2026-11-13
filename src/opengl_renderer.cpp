#include "opengl_renderer.h"

GLenum glCheckError_(const char *file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        constexpr u32 bufSize = 256;
        char msg[bufSize];
        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  sprintf_s(msg, bufSize, "INVALID_ENUM                  | %d | %s\n", line, file);
            case GL_INVALID_VALUE:                 sprintf_s(msg, bufSize, "INVALID_VALUE                 | %d | %s\n", line, file);
            case GL_INVALID_OPERATION:             sprintf_s(msg, bufSize, "INVALID_OPERATION             | %d | %s\n", line, file);
            case GL_STACK_OVERFLOW:                sprintf_s(msg, bufSize, "STACK_OVERFLOW                | %d | %s\n", line, file);
            case GL_STACK_UNDERFLOW:               sprintf_s(msg, bufSize, "STACK_UNDERFLOW               | %d | %s\n", line, file);
            case GL_OUT_OF_MEMORY:                 sprintf_s(msg, bufSize, "OUT_OF_MEMORY                 | %d | %s\n", line, file);
            case GL_INVALID_FRAMEBUFFER_OPERATION: sprintf_s(msg, bufSize, "INVALID_FRAMEBUFFER_OPERATION | %d | %s\n", line, file);
            OutputDebugStringA(msg);
        }
    }
    return errorCode;
}
#define glCheckError(x) x; glCheckError_(__FILE__, __LINE__)

//
// Shader
//

struct Shader
{
    u32 program;
};

internal Shader* ShaderLoadFromFile(Arena* arena, const char* filepath)
{
    Shader* shader = ArenaPushStruct(arena, Shader);

    StringView sv = SvReadEntireFile(arena, filepath);

    const char* vertex = nullptr;
    const char* fragment = nullptr;
    SvChopByDelim(&sv, ':');
    if (sv.size != 0)
    {
        StringView lhs = SvChopByDelim(&sv, '\n');

        lhs = SvTrim(lhs);
        char* line = SvMakeCstr(arena, lhs);
        if (strcmp(line, "type vertex") == 0 && vertex == NULL)
        {
            StringView vert = SvChopByDelim(&sv, ':');
            vert = SvTrim(vert);
            vertex = SvMakeCstr(arena, vert);

            lhs = SvChopByDelim(&sv, '\n');
            lhs = SvTrim(lhs);
            line = SvMakeCstr(arena, lhs);
            if (strcmp(line, "type fragment") == 0 && fragment == NULL)
            {
                sv = SvTrim(sv);
                fragment = SvMakeCstr(arena, sv);
            }
        }
    }

    glCheckError(const GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER));
    glCheckError(glShaderSource(vertexShader, 1, &vertex, NULL));
    glCheckError(glCompileShader(vertexShader));

    {
        GLint success;
        glCheckError(glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success));
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            BUOYANT_ASSERT(false);
        }
    }

    glCheckError(const GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER));
    glCheckError(glShaderSource(fragmentShader, 1, &fragment, NULL));
    glCheckError(glCompileShader(fragmentShader));

    {
        GLint success;
        glCheckError(glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success));
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            BUOYANT_ASSERT(false);
        }
    }
    
    glCheckError(shader->program = glCreateProgram());
    glCheckError(glAttachShader(shader->program, vertexShader));
    glCheckError(glAttachShader(shader->program, fragmentShader));
    glCheckError(glLinkProgram(shader->program));

    {
        GLint success;
        glCheckError(glGetProgramiv(shader->program, GL_LINK_STATUS, &success));
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(shader->program, 512, NULL, infoLog);
            BUOYANT_ASSERT(false);
        }
    }

    return shader;
}

internal void ShaderBind(const Shader* shader)
{
    glCheckError(glUseProgram(shader->program));
}

//
// Texture2D
//

struct Texture2D
{
    u32 rendererID;
};

internal Texture2D* Texture2DCreate(Arena* arena, u32 width, u32 height, u32 channels, const void* data, bool isPixelArt = false)
{
    // NOTE: Should it return the texture
    // OR should it fill out a texture that is passed in
    Texture2D* texture = ArenaPushStruct(arena, Texture2D);

    u32 internalFormat = 0;
    u32 dataFormat = 0;
    if (channels == 4)
    {
        internalFormat = GL_RGBA8;
        dataFormat = GL_RGBA;
    }
    else if (channels == 3)
    {
        internalFormat = GL_RGB8;
        dataFormat = GL_RGB;
    }

    glCheckError(glCreateTextures(GL_TEXTURE_2D, 1, &texture->rendererID));
    glCheckError(glTextureStorage2D(texture->rendererID, 1, internalFormat, width, height));

    if (isPixelArt)
    {
        glCheckError(glTextureParameteri(texture->rendererID, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        glCheckError(glTextureParameteri(texture->rendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    }
    else
    {
        glCheckError(glTextureParameteri(texture->rendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        glCheckError(glTextureParameteri(texture->rendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    }

    glCheckError(glTextureParameteri(texture->rendererID, GL_TEXTURE_WRAP_S,     GL_REPEAT));
    glCheckError(glTextureParameteri(texture->rendererID, GL_TEXTURE_WRAP_T,     GL_REPEAT));

    glCheckError(glTextureSubImage2D(texture->rendererID, 0, 0, 0, width, height, dataFormat, GL_UNSIGNED_BYTE, data));

    return texture;
}

internal Texture2D* Texture2DCreateFromFile(Arena* arena, const char* filepath, bool isPixelArt = false)
{
    stbi_set_flip_vertically_on_load(true);

    i32 textureWidth, textureHeight, channels;
    unsigned char* textureData = stbi_load(filepath, &textureWidth, &textureHeight, &channels, 0);

    Texture2D* texture = Texture2DCreate(arena, textureWidth, textureHeight, channels, textureData, isPixelArt);

    stbi_image_free(textureData);

    return texture;
}

internal void Texture2DBind(const Texture2D* texture, u32 slot = 0)
{
    glCheckError(glBindTextureUnit(slot, texture->rendererID));
}


//
// Renderer
//

struct QuadVertex
{
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 textureCoords;
    float     textureIndex;
};

#define QUAD_VERTEX_COUNT 4
#define QUAD_INDEX_COUNT  6

struct Renderer2D
{
    u32         MaxQuads;

    Shader*     QuadShader;

    u32         QuadVertexArray;
    u32         QuadVertexBuffer;
    u32         QuadIndexBuffer;

    QuadVertex* QuadVertexBase;
    QuadVertex* QuadVertexPtr;
    u32         QuadIndexCount;

    glm::vec4*  QuadVertexPositions;

    u32         MaxTextureSlots;
    u32*        TextureSamplers;
    Texture2D** TextureSlots;
    u32         TextureSlotIndex;
};

Renderer2D* Renderer2DInit(Arena* arena)
{
    Renderer2D* renderer = ArenaPushStruct(arena, Renderer2D);

    renderer->MaxQuads = 1024; // TODO:

    renderer->QuadShader = ShaderLoadFromFile(arena, "resources\\shaders\\basic_shader.glsl");

    glCheckError(glGenVertexArrays(1, &renderer->QuadVertexArray));
    glCheckError(glBindVertexArray(renderer->QuadVertexArray));

    renderer->QuadVertexBase = ArenaPushArray(arena, QuadVertex, renderer->MaxQuads * QUAD_VERTEX_COUNT);
    renderer->QuadVertexPtr  = renderer->QuadVertexBase;
    
    glCheckError(glCreateBuffers(1, &renderer->QuadVertexBuffer));
    glCheckError(glBindBuffer(GL_ARRAY_BUFFER, renderer->QuadVertexBuffer));
    glCheckError(glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertex) * QUAD_VERTEX_COUNT * renderer->MaxQuads, NULL, GL_DYNAMIC_DRAW));

    glCheckError(glEnableVertexAttribArray(0));
    glCheckError(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), offsetof(QuadVertex, position)));
    glCheckError(glEnableVertexAttribArray(1));
    glCheckError(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, color)));
    glCheckError(glEnableVertexAttribArray(2));
    glCheckError(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, textureCoords)));
    glCheckError(glEnableVertexAttribArray(3));
    glCheckError(glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, textureIndex)));

    glCheckError(glCreateBuffers(1, &renderer->QuadIndexBuffer));
    glCheckError(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->QuadIndexBuffer));

    GLsizeiptr indexBufferSize = renderer->MaxQuads * QUAD_INDEX_COUNT * sizeof(u32);
    u32* indices = ArenaPushArray(arena, u32, renderer->MaxQuads * QUAD_INDEX_COUNT);

    for (u32 i = 0, offset = 0; i < renderer->MaxQuads * QUAD_INDEX_COUNT; i += QUAD_INDEX_COUNT, offset += QUAD_VERTEX_COUNT)
    {
        indices[i + 0] = offset + 0;
        indices[i + 1] = offset + 1;
        indices[i + 2] = offset + 2;
        indices[i + 3] = offset + 2;
        indices[i + 4] = offset + 3;
        indices[i + 5] = offset + 0;
    }

    glCheckError(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, indices, GL_STATIC_DRAW));


    renderer->QuadVertexPositions = ArenaPushArray(arena, glm::vec4, QUAD_VERTEX_COUNT);
    renderer->QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
    renderer->QuadVertexPositions[1] = {  0.5f, -0.5f, 0.0f, 1.0f };
    renderer->QuadVertexPositions[2] = {  0.5f,  0.5f, 0.0f, 1.0f };
    renderer->QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

    renderer->MaxTextureSlots = 32;

    renderer->TextureSamplers = ArenaPushArray(arena, u32, 32);
    for (u32 i = 0; i < renderer->MaxTextureSlots; i++)
    {
        renderer->TextureSamplers[i] = i;
    }

    renderer->TextureSlots = ArenaPushArray(arena, Texture2D*, renderer->MaxTextureSlots);

    renderer->TextureSlotIndex = 0;

    u32 whiteTextureData = 0xFFFFFFFF;
    Texture2D* whiteTexture = Texture2DCreate(arena, 1, 1, 4, &whiteTextureData);

    // TODO:
    renderer->TextureSlots[renderer->TextureSlotIndex++] = whiteTexture;

    return renderer;
}

void Renderer2DBeginScene(Renderer2D* renderer, const glm::mat4& camera)
{
    ShaderBind(renderer->QuadShader);
    u32 location = glGetUniformLocation(renderer->QuadShader->program, "u_ViewProjection");
    glCheckError(glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(camera)));

    renderer->QuadVertexPtr = renderer->QuadVertexBase;
    renderer->QuadIndexCount = 0;
    renderer->TextureSlotIndex = 1; // TODO: resetting every frame is that correct?
}

void Renderer2DEndScene(const Renderer2D* renderer)
{
    if (renderer->QuadIndexCount)
    {
        ShaderBind(renderer->QuadShader);

        u64 size = (u8*)renderer->QuadVertexPtr - (u8*)renderer->QuadVertexBase;
        glCheckError(glBufferSubData(GL_ARRAY_BUFFER, 0, size, renderer->QuadVertexBase));

        for (u32 i = 0; i < renderer->TextureSlotIndex; i++)
        {
            Texture2DBind(renderer->TextureSlots[i], i);
        }

        // TODO:
        u32 location = glGetUniformLocation(renderer->QuadShader->program, "u_Textures");
        glCheckError(glUniform1iv(location, renderer->MaxTextureSlots, (int*)renderer->TextureSamplers));

        glCheckError(glBindVertexArray(renderer->QuadVertexArray));
        glCheckError(glDrawElements(GL_TRIANGLES, renderer->QuadIndexCount, GL_UNSIGNED_INT, NULL));
    }
}


void Renderer2DDrawQuad(Renderer2D* renderer, const glm::mat4& transform, const glm::vec4& color)
{
    Renderer2DDrawTexturedQuad(renderer, renderer->TextureSlots[0], transform, color);
}

void Renderer2DDrawTexturedQuad(Renderer2D* renderer, Texture2D* texture, const glm::mat4& transform, const glm::vec4& color)
{
    glm::vec2 textureCoords[4] =
    {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f }
    };

    Renderer2DDrawTexturedQuadUvs(renderer, texture, textureCoords, transform, color);
}

void Renderer2DDrawTexturedQuadUvs(Renderer2D* renderer, Texture2D* texture, const glm::vec2* textureCoords, const glm::mat4& transform, const glm::vec4& color)
{
    BUOYANT_ASSERT(renderer->QuadIndexCount + QUAD_INDEX_COUNT <= renderer->MaxQuads * QUAD_INDEX_COUNT);

    f32 textureIndex = -1.0f;
    for (u32 i = 0; i < renderer->TextureSlotIndex; i++)
    {
        if (renderer->TextureSlots[i]->rendererID == texture->rendererID)
        {
            textureIndex = (f32)i;
            break;
        }
    }

    if (textureIndex == -1.0f && renderer->TextureSlotIndex < renderer->MaxTextureSlots)
    {
        textureIndex = (f32)renderer->TextureSlotIndex;
        renderer->TextureSlots[renderer->TextureSlotIndex++] = texture;
    }

    for (u32 i = 0; i < QUAD_VERTEX_COUNT; i++)
    {
        renderer->QuadVertexPtr->position = transform * renderer->QuadVertexPositions[i];
        renderer->QuadVertexPtr->color = color;
        renderer->QuadVertexPtr->textureCoords = textureCoords[i];
        renderer->QuadVertexPtr->textureIndex = textureIndex;
        renderer->QuadVertexPtr++;
    }

    renderer->QuadIndexCount += QUAD_INDEX_COUNT;
}

