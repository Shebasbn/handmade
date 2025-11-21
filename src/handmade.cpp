/* ================================================================
   $File: handmade.cpp
   $Date: 
   $Revision:
   $Creator: Sebastian Bautista
   $Notice: 
   ================================================================*/
#include "handmade.h"
#include "handmade_intrinsics.h"

internal void
GameOutputSound(game_sound_output_buffer* SoundBuffer,game_state* GameState, int ToneHz)
                
{
    Assert(ToneHz != 0);
    int32 ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;
    int16* SampleOut = SoundBuffer->Samples;
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; SampleIndex++)
    {
#if 0
        real32 SineValue = sinf(GameState->tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
        int16 SampleValue = 0;
#endif
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;
#if 0
        GameState->tSine += 2.0f*Pi32*1.0f/(real32)WavePeriod;
        if (GameState->tSine > 2.0f * Pi32)
        {
            GameState->tSine -= 2.0f * Pi32;
        }
#else

#endif
    }
}

internal void
DrawRectangle(game_frame_buffer* Buffer, 
              real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY, 
              real32 R, real32 G, real32 B)
{
    int32 MinX = RoundReal32ToInt32(RealMinX);
    int32 MinY = RoundReal32ToInt32(RealMinY);
    int32 MaxX = RoundReal32ToInt32(RealMaxX);
    int32 MaxY = RoundReal32ToInt32(RealMaxY);

    if (MinX < 0)
    {
        MinX = 0;
    }
    if (MinY < 0)
    {
        MinY = 0;
    }
    if (MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    if (MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint32 Color = (uint32)((RoundReal32ToUint32(R * 255.0f) << 16) |
                            (RoundReal32ToUint32(G * 255.0f) << 8) |
                            (RoundReal32ToUint32(B * 255.0f) << 0));

    uint8* Row = ((uint8*)Buffer->Memory + (MinX * Buffer->BytesPerPixel) + (MinY * Buffer->Pitch));
    for (int Y = MinY; Y < MaxY; Y++)
    {
        uint32* Pixel = (uint32*)Row;
        for (int X = MinX; X < MaxX; X++)
        {
           *Pixel++ = Color;
        }
        Row += Buffer->Pitch;
    }
}

inline tile_chunk*
GetTileChunk(world* World, int32 TileChunkX, int32 TileChunkY)
{
    tile_chunk* TileChunk = 0;
    if ((TileChunkX >= 0) && (TileChunkX < World->TileChunkCountX) &&
        (TileChunkY >= 0) && (TileChunkY < World->TileChunkCountY))
    {
        TileChunk = &World->TileChunks[TileChunkY * World->TileChunkCountX + TileChunkX];
    }
    return TileChunk;
}

inline uint32
GetTileValueUnchecked(world* World, tile_chunk* TileChunk, uint32 TestTileX, uint32 TestTileY)
{
    Assert(World);
    Assert(TileChunk);
    Assert(TestTileX < World->ChunkDim);
    Assert(TestTileY < World->ChunkDim);
    uint32 TileValue = TileChunk->Tiles[TestTileY * World->ChunkDim + TestTileX];
    return TileValue;
}

internal uint32
GetTileValue(world* World, tile_chunk* TileChunk, uint32 TestTileX, uint32 TestTileY)
{
    uint32 TileChunkValue = 0;
    if (TileChunk)
    {
        TileChunkValue = GetTileValueUnchecked(World, TileChunk, TestTileX, TestTileY);
    }

    return TileChunkValue;
}

inline void
RecanonicalizeCoord(world* World, uint32* Tile, real32* TileRel)
{
    int32 Offset = FloorReal32ToInt32(*TileRel / (real32)World->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= (real32)Offset * (real32)World->TileSideInMeters;

    Assert(*TileRel >= 0);
    Assert(*TileRel < World->TileSideInMeters);
}

inline world_position
RecannonicalizePosition(world* World, world_position Pos)
{
    world_position Result = Pos;

    RecanonicalizeCoord(World, &Result.AbsTileX, &Result.TileRelX);
    RecanonicalizeCoord(World, &Result.AbsTileY, &Result.TileRelY);
    return Result;
}

inline tile_chunk_position
GetChunkPositionFor(world* World, uint32 AbsTileX, uint32 AbsTileY)
{
    tile_chunk_position Result;
    Result.TileChunkX = AbsTileX >> World->ChunkShift;
    Result.TileChunkY = AbsTileY >> World->ChunkShift;
    Result.RelTileX = AbsTileX & World->ChunkMask;
    Result.RelTileY = AbsTileY & World->ChunkMask;
    return Result;
}

inline uint32
GetTileValue(world* World, uint32 AbsTileX, uint32 AbsTileY)
{
    tile_chunk_position ChunkPos = GetChunkPositionFor(World, AbsTileX, AbsTileY);
    tile_chunk* TileChunk = GetTileChunk(World, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
    uint32 TileChunkValue = GetTileValue(World, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);

    return TileChunkValue;
}

internal bool32
IsWorldPointEmpty(world* World, world_position CanPos)
{
    uint32 TileChunkValue = GetTileValue(World, CanPos.AbsTileX, CanPos.AbsTileY);
    bool32 Empty = (TileChunkValue == 0);
    
    return Empty;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    game_state* GameState = (game_state*)Memory->PermanentStorage;
    if(!Memory->IsInitialized)
    {
        GameState->PlayerP.AbsTileX = 3;
        GameState->PlayerP.AbsTileY = 2;
        GameState->PlayerP.TileRelX = 0.0f;
        GameState->PlayerP.TileRelY = 0.0f;
        
        // TODO(Sebas): This may be more appropriate to do in the platform layer
        Memory->IsInitialized = true;
    }


#define TILE_CHUNK_COUNT_X 256
#define TILE_CHUNK_COUNT_Y 256
    uint32 TempTiles[TILE_CHUNK_COUNT_Y][TILE_CHUNK_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1}, // 0
        {1, 1, 0, 0,  0, 1, 0, 0,  0,  0, 0, 0, 0,  1, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1}, // 1
        {1, 1, 0, 0,  0, 0, 0, 0,  1,  0, 0, 0, 0,  0, 1, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1}, // 2

        {1, 0, 0, 0,  0, 0, 0, 0,  1,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1}, // 3
        {1, 0, 0, 0,  0, 1, 0, 0,  1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1}, // 4
        {1, 1, 0, 0,  0, 1, 0, 0,  1,  0, 0, 0, 0,  1, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1}, // 5

        {1, 0, 0, 0,  0, 1, 0, 0,  1,  0, 0, 0, 1,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1}, // 6
        {1, 0, 1, 1,  1, 0, 0, 0,  0,  0, 0, 0, 0,  1, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1}, // 7
        {1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1}, // 8

        {1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1}, // 9
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1}, // 10
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1}, // 11

        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1}, // 12
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1}, // 13
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1}, // 14

        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1}, // 15
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1}, // 16
        {1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1}  // 17
      // 0  1  2  3   4  5  6  7   8   9  10 11 12 13 14 15 16   17 18 19 20  21 22 23 24  25  26 27 28 29  30 31 32 33
    };

    world World{};
    // This is set to using 256x256 tile chunks
    World.ChunkShift = 8;
    World.ChunkMask = 0xFF;
    World.ChunkDim = 256;
    World.TileSideInMeters = 1.5f;
    World.TileSideInPixels = 60;
    World.MetersToPixels = (real32)World.TileSideInPixels / (real32)World.TileSideInMeters;
    World.TileChunkCountX = 1;
    World.TileChunkCountY = 1;

    tile_chunk TileChunk;
    TileChunk.Tiles = (uint32*)TempTiles;
    World.TileChunks = &TileChunk;

    real32 LowerLeftX = -(real32)World.TileSideInPixels / 2;
    real32 LowerLeftY = (real32)Buffer->Height;


    real32 PlayerHeight = 1.4f;
    real32 PlayerWidth = 0.75f * PlayerHeight;

    for (int ControllerIndex = 0; 
         ControllerIndex < ArrayCount(Input->Controllers);
         ControllerIndex++)
    {
        game_controller_input* Controller = GetController(Input, ControllerIndex);
        if (Controller->IsConnected)
        {
            if (Controller->IsAnalog)
            {
            }
            else
            {
                real32 dPlayerX = 0.0f;
                real32 dPlayerY = 0.0f;
                if (Controller->MoveUp.EndedDown)
                {
                    dPlayerY = 1.0f;
                }
                if (Controller->MoveDown.EndedDown)
                {
                    dPlayerY = -1.0f;
                }
                if (Controller->MoveLeft.EndedDown)
                {
                    dPlayerX = -1.0f;
                }
                if (Controller->MoveRight.EndedDown)
                {
                    dPlayerX = 1.0f;
                }
                dPlayerX *= 2.0f;
                dPlayerY *= 2.0f;

                world_position NewPlayerP = GameState->PlayerP;
                NewPlayerP.TileRelX += dPlayerX * Input->dtPerFrame;
                NewPlayerP.TileRelY += dPlayerY * Input->dtPerFrame;
                NewPlayerP = RecannonicalizePosition(&World, NewPlayerP);

                world_position PlayerLeft = NewPlayerP;
                PlayerLeft.TileRelX -= 0.5f * PlayerWidth;
                PlayerLeft = RecannonicalizePosition(&World, PlayerLeft);
                world_position PlayerRight = NewPlayerP;
                PlayerRight.TileRelX += 0.5f * PlayerWidth;
                PlayerRight = RecannonicalizePosition(&World, PlayerRight);
                if (IsWorldPointEmpty(&World, NewPlayerP) &&
                    IsWorldPointEmpty(&World, PlayerLeft) &&
                    IsWorldPointEmpty(&World, PlayerRight))
                {
                    GameState->PlayerP = NewPlayerP;
                }

            }
        }
    }

    /*tile_chunk_position PlayerChunkP = GetChunkPositionFor(&World, GameState->PlayerP.AbsTileX, GameState->PlayerP.AbsTileY);
    tile_chunk* NewTileChunk = GetTileChunk(&World, PlayerChunkP.TileChunkX, PlayerChunkP.TileChunkY);
    Assert(NewTileChunk);*/

    DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 
                  1.0f, 0.0f, 1.0f);

    real32 CenterX = 0.5f * (real32)Buffer->Width;
    real32 CenterY = 0.5f * (real32)Buffer->Height;

    for (int32 RelRow =  -10; RelRow < 10; RelRow++)
    {
        for (int32 RelColumn = -20; RelColumn < 20; RelColumn++)
        {
            uint32 Column = GameState->PlayerP.AbsTileX + RelColumn;
            uint32 Row = GameState->PlayerP.AbsTileY + RelRow;

            uint32 TileID = GetTileValue(&World, Column, Row);
            real32 Gray = 0.5f;
            if (TileID == 1)
            {
                Gray = 1.0f;
            }

            if ((Column == GameState->PlayerP.AbsTileX) && (Row == GameState->PlayerP.AbsTileY))
            {
                Gray = 0.0f;
            }

            real32 MinX = CenterX + ((real32)RelColumn) * World.TileSideInPixels;
            real32 MinY = CenterY - ((real32)RelRow) * World.TileSideInPixels;
            real32 MaxX = MinX + World.TileSideInPixels;
            real32 MaxY = MinY - World.TileSideInPixels;
            DrawRectangle(Buffer, MinX, MaxY, MaxX, MinY, Gray, Gray, Gray);
        }
    }

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;
    real32 PlayerLeft = CenterX + World.MetersToPixels * GameState->PlayerP.TileRelX - (PlayerWidth * World.MetersToPixels) / 2.0f;
    real32 PlayerTop = CenterY - World.MetersToPixels * GameState->PlayerP.TileRelY - PlayerHeight * World.MetersToPixels;
    DrawRectangle(Buffer, 
                  PlayerLeft, PlayerTop, 
                  PlayerLeft + PlayerWidth * World.MetersToPixels, 
                  PlayerTop + PlayerHeight * World.MetersToPixels, PlayerR, PlayerG, PlayerB);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state* GameState = (game_state*)Memory->PermanentStorage;
    GameOutputSound(SoundBuffer, GameState, 400);
}


//internal void
//RenderWeirdGradient(game_frame_buffer* Buffer, int BlueOffset, int GreenOffset)
//{
//#if 0
//    BlueOffset = 0;
//    GreenOffset = 0;
//#else
//#endif
//    uint8* Row = (uint8*)Buffer->Memory;
//    for (int y = 0; y < Buffer->Height; y++)
//    {
//
//        uint32* Pixel = (uint32*)Row;
//        for (int x = 0; x < Buffer->Width; x++)
//        {
//            uint8 Blue = (uint8)(x + BlueOffset);
//            uint8 Green = (uint8)(y + GreenOffset);
//            uint8 Red = 0;
//            uint8 Alpha = 255;
//            *Pixel++ = ((Alpha << 24) | (Red << 16) | (Green << 16) | (Blue)); // BLUE Value
//        }
//        Row += Buffer->Pitch;
//    }
//}
