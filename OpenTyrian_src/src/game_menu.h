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
#ifndef GAME_MENU_H
#define GAME_MENU_H

#include "helptext.h"
#include "opentyr.h"

typedef JE_byte JE_MenuChoiceType[MAX_MENU];

JE_longint JE_cashLeft( void );
void JE_itemScreen( void );

void load_cubes( void );
bool load_cube( int cube_slot, int cube_index );

void JE_drawItem( JE_byte itemType, JE_word itemNum, JE_word x, JE_word y );
void JE_drawMenuHeader( void );
void JE_drawMenuChoices( void );
void JE_updateNavScreen( void );
void JE_drawNavLines( JE_boolean dark );
void JE_drawLines( JE_boolean dark );
void JE_drawDots( void );
void JE_drawPlanet( JE_byte planetNum );
void JE_scaleBitmap( SDL_Surface *bitmap, JE_word x, JE_word y, JE_word x1, JE_word y1, JE_word x2, JE_word y2 );
void JE_initWeaponView( void );
void JE_computeDots( void );
JE_integer JE_partWay( JE_integer start, JE_integer finish, JE_byte dots, JE_byte dist );
void JE_doFunkyScreen( void );
void JE_drawMainMenuHelpText( void );
JE_boolean JE_quitRequest( void );
void JE_genItemMenu( JE_byte itemnum );
void JE_scaleInPicture( void );
void JE_drawScore( void );
void JE_menuFunction( JE_byte select );
void JE_funkyScreen( void );
void JE_weaponSimUpdate( void );
void JE_weaponViewFrame( void );

#endif // GAME_MENU_H

// kate: tab-width 4; vim: set noet:
