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

inline cannonical_position
GetCannonicalPosition(world* World, raw_position Pos)
{
    cannonical_position Result{};

    Result.TileMapX = Pos.TileMapX;
    Result.TileMapY = Pos.TileMapY;

    real32 X = Pos.X - World->UpperLeftX;
    real32 Y = Pos.Y - World->UpperLeftY;
    Result.TileX = FloorReal32ToInt32(X / World->TileSideInPixels);
    Result.TileY = FloorReal32ToInt32(Y / World->TileSideInPixels);
    Result.TileRelX = X - ((real32)Result.TileX * World->TileSideInPixels);
    Result.TileRelY = Y - ((real32)Result.TileY * World->TileSideInPixels);
    Assert(Result.TileRelX >= 0);
    Assert(Result.TileRelY >= 0);
    Assert(Result.TileRelX < World->TileSideInPixels);
    Assert(Result.TileRelY < World->TileSideInPixels);

    if (Result.TileX < 0)
    {
        Result.TileX = World->CountX + Result.TileX;
        Result.TileMapX--;
    }
    if (Result.TileY < 0)
    {
        Result.TileY = World->CountY + Result.TileY;
        Result.TileMapY--;
    }
    if (Result.TileX >= World->CountX)
    {
        Result.TileX = Result.TileX - World->CountX;
        Result.TileMapX++;
    }
    if (Result.TileY >= World->CountY)
    {
        Result.TileY = Result.TileY - World->CountY;
        Result.TileMapY++;
    }
    return Result;
}

internal bool32
IsWorldPointEmpty(world* World, raw_position TestPos)
{
    bool32 Empty = false;

    cannonical_position CanPos = GetCannonicalPosition(World, TestPos);

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
        GameState->PlayerX = 150;
        GameState->PlayerY = 120;
        GameState->PlayerTileMapX = 0;
        GameState->PlayerTileMapY = 0;
        
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
    World.UpperLeftX = -(real32)World.TileSideInPixels / 2;
    World.UpperLeftY = 0;
    World.TileMapCountX = 2;
    World.TileMapCountY = 2;
    World.CountX = TILE_MAP_COUNT_X;
    World.CountY = TILE_MAP_COUNT_Y;


    World.TileMaps = (tile_map*)TileMaps;

    real32 PlayerWidth = 0.5f * (real32)World.TileSideInPixels;
    real32 PlayerHeight = 0.80f * (real32)World.TileSideInPixels;

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
                dPlayerX *= 64.0f;
                dPlayerY *= 64.0f;

                real32 NewPlayerX = GameState->PlayerX + dPlayerX * Input->dtPerFrame;
                real32 NewPlayerY = GameState->PlayerY + dPlayerY * Input->dtPerFrame;

               
                raw_position PlayerPos = { GameState->PlayerTileMapX, GameState->PlayerTileMapY,  NewPlayerX, NewPlayerY };
                raw_position PlayerLeft = PlayerPos;
                PlayerLeft.X -= 0.5f * PlayerWidth;
                raw_position PlayerRight = PlayerPos;
                PlayerRight.X += 0.5f * PlayerWidth;
                if (IsWorldPointEmpty(&World, PlayerPos) &&
                    IsWorldPointEmpty(&World, PlayerLeft) &&
                    IsWorldPointEmpty(&World, PlayerRight))
                {
                    cannonical_position CanPos = GetCannonicalPosition(&World, PlayerPos);
                    GameState->PlayerTileMapX = CanPos.TileMapX;
                    GameState->PlayerTileMapY = CanPos.TileMapY;
                    GameState->PlayerX = CanPos.TileRelX + (CanPos.TileX * World.TileSideInPixels) + World.UpperLeftX;
                    GameState->PlayerY = CanPos.TileRelY + (CanPos.TileY * World.TileSideInPixels) + World.UpperLeftY;
                }

            }
        }
    }

    tile_map* TileMap = GetTileMap(&World, GameState->PlayerTileMapX, GameState->PlayerTileMapY);
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
    real32 PlayerLeft = GameState->PlayerX - PlayerWidth / 2.0f;
    real32 PlayerTop = GameState->PlayerY - PlayerHeight;
    DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + PlayerWidth, PlayerTop + PlayerHeight, PlayerR, PlayerG, PlayerB);
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
