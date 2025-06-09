#ifndef HANDMADE_H_
#define HANDMADE_H_
/* ================================================================
   $File: handmade.h
   $Date: 
   $Revision:
   $Creator: Sebastian Bautista
   $Notice: 
   ================================================================*/

// NOTE(Sebas): Services that the platform layer provides to the game.


// NOTE(Sebas): Services that the game provides to the platform layer.
// (This may expand in the future)
// Four things - timing, controller/keyboard input, bitmap to usel, sound to use

// TODO(Sebas): In the future, rendering _specifically_ will become a three-tiered abstraction!!!
struct game_frame_buffer
{
    void* Memory;
    int Width;
    int Height;
    int Pitch;
};

struct game_sound_output_buffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16* Samples;
};

internal void 
GameUpdateAndRender(game_frame_buffer* Buffer, int BlueOffset, int GreenOffset,
                    game_sound_output_buffer* SoundBuffer, int ToneHz);

#endif // HANDMADE_H_
