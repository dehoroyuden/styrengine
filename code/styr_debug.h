#ifndef DENGINE_DEBUG_H
/*====================================================================
 $File: $
 $Date: $
 $Revision: $
 $Creator: Denis Hoida $
 $Notice: (C) Copyright 2022 by Deden DEHO, Inc. All Rights Reserved. $
======================================================================*/

#define DEBUG_MAX_VARIABLE_STACK_DEPTH 64

struct debug_string
{
	u32 Length;
	char *Value;
};

struct used_bitmap_dim
{
	v2 Size;
	v2 Align;
	v3 P;
};

enum debug_text_op
{
	DEBUGTextOp_DrawText,
	DEBUGTextOp_SizeText,
};

#define DENGINE_DEBUG_H
#endif //DENGINE_DEBUG_H
