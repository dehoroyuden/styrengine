/*====================================================================
$File: $
$Date: $
$Revision: $
$Creator: Denis Hoida $
$Notice: (C) Copyright 2022 by Deden DEHO, Inc. All Rights Reserved. $
======================================================================*/

#include "dengine_intrinsics.h"
#include "dengine_math.h"

inline b32
IsEndOfLine(char C)
{
	b32 Result = ((C == '\n') ||
				  (C == '\r'));
	
	return Result;
}

inline b32
IsWhitespace(char C)
{
	b32 Result = ((C == ' ') ||
				  (C == '\t') ||
				  (C == '\v') ||
				  (C == '\f') ||
				  IsEndOfLine(C));
	
	return Result;
}

inline b32
StringsAreEqual(char *A, char *B)
{
	b32 Result = (A == B);
	
	if(A && B)
	{
		while(*A && *B && (*A == *B))
		{
			++A;
			++B;
		}
		Result = ((*A == 0) && (*B == 0));
	}
	
	return(Result);
}

inline b32
StringsAreEqual(umm ALength, char *A, char *B)
{
	char *At = B;
	for(umm Index = 0;
		Index < ALength;
		++Index, ++At)
	{
		if((*At == 0) ||
		   (A[Index] != *At))
		{
			return false;
		}
	}
	
	b32 Result = (*At == 0);
	
	return Result;
}

inline b32
StringsAreEqual(memory_index ALength, char *A, memory_index BLength, char *B)
{
	b32 Result = (ALength == BLength);
	
	if(Result)
	{
		Result = true;
		for(u32 Index = 0;
			Index < ALength;
			++Index)
		{
			if(A[Index] != B[Index])
			{
				Result = false;
				break;
			}
		}
	}
	return(Result);
}

internal void
EatAllWhitespaces(char *Tokenizer)
{
	for(;;)
	{
		if(IsWhitespace(Tokenizer[0]))
		{
			++Tokenizer;
		}
		else if((Tokenizer[0] == '/') &&
				(Tokenizer[1] == '/'))
		{
			Tokenizer += 2;
			while(Tokenizer[0] && !IsEndOfLine(Tokenizer[0]))
			{
				++Tokenizer;
			}
		}
		else if((Tokenizer[0] == '/') &&
				(Tokenizer[1] == '*'))
		{
			Tokenizer += 2;
			while(Tokenizer[0] && 
				  !((Tokenizer[0] == '*') && 
					(Tokenizer[1] == '/')))
			{
				++Tokenizer;
			}
			if(Tokenizer[0] == '*')
			{
				Tokenizer += 2;
			}
		}
		else
		{
			break;
		}
	}
}

#ifndef DENGINE_SHARED_H
#define DENGINE_SHARED_H

#endif //DENGINE_SHARED_H
