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

inline tile_map*
GetTileMap(world* World, int32 TileMapX, int32 TileMapY)
{
    tile_map* TileMap = 0;
    if ((TileMapX >= 0) && (TileMapX < World->TileMapCountX) &&
        (TileMapY >= 0) && (TileMapY < World->TileMapCountY))
    {
        TileMap = &World->TileMaps[TileMapY * World->TileMapCountX + TileMapX];
    }
    return TileMap;
}

inline uint32
GetTileValue(world* World, tile_map* TileMap, int32 TestTileX, int32 TestTileY)
{
    Assert(TileMap);
    Assert((TestTileX >= 0) && (TestTileX < World->CountX) &&
        (TestTileY >= 0) && (TestTileY < World->CountY));
    uint32 TileValue = TileMap->Tiles[TestTileY * World->CountX + TestTileX];
    return TileValue;
}

internal bool32
IsTileMapPointEmpty(world* World, tile_map* TileMap, int32 TestTileX, int32 TestTileY)
{
    Assert(World);
    Assert(TileMap);
    bool32 Empty = false;

    if ((TestTileX >= 0) && (TestTileX < World->CountX) &&
        (TestTileY >= 0) && (TestTileY < World->CountY))
    {
        uint32 TileMapValue = GetTileValue(World, TileMap, TestTileX, TestTileY);
        Empty = (TileMapValue == 0);
    }

    return Empty;
}

inline void
RecanonicalizeCoord(world* World, int32 TileCount, int32* TileMap, int32* Tile, real32* TileRel)
{
    int32 Offset = FloorReal32ToInt32(*TileRel / (real32)World->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= (real32)Offset * (real32)World->TileSideInMeters;

    Assert(*TileRel >= 0);
    Assert(*TileRel < World->TileSideInMeters);

    if (*Tile < 0)
    {
        *Tile = TileCount + *Tile;
        (*TileMap)--;
    }
    if (*Tile >= TileCount)
    {
        *Tile = *Tile - TileCount;
        (*TileMap)++;
    }
}

inline cannonical_position
RecannonicalizePosition(world* World, cannonical_position Pos)
{
    cannonical_position Result = Pos;

    RecanonicalizeCoord(World, World->CountX, &Result.TileMapX, &Result.TileX, &Result.TileRelX);
    RecanonicalizeCoord(World, World->CountY, &Result.TileMapY, &Result.TileY, &Result.TileRelY);
    return Result;
}

internal bool32
IsWorldPointEmpty(world* World, cannonical_position CanPos)
{
    bool32 Empty = false;

    tile_map* TileMap = GetTileMap(World, CanPos.TileMapX, CanPos.TileMapY);
    if (TileMap)
    {
        Empty = IsTileMapPointEmpty(World, TileMap, CanPos.TileX, CanPos.TileY);
    }

    return Empty;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    game_state* GameState = (game_state*)Memory->PermanentStorage;
    if(!Memory->IsInitialized)
    {
        GameState->PlayerP.TileMapX = 0;
        GameState->PlayerP.TileMapY = 0;
        GameState->PlayerP.TileX = 3;
        GameState->PlayerP.TileY = 3;
        GameState->PlayerP.TileRelX = 5.0f;
        GameState->PlayerP.TileRelY = 5.0f;
        
        // TODO(Sebas): This may be more appropriate to do in the platform layer
        Memory->IsInitialized = true;
    }


#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9
    uint32 Tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
        {1, 1, 0, 0,  0, 1, 0, 0,  0,  0, 0, 0, 0,  1, 0, 0, 1},
        {1, 1, 0, 0,  0, 0, 0, 0,  1,  0, 0, 0, 0,  0, 1, 0, 1},

        {1, 0, 0, 0,  0, 0, 0, 0,  1,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 1, 0, 0,  1,  0, 0, 0, 0,  0, 0, 0, 0},
        {1, 1, 0, 0,  0, 1, 0, 0,  1,  0, 0, 0, 0,  1, 0, 0, 1},

        {1, 0, 0, 0,  0, 1, 0, 0,  1,  0, 0, 0, 1,  0, 0, 0, 1},
        {1, 0, 1, 1,  1, 0, 0, 0,  0,  0, 0, 0, 0,  1, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
    };
    uint32 Tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},

        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 0},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},

        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
    };
    uint32 Tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},

        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {0, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},

        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
    };
    uint32 Tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  0,  1, 1, 1, 1,  1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},

        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {0, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},

        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0,  0, 0, 0, 0,  0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  1,  1, 1, 1, 1,  1, 1, 1, 1},
    };
    tile_map TileMaps[2][2]{};
    
    TileMaps[0][0].Tiles = (uint32*)Tiles00;
    TileMaps[0][1].Tiles = (uint32*)Tiles01;
    TileMaps[1][0].Tiles = (uint32*)Tiles10;
    TileMaps[1][1].Tiles = (uint32*)Tiles11;

    world World{};
    World.TileSideInMeters = 1.5f;
    World.TileSideInPixels = 60;
    World.MetersToPixels = (real32)World.TileSideInPixels / (real32)World.TileSideInMeters;
    World.UpperLeftX = -(real32)World.TileSideInPixels / 2;
    World.UpperLeftY = 0;
    World.TileMapCountX = 2;
    World.TileMapCountY = 2;
    World.CountX = TILE_MAP_COUNT_X;
    World.CountY = TILE_MAP_COUNT_Y;


    World.TileMaps = (tile_map*)TileMaps;

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
                    dPlayerY = -1.0f;
                }
                if (Controller->MoveDown.EndedDown)
                {
                    dPlayerY = 1.0f;
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

                cannonical_position NewPlayerP = GameState->PlayerP;
                NewPlayerP.TileRelX += dPlayerX * Input->dtPerFrame;
                NewPlayerP.TileRelY += dPlayerY * Input->dtPerFrame;
                NewPlayerP = RecannonicalizePosition(&World, NewPlayerP);

                cannonical_position PlayerLeft = NewPlayerP;
                PlayerLeft.TileRelX -= 0.5f * PlayerWidth;
                PlayerLeft = RecannonicalizePosition(&World, PlayerLeft);
                cannonical_position PlayerRight = NewPlayerP;
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

    tile_map* TileMap = GetTileMap(&World, GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY);
    Assert(TileMap);

    DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 
                  1.0f, 0.0f, 1.0f);

    for (int Row = 0; Row < 9; Row++)
    {
        for (int Column = 0; Column < 17; Column++)
        {
            uint32 TileID = GetTileValue(&World, TileMap, Column, Row);
            real32 Gray = 0.5f;
            if (TileID == 1)
            {
                Gray = 1.0f;
            }

            if ((Column == GameState->PlayerP.TileX) && (Row == GameState->PlayerP.TileY))
            {
                Gray = 0.0f;
            }
            real32 MinX = World.UpperLeftX + ((real32)Column) * World.TileSideInPixels;
            real32 MinY = World.UpperLeftY + ((real32)Row) * World.TileSideInPixels;
            real32 MaxX = MinX + World.TileSideInPixels;
            real32 MaxY = MinY + World.TileSideInPixels;
            DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
        }
    }

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;
    real32 PlayerLeft = World.UpperLeftX + World.MetersToPixels * GameState->PlayerP.TileRelX
                        + GameState->PlayerP.TileX * World.TileSideInPixels
                        - (PlayerWidth * World.MetersToPixels) / 2.0f;
    real32 PlayerTop = World.UpperLeftY + World.MetersToPixels * GameState->PlayerP.TileRelY
                       + GameState->PlayerP.TileY * World.TileSideInPixels
                       - PlayerHeight * World.MetersToPixels;
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
