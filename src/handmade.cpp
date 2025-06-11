/* ================================================================
   $File: handmade.cpp
   $Date: 
   $Revision:
   $Creator: Sebastian Bautista
   $Notice: 
   ================================================================*/
#include "handmade.h"

internal void
GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz)
                
{
    Assert(ToneHz != 0);
    local_persist real32 tSine;
    int32 ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;
    int16* SampleOut = SoundBuffer->Samples;
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; SampleIndex++)
    {
        real32 SineValue = sinf(tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f*Pi32*1.0f/(real32)WavePeriod;
    }
}

internal void
RenderWeirdGradient(game_frame_buffer* Buffer, int BlueOffset, int GreenOffset)
{
    uint8* Row = (uint8*)Buffer->Memory;
    for(int y = 0; y < Buffer->Height; y++)
    {

        uint32* Pixel = (uint32*)Row;
        for(int x = 0; x < Buffer->Width; x++)
        {
            uint8 Blue = (uint8)(x + BlueOffset);
            uint8 Green = (uint8)(y + GreenOffset);
            uint8 Red = 0;
            uint8 Alpha = 255;
            *Pixel++ = ((Alpha << 24) | (Red << 16) | (Green << 8) | (Blue)); // BLUE Value
        }
        Row += Buffer->Pitch;
    }
}

internal void
GameUpdateAndRender(game_memory* Memory,
                    game_frame_buffer* Buffer,
                    game_sound_output_buffer* SoundBuffer,
                    game_input* Input)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    game_state* GameState = (game_state*)Memory->PermanentStorage;
    if(!Memory->IsInitialized)
    {
        char* FileName = __FILE__;
        debug_read_file_result File = DEBUGPlatformReadEntireFile(FileName);
        if (File.Contents)
        {
            DEBUGPlatformWriteEntireFile("test.out", File.Contents, File.ContentsSize);
            DEBUGPlatformFreeFileMemory(File.Contents);
            File = {};
        }
        GameState->ToneHz = 255;

        // TODO(Sebas): This may be more appropriate to do in the platform layer
        Memory->IsInitialized = true;
    }

    for (int ControllerIndex = 0; 
         ControllerIndex < ArrayCount(Input->Controllers);
         ControllerIndex++)
    {
        game_controller_input* Controller = GetController(Input, ControllerIndex);
        if (Controller->IsConnected)
        {
            if(Controller->IsAnalog)
            {
                // NOTE(Sebas): Use analog movement tuning 
                GameState->ToneHz = 256 + (int)(128.0f * ((real32)Controller->RStick.AverageY));
                GameState->BlueOffset += (int)(4.0f * ((real32)Controller->LStick.AverageX));
                GameState->GreenOffset -= (int)(4.0f * ((real32)Controller->LStick.AverageY));
            }
            else
            {
                if(Controller->MoveDown.EndedDown)
                { 
                    GameState->GreenOffset += 1;
                }
                else if(Controller->MoveUp.EndedDown)
                {
                    GameState->GreenOffset -= 1;
                }
                if(Controller->MoveRight.EndedDown)
                { 
                    GameState->BlueOffset += 1;
                }
                else if(Controller->MoveLeft.EndedDown)
                {
                    GameState->BlueOffset -= 1;
                }
                // NOTE(Sebas): Use Digital movement tuning
            }

            // Input.AButtonEndedDown;
            // Input.AButtonHalfTransitionCount;


            // Input.StartX
            // Input.MinX;
            // Input.MaxX;
            // Input.EndX;
            // Input.StartY
            // Input.MinY;
            // Input.MaxY;
            // Input.EndY
        }
        else
        {
           // TODO(Sebas): What do we do if controller is not connected? 
        }
    }
    // TODO(Sebas): Allow sample offsets for more roubst platform options
    GameOutputSound(SoundBuffer, GameState->ToneHz);
    RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}
