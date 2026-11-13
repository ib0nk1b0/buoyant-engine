#pragma once

// TODO: these should be declared in platform independant header and the
// implementations will be platform specific
// only some implementations are actually platform dependant
// e.g. ArenaCreate/Release platform specific
// ArenaPush/Pop can be platform independant as its just pointer maths

#define Kilobytes(x) 1024 * (x)
#define Megabytes(x) 1024 * Kilobytes(x)
#define Gigabytes(x) 1024 * Megabytes(x)
#define Terabytes(x) 1024 * Gigabytes(x)

struct Arena
{
    u64 pos;
    u64 size;
};

Arena* ArenaCreate(u64 size);
void   ArenaRelease(Arena* arena);
void*  ArenaPush(Arena* arena, u64 size);
void*  ArenaPop(Arena* arena, u64 size);

#define ArenaPushStruct(arena, T)   ((T*)ArenaPush((arena), sizeof(T)))
#define ArenaPopStruct(arena, T)    ((T*)ArenaPop((arena),  sizeof(T)))
#define ArenaPushArray(arena, T, n) ((T*)ArenaPush((arena), sizeof(T) * (n)))
#define ArenaPopArray(arena, T, n)  ((T*)ArenaPop((arena),  sizeof(T) * (n)))

struct StringView
{
    char* data;
    u64   size;
};

char*      SvMakeCstr(Arena* arena, StringView sv);
StringView SvReadEntireFile(Arena* arena, const char* filepath);
StringView SvChopByDelim(StringView* sv, char delim);
StringView SvTrimLeft(StringView sv);
StringView SvTrimRight(StringView sv);
StringView SvTrim(StringView sv);
