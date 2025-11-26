#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H
/* ================================================================
   $File: handmade_math.h
   $Date:
   $Revision:
   $Creator: Sebastian Bautista
   $Notice:
   ================================================================*/

struct v2
{
	union 
	{
		struct
		{
			real32 X;
			real32 Y;
		};
		real32 E[2];
	};
	real32& operator[](int Index) { Assert((Index >= 0) && (Index < 2)); return E[Index]; }
};

inline v2 V2(real32 X, real32 Y)
{
	v2 Result;

	Result.X = X;
	Result.Y = Y;
	
	return Result;
}

inline v2 
operator*(real32 A, v2 B)
{
	v2 Result;
	
	Result.X = A * B.X;
	Result.Y = A * B.Y;
	
	return Result;
}

inline v2 
operator*(v2 A, real32 B)
{
	v2 Result = B * A;
	
	return Result;
}

inline v2& 
operator*=(v2& A, real32 B)
{
	A = A * B;
	
	return A;
}

inline v2 
operator-(v2 A)
{
	v2 Result;
	
	Result.X = -A.X;
	Result.Y = -A.Y;
	
	return Result;
}

inline v2 
operator+(v2 A, v2 B)
{
	v2 Result;
	
	Result.X = A.X + B.X;
	Result.Y = A.Y + B.Y;
	
	return Result;
}

inline v2& 
operator+=(v2& A, v2 B)
{
	A = A + B;
	
	return A;
}

inline v2 
operator-(v2 A, v2 B)
{
	v2 Result;
	
	Result.X = A.X - B.X;
	Result.Y = A.Y - B.Y;
	
	return Result;
}

inline v2& 
operator-=(v2& A, v2 B)
{
	A = A - B;
	
	return A;
}

inline real32
Square(real32 A)
{
    real32 Result = A * A;
    return Result;
}

#endif
