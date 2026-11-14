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
            OutputDebugStringA(infoLog);
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
            OutputDebugStringA(infoLog);
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
            OutputDebugStringA(infoLog);
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
    u32 width;
    u32 height;
};

internal Texture2D* Texture2DCreate(Arena* arena, u32 width, u32 height, u32 channels, const void* data, bool isPixelArt = false)
{
    // NOTE: Should it return the texture
    // OR should it fill out a texture that is passed in
    Texture2D* texture = ArenaPushStruct(arena, Texture2D);
    texture->width = width;
    texture->height = height;

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
    i32       textureIndex;
};

#define QUAD_VERTEX_COUNT 4
#define QUAD_INDEX_COUNT  6

struct Renderer2D
{
    u32         MaxQuads;

    u32         IndexBuffer;

    Shader*     QuadShader;

    u32         QuadVertexArray;
    u32         QuadVertexBuffer;

    QuadVertex* QuadVertexBase;
    QuadVertex* QuadVertexPtr;
    u32         QuadIndexCount;

    Shader*     TextShader;

    u32         TextVertexArray;
    u32         TextVertexBuffer;

    QuadVertex* TextVertexBase;
    QuadVertex* TextVertexPtr;
    u32         TextIndexCount;

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

    //
    // Quad
    //

    renderer->QuadShader = ShaderLoadFromFile(arena, "resources\\shaders\\basic_shader.glsl");

    renderer->QuadVertexBase = ArenaPushArray(arena, QuadVertex, renderer->MaxQuads * QUAD_VERTEX_COUNT);
    renderer->QuadVertexPtr  = renderer->QuadVertexBase;

    glCheckError(glGenVertexArrays(1, &renderer->QuadVertexArray));
    glCheckError(glBindVertexArray(renderer->QuadVertexArray));

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
    glCheckError(glVertexAttribIPointer(3, 1, GL_INT, sizeof(QuadVertex), (void*)offsetof(QuadVertex, textureIndex)));

    //
    // Text
    //

    renderer->TextShader = ShaderLoadFromFile(arena, "resources\\shaders\\text_shader.glsl");

    renderer->TextVertexBase = ArenaPushArray(arena, QuadVertex, renderer->MaxQuads * QUAD_VERTEX_COUNT);
    renderer->TextVertexPtr  = renderer->TextVertexBase;

    glCheckError(glGenVertexArrays(1, &renderer->TextVertexArray));
    glCheckError(glBindVertexArray(renderer->TextVertexArray));

    glCheckError(glCreateBuffers(1, &renderer->TextVertexBuffer));
    glCheckError(glBindBuffer(GL_ARRAY_BUFFER, renderer->TextVertexBuffer));
    glCheckError(glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertex) * QUAD_VERTEX_COUNT * renderer->MaxQuads, NULL, GL_DYNAMIC_DRAW));

    glCheckError(glEnableVertexAttribArray(0));
    glCheckError(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), offsetof(QuadVertex, position)));
    glCheckError(glEnableVertexAttribArray(1));
    glCheckError(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, color)));
    glCheckError(glEnableVertexAttribArray(2));
    glCheckError(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, textureCoords)));
    glCheckError(glEnableVertexAttribArray(3));
    glCheckError(glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, textureIndex)));

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

    glCheckError(glCreateBuffers(1, &renderer->IndexBuffer));
    glCheckError(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IndexBuffer));
    glCheckError(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, indices, GL_STATIC_DRAW));

    glCheckError(glBindVertexArray(renderer->QuadVertexArray));
    glCheckError(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IndexBuffer));
    // glCheckError(glBindVertexArray(renderer->TextVertexArray));
    // glCheckError(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IndexBuffer));

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

    ShaderBind(renderer->TextShader);
    location = glGetUniformLocation(renderer->TextShader->program, "u_ViewProjection");
    glCheckError(glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(camera)));

    renderer->TextVertexPtr = renderer->TextVertexBase;
    renderer->TextIndexCount = 0;

    renderer->TextureSlotIndex = 1; // TODO: resetting every frame is that correct?
}

void Renderer2DEndScene(const Renderer2D* renderer)
{
    if (renderer->QuadIndexCount)
    {
        ShaderBind(renderer->QuadShader);

        glCheckError(glBindBuffer(GL_ARRAY_BUFFER, renderer->QuadVertexBuffer));
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

    if (renderer->TextIndexCount)
    {
        ShaderBind(renderer->TextShader);

        glCheckError(glBindBuffer(GL_ARRAY_BUFFER, renderer->TextVertexBuffer));
        u64 size = (u8*)renderer->TextVertexPtr - (u8*)renderer->TextVertexBase;
        glCheckError(glBufferSubData(GL_ARRAY_BUFFER, 0, size, renderer->TextVertexBase));

        for (u32 i = 0; i < renderer->TextureSlotIndex; i++)
        {
            Texture2DBind(renderer->TextureSlots[i], i);
        }

        // TODO:
        u32 location = glGetUniformLocation(renderer->TextShader->program, "u_Textures");
        glCheckError(glUniform1iv(location, renderer->MaxTextureSlots, (int*)renderer->TextureSamplers));

        glCheckError(glBindVertexArray(renderer->TextVertexArray));
        glCheckError(glDrawElements(GL_TRIANGLES, renderer->TextIndexCount, GL_UNSIGNED_INT, NULL));
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
        renderer->QuadVertexPtr->textureIndex = (int)textureIndex;
        renderer->QuadVertexPtr++;
    }

    renderer->QuadIndexCount += QUAD_INDEX_COUNT;
}

#define DEFAULT_ANGLE_THRESHOLD 3.0
#define LCG_MULTIPLIER 6364136223846793005ull
#define LCG_INCREMENT 1442695040888963407ull
#define THREAD_COUNT 8
// NOTE:This is bad ^

struct CharsetRange
{
    u32 begin;
    u32 end;
};

// From imgui_draw.cpp
global const CharsetRange charsetRanges[] =
{
    { 0x0020, 0x00FF }
};

struct MSDFData
{
    std::vector<msdf_atlas::GlyphGeometry> Glyphs;
    msdf_atlas::FontGeometry FontGeometry;
};

struct Font
{
    MSDFData*  fontData;
    Texture2D* fontAtlas;
    f32        lineSpacing = 0.0f;
    f32        kerning = 0.0f;
};

Font* Renderer2DFontLoadFromFile(Arena* arena, const char* filepath)
{
    MSDFData* fontData = ArenaPushStruct(arena, MSDFData);
    fontData = new (fontData) MSDFData{};
    msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
    msdfgen::FontHandle* font = msdfgen::loadFont(ft, filepath);

    BUOYANT_ASSERT(font);

    msdf_atlas::Charset charset;
    for (CharsetRange range : charsetRanges)
    {
        for (u32 c = range.begin; c <= range.end; c++)
            charset.add(c);
    }

    f64 fontScale = 1.0;
    fontData->FontGeometry = msdf_atlas::FontGeometry(&fontData->Glyphs);
    i32 glyphsLoaded = fontData->FontGeometry.loadCharset(font, fontScale, charset);
    // ConsolePrint("Loaded %d glyphs from font '%s' (out of %zu)", glyphsLoaded, path, charset.size());

    f64 emSize = 40.0;

    msdf_atlas::TightAtlasPacker atlasPacker;
    atlasPacker.setPixelRange(2.0);
    atlasPacker.setMiterLimit(1.0);
    atlasPacker.setSpacing(0); // might not be right
    atlasPacker.setScale(emSize);

    i32 remaining = atlasPacker.pack(fontData->Glyphs.data(), (i32)fontData->Glyphs.size());
    BUOYANT_ASSERT(remaining == 0);

    i32 atlasWidth, atlasHeight;
    atlasPacker.getDimensions(atlasWidth, atlasHeight);
    emSize = atlasPacker.getScale();


    u64 coloringSeed = 0;
    bool expensiveColoring = false;
    if (expensiveColoring)
    {
        // msdf_atlas::Workload([&glyphs = fontData->Glyphs, &coloringSeed](int i, int threadNo) -> bool {
        //         unsigned long long glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
        //         glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
        //         return true;
        //         }, fontData->Glyphs.size()).finish(THREAD_COUNT);
    }
    else {
        u64 glyphSeed = coloringSeed;
        for (msdf_atlas::GlyphGeometry& glyph : fontData->Glyphs)
        {
            glyphSeed *= LCG_MULTIPLIER;
            glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
        }
    }

    msdf_atlas::GeneratorAttributes attributes;
    attributes.config.overlapSupport = true;
    attributes.scanlinePass = true;

    msdf_atlas::ImmediateAtlasGenerator<f32, 3, msdf_atlas::msdfGenerator, msdf_atlas::BitmapAtlasStorage<u8, 3>> generator(atlasWidth, atlasHeight);
    generator.setAttributes(attributes);
    generator.setThreadCount(THREAD_COUNT);
    generator.generate(fontData->Glyphs.data(), (i32)fontData->Glyphs.size());

    msdfgen::BitmapConstRef<u8, 3> bitmap = (msdfgen::BitmapConstRef<u8, 3>)generator.atlasStorage();

    Texture2D* atlas = Texture2DCreate(arena, atlasWidth, atlasHeight, 3, (void*)bitmap.pixels);

    msdfgen::destroyFont(font);
    msdfgen::deinitializeFreetype(ft);

    Font* result = ArenaPushStruct(arena, Font);
    result->fontData = fontData;
    result->fontAtlas = atlas;

    return result;
}

void Renderer2DDrawString(Renderer2D* renderer, const char* string, Font* font, const glm::mat4& transform, const glm::vec4& color)
{
    u64 stringLength = strlen(string);
    BUOYANT_ASSERT(renderer->TextIndexCount + QUAD_INDEX_COUNT * stringLength <= renderer->MaxQuads * QUAD_INDEX_COUNT);

    const auto& fontGeometry = font->fontData->FontGeometry;
    const auto& metrics = fontGeometry.getMetrics();

    i32 textureIndex = -1;
    for (u32 i = 0; i < renderer->TextureSlotIndex; i++)
    {
        if (renderer->TextureSlots[i]->rendererID == font->fontAtlas->rendererID)
        {
            textureIndex = i;
            break;
        }
    }

    if (textureIndex == -1 && renderer->TextureSlotIndex < renderer->MaxTextureSlots)
    {
        textureIndex = renderer->TextureSlotIndex;
        renderer->TextureSlots[renderer->TextureSlotIndex++] = font->fontAtlas;
    }

    f64 x = 0.0;
    f64 fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);
    f64 y = 0.0;

    const f32 spaceGlyphAdvance = (f32)fontGeometry.getGlyph(' ')->getAdvance();

    for (u64 i = 0; i < stringLength; i++)
    {
        char character = string[i];

        if (character == '\r')
            continue;

        if (character == '\n')
        {
            x = 0;
            y -= fsScale * metrics.lineHeight + font->lineSpacing;
            continue;
        }

        if (character == ' ')
        {
            f32 advance = spaceGlyphAdvance;
            if (i < stringLength - 1)
            {
                char nextCharacter = string[i + 1];
                f64 dAdvance;
                fontGeometry.getAdvance(dAdvance, character, nextCharacter);
                advance = (f32)dAdvance;
            }

            x += fsScale * advance + font->kerning;
            continue;
        }

        if (character == '\t')
        {
            x += 4.0f * (fsScale * spaceGlyphAdvance + font->kerning);
            continue;
        }

        auto glyph = fontGeometry.getGlyph(character);
        if (!glyph)
            glyph = fontGeometry.getGlyph('?');
        if (!glyph)
            return;

        f64 al, ab, ar, at;
        glyph->getQuadAtlasBounds(al, ab, ar, at);
        glm::vec2 texCoordMin((f32)al, (f32)ab);
        glm::vec2 texCoordMax((f32)ar, (f32)at);

        f64 pl, pb, pr, pt;
        glyph->getQuadPlaneBounds(pl, pb, pr, pt);
        glm::vec2 quadMin((f32)pl, (f32)pb);
        glm::vec2 quadMax((f32)pr, (f32)pt);

        quadMin *= fsScale, quadMax *= fsScale;
        quadMin += glm::vec2(x, y);
        quadMax += glm::vec2(x, y);

        f32 texelWidth = 1.0f / font->fontAtlas->width;
        f32 texelHeight = 1.0f / font->fontAtlas->height;
        texCoordMin *= glm::vec2(texelWidth, texelHeight);
        texCoordMax *= glm::vec2(texelWidth, texelHeight);

        renderer->TextVertexPtr->position = transform * glm::vec4(quadMin, 0.0f, 1.0f);
        renderer->TextVertexPtr->color = color;
        renderer->TextVertexPtr->textureIndex = textureIndex;
        renderer->TextVertexPtr->textureCoords = texCoordMin;
        renderer->TextVertexPtr++;

        renderer->TextVertexPtr->position = transform * glm::vec4(quadMin.x, quadMax.y, 0.0f, 1.0f);
        renderer->TextVertexPtr->color = color;
        renderer->TextVertexPtr->textureIndex = textureIndex;
        renderer->TextVertexPtr->textureCoords = { texCoordMin.x, texCoordMax.y };
        renderer->TextVertexPtr++;

        renderer->TextVertexPtr->position = transform * glm::vec4(quadMax, 0.0f, 1.0f);
        renderer->TextVertexPtr->color = color;
        renderer->TextVertexPtr->textureIndex = textureIndex;
        renderer->TextVertexPtr->textureCoords = texCoordMax;
        renderer->TextVertexPtr++;

        renderer->TextVertexPtr->position = transform * glm::vec4(quadMax.x, quadMin.y, 0.0f, 1.0f);
        renderer->TextVertexPtr->color = color;
        renderer->TextVertexPtr->textureIndex = textureIndex;
        renderer->TextVertexPtr->textureCoords = { texCoordMax.x, texCoordMin.y };
        renderer->TextVertexPtr++;

        renderer->TextIndexCount += QUAD_INDEX_COUNT;

        if (i < stringLength - 1)
        {
            f64 advance = glyph->getAdvance();
            char nextCharacter = string[i + 1];
            fontGeometry.getAdvance(advance, character, nextCharacter);

            x += fsScale * advance + font->kerning;
        }
    }
}

