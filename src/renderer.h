#pragma once

// TODO: opaque for now...
struct Renderer2D;
struct Texture2D;
struct Font;

Texture2D* Texture2DCreate(Arena* arena, u32 width, u32 height, u32 channels, const void* data, bool isPixelArt = false);
Texture2D* Texture2DCreateFromFile(Arena* arena, const char* filepath, bool isPixelArt = false);
void Texture2DBind(const Texture2D* texture, u32 slot = 0);

Renderer2D* Renderer2DInit(Arena* arena);

void Renderer2DBeginScene(Renderer2D* renderer, const glm::mat4& camera);
void Renderer2DEndScene(const Renderer2D* renderer);

void Renderer2DDrawQuad(Renderer2D* renderer, const glm::mat4& transform, const glm::vec4& color);
void Renderer2DDrawTexturedQuad(Renderer2D* renderer, Texture2D* texture, const glm::mat4& transform, const glm::vec4& color);
void Renderer2DDrawTexturedQuadUvs(Renderer2D* renderer, Texture2D* texture, const glm::vec2* textureCoords, const glm::mat4& transform, const glm::vec4& color);

Font* Renderer2DFontLoadFromFile(Arena* arena, const char* filepath);
void Renderer2DDrawString(Renderer2D* renderer, const char* string, Font* font, const glm::mat4& transform, const glm::vec4& color);
