#ifndef HANDMADE_INTRINSICS_H
#define HANDMADE_INTRINSICS_H
/* ================================================================
   $File: handmade_intrinsics.h
   $Date:
   $Revision:
   $Creator: Sebastian Bautista
   $Notice:
   ================================================================*/

//#include "handmade_platform.h"
#include <math.h>

inline uint32
RoundReal32ToUint32(real32 Real32)
{
    uint32 Result = (uint32)roundf(Real32);
    return Result;
}

inline int32
RoundReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)roundf(Real32);
    return Result;
}

inline uint32
FloorReal32ToUint32(real32 real32)
{
    uint32 result = (uint32)floorf(real32);
    return result;
}

inline int32
FloorReal32ToInt32(real32 real32)
{
    int32 result = (int32)floorf(real32);
    return result;
}

inline int32
TruncateReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)Real32;
    return Result;
}

inline real32
Sin(real32 Angle)
{
    real32 Result = sinf(Angle);
    return Result;
}

inline real32
Cos(real32 Angle)
{
    real32 Result = cosf(Angle);
    return Result;
}

inline real32
Tan(real32 Angle)
{
    real32 Result = tanf(Angle);
    return Result;
}

inline real32
Atan2(real32 Y, real32 X)
{
    real32 Result = atan2f(Y, X);
    return Result;
}

// TODO(Sebas): Move this into the intrinsics and calll MSVC version
struct bit_scan_result
{
    bool32 Found;
    uint32 Index;
};

inline bit_scan_result
FindLeastSignificantSetBit(uint32 Value)
{
    bit_scan_result Result = {};

#if COMPILER_MSVC
    Result.Found = _BitScanForward((unsigned long*)&Result.Index, Value);
#else
    for (uint32 Test = 0;
         Test < 32;
         ++Test)
    {
        if (Value & (1 << Test))
        {
            Result.Index = Test;
            Result.Found = true;
            break;
        }
    }
#endif
    return Result;
}

#endif // !HANDMADE_INTRINSICS_H


