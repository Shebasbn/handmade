/* ================================================================
   $File: handmade.cpp
   $Date: 
   $Revision:
   $Creator: Sebastian Bautista
   $Notice: 
   ================================================================*/
#include "handmade.h"
#include "handmade_random.h"

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
DrawRectangle(game_frame_buffer* Buffer, v2 vMin, v2 vMax, real32 R, real32 G, real32 B)
{
    int32 MinX = RoundReal32ToInt32(vMin.X);
    int32 MinY = RoundReal32ToInt32(vMin.Y);
    int32 MaxX = RoundReal32ToInt32(vMax.X);
    int32 MaxY = RoundReal32ToInt32(vMax.Y);

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

#pragma pack(push, 1)
struct bitmap_header
{
    uint16 FileType;
    uint32 FileSize;
    uint16 Reserved1;
    uint16 Reserved2;
    uint32 BitmapOffset;
    uint32 Size;
    int32  Width;
    int32  Height;
    uint16 Planes;
    uint16 BitsPerPixel;
    uint32 Compression;
    uint32 SizeOfBitmap;
    int32  HorzResolution;
    int32  VertResolution;
    uint32 ColorsUsed;
    uint32 ColorsImportant;

    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;
};
#pragma pack(pop)

internal loaded_bitmap
DEBUGLoadBMP(thread_context* Thread, debug_platform_read_entire_file* ReadEntireFile, char* FileName)
{
    loaded_bitmap Result = {};
    // NOTE(Sebas): Byte order in memory BB GG RR AA or AA RR GG BB
    debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);
    
    if (ReadResult.ContentsSize != 0)
    {
        bitmap_header* Header = (bitmap_header*)ReadResult.Contents;
        uint32* Pixels = (uint32*)((uint8*)ReadResult.Contents + Header->BitmapOffset);
        Result.Pixels = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;
#if 0 
        uint32* SourceDest = Pixels;
        for (int32 Y = 0;
             Y < Header->Width;
             ++Y)
        {

            for (int32 X = 0;
                 X < Header->Height;
                 ++X)
            {
                *SourceDest = (*SourceDest >> 8) | (*SourceDest << 24);
                ++SourceDest;
            }
        }
#endif
    }

    return Result;
}

internal void
DrawBitmap(game_frame_buffer* Buffer, loaded_bitmap* Bitmap, real32 RealX, real32 RealY)
{
    int32 MinX = RoundReal32ToInt32(RealX);
    int32 MinY = RoundReal32ToInt32(RealY);
    int32 MaxX = RoundReal32ToInt32(Bitmap->Width + RealX);
    int32 MaxY = RoundReal32ToInt32(Bitmap->Height + RealY);

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


    uint32* SourceRow = Bitmap->Pixels + Bitmap->Width * (Bitmap->Height - 1);
    uint8* DestRow = ((uint8*)Buffer->Memory + (MinX * Buffer->BytesPerPixel) + (MinY * Buffer->Pitch));
    for (int32 Y = MinY;
        Y < MaxY;
        ++Y)
    {
        uint32* Dest = (uint32*)DestRow;
        uint32* Source = SourceRow;
        for (int32 X = MinX;
            X < MaxX;
            ++X)
        {
            if (*Source)
            {
               *Dest++ = *Source++;
            }
            else
            {
                Dest++;
                Source++;
            }
        }
        DestRow += Buffer->Pitch;
        SourceRow -= Bitmap->Width;
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    
    game_state* GameState = (game_state*)Memory->PermanentStorage;

    if(!Memory->IsInitialized)
    {
        GameState->Background = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/my_test_background.bmp");
        GameState->PlayerHead = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/my_test_hero_front_head.bmp");
        GameState->PlayerCape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/my_test_hero_front_cape.bmp");
        GameState->PlayerTorso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/my_test_hero_front_torso.bmp");
        GameState->PlayerP.AbsTileX = 3;
        GameState->PlayerP.AbsTileY = 2;
        GameState->PlayerP.Offset.X = 0;
        GameState->PlayerP.Offset.Y = 0;
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
        TileMap->TileChunkCountZ = 2;

        TileMap->TileChunks = PushArray(&GameState->WorldArena, 
                                        TileMap->TileChunkCountX*
                                        TileMap->TileChunkCountY*
                                        TileMap->TileChunkCountZ,
                                        tile_chunk);

        TileMap->TileSideInMeters = 1.5f;
        

        

        uint32 RandomNumberIndex = 0;

        uint32 TilesPerWidth = 17;
        uint32 TilesPerHeight = 9;
        uint32 ScreenX = 0;
        uint32 ScreenY = 0;

        bool32 DoorLeft = false;
        bool32 DoorRight = false;
        bool32 DoorTop = false;
        bool32 DoorBottom = false;
        bool32 DoorUp = false;
        bool32 DoorDown = false;
        uint32 AbsTileZ = 0;

        for (uint32 ScreenIndex = 0;
             ScreenIndex < 100;
             ++ScreenIndex)
        {
            Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
            uint32 RandomChoice;
            if (DoorUp || DoorDown)
            {
                RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
            }
            else
            {
                RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
            }

            bool32 CreatedZDoor = false;
            if (RandomChoice == 2)
            {
                if (AbsTileZ == 0)
                {
                    DoorUp = true;
                }
                else
                {
                    DoorDown = true;
                }
                CreatedZDoor = true;
            }
            else if (RandomChoice == 1)
            {
                DoorRight = true;
            }
            else
            {
                DoorTop = true;
            }

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

                    uint32 TileValue = 1;
                    if (TileX == 0)
                    {
                        if ((TileY == (TilesPerHeight / 2)) && DoorLeft)
                        {
                            TileValue = 1;
                        }
                        else
                        {
                            TileValue = 2;
                        }
                    }
                    if(TileX == TilesPerWidth - 1)
                    {
                        if ((TileY == (TilesPerHeight / 2)) && DoorRight)
                        {
                            TileValue = 1;
                        }
                        else
                        {
                            TileValue = 2;
                        }
                    }
                    if (TileY == 0)
                    {
                        if ((TileX == (TilesPerWidth / 2)) && DoorBottom)
                        {
                            TileValue = 1;
                        }
                        else
                        {
                            TileValue = 2;
                        }
                    }
                    if (TileY == TilesPerHeight - 1)
                    {
                        if ((TileX == (TilesPerWidth / 2)) && DoorTop)
                        {
                            TileValue = 1;
                        }
                        else
                        {
                            TileValue = 2;
                        }
                    }
                    if ((TileX == 5) && (TileY == 5))
                    {
                        if (DoorUp)
                        {
                            TileValue = 3;
                        }
                        else if (DoorDown)
                        {
                            TileValue = 4;
                        }
                    }

                    SetTileValue(&GameState->WorldArena, World->TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
                }
            }
            
            DoorLeft = DoorRight;
            DoorBottom = DoorTop;
            if (CreatedZDoor)
            {

                DoorDown = !DoorDown;
                DoorUp = !DoorUp;
            }
            else
            {
                DoorDown = false;
                DoorUp = false;
            }

            DoorRight = false;
            DoorTop = false;

            if (RandomChoice == 2)
            {
                if (AbsTileZ == 0)
                {
                    AbsTileZ = 1;
                }
                else
                {
                    AbsTileZ = 0;
                }
            }
            else if (RandomChoice == 1)
            {
                ScreenX += 1;
            }
            else
            {
                ScreenY += 1;
            }
        }
        
        // TODO(Sebas): This may be more appropriate to do in the platform layer
        Memory->IsInitialized = true;
    }
    world* World = GameState->World;
    tile_map* TileMap = World->TileMap;

    uint32 TileSideInPixels = 60;
    real32 MetersToPixels = (real32)TileSideInPixels / (real32)TileMap->TileSideInMeters;
    
    real32 LowerLeftX = -(real32)TileSideInPixels / 2.0f;
    real32 LowerLeftY = (real32)Buffer->Height;

    real32 PlayerHeight = 1.4f;
    real32 PlayerWidth = 0.75f * PlayerHeight;
    v2 PlayerDim = { PlayerWidth, PlayerHeight };


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
                v2 dPlayer = {};

                if (Controller->MoveUp.EndedDown)
                {
                    dPlayer.Y = 1.0f;
                }
                if (Controller->MoveDown.EndedDown)
                {
                    dPlayer.Y = -1.0f;
                }
                if (Controller->MoveLeft.EndedDown)
                {
                    dPlayer.X = -1.0f;
                }
                if (Controller->MoveRight.EndedDown)
                {
                    dPlayer.X = 1.0f;
                }

                if ((dPlayer.X != 0.0f) && (dPlayer.Y != 0.0f))
                {
                    dPlayer *= 0.70710678118f;
                }

                real32 PlayerSpeed = 2.0f;

                if (Controller->ActionUp.EndedDown)
                {
                    PlayerSpeed = 10.0f;
                }
                dPlayer *= PlayerSpeed;
                

                tile_map_position NewPlayerP = GameState->PlayerP;
                NewPlayerP.Offset += dPlayer * Input->dtPerFrame;
                NewPlayerP = RecannonicalizePosition(TileMap, NewPlayerP);

                tile_map_position PlayerLeft = NewPlayerP;
                PlayerLeft.Offset.X -= 0.5f * PlayerDim.X;
                PlayerLeft = RecannonicalizePosition(TileMap, PlayerLeft);
                tile_map_position PlayerRight = NewPlayerP;
                PlayerRight.Offset.X += 0.5f * PlayerDim.X;
                PlayerRight = RecannonicalizePosition(TileMap, PlayerRight);
                if (IsTileMapPointEmpty(TileMap, NewPlayerP) &&
                    IsTileMapPointEmpty(TileMap, PlayerLeft) &&
                    IsTileMapPointEmpty(TileMap, PlayerRight))
                {
                    if (!AreOnSameTile(GameState->PlayerP, NewPlayerP))
                    {
                        uint32 NewTileValue = GetTileValue(TileMap, NewPlayerP);
                        if (NewTileValue == 3)
                        {
                            NewPlayerP.AbsTileZ++;
                        }
                        else if (NewTileValue == 4)
                        {
                            NewPlayerP.AbsTileZ--;
                        }
                    }
                    GameState->PlayerP = NewPlayerP;
                }

            }
        }
    }
    DrawBitmap(Buffer, &GameState->Background, 0, 0);
    v2 ScreenCenter = V2(0.5f * (real32)Buffer->Width, 0.5f * (real32)Buffer->Height);

    for (int32 RelRow =  -10; RelRow < 10; RelRow++)
    {
        for (int32 RelColumn = -20; RelColumn < 20; RelColumn++)
        {
            uint32 Column = GameState->PlayerP.AbsTileX + RelColumn;
            uint32 Row = GameState->PlayerP.AbsTileY + RelRow;

            uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->PlayerP.AbsTileZ);
            if (TileID > 1)
            {
                real32 Gray = 0.5f;
                if (TileID == 2)
                {
                    Gray = 1.0f;
                }

                if (TileID > 2)
                {
                    Gray = 0.25f;
                }

                if ((Column == GameState->PlayerP.AbsTileX) && (Row == GameState->PlayerP.AbsTileY))
                {
                    Gray = 0.0f;
                }

                //v2 RelColRow = V2((real32)RelColumn, (real32)RelRow);
                //v2 ScreenOffset = (MetersToPixels * GameState->PlayerP.Offset) + (RelColRow * (real32)TileSideInPixels);
                v2 Center = { 
                    ScreenCenter.X - MetersToPixels * GameState->PlayerP.Offset.X + ((real32)RelColumn) * TileSideInPixels,
                    ScreenCenter.Y + MetersToPixels * GameState->PlayerP.Offset.Y - ((real32)RelRow) * TileSideInPixels 
                };
                v2 TileSide = { 0.5f * (real32)TileSideInPixels, 0.5f * (real32)TileSideInPixels };
                v2 Min = Center - TileSide;
                v2 Max = Center + TileSide;
                DrawRectangle(Buffer, Min, Max, Gray, Gray, Gray);
            }
        }
    }

    real32 PlayerR = 1.0f;
    real32 PlayerG = 1.0f;
    real32 PlayerB = 0.0f;
    v2 PlayerLeftTop = 
    { 
        ScreenCenter.X - PlayerWidth * 0.5f * MetersToPixels, 
        ScreenCenter.Y - PlayerHeight * MetersToPixels 
    };

    v2 PlayerBottomRight = PlayerLeftTop + PlayerDim * MetersToPixels;
    DrawRectangle(Buffer, 
                  PlayerLeftTop,
                  PlayerBottomRight,
                  PlayerR, PlayerG, PlayerB);
    DrawBitmap(Buffer, &GameState->PlayerTorso, ScreenCenter.X - GameState->PlayerTorso.Width * 0.5f, PlayerLeftTop.Y);
    DrawBitmap(Buffer, &GameState->PlayerHead, ScreenCenter.X - GameState->PlayerTorso.Width * 0.5f, PlayerLeftTop.Y);
    DrawBitmap(Buffer, &GameState->PlayerCape, ScreenCenter.X - GameState->PlayerTorso.Width * 0.5f, PlayerLeftTop.Y);
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
