#ifndef HANDMADE_PLATFORM_H
#define HANDMADE_PLATFORM_H
/* ================================================================
   $File: handmade_platform.h
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

#ifdef __cplusplus
extern "C" {
#endif

//
// NOTE: Compilers
//

#ifndef COMPILER_MSVC
#define COMPILER_MSVC 0
#endif

#ifndef COMPILER_LLVM
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else // TODO(Sebas): More Compilers  
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#endif

//
// NOTE: Types
//
#include <stdint.h>
#include <stddef.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef size_t memory_index;

typedef float real32;
typedef double real64;

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

#if !defined(AssertBreak)
#define AssertBreak() (*(int*)0 = 0)
#endif

#define Stmnt(S) do{ S }while(0)
#if HANDMADE_SLOW 
#define Assert(c) Stmnt(if(!(c)){ AssertBreak(); })
#else
#define Assert(c)
#endif

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32
SafeTruncateUInt64(uint64 Value)
{
    // TODO(Sebas): Defines for maximum values
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return Result;
}

// NOTE(Sebas): Services that the platform layer provides to the game.
#if HANDMADE_INTERNAL
struct debug_read_file_result
{
    uint32 ContentsSize;
    void* Contents;
};

struct thread_context
{
    int Placeholder;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context* Thread, void* Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context* Thread, char* FileName)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context* Thread, char* FileName, void* Memory, uint32 MemorySize)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif

// NOTE(Sebas): Services that the game provides to the platform layer.
// (This may expand in the future)
// Four things - timing, controller/keyboard input, bitmap to usel, sound to use
// TODO(Sebas): In the future, rendering _specifically_ will become a three-tiered abstraction!!!
struct game_frame_buffer
{
    // NOTE(Sebas): Pixels are always 32 bit wide, Memory order BB GG RR XX
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
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
    real32 AverageX;
    real32 AverageY;
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
            real32 LAverageX;
            real32 LAverageY;
        };
    };

    union
    {
        game_stick_state RStick;
        struct
        {
            real32 RAverageX;
            real32 RAverageY;
        };
    };

    union
    {
        game_button_state Buttons[16];
        struct
        {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state CameraMoveUp;
            game_button_state CameraMoveDown;
            game_button_state CameraMoveLeft;
            game_button_state CameraMoveRight;

            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;

            game_button_state LeftShoulder;
            game_button_state RightShoulder;
            game_button_state Start;
            game_button_state Back;


            // NOTE(Sebas): All buttons most be added above this line
            game_button_state Terminator;
        };
    };
};

struct game_input
{
    game_button_state MouseButtons[5];
    int32 MouseX, MouseY, MouseZ;

    real32 dtPerFrame;

    game_controller_input Controllers[5];
};

struct game_memory
{
    bool32 IsInitialized;
    uint64 PermanentStorageSize;
    void* PermanentStorage; // NOTE(Sebas): REQUIRED to be cleared to zero at startup
    uint64 TransientStorageSize;
    void* TransientStorage; // NOTE(Sebas): REQUIRED to be cleared to zero at startup

    debug_platform_free_file_memory* DEBUGPlatformFreeFileMemory;
    debug_platform_read_entire_file* DEBUGPlatformReadEntireFile;
    debug_platform_write_entire_file* DEBUGPlatformWriteEntireFile;
};

// NOTE(Sebas): GameUpdateAndRender
#define GAME_UPDATE_AND_RENDER(name) void name(thread_context* Thread, game_memory* Memory, game_frame_buffer* Buffer, game_input* Input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);


// NOTE(Sebas): GameGetSoundSamples
#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context* Thread, game_memory* Memory, game_sound_output_buffer* SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

inline game_controller_input* GetController(game_input* Input, unsigned int ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    return &Input->Controllers[ControllerIndex];
}

#ifdef __cplusplus
}
#endif
#endif // !HANDMADE_CORE_H
