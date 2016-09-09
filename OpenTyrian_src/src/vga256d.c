/*
 * OpenTyrian Classic: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "config.h" // For fullscreen stuff
#include "keyboard.h"
#include "opentyr.h"
#include "palette.h"
#include "vga256d.h"
#include "video.h"

#include "SDL.h"
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void JE_pix( JE_word x, JE_word y, JE_byte c )
{
	/* Bad things happen if we don't clip */
	if (x <  VGAScreen->pitch && y <  VGAScreen->h)
	{
		Uint8 *vga = VGAScreen->pixels;
		vga[y * VGAScreen->pitch + x] = c;
	}
}

void JE_pix3( JE_word x, JE_word y, JE_byte c )
{
	/* Originally impemented as several direct accesses */
	JE_pix(x, y, c);
	JE_pix(x - 1, y, c);
	JE_pix(x + 1, y, c);
	JE_pix(x, y - 1, c);
	JE_pix(x, y + 1, c);
}

void JE_rectangle( JE_word a, JE_word b, JE_word c, JE_word d, JE_word e ) /* x1, y1, x2, y2, color */
{
	if (a < VGAScreen->pitch && b < VGAScreen->h &&
	    c < VGAScreen->pitch && d < VGAScreen->h)
	{
		Uint8 *vga = VGAScreen->pixels;
		int i;

		/* Top line */
		memset(&vga[b * VGAScreen->pitch + a], e, c - a + 1);

		/* Bottom line */
		memset(&vga[d * VGAScreen->pitch + a], e, c - a + 1);

		/* Left line */
		for (i = (b + 1) * VGAScreen->pitch + a; i < (d * VGAScreen->pitch + a); i += VGAScreen->pitch)
		{
			vga[i] = e;
		}

		/* Right line */
		for (i = (b + 1) * VGAScreen->pitch + c; i < (d * VGAScreen->pitch + c); i += VGAScreen->pitch)
		{
			vga[i] = e;
		}
	} else {
		printf("!!! WARNING: Rectangle clipped: %d %d %d %d %d\n", a, b, c, d, e);
	}
}

void JE_bar( JE_word a, JE_word b, JE_word c, JE_word d, JE_byte e ) /* x1, y1, x2, y2, color */
{
	if (a < VGAScreen->pitch && b < VGAScreen->h &&
	    c < VGAScreen->pitch && d < VGAScreen->h)
	{
		Uint8 *vga = VGAScreen->pixels;
		int i, width;

		width = c - a + 1;

		for (i = b * VGAScreen->pitch + a; i <= d * VGAScreen->pitch + a; i += VGAScreen->pitch)
		{
			memset(&vga[i], e, width);
		}
	} else {
		printf("!!! WARNING: Filled Rectangle clipped: %d %d %d %d %d\n", a, b, c, d, e);
	}
}

void JE_c_bar( JE_word a, JE_word b, JE_word c, JE_word d, JE_byte e )
{
	if (a < VGAScreen->pitch && b < VGAScreen->h &&
	    c < VGAScreen->pitch && d < VGAScreen->h)
	{
		Uint8 *vga = VGAScreenSeg->pixels;
		int i, width;

		width = c - a + 1;

		for (i = b * VGAScreen->pitch + a; i <= d * VGAScreen->pitch + a; i += VGAScreen->pitch)
		{
			memset(&vga[i], e, width);
		}
	} else {
		printf("!!! WARNING: Filled Rectangle clipped: %d %d %d %d %d\n", a,b,c,d,e);
	}
}

void JE_barShade( JE_word a, JE_word b, JE_word c, JE_word d ) /* x1, y1, x2, y2 */
{
	if (a < VGAScreen->pitch && b < VGAScreen->h &&
	    c < VGAScreen->pitch && d < VGAScreen->h)
	{
		Uint8 *vga = VGAScreen->pixels;
		int i, j, width;

		width = c - a + 1;

		for (i = b * VGAScreen->pitch + a; i <= d * VGAScreen->pitch + a; i += VGAScreen->pitch)
		{
			for (j = 0; j < width; j++)
			{
				vga[i + j] = ((vga[i + j] & 0x0F) >> 1) | (vga[i + j] & 0xF0);
			}
		}
	} else {
		printf("!!! WARNING: Darker Rectangle clipped: %d %d %d %d\n", a,b,c,d);
	}
}

void JE_barBright( JE_word a, JE_word b, JE_word c, JE_word d ) /* x1, y1, x2, y2 */
{
	if (a < VGAScreen->pitch && b < VGAScreen->h &&
	    c < VGAScreen->pitch && d < VGAScreen->h)
	{
		Uint8 *vga = VGAScreen->pixels;
		int i, j, width;

		width = c-a+1;

		for (i = b * VGAScreen->pitch + a; i <= d * VGAScreen->pitch + a; i += VGAScreen->pitch)
		{
			for (j = 0; j < width; j++)
			{
				JE_byte al, ah;
				al = ah = vga[i + j];

				ah &= 0xF0;
				al = (al & 0x0F) + 2;

				if (al > 0x0F)
				{
					al = 0x0F;
				}

				vga[i + j] = al + ah;
			}
		}
	} else {
		printf("!!! WARNING: Brighter Rectangle clipped: %d %d %d %d\n", a,b,c,d);
	}
}

// kate: tab-width 4; vim: set noet:
