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
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "oximtool.h"
#include "module.h"
#include "unicode.h"

static uch_t etymon_list[N_KEYCODE];
/*----------------------------------------------------------------------------

        hex_init()

----------------------------------------------------------------------------*/
static int
hex_init(void *conf, char *objname)
{
    char *cmd[2], value[50], buf[100];
    char *hex_key = "0123456789abcdef";
    char *hex_name[16] = {"０","１","２","３","４","５","６","７","８","９","Ａ","Ｂ","Ｃ","Ｄ","Ｅ","Ｆ"};
    int keylen = strlen(hex_key), i, idx;

    for (i = 0 ; i < keylen ; i++)
    {
	idx = oxim_key2code(hex_key[i]);
	strcpy((char *)etymon_list[idx].s, hex_name[i]); 
    }

    return  True;
}

/*----------------------------------------------------------------------------

        hex_xim_init(), hex_xim_end()

----------------------------------------------------------------------------*/

static int
hex_xim_init(void *conf, inpinfo_t *inpinfo)
{
    inpinfo->iccf = oxim_malloc(sizeof(hex_iccf_t), 1);
    inpinfo->etymon_list = etymon_list;
    inpinfo->guimode = 0;

    inpinfo->keystroke_len = 0;
    inpinfo->s_keystroke = oxim_malloc((KEY_CODE_LEN+1)*sizeof(uch_t), 1);
    inpinfo->suggest_skeystroke = NULL;

    inpinfo->n_selkey = 0;
    inpinfo->s_selkey = NULL;
    inpinfo->n_mcch = 0;
    inpinfo->mcch = NULL;
    inpinfo->mcch_grouping = NULL;

    inpinfo->n_lcch = 0;
    inpinfo->lcch = NULL;
    inpinfo->lcch_grouping = NULL;
    inpinfo->cch_publish.uch = (uchar_t)0;
    return True;
}

static unsigned int
hex_xim_end(void *conf, inpinfo_t *inpinfo)
{
    free(inpinfo->s_keystroke);
    free(inpinfo->iccf);
    inpinfo->s_keystroke = NULL;
    inpinfo->iccf = NULL;

    return IMKEY_ABSORB;
}


/*----------------------------------------------------------------------------

        hex_switch_in(), hex_switch_out()

----------------------------------------------------------------------------*/

static uchar_t 
hex_check_char(char *keystroke)
{
    char merge[32];
    unsigned int ucs4;
    uch_t cch;
    int base = 16;

    if (keystroke[0]=='#')
    {
	base = 10;
	char *p = keystroke;
	strcpy(merge, ++p);
    }
    else
    {
	strcpy(merge, "0x");
	strcat(merge, keystroke);
    }
    cch.uch = 0;

    ucs4 = (unsigned int)strtoll(merge, NULL, base);
    oxim_ucs4_to_utf8(ucs4, (char *)cch.s);

    return cch.uch;
}

static unsigned int
hex_keystroke(void *conf, inpinfo_t *inpinfo, keyinfo_t *keyinfo)
{
    hex_iccf_t *iccf = (hex_iccf_t *)inpinfo->iccf;
    KeySym keysym = keyinfo->keysym;
    uch_t cch_w;
    static char cch_s[UCH_SIZE+1];
    int len;
    char keychar;

    len = inpinfo->keystroke_len;
    inpinfo->cch = NULL;

    if ((keysym == XK_BackSpace || keysym == XK_Delete) && len) {
        inpinfo->cch_publish.uch = (uchar_t)0;
	iccf->keystroke[len-1] = '\0';
	inpinfo->s_keystroke[len-1].uch = (uchar_t)0;
	inpinfo->keystroke_len --;
        return IMKEY_ABSORB;
    }
    else if (keysym == XK_Escape && len) {
        inpinfo->cch_publish.uch = (uchar_t)0;
	iccf->keystroke[0] = '\0';
	inpinfo->s_keystroke[0].uch = (uchar_t)0;
	inpinfo->keystroke_len = 0;
        return IMKEY_ABSORB;
    }
    else if ((keysym == XK_Return || keysym == XK_space) && len ) {
	if ((cch_w.uch = hex_check_char(iccf->keystroke))) {
	    strncpy(cch_s, (char *)cch_w.s, UCH_SIZE);
            cch_s[UCH_SIZE] = '\0';

            inpinfo->keystroke_len = 0;
            inpinfo->s_keystroke[0].uch = (uchar_t)0;
            inpinfo->cch_publish.uch = cch_w.uch;
            inpinfo->cch = cch_s;
            return IMKEY_COMMIT;
	}
    }
    else if ((XK_0 <= keysym && keysym <= XK_9) ||
             (XK_A <= keysym && keysym <= XK_F) ||
             (XK_a <= keysym && keysym <= XK_f) || keysym == XK_numbersign) {
	if ((keyinfo->keystate & ShiftMask) && keysym != XK_numbersign)
	    return IMKEY_SHIFTESC;
	else if ((keyinfo->keystate & ControlMask) || 
		 (keyinfo->keystate & Mod1Mask))
	    return IMKEY_IGNORE;
	else if (len >= KEY_CODE_LEN)
	    return IMKEY_ABSORB;
	
	if (len && keysym == XK_numbersign)
	    return IMKEY_IGNORE;

        inpinfo->cch_publish.uch = (uchar_t)0;
	keychar = (char)toupper(keyinfo->keystr[0]);
	iccf->keystroke[len] = keychar;
	iccf->keystroke[len+1] = '\0';
	inpinfo->s_keystroke[len].uch = (uchar_t)0;
	inpinfo->s_keystroke[len].s[0] = (char)keychar;
	inpinfo->s_keystroke[len+1].uch = (uchar_t)0;
	len ++;

	/* UCS4 */
	if (len < KEY_CODE_LEN) {
	    inpinfo->keystroke_len ++;
	    return IMKEY_ABSORB;
	}
	else {
	    if ((cch_w.uch = hex_check_char(iccf->keystroke))) {
		strncpy(cch_s, (char *)cch_w.s, UCH_SIZE);
		cch_s[UCH_SIZE] = '\0';

	        inpinfo->keystroke_len = 0;
	        inpinfo->s_keystroke[0].uch = (uchar_t)0;
		inpinfo->cch_publish.uch = cch_w.uch;
		inpinfo->cch = cch_s;
	        return IMKEY_COMMIT;
	    }
	    else {
		inpinfo->keystroke_len ++;
		return IMKEY_ABSORB;
	    }
	}
    }
    else
	return IMKEY_IGNORE;

    return IMKEY_IGNORE;
}

static int 
hex_show_keystroke(void *conf, simdinfo_t *simdinfo)
{
    simdinfo->s_keystroke = NULL;
    return False;
}

/*----------------------------------------------------------------------------

        Definition of general input method module (template).

----------------------------------------------------------------------------*/

static char *hex_valid_objname[] = { "unicode", NULL };

static char hex_comments[] = N_(
	"This is the Unicode input method module.\n");

module_t module_ptr = {
    { MTYPE_IM,					/* module_type */
      "Unicode",
      MODULE_VERSION,				/* version */
      hex_comments },				/* comments */
      hex_valid_objname,			/* valid_objname */
      sizeof(NULL),				/* conf_size */
      hex_init,					/* init */
      hex_xim_init,				/* xim_init */
      hex_xim_end,				/* xim_end */
      hex_keystroke,				/* keystroke */
      hex_show_keystroke,			/* show_keystroke */
      NULL					/* terminate */
};
