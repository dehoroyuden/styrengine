/*====================================================================
 $File: $
 $Date: $
 $Revision: $
 $Creator: Denis Hoida $
 $Notice: (C) Copyright 2022 by Deden DEHO, Inc. All Rights Reserved. $
======================================================================*/

#ifndef HANDMADE_DENGINE_MATH_H

inline v2
V2i(int32 x, int32 y)
{
	v2 Result = {(real32)x, (real32)y};
	
	return(Result);
}

inline v2
V2i(uint32 x, uint32 y)
{
	v2 Result = {(real32)x, (real32)y};
	
	return(Result);
}

inline v2 
V2(real32 x, real32 y)
{
	v2 Result;
	
	Result.x = x;
	Result.y = y;
	
	return(Result);
}

inline v3
V3(real32 x, real32 y, real32 z)
{
	v3 Result;
	
	Result.x = x;
	Result.y = y;
	Result.z = z;
	
	return(Result);
}

inline v3
V3(v2 xy, real32 z)
{
	v3 Result;
	
	Result.x = xy.x;
	Result.y = xy.y;
	Result.z = z;
	
	return(Result);
}

inline v4 
V4(real32 x, real32 y, real32 z, real32 w)
{
	v4 Result;
	
	Result.x = x;
	Result.y = y;
	Result.z = z;
	Result.w = w;
	
	return(Result);
}

inline v3
V4(v2 XY, real32 Z)
{
	v3 Result;
	
	Result.xy = XY;
	Result.z = Z;
	
	return(Result);
}

inline v4
V4(v3 XYZ, real32 W)
{
	v4 Result;
	
	Result.xyz = XYZ;
	Result.w = W;
	
	return(Result);
}

//
// NOTE(Denis): Scalar operations
//

inline real32
Square(real32 A)
{
	real32 Result = A*A;
	
	return(Result);
}

inline real32
Lerp(real32 A, real32 t, real32 B)
{
	real32 Result = (1.0f-t)*A + B*t;
	
	return(Result);
}

inline real32
Clamp(real32 Min, real32 Value, real32 Max)
{
	real32 Result = Value;
	
	if(Result < Min)
	{
		Result = Min;
	}
	else if(Result > Max)
	{
		Result = Max;
	}
	
	return(Result);
}

inline real32
Clamp01(real32 Value)
{
	real32 Result = Clamp(0.0f,Value,1.0f);
	
	return(Result);
}

inline real32
Clamp01MapToRange(real32 Min, real32 t, real32 Max)
{
	real32 Result = 0.0f;
	real32 Range = Max - Min;
	if(Range != 0)
	{
		Result = Clamp01((t - Min) / Range);
	}
	
	return(Result);
}

inline real32
SafeRatioN(real32 Numerator, real32 Divisor, real32 N)
{
	real32 Result = N;
	
	if(Divisor != 0.0f)
	{
		Result = Numerator / Divisor;
	}
	
	return(Result);
}


inline real32
SafeRatio0(real32 Numerator, real32 Divisor)
{
	real32 Result = SafeRatioN(Numerator, Divisor, 0.0f);
	
	return(Result);
}


inline real32
SafeRatio1(real32 Numerator, real32 Divisor)
{
	real32 Result = SafeRatioN(Numerator, Divisor, 1.0f);
	
	return(Result);
}


//
// NOTE(Denis): v2 operations
//

inline v2
Perp(v2 A)
{
	v2 Result = {-A.y,A.x};
	return(Result);
}

inline v2 
operator*(real32 A, v2 B)
{
	v2 Result;
	
	Result.x = A*B.x;
	Result.y = A*B.y;
	
	return(Result);
}

inline v2 
operator*(v2 B, real32 A)
{
	v2 Result = A * B;
	
	return(Result);
}

inline v2 &
operator*=(v2 &B, real32 A)
{
	B = A * B;
	
	return(B);
}

inline v2 
operator-(v2 A)
{
	v2 Result;
	
	Result.x = -A.x;
	Result.y = -A.y;
	
	return(Result);
}

inline v2 
operator+(v2 A, v2 B)
{
	v2 Result;
	
	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	
	return(Result);
}

inline v2 &
operator+=(v2 &A, v2 B)
{
	A = A + B;
	
	return(A);
}

inline v2 
operator-(v2 A, v2 B)
{
	v2 Result;
	
	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	
	return(Result);
}

inline v2 &
operator-=(v2 &A, v2 B)
{
	A = A - B;
	
	return(A);
}

inline b32
operator==(v2 A, v2 B)
{
	b32 Result = ((A.x == B.x) && (A.y == B.y));
	return Result;
}

inline v2
Hadamard(v2 A, v2 B)
{
	v2 Result = { A.x*B.x,A.y*B.y};
	
	return(Result);
}

inline real32
Inner(v2 A, v2 B)
{
	real32 Result = A.x*B.x + A.y*B.y;
	return(Result);
}

inline real32
LengthSq(v2 A)
{
	real32 Result = Inner(A,A);
	
	return(Result);
}

inline real32
Length(v2 A)
{
	real32 Result = SquareRoot(LengthSq(A));
	
	return(Result);
}

inline v2
Clamp01(v2  Value)
{
	v2 Result;
	
	Result.x = Clamp01(Value.x);
	Result.y = Clamp01(Value.y);
	
	return(Result);
}

inline v2
Arm2(r32 Angle)
{
	v2 Result = {Cos(Angle), Sin(Angle)};
	return Result;
}

//
// NOTE(Denis): v3 operations
//

inline v3 
operator*(real32 A, v3 B)
{
	v3 Result;
	
	Result.x = A*B.x;
	Result.y = A*B.y;
	Result.z = A*B.z;
	
	return(Result);
}

inline v3 
operator*(v3 B, real32 A)
{
	v3 Result = A * B;
	
	return(Result);
}

inline v3 &
operator*=(v3 &B, real32 A)
{
	B = A * B;
	
	return(B);
}

inline v3 
operator-(v3 A)
{
	v3 Result;
	
	Result.x = -A.x;
	Result.y = -A.y;
	Result.z = -A.z;
	
	return(Result);
}

inline v3 
operator+(v3 A, v3 B)
{
	v3 Result;
	
	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	Result.z = A.z + B.z;
	
	return(Result);
}

inline v3 &
operator+=(v3 &A, v3 B)
{
	A = A + B;
	
	return(A);
}

inline v3 
operator-(v3 A, v3 B)
{
	v3 Result;
	
	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	Result.z = A.z - B.z;
	
	return(Result);
}

inline v3 &
operator-=(v3 &A, v3 B)
{
	A = A - B;
	
	return(A);
}

inline b32
operator==(v3 A, v3 B)
{
	b32 Result = ((A.x == B.x) && (A.y == B.y) && (A.z == B.z));
	return Result;
}

inline v3
Hadamard(v3 A, v3 B)
{
	v3 Result = { A.x*B.x, A.y*B.y, A.z*B.z};
	
	return(Result);
}

inline real32
Inner(v3 A, v3 B)
{
	real32 Result = A.x*B.x + A.y*B.y + A.z*B.z;
	return(Result);
}

inline real32
LengthSq(v3 A)
{
	real32 Result = Inner(A,A);
	
	return(Result);
}

inline real32
Length(v3 A)
{
	real32 Result = SquareRoot(LengthSq(A));
	
	return(Result);
}

inline v3
Normalize(v3 A)
{
	v3 Result = A * (1.0f / Length(A));
	
	return(Result);
}

inline v3
Clamp01(v3  Value)
{
	v3 Result;
	
	Result.x = Clamp01(Value.x);
	Result.y = Clamp01(Value.y);
	Result.z = Clamp01(Value.z);
	
	return(Result);
}

inline v3
Lerp(v3 A, real32 t, v3 B)
{
	v3 Result = (1.0f - t)*A + t*B;
	
	return(Result);
}


//
// NOTE(Denis): v4 operations
//


inline v4
operator*(real32 A, v4 B)
{
	v4 Result;
	
	Result.x = A*B.x;
	Result.y = A*B.y;
	Result.z = A*B.z;
	Result.w = A*B.w;
	
	return(Result);
}

inline v4 
operator*(v4 B, real32 A)
{
	v4 Result = A * B;
	
	return(Result);
}

inline v4 &
operator*=(v4 &B, real32 A)
{
	B = A * B;
	
	return(B);
}

inline v4 
operator-(v4 A)
{
	v4 Result;
	
	Result.x = -A.x;
	Result.y = -A.y;
	Result.z = -A.z;
	Result.w = -A.w;
	
	return(Result);
}

inline v4
operator+(v4 A, v4 B)
{
	v4 Result;
	
	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	Result.z = A.z + B.z;
	Result.w = A.w + B.w;
	
	return(Result);
}

inline v4 &
operator+=(v4 &A, v4 B)
{
	A = B + A;
	
	return(A);
}

inline v4 
operator-(v4 A, v4 B)
{
	v4 Result;
	
	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	Result.z = A.z - B.z;
	Result.w = A.w - B.w;
	
	return(Result);
}

inline v4 &
operator-=(v4 &A, v4 B)
{
	A = A - B;
	
	return(A);
}

inline b32
operator==(v4 A, v4 B)
{
	b32 Result = ((A.x == B.x) && (A.y == B.y) && (A.z == B.z) && (A.w == B.w));
	return Result;
}

inline v4
Hadamard(v4 A, v4 B)
{
	v4 Result = { A.x*B.x, A.y*B.y, A.z*B.z, A.w*B.w};
	
	return(Result);
}

inline real32
Inner(v4 A, v4 B)
{
	real32 Result = A.x*B.x + A.y*B.y + A.z*B.z + A.w*B.w;
	return(Result);
}

inline real32
LengthSq(v4 A)
{
	real32 Result = Inner(A,A);
	
	return(Result);
}

inline real32
Length(v4 A)
{
	real32 Result = SquareRoot(LengthSq(A));
	
	return(Result);
}

inline v4
Clamp01(v4  Value)
{
	v4 Result;
	
	Result.x = Clamp01(Value.x);
	Result.y = Clamp01(Value.y);
	Result.z = Clamp01(Value.z);
	Result.w = Clamp01(Value.w);
	
	return(Result);
}

inline v4
Lerp(v4 A, real32 t, v4 B)
{
	v4 Result = (1.0f - t)*A + t*B;
	
	return(Result);
}

//
// NOTE(Denis): Rectangle2 
//

inline rectangle2
InvertedInfinityRectangle2()
{
	rectangle2 Result;
	
	Result.Min.x = Result.Min.y = Real32Maximum;
	Result.Max.x = Result.Max.y = -Real32Maximum;
	
	return(Result);
}

inline rectangle2
Union(rectangle2 A, rectangle2 B)
{
	rectangle2 Result;
	
	Result.Min.x = (A.Min.x < B.Min.x) ? A.Min.x : B.Min.x;
	Result.Min.y = (A.Min.y < B.Min.y) ? A.Min.y : B.Min.y;
	Result.Max.x = (A.Max.x > B.Max.x) ? A.Max.x : B.Max.x;
	Result.Max.y = (A.Max.y > B.Max.y) ? A.Max.y : B.Max.y;
	
	return(Result);
}

inline v2
GetMinCorner(rectangle2 Rect)
{
	v2 Result = Rect.Min;
	return(Result);
}

inline v2
GetMaxCorner(rectangle2 Rect)
{
	v2 Result = Rect.Max;
	return(Result);
}

inline v2
GetDim(rectangle2 Rect)
{
	v2 Result = Rect.Max - Rect.Min;
	return(Result);
}

inline v3
GetDim(rectangle3 Rect)
{
	v3 Result = Rect.Max - Rect.Min;
	return(Result);
}


inline v2
GetCenter(rectangle2 Rect)
{
	v2 Result = 0.5f*(Rect.Min + Rect.Max);
	return(Result);
}


inline rectangle2
RectMinMax(v2 Min, v2 Max)
{
	rectangle2 Result;
	
	Result.Min = Min;
	Result.Max = Max;
	
	return(Result);
}

inline rectangle2
AddRadiusTo(rectangle2 A, v2 Radius)
{
	rectangle2 Result;
	
	Result.Min = A.Min - Radius;
	Result.Max = A.Max + Radius;
	
	return(Result);
}

inline rectangle2
Offset(rectangle2 A, v2 Offset)
{
	rectangle2 Result;
	
	Result.Min = A.Min + Offset;
	Result.Max = A.Max + Offset;
	
	return(Result);
}

inline rectangle2
RectMinDim(v2 Min, v2 Dim)
{
	rectangle2 Result;
	
	Result.Min = Min;
	Result.Max = Min + Dim;
	
	return(Result);
}

inline rectangle2
RectCenterHalfDim(v2 Center, v2 HalfDim)
{
	rectangle2 Result;
	
	Result.Min = Center - HalfDim;
	Result.Max = Center + HalfDim;
	
	return(Result);
}

inline rectangle2
RectCenterDim(v2 Center, v2 Dim)
{
	rectangle2 Result = RectCenterHalfDim(Center, 0.5f*Dim);
	
	return(Result);
}

inline bool32
IsInRectangle(rectangle2 Rectangle, v2 Test)
{
	bool32 Result = ((Test.x >= Rectangle.Min.x) &&
					 (Test.y >= Rectangle.Min.y) &&
					 (Test.x < Rectangle.Max.x) &&
					 (Test.y < Rectangle.Max.y));
	
	return(Result);
}

inline v2
GetBarycentric(rectangle2 A, v2 P)
{
	v2 Result;
	
	Result.x = SafeRatio0(P.x - A.Min.x, A.Max.x - A.Min.x);
	Result.y = SafeRatio0(P.y - A.Min.y, A.Max.y - A.Min.y);
	
	return(Result);
}


//
// NOTE(Denis): Rectangle3
//

inline v3
GetMinCorner(rectangle3 Rect)
{
	v3 Result = Rect.Min;
	return(Result);
}

inline v3
GetMaxCorner(rectangle3 Rect)
{
	v3 Result = Rect.Max;
	return(Result);
}

inline v3
GetCenter(rectangle3 Rect)
{
	v3 Result = 0.5f*(Rect.Min + Rect.Max);
	return(Result);
}


inline rectangle3
RectMinMax(v3 Min, v3 Max)
{
	rectangle3 Result;
	
	Result.Min = Min;
	Result.Max = Max;
	
	return(Result);
}

inline rectangle3
AddRadiusTo(rectangle3 A, v3 Radius)
{
	rectangle3 Result;
	
	Result.Min = A.Min - Radius;
	Result.Max = A.Max + Radius;
	
	return(Result);
}

inline rectangle3
Offset(rectangle3 A, v3 Offset)
{
	rectangle3 Result;
	
	Result.Min = A.Min + Offset;
	Result.Max = A.Max + Offset;
	
	return(Result);
}

inline rectangle3
RectMinDim(v3 Min, v3 Dim)
{
	rectangle3 Result;
	
	Result.Min = Min;
	Result.Max = Min + Dim;
	
	return(Result);
}

inline rectangle3
RectCenterHalfDim(v3 Center, v3 HalfDim)
{
	rectangle3 Result;
	
	Result.Min = Center - HalfDim;
	Result.Max = Center + HalfDim;
	
	return(Result);
}

inline rectangle3
RectCenterDim(v3 Center, v3 Dim)
{
	rectangle3 Result = RectCenterHalfDim(Center, 0.5f*Dim);
	
	return(Result);
}

inline bool32
IsInRectangle(rectangle3 Rectangle, v3 Test)
{
	bool32 Result = ((Test.x >= Rectangle.Min.x) &&
					 (Test.y >= Rectangle.Min.y) &&
					 (Test.z >= Rectangle.Min.z) &&
					 (Test.x < Rectangle.Max.x) &&
					 (Test.y < Rectangle.Max.y) &&
					 (Test.z < Rectangle.Max.z));
	
	return(Result);
}

inline bool32
RectanglesIntersect(rectangle3 A, rectangle3 B)
{
	bool32 Result = !((B.Max.x <= A.Min.x) ||
					  (B.Min.x >= A.Max.x) ||
					  (B.Max.y <= A.Min.y) ||
					  (B.Min.y >= A.Max.y) ||
					  (B.Max.z <= A.Min.z) ||
					  (B.Min.z >= A.Max.z));
	return(Result);
}

inline v3
GetBarycentric(rectangle3 A, v3 P)
{
	v3 Result;
	
	Result.x = SafeRatio0(P.x - A.Min.x, A.Max.x - A.Min.x);
	Result.y = SafeRatio0(P.y - A.Min.y, A.Max.y - A.Min.y);
	Result.z = SafeRatio0(P.z - A.Min.z, A.Max.z - A.Min.z);
	
	return(Result);
}

inline rectangle2
ToRectanglexy(rectangle3 A)
{
	rectangle2 Result;
	
	Result.Min = A.Min.xy;
	Result.Max = A.Max.xy;
	
	return(Result);
}

//
//
//

struct rectangle2i
{
	int32 MinX, MinY;
	int32 MaxX, MaxY;
};

inline rectangle2i 
Intersect(rectangle2i A, rectangle2i B)
{
	rectangle2i Result;
	
	Result.MinX = (A.MinX < B.MinX) ? B.MinX : A.MinX;
	Result.MinY = (A.MinY < B.MinY) ? B.MinY : A.MinY;
	Result.MaxX = (A.MaxX > B.MaxX) ? B.MaxX : A.MaxX;
	Result.MaxY = (A.MaxY > B.MaxY) ? B.MaxY : A.MaxY;
	
	return(Result);
}

inline rectangle2i 
Union(rectangle2i A, rectangle2i B)
{
	rectangle2i Result;
	
	Result.MinX = (A.MinX < B.MinX) ? A.MinX : B.MinX;
	Result.MinY = (A.MinY < B.MinY) ? A.MinY : B.MinY;
	Result.MaxX = (A.MaxX > B.MaxX) ? A.MaxX : B.MaxX;
	Result.MaxY = (A.MaxY > B.MaxY) ? A.MaxY : B.MaxY;
	
	return(Result);
}

inline int32
GetClampedRectArea(rectangle2i A)
{
	int32 Width = (A.MaxX - A.MinX);
	int32 Height =  (A.MaxY - A.MinY);
	int32 Result = 0;
	
	if((Width > 0 && (Height > 0)))
	{
		Result = Width * Height;
	}
	
	return(Result);
	
}

inline bool32
HasArea(rectangle2i A)
{
	bool32 Result = ((A.MinX < A.MaxX) && (A.MinY < A.MaxY));
	
	return(Result);
}

inline rectangle2i 
InvertedInfinityRectangle2i()
{
	rectangle2i Result;
	
	Result.MinX = Result.MinY = INT_MAX;
	Result.MaxX = Result.MaxY = -INT_MAX;
	
	return(Result);
}

inline v4
SRGB255ToLinear1(v4 C)
{
	v4 Result;
	
	real32 Inv255 = 1.0f / 255.0f;
	
	Result.r = Square(Inv255*C.r);
	Result.g = Square(Inv255*C.g);
	Result.b = Square(Inv255*C.b);
	Result.a = Inv255*C.a;
	
	return(Result);
}

inline v4
Linear1ToSRGB255(v4 C)
{
	v4 Result;
	
	real32 One255 = 255.0f;
	
	Result.r = One255*SquareRoot(C.r);
	Result.g = One255*SquareRoot(C.g);
	Result.b = One255*SquareRoot(C.b);
	Result.a = One255*C.a;
	
	return(Result);
}

//
//
//

// mat4

// Matrix 4x4
inline mat4
Identity()
{
	mat4 Result = {};
	
	Result.Value1_1 = 1.0f;
	Result.Value2_2 = 1.0f;
	Result.Value3_3 = 1.0f;
	Result.Value4_4 = 1.0f;
	
	return Result;
}

inline v3
operator*(mat4 A, v3 B)
{
	v3 Result;
	
	Result.x = B.x * A.Value1_1 + B.x * A.Value1_2 + B.x * A.Value1_3 + B.x * A.Value1_4;
	Result.y = B.y * A.Value2_1 + B.y * A.Value2_2 + B.y * A.Value2_3 + B.y * A.Value2_4;
	Result.z = B.z * A.Value3_1 + B.z * A.Value3_2 + B.z * A.Value3_3 + B.z * A.Value3_4;
	
	return Result;
}

inline v3
operator*(v3 A, mat4 B)
{
	v3 Result = B * A;
	
	return Result;
}

inline v4
operator*(mat4 A, v4 B)
{
	v4 Result;
	
	Result.x = B.x * A.Value1_1 + B.x * A.Value1_2 + B.x * A.Value1_3 + B.x * A.Value1_4;
	Result.y = B.y * A.Value2_1 + B.y * A.Value2_2 + B.y * A.Value2_3 + B.y * A.Value2_4;
	Result.z = B.z * A.Value3_1 + B.z * A.Value3_2 + B.z * A.Value3_3 + B.z * A.Value3_4;
	Result.w = B.w * A.Value4_1 + B.w * A.Value4_2 + B.w * A.Value4_3 + B.w * A.Value4_4;
	
	return Result;
}

inline v4
operator*(v4 A, mat4 B)
{
	v4 Result = B * A;
	
	return Result;
}


//
//
//

// Misc Common Function

inline u32 
Maximum(u32 A, u32 B)
{
	u32 Result = ((A < B) ? (A):(B));
	return Result;
}

inline u32 
Minimum(u32 A, u32 B)
{
	u32 Result = ((A < B) ? (A):(B));
	return Result;
}

inline s32
Maximum(s32 A, s32 B)
{
	s32 Result = ((A < B) ? (A):(B));
	return Result;
}

inline s32 
Minimum(s32 A, s32 B)
{
	s32 Result = ((A < B) ? (A):(B));
	return Result;
}

inline r32
Maximum(r32 A, r32 B)
{
	r32 Result = ((A < B) ? (A):(B));
	return Result;
}

inline r32 
Minimum(r32 A, r32 B)
{
	r32 Result = ((A < B) ? (A):(B));
	return Result;
}

inline u32
Log2(u32 Value)
{
	u32 Result = 0;
	
	u64 B[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
	u32 S[] = {1, 2, 4, 8, 16};
	
	for(s32 i = 4; i >= 0; --i)
	{
		if(Value & B[i])
		{
			Value >>= S[i];
			Result |= S[i];
		}
	}
	
	return Result;
}

//
//
//

// Misc Trigonometry Functions

inline r32 Factorial(r32 N)
{
	if(N == 0)
	{
		return 1;
	}
	
	return N * (Factorial(N-1));
}

inline r32
Power(r32 N, r32 Power)
{
	r32 Result = N;
	
	for(u32 Index = 1;
		Index < Power;
		++Index)
	{
		Result = N * Result;
	}
	
	return Result;
}

inline r32 Fmod(r32 A, r32 B)
{
	r32 Frac = A / B;
	s32 Floor = (Frac > 0) ? (s32)Frac : ((s32)(Frac - 0.9999999999999f));
	
	return (A - B * Floor);
}

inline r32 Sinus(r32 N)
{
	r32 Result = N;
	r32 Coefficient = 3.0f;
	
#if 1
	N = Fmod(N, Tau32);
	
	if(N < 0)
	{
		N = Tau32 - N;
	}
	
	char Sign = 1;
	if(N > Pi32)
	{
		N -= Pi32;
		Sign = -1;
	}
#endif
	
	for(u32 t = 0; 
		t < 6;
		++t)
	{
		r32 Pow = Power(N, Coefficient);
		r32 Frac = Factorial(Coefficient);
		
		Result = ((t % 2) == 0) ? (Result - (Pow/Frac)) : (Result + (Pow/Frac));
		
		Coefficient = Coefficient + 2;
	}
	
	return Result;//Sign * Result;
}

inline r32 RadiansFromDegrees(r32 Angle)
{
	r32 Result = Angle * DegreeInRadians;
	
	return Result;
}

inline r32 DegreesFromRadians(r32 Radians)
{
	r32 Result = Radians * RadianInDegrees;
	
	return Result;
}

inline mat4
Rotate(mat4 View, r32 Angle, v3 Axis)
{
	mat4 Result = 
	{
		
	};
	
	return Result;
}

inline mat4
ConvertToMatrix4x4(glm::mat4 Matrix)
{
	mat4 Result = {};
	
	Result.Value1_1 = Matrix[0].x;
	Result.Value1_2 = Matrix[0].y;
	Result.Value1_3 = Matrix[0].z;
	Result.Value1_4 = Matrix[0].w;
	Result.Value2_1 = Matrix[1].x;
	Result.Value2_2 = Matrix[1].y;
	Result.Value2_3 = Matrix[1].z;
	Result.Value2_4 = Matrix[1].w;
	Result.Value3_1 = Matrix[2].x;
	Result.Value3_2 = Matrix[2].y;
	Result.Value3_3 = Matrix[2].z;
	Result.Value3_4 = Matrix[2].w;
	Result.Value4_1 = Matrix[3].x;
	Result.Value4_2 = Matrix[3].y;
	Result.Value4_3 = Matrix[3].z;
	Result.Value4_4 = Matrix[3].w;
	
	return Result;
}

inline glm::mat4
ConvertFromMatrix4x4(mat4 Matrix)
{
	glm::mat4 Result = {};
	
	Result[0].x = Matrix.Value1_1;
	Result[0].y = Matrix.Value1_2;
	Result[0].z = Matrix.Value1_3;
	Result[0].w = Matrix.Value1_4;
	Result[1].x = Matrix.Value2_1;
	Result[1].y = Matrix.Value2_2;
	Result[1].z = Matrix.Value2_3;
	Result[1].w = Matrix.Value2_4;
	Result[2].x = Matrix.Value3_1;
	Result[2].y = Matrix.Value3_2;
	Result[2].z = Matrix.Value3_3;
	Result[2].w = Matrix.Value3_4;
	Result[3].x = Matrix.Value4_1;
	Result[3].y = Matrix.Value4_2;
	Result[3].z = Matrix.Value4_3;
	Result[3].w = Matrix.Value4_4;
	
	return Result;
}

// NOTE(Denis): This is converts objects into _image space_
inline mat4
PerspectiveProjection(r32 FovyInRadians, r32 Aspect, r32 NearPlane, r32 FarPlane)
{
	mat4 Result = {};
	
	r32 FovScalingFactor = 1.0f / Tan(FovyInRadians / 2.0f);
	
	Result.Value1_1 = 1.0f / (Aspect * Tan(FovyInRadians / 2.0f));
	Result.Value2_2 = FovScalingFactor;
	Result.Value3_3 = -(FarPlane + NearPlane) / (FarPlane - NearPlane);
	Result.Value3_4 = -1.0f;
	Result.Value4_3 = -(2 * FarPlane * NearPlane) / (FarPlane - NearPlane);
	
	return Result;
}

#define HANDMADE_DENGINE_MATH_H
#endif //HANDMADE_DENGINE_MATH_H
