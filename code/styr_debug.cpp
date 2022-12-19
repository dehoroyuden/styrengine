/*====================================================================
$File: $
$Date: $
$Revision: $
$Creator: Denis Hoida $
$Notice: (C) Copyright 2022 by Deden DEHO, Inc. All Rights Reserved. $
======================================================================*/

// TODO(Denis): Stop using stdio!
#include <stdio.h>
#include <stdlib.h>

#include "styr_debug.h"

inline b32
IsHex(char Char)
{
	b32 Result = (((Char >= '0') && (Char <= '9')) ||
				  ((Char >= 'A') && (Char <= 'F')));
	
	return(Result);
}

inline u32
GetHex(char Char)
{
	u32 Result = 0;
	
	if((Char >= '0') && (Char <= '9'))
	{
		Result = Char - '0';
	}
	
	if((Char >= 'A') && (Char <= 'F'))
	{
		Result = 0xA + (Char - 'A');
	}
	
	return(Result);
}

inline v2
GetAlignedRectPosition(loaded_bitmap *Bitmap, v3 RectPosition)
{
	v2 Result = {};
	Result = (RectPosition - V3(Bitmap->AlignPercentage, 0)).xy;
	
    return(Result);
}

internal rectangle2
DEBUGTextOp(game_assets *Assets, basic_ui_text_state *ui_text_state, u32 screen_width, u32 screen_height, debug_text_op Op, v2 P, char *String, v4 Color = V4(1, 1, 1, 1))
{
	rectangle2 Result = InvertedInfinityRectangle2();
	if(ui_text_state->font_info.DebugFont)
	{
		loaded_font *Font = ui_text_state->font_info.DebugFont;
		sta_font *Info = ui_text_state->font_info.DebugFontInfo;
		
		u32 PrevCodepoint = 0;
		f32 CharScale = 1.0f;
		f32 AtY = P.y;
		f32 AtX = P.x;
		for(char *At = String;
			*At;
			)
		{
			if((At[0] == '\\') &&
			   (At[1] == '#') &&
			   (At[2] != 0) &&
			   (At[3] != 0) &&
			   (At[4] != 0))
			{
				f32 CScale = 1.0f / 9.0f;
				Color = V4(Clamp01(CScale*(f32)(At[2] - '0')),
						   Clamp01(CScale*(f32)(At[3] - '0')),
						   Clamp01(CScale*(f32)(At[4] - '0')),
						   1.0f);
				At += 5;
			}
			else if((At[0] == '\\') &&
					(At[1] == '^') &&
					(At[2] != 0))
			{
				f32 CScale = 1.0f / 9.0f;
				CharScale = ui_text_state->font_info.FontScale*Clamp01(CScale*(f32)(At[2] - '0'));
				At += 3;
			}
			else
			{
				u32 CodePoint = *At;
				if((At[0] == '\\') &&
				   (IsHex(At[1])) &&
				   (IsHex(At[2])) &&
				   (IsHex(At[3])) &&
				   (IsHex(At[4])))
				{
					CodePoint = ((GetHex(At[1]) << 12) |
								 (GetHex(At[2]) << 8) |
								 (GetHex(At[3]) << 4) |
								 (GetHex(At[4]) << 0));
					At += 4;
				}
				
				f32 AdvanceX = CharScale*GetHorizontalAdvanceForPair(Info, Font, PrevCodepoint, CodePoint);
				AtX += AdvanceX;
				
				if(CodePoint != ' ')
				{
					bitmap_id BitmapID = GetBitmapForGlyph(Info, Font, CodePoint);
					sta_bitmap Info = GetBitmapInfo(*Assets, BitmapID);
					
					f32 BitmapScale = CharScale*(f32)Info.Dim[1];
					v3 BitmapOffset = V3(AtX,AtY,0);
					if(Op == DEBUGTextOp_DrawText)
					{
						v4 TextShadowColor = {0,0,0,0.8f};
						draw_ui_text(ui_text_state, screen_width, screen_height, BitmapOffset.xy + V2(1.2f,1.2f), BitmapScale, CodePoint, TextShadowColor);
						draw_ui_text(ui_text_state, screen_width, screen_height, BitmapOffset.xy, BitmapScale, CodePoint, Color);
					} 
					else
					{
						Assert(Op == DEBUGTextOp_SizeText);
						
						loaded_bitmap *Bitmap = GetBitmap(Assets, BitmapID);
						if(Bitmap)
						{
							rectangle2 GlyphDim = RectMinDim(GetAlignedRectPosition(Bitmap, BitmapOffset),  V2((f32)Bitmap->Height * (f32)Bitmap->WidthOverHeight * CharScale, (f32)Bitmap->Height * CharScale));
							Result = Union(Result, GlyphDim);
						}
					}
				}
				
				PrevCodepoint = CodePoint;
				
				++At;
			}
		}
	}
	return Result;
}

internal void
DEBUGTextOutAt(game_assets *Assets, basic_ui_text_state *ui_text_state, u32 screen_width, u32 screen_height, v2 P, char *String, v4 Color = V4(1,1,1,1))
{
	DEBUGTextOp(Assets, ui_text_state, screen_width, screen_height, DEBUGTextOp_DrawText, P, String, Color);
}

internal rectangle2
DEBUGGetTextSize(game_assets *Assets, basic_ui_text_state *ui_text_state, u32 screen_width, u32 screen_height,char *String)
{
	rectangle2 Result = DEBUGTextOp(Assets, ui_text_state, screen_width, screen_height, DEBUGTextOp_SizeText,V2(0,0), String);
	
	return Result;
}

internal f32
DEBUGTextLine(game_assets *Assets, basic_ui_text_state *ui_text_state, u32 screen_width, u32 screen_height, char *String)
{
	engine_font_info font_info = ui_text_state->font_info;
	f32 result = font_info.AtY;
	
	sta_font *Info = GetFontInfo(Assets, font_info.FontID);
	DEBUGTextOutAt(Assets, ui_text_state, screen_width, screen_height, V2(font_info.LeftEdge, 
																		  font_info.AtY - font_info.FontScale * GetStartingBaselineY(font_info.DebugFontInfo)), String);
	
	result += GetLineAdvanceFor(Info) * font_info.FontScale;
	
	return result;
}

internal f32
DEBUGTextLine(game_assets *Assets, basic_ui_text_state *ui_text_state, u32 screen_width, u32 screen_height, char *String, v4 color)
{
	engine_font_info font_info = ui_text_state->font_info;
	f32 result = font_info.AtY;
	
	sta_font *Info = GetFontInfo(Assets, font_info.FontID);
	DEBUGTextOutAt(Assets, ui_text_state, screen_width, screen_height, V2(font_info.LeftEdge, 
																		  font_info.AtY - font_info.FontScale * GetStartingBaselineY(font_info.DebugFontInfo)), String, color);
	
	result += GetLineAdvanceFor(Info) * font_info.FontScale;
	
	return result;
}

internal v2
DEBUGGetNextTextOutPos(engine_font_info font_info, engine_state *EngineState)
{
	v2 result = {};
	result = V2(font_info.LeftEdge, 
				font_info.AtY - font_info.FontScale * GetStartingBaselineY(font_info.DebugFontInfo));
	
	return result;
}