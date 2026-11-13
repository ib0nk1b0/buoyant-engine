#pragma once

struct Renderer2D;
struct Texture2D;
struct Font;

Renderer2D* Renderer2DInit(Arena* arena);

void Renderer2DBeginScene(Renderer2D* renderer, const glm::mat4& camera);
void Renderer2DEndScene(const Renderer2D* renderer);

void Renderer2DDrawQuad(Renderer2D* renderer, const glm::mat4& transform, const glm::vec4& color);
void Renderer2DDrawTexturedQuad(Renderer2D* renderer, Texture2D* texture, const glm::mat4& transform, const glm::vec4& color);
void Renderer2DDrawTexturedQuadUvs(Renderer2D* renderer, Texture2D* texture, const glm::vec2* textureCoords, const glm::mat4& transform, const glm::vec4& color);
void Renderer2DDrawString(Renderer2D* renderer, Font* font, const glm::mat4& transform, const glm::vec3& color);
