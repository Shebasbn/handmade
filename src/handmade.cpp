/* ================================================================
   $File: handmade.cpp
   $Date: 
   $Revision:
   $Creator: Sebastian Bautista
   $Notice: 
   ================================================================*/
#include "handmade.h"


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
        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                        (uint8*)Memory->PermanentStorage + sizeof(game_state));

        GameState->World = PushSize(&GameState->WorldArena, world);
        world* World = GameState->World;
        World->TileMap = PushSize(&GameState->WorldArena, tile_map);
        
        tile_map* TileMap = World->TileMap;

        TileMap->ChunkShift = 4;
        TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
        TileMap->ChunkDim = (1 << TileMap->ChunkShift);

        TileMap->TileChunkCountX = 128;
        TileMap->TileChunkCountY = 128;

        TileMap->TileChunks = PushArray(&GameState->WorldArena, 
                                        TileMap->TileChunkCountX*TileMap->TileChunkCountY, 
                                        tile_chunk);

        TileMap->TileSideInMeters = 1.5f;
        TileMap->TileSideInPixels = 60;

        TileMap->MetersToPixels = (real32)TileMap->TileSideInPixels / (real32)TileMap->TileSideInMeters;
        for (uint32 Y = 0;
             Y <TileMap->TileChunkCountY;
             Y++)
        {
            for (uint32 X = 0;
                X < TileMap->TileChunkCountX;
                X++)
            {
                TileMap->TileChunks[Y * TileMap->TileChunkCountX + X].Tiles = 
                    PushArray(&GameState->WorldArena, TileMap->ChunkDim * TileMap->ChunkDim, uint32);
            }
        }

        real32 LowerLeftX = -(real32)TileMap->TileSideInPixels / 2;
        real32 LowerLeftY = (real32)Buffer->Height;

        uint32 TilesPerWidth = 17;
        uint32 TilesPerHeight = 9;
        for (uint32 ScreenY = 0;
            ScreenY < 32;
            ScreenY++)
        {
            for (uint32 ScreenX = 0;
                ScreenX < 32;
                ScreenX++)
            {
                for (uint32 TileY = 0;
                    TileY < TilesPerHeight;
                    ++TileY)
                {
                    for (uint32 TileX = 0;
                        TileX < TilesPerWidth;
                        ++TileX)
                    {
                        uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
                        uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;
                        SetTileValue(&GameState->WorldArena, World->TileMap, AbsTileX, AbsTileY, 
                            ((TileX == TileY) && (TileY % 2)) ? 1 : 0);
                    }
                }
            }
        }
        
        // TODO(Sebas): This may be more appropriate to do in the platform layer
        Memory->IsInitialized = true;
    }

    real32 PlayerHeight = 1.4f;
    real32 PlayerWidth = 0.75f * PlayerHeight;

    world* World = GameState->World;
    tile_map* TileMap = World->TileMap;

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
                real32 PlayerSpeed = 2.0f;

                if (Controller->ActionUp.EndedDown)
                {
                    PlayerSpeed = 10.0f;
                }
                dPlayerX *= PlayerSpeed;
                dPlayerY *= PlayerSpeed;

                tile_map_position NewPlayerP = GameState->PlayerP;
                NewPlayerP.TileRelX += dPlayerX * Input->dtPerFrame;
                NewPlayerP.TileRelY += dPlayerY * Input->dtPerFrame;
                NewPlayerP = RecannonicalizePosition(TileMap, NewPlayerP);

                tile_map_position PlayerLeft = NewPlayerP;
                PlayerLeft.TileRelX -= 0.5f * PlayerWidth;
                PlayerLeft = RecannonicalizePosition(TileMap, PlayerLeft);
                tile_map_position PlayerRight = NewPlayerP;
                PlayerRight.TileRelX += 0.5f * PlayerWidth;
                PlayerRight = RecannonicalizePosition(TileMap, PlayerRight);
                if (IsTileMapPointEmpty(TileMap, NewPlayerP) &&
                    IsTileMapPointEmpty(TileMap, PlayerLeft) &&
                    IsTileMapPointEmpty(TileMap, PlayerRight))
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

    real32 ScreenCenterX = 0.5f * (real32)Buffer->Width;
    real32 ScreenCenterY = 0.5f * (real32)Buffer->Height;

    for (int32 RelRow =  -10; RelRow < 10; RelRow++)
    {
        for (int32 RelColumn = -20; RelColumn < 20; RelColumn++)
        {
            uint32 Column = GameState->PlayerP.AbsTileX + RelColumn;
            uint32 Row = GameState->PlayerP.AbsTileY + RelRow;

            uint32 TileID = GetTileValue(TileMap, Column, Row);
            real32 Gray = 0.5f;
            if (TileID == 1)
            {
                Gray = 1.0f;
            }

            if ((Column == GameState->PlayerP.AbsTileX) && (Row == GameState->PlayerP.AbsTileY))
            {
                Gray = 0.0f;
            }

            real32 CenterX = ScreenCenterX - TileMap->MetersToPixels * GameState->PlayerP.TileRelX 
                             + ((real32)RelColumn) * TileMap->TileSideInPixels;
            real32 CenterY = ScreenCenterY + TileMap->MetersToPixels * GameState->PlayerP.TileRelY 
                             - ((real32)RelRow) * TileMap->TileSideInPixels;
            real32 MinX = CenterX - 0.5f * TileMap->TileSideInPixels;//ScreenCenterX - TileMap->MetersToPixels * (GameState->PlayerP.TileRelX + TileMap->TileSideInMeters/2.0f) + ((real32)RelColumn) * TileMap->TileSideInPixels;
            real32 MinY = CenterY - 0.5f * TileMap->TileSideInPixels;//ScreenCenterY + TileMap->MetersToPixels * (GameState->PlayerP.TileRelY + TileMap->TileSideInMeters / 2.0f) - ((real32)RelRow) * TileMap->TileSideInPixels;
            real32 MaxX = CenterX + 0.5f * TileMap->TileSideInPixels;
            real32 MaxY = CenterY + 0.5f * TileMap->TileSideInPixels;
            DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
        }
    }

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;
    real32 PlayerLeft = ScreenCenterX - PlayerWidth * 0.5f * TileMap->MetersToPixels;
    real32 PlayerTop = ScreenCenterY - PlayerHeight * TileMap->MetersToPixels;
    DrawRectangle(Buffer, 
                  PlayerLeft, PlayerTop, 
                  PlayerLeft + PlayerWidth * TileMap->MetersToPixels, 
                  PlayerTop + PlayerHeight * TileMap->MetersToPixels, 
                  PlayerR, PlayerG, PlayerB);
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
