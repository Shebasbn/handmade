#ifndef HANDMADE_H_
#define HANDMADE_H_
/* ================================================================
   $File: handmade.h
   $Date: 
   $Revision:
   $Creator: Sebastian Bautista
   $Notice: 
   ================================================================*/
/*
 * NOTE(Sebas): 
 * HANDMADE_INTERNAL:
 *  0 - Build for public release
 *  1 - Build for developer only
 * HANDMADE_SLOW:
 *  0 - No slow code allowed!
 *  1 - Slow code welcome.
*/
#if !defined(AssertBreak)
    #define AssertBreak() (*(int*)0 = 0)
#endif

#define Stmnt(S) do{ S }while(0)
#if HANDMADE_SLOW 
    #define Assert(c) Stmnt(if(!(c)){ AssertBreak(); })
#else
    #define Assert(c)
#endif

#define Kilobytes(Value) ((Value)*1024)
#define Megabytes(Value) (Kilobytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)
#define Terabytes(Value) (Gigabytes(Value)*1024)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
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

struct game_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct game_stick_state
{
    real32 StartX;
    real32 StartY;
    real32 MinX;
    real32 MinY;
    real32 MaxX;
    real32 MaxY;
    real32 EndX;
    real32 EndY;
};

struct game_controller_input
{
    bool32 IsAnalog;
    bool32 IsConnected;

    union
    {
        game_stick_state LStick;
        struct
        {
            real32 LStartX;
            real32 LStartY;
            real32 LMinX;
            real32 LMinY;
            real32 LMaxX;
            real32 LMaxY;
            real32 LEndX;
            real32 LEndY;
        };
    };
    
    union
    {
        game_stick_state RStick;
        struct
        {
            real32 RStartX;
            real32 RStartY;
            real32 RMinX;
            real32 RMinY;
            real32 RMaxX;
            real32 RMaxY;
            real32 REndX;
            real32 REndY;
        };
    };    

    union
    {
        game_button_state Buttons[8];
        struct
        {
            game_button_state DPadUp;
            game_button_state DPadDown;
            game_button_state DPadLeft;
            game_button_state DPadRight;
            game_button_state Up;
            game_button_state Down;
            game_button_state Left;
            game_button_state Right;
            game_button_state LeftShoulder;
            game_button_state RightShoulder;
            game_button_state Start;
            game_button_state Back;
        };
    };
};

struct game_input
{
    // TODO(Sebas): Insert clock values here.
    // real32 GameClock;
    game_controller_input Controllers[4];
};

struct game_memory
{
    bool32 IsInitialized;
    uint64 PermanentStorageSize;
    void* PermanentStorage; // NOTE(Sebas): REQUIRED to be cleared to zero at startup
    uint64 TransientStorageSize;
    void* TransientStorage; // NOTE(Sebas): REQUIRED to be cleared to zero at startup
};

internal void 
GameUpdateAndRender(game_memory* Memory,
                    game_frame_buffer* Buffer, 
                    game_sound_output_buffer* SoundBuffer,
                    game_input* Input);

/////////////////////////////////////
struct game_state
{
    int ToneHz;
    int GreenOffset;
    int BlueOffset;
};


#endif // HANDMADE_H_
