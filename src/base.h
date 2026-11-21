#pragma once

#include <stdbool.h>
#include <stdint.h>

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

#define Kilobytes(x) 1024 * (x)
#define Megabytes(x) 1024 * Kilobytes(x)
#define Gigabytes(x) 1024 * Megabytes(x)
#define Terabytes(x) 1024 * Gigabytes(x)

struct Arena
{
    u64 pos;
    u64 size;
};

// Arena* ArenaCreate(u64 size);
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
