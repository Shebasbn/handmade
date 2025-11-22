#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H
/* ================================================================
   $File: handmade_tile.h
   $Date:
   $Revision:
   $Creator: Sebastian Bautista
   $Notice:
   ================================================================*/
#include "handmade_core.h"

struct tile_map_position
{
    // Fixed point tile locations. 
    // The High bits are the tile chunk index, and the low bits are the
    // tile index in the chunk.
    uint32 AbsTileX;
    uint32 AbsTileY;
    uint32 AbsTileZ;

    // NOTE(sebas): This is tile-relative X and Y
    real32 TileRelX;
    real32 TileRelY;
};

struct tile_chunk_position
{
    uint32 TileChunkX;
    uint32 TileChunkY;
    uint32 TileChunkZ;

    uint32 RelTileX;
    uint32 RelTileY;
};

struct tile_chunk
{
    uint32* Tiles;
};

struct tile_map
{
    uint32 ChunkShift;
    uint32 ChunkMask;
    uint32 ChunkDim;

    real32 TileSideInMeters;

    uint32 TileChunkCountX;
    uint32 TileChunkCountY;
    uint32 TileChunkCountZ;

    tile_chunk* TileChunks;
};

#include "handmade_tile.cpp"
#endif