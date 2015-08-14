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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include "module.h"

#define N_NORMAL_KEYS  47
static char keymap_normal[128] = {'\0', '\0'};
static char *ichmap_normal = " 1234567890abcdefghijklmnopqrstuvwxyz`-=[];',./\\";

static void
keymap_init(void)
{
    bzero(keymap_normal, 128);
    keymap_normal['1'] = 1;
    keymap_normal['2'] = 2;
    keymap_normal['3'] = 3;
    keymap_normal['4'] = 4;
    keymap_normal['5'] = 5;
    keymap_normal['6'] = 6;
    keymap_normal['7'] = 7;
    keymap_normal['8'] = 8;
    keymap_normal['9'] = 9;
    keymap_normal['0'] = 10;

    keymap_normal['a'] = 11;
    keymap_normal['b'] = 12;
    keymap_normal['c'] = 13;
    keymap_normal['d'] = 14;
    keymap_normal['e'] = 15;
    keymap_normal['f'] = 16;
    keymap_normal['g'] = 17;
    keymap_normal['h'] = 18;
    keymap_normal['i'] = 19;
    keymap_normal['j'] = 20;
    keymap_normal['k'] = 21;
    keymap_normal['l'] = 22;
    keymap_normal['m'] = 23;
    keymap_normal['n'] = 24;
    keymap_normal['o'] = 25;
    keymap_normal['p'] = 26;
    keymap_normal['q'] = 27;
    keymap_normal['r'] = 28;
    keymap_normal['s'] = 29;
    keymap_normal['t'] = 30;
    keymap_normal['u'] = 31;
    keymap_normal['v'] = 32;
    keymap_normal['w'] = 33;
    keymap_normal['x'] = 34;
    keymap_normal['y'] = 35;
    keymap_normal['z'] = 36;

    keymap_normal['`'] = 37;
    keymap_normal['-'] = 38;
    keymap_normal['='] = 39;
    keymap_normal['['] = 40;
    keymap_normal[']'] = 41;
    keymap_normal[';'] = 42;
    keymap_normal['\''] = 43;
    keymap_normal[','] = 44;
    keymap_normal['.'] = 45;
    keymap_normal['/'] = 46;
    keymap_normal['\\'] = 47;

    keymap_normal['!'] = 1;
    keymap_normal['@'] = 2;
    keymap_normal['#'] = 3;
    keymap_normal['$'] = 4;
    keymap_normal['%'] = 5;
    keymap_normal['^'] = 6;
    keymap_normal['&'] = 7;
    keymap_normal['*'] = 8;
    keymap_normal['('] = 9;
    keymap_normal[')'] = 10;

    keymap_normal['A'] = 11;
    keymap_normal['B'] = 12;
    keymap_normal['C'] = 13;
    keymap_normal['D'] = 14;
    keymap_normal['E'] = 15;
    keymap_normal['F'] = 16;
    keymap_normal['G'] = 17;
    keymap_normal['H'] = 18;
    keymap_normal['I'] = 19;
    keymap_normal['J'] = 20;
    keymap_normal['K'] = 21;
    keymap_normal['L'] = 22;
    keymap_normal['M'] = 23;
    keymap_normal['N'] = 24;
    keymap_normal['O'] = 25;
    keymap_normal['P'] = 26;
    keymap_normal['Q'] = 27;
    keymap_normal['R'] = 28;
    keymap_normal['S'] = 29;
    keymap_normal['T'] = 30;
    keymap_normal['U'] = 31;
    keymap_normal['V'] = 32;
    keymap_normal['W'] = 33;
    keymap_normal['X'] = 34;
    keymap_normal['Y'] = 35;
    keymap_normal['Z'] = 36;

    keymap_normal['~'] = 37;
    keymap_normal['_'] = 38;
    keymap_normal['+'] = 39;
    keymap_normal['{'] = 40;
    keymap_normal['}'] = 41;
    keymap_normal[':'] = 42;
    keymap_normal['\"'] = 43;
    keymap_normal['<'] = 44;
    keymap_normal['>'] = 45;
    keymap_normal['?'] = 46;
    keymap_normal['|'] = 47;
}


/*------------------------------------------------------------------------*/

int
oxim_key2code(int key)
{
    if (! keymap_normal['1'])
	keymap_init();
    return (key < 128) ? keymap_normal[key] : 0;
}

int
oxim_code2key(int code)
{
    return (code <= N_NORMAL_KEYS && code > 0) ? ichmap_normal[code] : 0;
}

#define N_KEY_IN_CODE 5		/* Number of keys in code. */

int
oxim_keys2codes(unsigned int *klist, int klist_size, char *keystroke)
{
    int i, j;
    unsigned int k, *kk=klist;
    char *kc = keystroke;

    if (! keymap_normal['1'])
	keymap_init();

    *kk = 0;
    for (i=0, j=0; *kc; i++, kc++) {
        k = keymap_normal[(int) *kc];
        *kk |= (k << (24 - (i - j*N_KEY_IN_CODE) * 6));
	
        if (i % N_KEY_IN_CODE == N_KEY_IN_CODE-1) {
	    if (++j >= klist_size)
		break;
	    kk++;
	    *kk = 0;
        }
    }
    return j;
}

void
oxim_codes2keys(unsigned int *klist, int n_klist, char *keystroke, int keystroke_len)
{
    int i, j, n_ch=0, shift;
    unsigned int mask = 0x3f, idx;    
    char *s;

    for (j=0; j<n_klist; j++) {
	for (i=0; i<N_KEY_IN_CODE; i++) {
	    shift = 24 - i * 6;
	    if (n_ch < keystroke_len-1) {
	        idx = (klist[j] & (mask << shift)) >> shift;
		keystroke[n_ch] = ichmap_normal[idx];
		n_ch ++;
	    }
	    else
		break;
	}
    }
    keystroke[n_ch] = '\0';

    if ((s = strchr(keystroke, ' ')))
	*s = '\0';
}

