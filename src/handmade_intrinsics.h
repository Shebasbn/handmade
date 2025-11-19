#ifndef HANDMADE_INTRINSICS_H
#define HANDMADE_INTRINSICS_H
/* ================================================================
   $File: handmade_intrinsics.h
   $Date:
   $Revision:
   $Creator: Sebastian Bautista
   $Notice:
   ================================================================*/

#include <math.h>

inline uint32
RoundReal32ToUint32(real32 Real32)
{
    uint32 Result = (uint32)(Real32 + 0.5f);
    return Result;
}

inline int32
RoundReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)(Real32 + 0.5f);
    return Result;
}

inline int32
FloorReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)floorf(Real32);
    return Result;
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
#endif // !HANDMADE_INTRINSICS_H


