/*
    Copyright (C) 2006 by OXIM TEAM

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/      

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "oximint.h"
#include "module.h"

typedef struct xkeymap_s {
    KeySym xkey;
    uch_t uch;
    uch_t uch_c;
} xkeymap_t; 

static xkeymap_t fullchar[] = {		/* sorted by ascii code */
	{XK_space,		{"　"},{"　"}},	/*     */
	{XK_exclam,		{"！"},{"！"}},	/*  !  */
	{XK_quotedbl,		{"”"},{"》"}},	/*  "  */
	{XK_numbersign,		{"＃"},{"＃"}},	/*  #  */
	{XK_dollar,		{"＄"},{"＄"}},	/*  $  */
	{XK_percent,		{"％"},{"％"}},	/*  %  */
	{XK_ampersand,		{"＆"},{"＆"}},	/*  &  */
	{XK_apostrophe,		{"’"},{"《"}},	/*  '  */
	{XK_parenleft,		{"（"},{"（"}},	/*  (  */
	{XK_parenright,		{"）"},{"）"}},	/*  )  */
	{XK_asterisk,		{"＊"},{"＊"}},	/*  *  */
	{XK_plus,		{"＋"},{"＋"}},	/*  +  */
	{XK_comma,		{"，"},{"，"}},	/*  ,  */
	{XK_minus,		{"－"},{"－"}},	/*  -  */
	{XK_period,		{"．"},{"。"}},	/*  .  */
	{XK_slash,		{"╱"},{"╱"}},	/*  /  (undefined) */
	{XK_0,			{"０"},{"0"}},	/*  0  */
	{XK_1,			{"１"},{"1"}},	/*  1  */
	{XK_2,			{"２"},{"2"}},	/*  2  */
	{XK_3,			{"３"},{"3"}},	/*  3  */
	{XK_4,			{"４"},{"4"}},	/*  4  */
	{XK_5,			{"５"},{"5"}},	/*  5  */
	{XK_6,			{"６"},{"6"}},	/*  6  */
	{XK_7,			{"７"},{"7"}},	/*  7  */
	{XK_8,			{"８"},{"8"}},	/*  8  */
	{XK_9,			{"９"},{"9"}},	/*  9  */
	{XK_colon,		{"："},{"："}},	/*  :  */
	{XK_semicolon,		{"；"},{"；"}},	/*  ;  */
	{XK_less,		{"＜"},{"，"}},	/*  <  */
	{XK_equal,		{"＝"},{"＝"}},	/*  =  */
	{XK_greater,		{"＞"},{"。"}},	/*  >  */
	{XK_question,		{"？"},{"？"}},	/*  ?  */
	{XK_at,			{"＠"},{"＠"}},	/*  @  */
	{XK_A,			{"Ａ"},{"A"}},	/*  A  */
	{XK_B,			{"Ｂ"},{"B"}},	/*  B  */
	{XK_C,			{"Ｃ"},{"C"}},	/*  C  */
	{XK_D,			{"Ｄ"},{"D"}},	/*  D  */
	{XK_E,			{"Ｅ"},{"E"}},	/*  E  */
	{XK_F,			{"Ｆ"},{"F"}},	/*  F  */
	{XK_G,			{"Ｇ"},{"G"}},	/*  G  */
	{XK_H,			{"Ｈ"},{"H"}},	/*  H  */
	{XK_I,			{"Ｉ"},{"I"}},	/*  I  */
	{XK_J,			{"Ｊ"},{"J"}},	/*  J  */
	{XK_K,			{"Ｋ"},{"K"}},	/*  K  */
	{XK_L,			{"Ｌ"},{"L"}},	/*  L  */
	{XK_M,			{"Ｍ"},{"M"}},	/*  M  */
	{XK_N,			{"Ｎ"},{"N"}},	/*  N  */
	{XK_O,			{"Ｏ"},{"O"}},	/*  O  */
	{XK_P,			{"Ｐ"},{"P"}},	/*  P  */
	{XK_Q,			{"Ｑ"},{"Q"}},	/*  Q  */
	{XK_R,			{"Ｒ"},{"R"}},	/*  R  */
	{XK_S,			{"Ｓ"},{"S"}},	/*  S  */
	{XK_T,			{"Ｔ"},{"T"}},	/*  T  */
	{XK_U,			{"Ｕ"},{"U"}},	/*  U  */
	{XK_V,			{"Ｖ"},{"V"}},	/*  V  */
	{XK_W,			{"Ｗ"},{"W"}},	/*  W  */
	{XK_X,			{"Ｘ"},{"X"}},	/*  X  */
	{XK_Y,			{"Ｙ"},{"Y"}},	/*  Y  */
	{XK_Z,			{"Ｚ"},{"Z"}},	/*  Z  */
	{XK_bracketleft,	{"〔"},{"「"}},	/*  [  */
	{XK_backslash,		{"╲"},{"、"}},	/*  \  (undefined) */
	{XK_bracketright,	{"〕"},{"」"}},	/*  ]  */
	{XK_asciicircum,	{"︿"},{"︿"}},	/*  ^  */
	{XK_underscore,		{"ˍ"},{"—"}},	/*  _  (undefined) */
	{XK_grave,		{"‵"},{"…"}},	/*  `  */
	{XK_a,			{"ａ"},{"a"}},	/*  a  */
	{XK_b,			{"ｂ"},{"b"}},	/*  b  */
	{XK_c,			{"ｃ"},{"c"}},	/*  c  */
	{XK_d,			{"ｄ"},{"d"}},	/*  d  */
	{XK_e,			{"ｅ"},{"e"}},	/*  e  */
	{XK_f,			{"ｆ"},{"f"}},	/*  f  */
	{XK_g,			{"ｇ"},{"g"}},	/*  g  */
	{XK_h,			{"ｈ"},{"h"}},	/*  h  */
	{XK_i,			{"ｉ"},{"i"}},	/*  i  */
	{XK_j,			{"ｊ"},{"j"}},	/*  j  */
	{XK_k,			{"ｋ"},{"k"}},	/*  k  */
	{XK_l,			{"ｌ"},{"l"}},	/*  l  */
	{XK_m,			{"ｍ"},{"m"}},	/*  m  */
	{XK_n,			{"ｎ"},{"n"}},	/*  n  */
	{XK_o,			{"ｏ"},{"o"}},	/*  o  */
	{XK_p,			{"ｐ"},{"p"}},	/*  p  */
	{XK_q,			{"ｑ"},{"q"}},	/*  q  */
	{XK_r,			{"ｒ"},{"r"}},	/*  r  */
	{XK_s,			{"ｓ"},{"s"}},	/*  s  */
	{XK_t,			{"ｔ"},{"t"}},	/*  t  */
	{XK_u,			{"ｕ"},{"u"}},	/*  u  */
	{XK_v,			{"ｖ"},{"v"}},	/*  v  */
	{XK_w,			{"ｗ"},{"w"}},	/*  w  */
	{XK_x,			{"ｘ"},{"x"}},	/*  x  */
	{XK_y,			{"ｙ"},{"y"}},	/*  y  */
	{XK_z,			{"ｚ"},{"z"}},	/*  z  */
	{XK_braceleft,		{"｛"},{"『"}},	/*  {  */
	{XK_bar,		{"｜"},{"※"}},	/*  |  */
	{XK_braceright,		{"｝"},{"』"}},	/*  }  */
	{XK_asciitilde,		{"～"},{"～"}},	/*  ~  */
	{0L,			{'\0'},{'\0'}}
};

static char cch[UCH_SIZE+1];

char *
fullchar_keystroke(inpinfo_t *inpinfo, KeySym keysym, int is_cinput)
{
    xkeymap_t *fc=fullchar;
    
    while (fc->xkey != 0) {
	if (keysym == fc->xkey) {
	    if (!is_cinput)
		strncpy(cch, (char *)fc->uch.s, UCH_SIZE);
	    else
		strncpy(cch, (char *)fc->uch_c.s, UCH_SIZE);
	    cch[UCH_SIZE] = '\0';
	    return cch;
	}
	fc ++;
    }
    return NULL;
}

char *
fullchar_keystring(int ch)
{
    int i;

    if ((i = ch - (int)' ') >= 95 || i < 0)
	return NULL;
    strncpy(cch, (char *)fullchar[i].uch.s, UCH_SIZE);
    cch[UCH_SIZE] = '\0';
    return cch;
}

char *
fullchar_ascii(inpinfo_t *inpinfo, int mode, keyinfo_t *keyinfo, int is_cinput)
{
    int i;

    if (keyinfo->keystr_len != 1)
	return fullchar_keystroke(inpinfo, keyinfo->keysym, is_cinput);

    if ((i = (int)(keyinfo->keystr[0]-' ')) >= 95 || i < 0)
	return NULL;

    if (mode) {
	if ((keyinfo->keystate & LockMask) && (keyinfo->keystate & ShiftMask))
	    i = toupper(keyinfo->keystr[0]) - ' ';
	else
	    i = tolower(keyinfo->keystr[0]) - ' ';
    }
    if( !is_cinput )
	strncpy(cch, (char *)fullchar[i].uch.s, UCH_SIZE);
    else
	strncpy(cch, (char *)fullchar[i].uch_c.s, UCH_SIZE);
    cch[UCH_SIZE] = '\0';
    return cch;
}

char *
halfchar_ascii(inpinfo_t *inpinfo, int mode, keyinfo_t *keyinfo)
{
    int i;

    if (keyinfo->keystr_len != 1)
	return NULL;

    if ((i = (int)(keyinfo->keystr[0]-' ')) >= 95 || i < 0)
	return NULL;

    if (mode) {
	if ((keyinfo->keystate & LockMask) && (keyinfo->keystate & ShiftMask))
	    i = toupper(keyinfo->keystr[0]);
	else
	    i = tolower(keyinfo->keystr[0]);

        cch[0] = (char)i;
        cch[1] = '\0';
        return cch;
    }
    else
	return NULL;
}

KeySym
keysym_ascii(int ch)
{
    int i;

    if ((i = (int)(ch - ' ')) >= 95 || i < 0)
	return (KeySym)0;
    else
	return fullchar[i].xkey;
}
