#ifndef HANDMADE_H_
#define HANDMADE_H_
/* ================================================================
   $File: handmade.h
   $Date: 
   $Revision:
   $Creator: Sebastian Bautista
   $Notice: 
   ================================================================*/

#include "handmade_platform.h"

struct memory_arena
{
    memory_index Size;
    memory_index Used;
    uint8* Base;
};

internal void
InitializeArena(memory_arena* Arena, memory_index Size, uint8* Base)
{
    Arena->Size = Size;
    Arena->Base = Base;
    Arena->Used = 0;
}

#define PushSize(Arena, type) (type*)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type*)PushSize_(Arena, (Count) * sizeof(type))
internal void*
PushSize_(memory_arena* Arena, memory_index Size)
{
    Assert((Arena->Used + Size) <= Arena->Size);
    void* Result = Arena->Base + Arena->Used;
    Arena->Used += Size;

    return Result;
}

#include "handmade_math.h"
#include "handmade_intrinsics.h"
#include "handmade_tile.h"

struct world
{
    tile_map* TileMap;

};

struct loaded_bitmap
{
    int32 Width;
    int32 Height;
    uint32* Pixels;
};

struct game_state
{
    memory_arena WorldArena;
    world* World;

    tile_map_position PlayerP;

    loaded_bitmap Background;
    loaded_bitmap PlayerHead;
    loaded_bitmap PlayerCape;
    loaded_bitmap PlayerTorso;
};


#endif // HANDMADE_H_
