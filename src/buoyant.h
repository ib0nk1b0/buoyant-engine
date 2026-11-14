
// NOTE: Services provided to the platform layer

struct GameMemory
{
    Arena*      permenantArena;
    Arena*      transientArena;

    bool        initialized;

    Renderer2D* renderer;
    Texture2D** textures;
    Font*       font;
};

void BuoyantUpdate(GameMemory* memory, f32 dt);
