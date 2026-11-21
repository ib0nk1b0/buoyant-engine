
// NOTE: Services provided to the platform layer

struct GameState
{
    Renderer2D* renderer;
    Texture2D** textures;
    Font*       font;
};

struct GameMemory
{
    Arena*      permenantArena;
    Arena*      transientArena;
    GameState*  gameState;

    bool        initialized;
};

void BuoyantUpdate(GameMemory* memory, f32 dt);
