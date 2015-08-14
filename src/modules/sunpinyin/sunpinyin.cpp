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
#define SIZEOF_LONG_LONG 8
#include "oximtool.h"
#include "module.h"

#include "sunpinyinh.h"

#include <sunpinyin.h>
#include "imi_win.h"

static int selKey_define[10] = 
	{'1','2','3','4','5','6','7','8','9','0'};
// static CIMIView *pv;
static CWinHandler *pwh;
static uch_t etymon_list[N_KEYCODE];
static CSunpinyinSessionFactory::EPyScheme scheme;
/*----------------------------------------------------------------------------

        pinyin_init()

----------------------------------------------------------------------------*/
static int
pinyin_init(void *conf, char *objname)
{
    const char *pinyin_key = "abcdefghijklmnopqrstuvwxyz";
    //const char *pinyin_name[26] = {"Ａ","Ｂ","Ｃ","Ｄ","Ｅ","Ｆ","Ｇ","Ｈ","Ｉ","Ｊ","Ｋ","Ｌ","Ｍ","Ｎ","Ｏ","Ｐ","Ｑ","Ｒ","Ｓ","Ｔ","Ｕ","Ｖ","Ｗ","Ｘ","Ｙ","Ｚ"};
    const char *pinyin_name[26] = {"ａ","ｂ","ｃ","ｄ","ｅ","ｆ","ｇ","ｈ","ｉ","ｊ","ｋ","ｌ","ｍ","ｎ","ｏ","ｐ","ｑ","ｒ","ｓ","ｔ","ｕ","ｖ","ｗ","ｘ","ｙ","ｚ"};
    int keylen = strlen(pinyin_key), i, idx;

    settings_t *im_settings = oxim_get_im_settings(objname);
    if (!im_settings)
    {
        printf("沒有 %s 的設定!\n", objname);
        return False;
    }

    int PinyinScheme;
    if (oxim_setting_GetInteger(im_settings, (char *)"PinyinScheme", &PinyinScheme))
    {
	scheme = PinyinScheme == 1 ? CSunpinyinSessionFactory::QUANPIN : CSunpinyinSessionFactory::SHUANGPIN;
    }

    for (i = 0 ; i < keylen ; i++)
    {
	idx = oxim_key2code(pinyin_key[i]);
	strcpy((char *)etymon_list[idx].s, pinyin_name[i]); 
    }

    return  True;
}

/*----------------------------------------------------------------------------

        pinyin_xim_init(), pinyin_xim_end()

----------------------------------------------------------------------------*/

static int
pinyin_xim_init(void *conf, inpinfo_t *inpinfo)
{
    int i = 0;
    static char cchBuffer[1024 * 6 + 1];
    
    CSunpinyinSessionFactory& factory = CSunpinyinSessionFactory::getFactory ();
//     factory.setPinyinScheme (CSunpinyinSessionFactory::QUANPIN);
    factory.setPinyinScheme (scheme);
    CIMIView *pv = factory.createSession ();

    /*CWinHandler **/pwh = new CWinHandler(pv, inpinfo);
    pv->getIC()->setCharsetLevel(1);// GBK
    pv->attachWinHandler(pwh);

    inpinfo->etymon_list = etymon_list;
    inpinfo->guimode = 0;

    inpinfo->keystroke_len = 0;
    inpinfo->s_keystroke = (uch_t *)oxim_malloc((KEY_CODE_LEN+1)*sizeof(uch_t)*2, 1);

    inpinfo->n_selkey = 10;
    inpinfo->s_selkey = (uch_t *) calloc(inpinfo->n_selkey, sizeof(uch_t));
    
    for (i = 0; i < inpinfo->n_selkey; i++)
    {
	inpinfo->s_selkey[i].uch = (uchar_t) 0;
	inpinfo->s_selkey[i].s[0] = (char)selKey_define[i];
    }

    inpinfo->mcch = (uch_t *) calloc(MAX_CHOICE_BUF, sizeof(uch_t));
    inpinfo->mcch_grouping = (uint_t *)calloc( inpinfo->n_selkey, sizeof(uint_t));
    inpinfo->n_mcch = 0;
    
/*    inpinfo->n_lcch = 0;
    inpinfo->lcch = (uch_t *) calloc(1024, sizeof(uch_t));
    inpinfo->lcch_grouping = (ubyte_t *) calloc(1024, sizeof(ubyte_t));*/
    inpinfo->cch = cchBuffer;
    inpinfo->cch_publish.uch = (uchar_t)0;
    return True;
}

static unsigned int
pinyin_xim_end(void *conf, inpinfo_t *inpinfo)
{
    free(inpinfo->s_keystroke);
//     free(inpinfo->lcch);
    free(inpinfo->mcch);
    free(inpinfo->mcch_grouping);
    free(inpinfo->s_selkey);

    inpinfo->s_keystroke = NULL;
//     inpinfo->lcch = NULL;
    inpinfo->mcch = NULL;
    inpinfo->mcch_grouping = NULL;
    inpinfo->s_selkey = NULL;

    return IMKEY_ABSORB;
}

static void 
ShowText(inpinfo_t *inpinfo, char *buf)
{
    int chiSymbolBufLen = 1024;

    bzero(inpinfo->lcch, sizeof(uch_t)*1024) ;
    char *bufferStr = buf;
    unsigned int ucs4, olen = strlen(bufferStr);
    int nbytes, nchars = 0, len = 0;
    /* for each char in the phrase, copy to mcch */
    while (olen && (nbytes = oxim_utf8_to_ucs4(bufferStr, &ucs4, olen)) > 0)
    {
	inpinfo->lcch[len].uch = (uchar_t)0;
	memcpy(inpinfo->lcch[len++].s, bufferStr, nbytes);
	nchars ++;
	bufferStr += nbytes;
	olen -= nbytes;
    }

    inpinfo->n_lcch = nchars ;
}

#if 0
static void 
DeleteBuffer(inpinfo_t *inpinfo)
{
    int location = inpinfo->edit_pos;
    char *bufferStr = inpinfo->cch;
    unsigned int ucs4, olen = strlen(bufferStr);
    int nbytes, nchars = 0, len = 0;
//     bzero(inpinfo->lcch, sizeof(uch_t)*1024) ;
    /* for each char in the phrase, copy to mcch */
    while (olen && (nbytes = oxim_utf8_to_ucs4(bufferStr, &ucs4, olen)) > 0)
    {
	if ( nchars == location )
	{
	    olen -= nbytes;
	    continue;
	}
	inpinfo->cch[len++] = (uchar_t)0;
	memcpy(inpinfo->cch, bufferStr, nbytes);
	nchars ++;
	bufferStr += nbytes;
	olen -= nbytes;
    }
    ShowText(inpinfo, inpinfo->cch);
puts(inpinfo->cch);
//     inpinfo->n_lcch = nchars ;
}
#endif

static void
onKeyEvent(keyinfo_t *keyinfo)
{
    unsigned keycode = keyinfo->keysym;
    unsigned keyvalue = keyinfo->keysym;
    
    if (keyvalue < 0x20 && keyvalue > 0x7E)
	keyvalue = 0;
    if (XK_Left == keyvalue)
	keyvalue = XK_Page_Up;
    if (XK_Right == keyvalue)
	keyvalue = XK_Page_Down;
    keycode = keyvalue;

    CKeyEvent key_event (keycode, keyvalue, keyinfo->keystate);
    pwh->getView()->onKeyEvent(key_event);
}

/*----------------------------------------------------------------------------

        pinyin_keystroke(), pinyin_show_keystroke()

----------------------------------------------------------------------------*/

static unsigned int
pinyin_keystroke(void *conf, inpinfo_t *inpinfo, keyinfo_t *keyinfo)
{
    KeySym keysym = keyinfo->keysym;
    uch_t cch_w;
    int len;

    len = inpinfo->keystroke_len;
    
    if ( (XK_A <= keysym && keysym <= XK_Z) ||
	    (XK_a <= keysym && keysym <= XK_z) )
    {
	if (len >= KEY_CODE_LEN)
	    return IMKEY_ABSORB;
	
	len++;
	onKeyEvent(keyinfo);
	
	inpinfo->guimode = 0;
	inpinfo->guimode |= GUIMOD_LISTCHAR;
	
	if ((keyinfo->keystate & ShiftMask))
	    return IMKEY_SHIFTESC;
	else if ((keyinfo->keystate & ControlMask) || 
		(keyinfo->keystate & Mod1Mask))
	    return IMKEY_IGNORE;
	else if (len >= KEY_CODE_LEN)
	    return IMKEY_ABSORB;
// 	inpinfo->keystroke_len ++;
	return IMKEY_ABSORB;
	
    }
    else if( (XK_space == keysym) && len )
    {
	if( inpinfo->guimode & GUIMOD_LISTCHAR )
	{
	    inpinfo->guimode = 0;
	    inpinfo->guimode |= GUIMOD_SELKEYSPOT;

	}
	else
	{
	    keyinfo->keysym = XK_Right;
	    onKeyEvent(keyinfo);
	}
	    return IMKEY_ABSORB;
    }
    else if( XK_Left == keysym || XK_Right == keysym)
    {
	if ( (inpinfo->guimode & GUIMOD_SELKEYSPOT) && !inpinfo->n_lcch && len)
	{
	    onKeyEvent(keyinfo);
	    return IMKEY_ABSORB;
	}
/*	else
	{
	    inpinfo->edit_pos = ( XK_Left == keysym ) ?
		( ( inpinfo->edit_pos>0 ) ? inpinfo->edit_pos - 1 : 0 )
		:
		( inpinfo->edit_pos + 1 )
		;
	}*/
	if (len)
	    return IMKEY_ABSORB;
	return IMKEY_IGNORE;
    }
    else if ( (XK_0 <= keysym && keysym <= XK_9) && len )
    {
// 	if( inpinfo->guimode & GUIMOD_SELKEYSPOT )
	{
	    onKeyEvent(keyinfo);
	    if ( !inpinfo->n_lcch )
		inpinfo->cch[0] = '\0';
	    strcat(inpinfo->cch, (const char*) pwh->getBuffer());
	    pwh->clearBuffer();
    // 	ShowText(inpinfo, inpinfo->cch);
    // 	inpinfo->edit_pos = oxim_utf8len(inpinfo->cch);
	    return IMKEY_COMMIT;
	}
    }
    else if ( (XK_Return == keysym)/* && len*/)
    {
	onKeyEvent(keyinfo);
	strcpy(inpinfo->cch, (const char*) pwh->getBuffer());
	pwh->clearBuffer();

// 	inpinfo->n_mcch = 0;
// 	inpinfo->keystroke_len = 0;
// 	inpinfo->n_lcch = 0;
// 	inpinfo->lcch[0].uch = (uchar_t)0;
// 	inpinfo->s_keystroke[0].uch = (uchar_t)0;
// 	inpinfo->cch_publish.uch = cch_w.uch;
	if (len)
	    return IMKEY_COMMIT;
	return IMKEY_IGNORE;
    }
    else if ( (XK_Escape == keysym) && len) {
	onKeyEvent(keyinfo);

	inpinfo->n_mcch = 0;
	inpinfo->cch_publish.uch = (uchar_t)0;
// 	iccf->keystroke[0] = '\0';
	inpinfo->s_keystroke[0].uch = (uchar_t)0;
	inpinfo->keystroke_len = 0;
	return IMKEY_ABSORB;
    }
    else if ((keysym == XK_BackSpace || keysym == XK_Delete)/* && len*/) {
// 	if ( !inpinfo->n_lcch )
	if ( (inpinfo->guimode & GUIMOD_LISTCHAR) && len)
	{
	    onKeyEvent(keyinfo);
	    inpinfo->cch_publish.uch = (uchar_t)0;
// 	    iccf->keystroke[len-1] = '\0';
// 	    inpinfo->s_keystroke[len-1].uch = (uchar_t)0;
// 	    inpinfo->keystroke_len --;
	    
	    return IMKEY_ABSORB;
	}

	return IMKEY_IGNORE;
/*	else
	{
	    DeleteBuffer(inpinfo);
	    
	    return IMKEY_ABSORB;
	}*/
    }
    else
    {
	char *ptr_buf = pwh->getBuffer();
	
	onKeyEvent(keyinfo);

	if ( !*ptr_buf )
	{
	    return IMKEY_IGNORE;
	}
	
	if ( !inpinfo->n_lcch )
	    inpinfo->cch[0] = '\0';
	strcat(inpinfo->cch, (const char*) ptr_buf);
	pwh->clearBuffer();
	
	return IMKEY_COMMIT;
    }

    return IMKEY_IGNORE;
}

static int 
pinyin_show_keystroke(void *conf, simdinfo_t *simdinfo)
{
    simdinfo->s_keystroke = NULL;
    return False;
}

/*----------------------------------------------------------------------------

        Definition of general input method module (template).

----------------------------------------------------------------------------*/

static char *pinyin_valid_objname[] = { (char *)"sunpinyin", NULL };

static char pinyin_comments[] = N_(
	"This is the Unicode input method module.\n");

module_t module_ptr = {
    { MTYPE_IM,					/* module_type */
      (char *)"Sunpinyin",
      (char *)MODULE_VERSION,				/* version */
      pinyin_comments },				/* comments */
      pinyin_valid_objname,			/* valid_objname */
      sizeof(NULL),				/* conf_size */
      pinyin_init,					/* init */
      pinyin_xim_init,				/* xim_init */
      pinyin_xim_end,				/* xim_end */
      pinyin_keystroke,				/* keystroke */
      pinyin_show_keystroke,			/* show_keystroke */
      NULL					/* terminate */
};
