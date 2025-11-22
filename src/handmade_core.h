#ifndef HANDMADE_CORE_H
#define HANDMADE_CORE_H
/* ================================================================
   $File: handmade_core.h
   $Date:
   $Revision:
   $Creator: Sebastian Bautista
   $Notice:
   ================================================================*/

#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef size_t memory_index;

typedef float real32;
typedef double real64;

#if !defined(AssertBreak)
#define AssertBreak() (*(int*)0 = 0)
#endif

#define Stmnt(S) do{ S }while(0)
#if HANDMADE_SLOW 
#define Assert(c) Stmnt(if(!(c)){ AssertBreak(); })
#else
#define Assert(c)
#endif

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#include "handmade_intrinsics.h"

#endif // !HANDMADE_CORE_H