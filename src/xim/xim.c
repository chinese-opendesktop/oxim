/*
    Copyright (C) 1999 by  XCIN TEAM

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
#include <time.h>
#include <signal.h>
#include <X11/Xlocale.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <langinfo.h>
#include <iconv.h>
#include "IMdkit.h"
#include "Xi18n.h"
#include "oximtool.h"
#include "fkey.h"
#include "oxim.h"

static xccore_t *xccore;

static struct {
    unsigned int nlocale;
    unsigned int nbytes;
    char 	**locale;
} xim_support_locales = {0, 0, NULL};

XIMS ims;

static int nencodings = 0;
static int *im_enc = NULL;

#define BUFLEN             1024

void gui_update_winlist(void);
int ic_create(XIMS ims, IMChangeICStruct *call_data, xccore_t *xccore);
int ic_destroy(int icid, xccore_t *xccore);
int ic_clean_all(CARD16 connect_id, xccore_t *xccore);
void ic_get_values(IC *ic, IMChangeICStruct *call_data, xccore_t *xccore);
void ic_set_values(IC *ic, IMChangeICStruct *call_data, xccore_t *xccore);
//void check_ic_exist(int icid, xccore_t *xccore);
void xim_set_trigger_keys(void);

char *xim_fix_big5_bug(Display *display, char *s)
{
    XTextProperty tp;
#if 0
    XmbTextListToTextProperty(display, &s, 1, XCompoundTextStyle, &tp);
#else
    char *codeset = nl_langinfo(CODESET);
    /* X Bug ! utf8 的 "裏、碁" 無法用 Xutf8TextListToTextProperty 轉出 Big5 */

    if (strcasecmp(codeset, "BIG5") == 0)
    {
	size_t inbytesleft  = strlen(s);
	size_t outbytesleft = inbytesleft;
	char *ret_string = (char *)oxim_malloc(inbytesleft+1, True);
	char *inptr = s;
	char *outptr = ret_string;

	iconv_t cd;
	cd = iconv_open("BIG5", "UTF-8");
	iconv(cd, (char **)&inptr, &inbytesleft, &outptr, &outbytesleft);
	iconv_close(cd);

	XmbTextListToTextProperty(display, &ret_string, 1, XCompoundTextStyle, &tp);
	free(ret_string);
    }
    else
    {
	Xutf8TextListToTextProperty(display, &s, 1, XCompoundTextStyle, &tp);
    }
#endif
    return (char *)tp.value;
}

void
xim_preeditcallback_done(IC *ic)
{
    IMPreeditCBStruct data;
    XIMText text;
    XIMFeedback feedback[256] = {0};
    if (ic->preedit_length)
    {
	data.major_code = XIM_PREEDIT_DRAW;
	data.connect_id = ic->connect_id;
	data.icid = ic->id;
	data.todo.draw.caret = XIMIsInvisible;
	data.todo.draw.chg_first = 0;
	data.todo.draw.chg_length = ic->preedit_length;
	data.todo.draw.text = &text;
	text.encoding_is_wchar = False;
	text.feedback = feedback;
	text.length = 0;
	text.string.multi_byte = "";
	feedback[0] = 0;
	IMCallCallback(ims, (XPointer)&data);
    }

    data.major_code = XIM_PREEDIT_DONE;
    data.connect_id = ic->connect_id;
    data.icid       = ic->id;
    IMCallCallback (ims, (XPointer)&data);
    ic->preedit_length = 0;
}

void xim_update_winlist(void)
{
    gui_update_winlist();
}

/*----------------------------------------------------------------------------

	XIM Tool Functions.

----------------------------------------------------------------------------*/
static void load_support_locales(void)
{
    unsigned int flen = strlen(oxim_sys_rcdir()) + strlen(OXIM_DEFAULT_LOCALE_LIST) + 2;
    char *locale_list = oxim_malloc(flen, False);
    char *locale = oxim_malloc(128, False);

    sprintf(locale_list, "%s/%s", oxim_sys_rcdir(), OXIM_DEFAULT_LOCALE_LIST);

    if (oxim_check_file_exist(locale_list, FTYPE_FILE))
    {
	gzFile *fp = oxim_open_file(locale_list, "r", OXIMMSG_WARNING);
        if (fp)
	{
	    while (oxim_get_line(locale, 128, fp, NULL, "#\n"))
	    {
		if (xim_support_locales.nlocale == 0)
		{
		    xim_support_locales.locale = (char **)malloc(sizeof(char *));
		}
		else
		{
		    xim_support_locales.locale = (char **)realloc(xim_support_locales.locale, (xim_support_locales.nlocale+1) * sizeof(char *));
		}

		xim_support_locales.nbytes += strlen(locale);

    		xim_support_locales.locale[xim_support_locales.nlocale++] = strdup(locale);
	    }
	}
	gzclose(fp);
    }
    else
    {
	oxim_perr(OXIMMSG_ERROR, N_("File: '%s' Not exist.\n"), locale_list);
    }
    free(locale);
    free(locale_list);
}

struct commit_buf_s {
    int slen, size;
    char *str;
} commit_buf;

static void
xim_commit(IC *ic)
{
    char *cch_str_list[1];
    char *value;

    IMCommitStruct call_data;

    if (commit_buf.slen == 0)
	return;

    DebugLog(1, ("commit str (%u,%u): %s\n",
		ic->id, ic->connect_id, commit_buf.str));

    
    char *outstring = NULL;
    char *charset = nl_langinfo(CODESET);
    if (!strcasecmp(charset, "UTF-8"))
    {
	if (ic->imc->inp_state & IM_OUTSIMP)
	{
	    outstring = oxim_output_filter(commit_buf.str, True);
	}
	else if (ic->imc->inp_state & IM_OUTTRAD)
	{
	    outstring = oxim_output_filter(commit_buf.str, False);
	}
    }

    value = xim_fix_big5_bug(xccore->display, (outstring ? outstring : commit_buf.str));

    bzero(&call_data, sizeof(call_data));
    call_data.major_code = XIM_COMMIT;
    call_data.icid = ic->id;
    call_data.connect_id = ic->connect_id;
    call_data.flag = XimLookupChars;
    call_data.commit_string = value;
    IMCommitString(ims, (XPointer)(&call_data));
    XFree(value);
    commit_buf.slen = 0;

    if (outstring)
    {
	free(outstring);
    }
}

void xim_commit_string_raw(IC *ic, char *str)
{
    int cch_len;

    if (ic->ic_rec.input_style & XIMPreeditCallbacks)
    {
	xim_preeditcallback_done(ic);
    }

    if (!str)
    {
	str = ic->imc->cch;
	cch_len = strlen(str);
    }
    else
    {
        cch_len = strlen(str);
        if (ic->imc->cch_size < cch_len+1)
	{
	    ic->imc->cch_size = cch_len+1;
	    ic->imc->cch = oxim_realloc(ic->imc->cch, ic->imc->cch_size);
        }
        strcpy(ic->imc->cch, str);
    }    
#if 1
    if (commit_buf.size-commit_buf.slen < cch_len+1)
    {
	commit_buf.size = commit_buf.slen+cch_len+1;
	commit_buf.str = oxim_realloc(commit_buf.str, commit_buf.size);
    }
    strcpy(commit_buf.str+commit_buf.slen, str);
    commit_buf.slen += cch_len;

    xim_commit(ic);
#else
    // 將字串拆成單一字元送出，避免某些 client 端 buffer 不夠(如 wine)
    char *p = str;
    unsigned int ucs4;
#endif
}

void xim_commit_string(IC *ic, char *str)
{
    char command[BUFLEN], cmd_with_arg[BUFLEN];
    struct stat buf;

    IM_Context_t *imc;
    imc = ic->imc;
    
//     strcpy(command, oxim_user_dir());
//     strcat(command, "/filter");
    if((imc->inp_state & IM_FILTER) &&
    	!is_filter_default(&ic->filter_current)
    	)
    {
//     if( NULL != str[0] && (0==stat(command, &buf)) && (S_ISREG(buf.st_mode)) && (buf.st_mode&S_IXOTH) )
    {
	strcpy(command, (char *)change_filter(0, False, &ic->filter_current));
	strcpy(cmd_with_arg, command);
	strcat(cmd_with_arg, " '");
	strcat(cmd_with_arg, str);
	strcat(cmd_with_arg, "' & ");
// 	if( 0 == system(command) )
	    system(cmd_with_arg);
	return;
    }
    }
    xim_commit_string_raw(ic, str);
}

int
xim_connect(IC *ic)
{
    IMPreeditStateStruct call_data;

    if ((ic->ic_state & IC_CONNECT))
	return True;
    call_data.major_code = XIM_PREEDIT_START;
    call_data.minor_code = 0;
    call_data.connect_id = (CARD16)(ic->connect_id);
    call_data.icid = (CARD16)(ic->id);
    ic->ic_state |= IC_CONNECT;
    return IMPreeditStart(ims, (XPointer)&call_data);
}

int
xim_disconnect(IC *ic)
{
    IMTriggerNotifyStruct call_data;

    if (! (ic->ic_state & IC_CONNECT))
	return True;

    if (ic->ic_rec.input_style & XIMPreeditCallbacks)
        xim_preeditcallback_done(ic);

    call_data.connect_id = (CARD16)(ic->connect_id);
    call_data.icid = (CARD16)(ic->id);
    ic->ic_state &= ~IC_CONNECT;
    return IMPreeditEnd(ims, (XPointer)(&call_data));
}

static int
process_IMKEY(IC *ic, keyinfo_t *keyinfo, unsigned int ret)
{
    char *cch = NULL;
    IM_Context_t *imc = ic->imc;

    if (ret == 0) {
	/* This is IMKEY_ABSORB */
	return True;
    }
    if ((ret & IMKEY_COMMIT) && imc->inpinfo.cch)
    {
	/* Send the result to IM lib. */
	xim_commit_string(ic, imc->inpinfo.cch);
    }
    if (keyinfo)
    {
	if (!cch)
	{
	    if ((ret & IMKEY_SHIFTESC))
	    {
		if ((imc->inp_state & IM_2BYTES))
		{
		    cch = fullchar_ascii(&(imc->inpinfo), 1, keyinfo, imc->inp_state & IM_CINPUT);
		}
		else
		{
		    cch = halfchar_ascii(&(imc->inpinfo), 1, keyinfo);
		}

		if (!cch)
		{
		    ret |= IMKEY_IGNORE;
		}
	    }

	    if ((ret & IMKEY_FALLBACKPHR))
	    {
		ret |= IMKEY_IGNORE;
	    }
	}

	if (cch)
	{
	    xim_commit_string(ic, cch);
	    ret &= ~IMKEY_IGNORE;
	}
    }

    if (! (ret & IMKEY_IGNORE))
    {
	/* The key will not be processed further more, and will not be
	   forewared back to XIM client. */
	return True;
    }
    return False;
}

/*----------------------------------------------------------------------------

	IM Context Handlers.

----------------------------------------------------------------------------*/

void
call_xim_init(IC *ic, int reset)
{
    IM_Context_t *imc = ic->imc;
    if (imc->imodp && ! (imc->inp_state & IM_CINPUT))
    {
	if (reset)
	{
	    imc->imodp->xim_init(imc->imodp->conf, &(imc->inpinfo));
	}
    }
    imc->inp_state |= (IM_CINPUT | IM_XIMFOCUS);
}

void
call_xim_end(IC *ic, int ic_delete, int clear)
{
    unsigned int ret, i;
    IM_Context_t *imc = ic->imc;

    if (imc->imodp && (imc->inp_state & IM_CINPUT))
    {
	if (clear)
	{
	    ret = imc->imodp->xim_end(imc->imodp->conf, &(imc->inpinfo));
	    if (! ic_delete && ret)
		process_IMKEY(ic, NULL, ret);
	}
    }

    if (! ic_delete)
    {
	imc->inp_state &= ~(IM_CINPUT | IM_XIMFOCUS);
    }
}

int
change_IM(IC *ic, int inp_num)
{
    static int first_call;
    imodule_t *imodp=NULL;
    int reset_inpinfo, nextim_english=0;
    IM_Context_t *imc = ic->imc;
/*
 *  Check if the module of the desired IM has been loaded or not.
 */
    if (inp_num < 0 || inp_num >= oxim_get_NumberOfIM())
	nextim_english = 1;
    else if (! (imodp = OXIM_IMGet(inp_num)))
	return False;

    if (first_call == 0) {
	/* in order to let xim_init() take effect */
	reset_inpinfo = 1;
	first_call = 1;
    }
    else if ((xccore->oxim_mode & OXIM_SINGLE_IMC) &&
	     ((int)(imc->inp_num) == inp_num || nextim_english)) {
	/* switch to / from English IM && do not change IM */
	reset_inpinfo = 0;
    }
    else
	reset_inpinfo = 1;

/*
 *  The switch_out() xim_end() will only be called when really change IM.
 */
    if (imc->imodp && imodp != imc->imodp)
    {
	call_xim_end(ic, 0, reset_inpinfo);
    }
 
    if (nextim_english) {
    /* This will switch to English mode. */
	imc->imodp = NULL;
        if ((xccore->oxim_mode & OXIM_IM_FOCUS))
	    xccore->oxim_mode &= ~(OXIM_RUN_IM_FOCUS);
	return True;
    }

/*
 *  Initialize the new IM for this IC.
 */
    imc->imodp = imodp;
    imc->inp_num = inp_num;
    if ((xccore->oxim_mode & OXIM_IM_FOCUS))
    {
	xccore->oxim_mode |= OXIM_RUN_IM_FOCUS;
	xccore->im_focus = imc->inp_num;
    }
    call_xim_init(ic, reset_inpinfo);

    if ((xccore->oxim_mode & OXIM_SHOW_CINPUT)) {
	imc->s_imodp = imc->imodp;
	imc->sinp_num = imc->inp_num;
    }
    else {
	imc->s_imodp = OXIM_IMGet(xccore->default_im_sinmd);
	imc->sinp_num = xccore->default_im_sinmd;
    }
    return True;
}

// static inp_num = 0;
static int current_inp_num = -1;
static int looping = 0;
/*static*/ void
circular_change_IM(IC *ic, int direction)
{
    int i, j;
    inp_state_t inpnum;
    int NumberOfIM = oxim_get_NumberOfIM();

    if ( -1 == current_inp_num )
	current_inp_num = ic->imc->inp_num;

    if (!(ic->imc->inp_state & IM_CINPUT))
    {
	change_IM(ic, ic->imc->inp_num);
	return;
    }
    
    inpnum = ((ic->imc->inp_state & IM_CINPUT)) ? 
			ic->imc->inp_num : NumberOfIM;
/*FILE *f=fopen("/tmp/oximlog", "a");*/
    for (i=0, j=inpnum+direction; i<NumberOfIM+1; i++, j+=direction) {
	if (j > NumberOfIM || j < 0)
	    j += ( (j < 0) ? (NumberOfIM+1) : -(NumberOfIM+1) );


/*fprintf(f, "j=%d\n", j);*/

	if (oxim_IMisCircular(j))
	{
	    if (change_IM(ic, j))
	    {
		/*fprintf(f, "changed\n", j);*/
		if (current_inp_num == j)
		    looping++;
		if (current_inp_num == j && looping>0)
		{
		    looping = 0;
		    change_IM(ic, -1);
		}
		return;
	    }
	}
	/*else
	    fprintf(f, "no changed\n", j);*/
	
    }
/*fclose(f);*/
}

static void
call_simd(IC *ic)
{
    static uchar_t prev_uch;
    simdinfo_t simdinfo;
    uch_t *skey = NULL;
    int skey_len;
    IM_Context_t *imc = ic->imc;

    if (imc->inpinfo.cch_publish.uch) {
	if (imc->inpinfo.cch_publish.uch != prev_uch) {
	    prev_uch = imc->inpinfo.cch_publish.uch;
	}
	if (imc->inp_num == imc->sinp_num && 
	    imc->inpinfo.suggest_skeystroke &&
	    imc->inpinfo.suggest_skeystroke[0].uch)
	    skey = imc->inpinfo.suggest_skeystroke;
	else {
	    simdinfo.imid = ic->imc->id;
	    simdinfo.guimode = imc->inpinfo.guimode;
	    simdinfo.cch_publish.uch = imc->inpinfo.cch_publish.uch;
	
	    if (imc->s_imodp->show_keystroke(imc->s_imodp->conf, &simdinfo))
		skey = simdinfo.s_keystroke;
	}
    }
    if (! skey) {
	imc->sinmd_keystroke[0].uch = (uchar_t)0;
	return;
    }

/*
    skey_len = uchs_len(skey);
*/
    for (skey_len=0; skey[skey_len].uch != (uchar_t)0; skey_len++);

    if (imc->skey_size < skey_len+1) {
	imc->skey_size = skey_len+1;
	imc->sinmd_keystroke = oxim_realloc(
		imc->sinmd_keystroke, imc->skey_size * sizeof(uch_t));
    }
    memcpy(imc->sinmd_keystroke, skey, (skey_len+1)*sizeof(uch_t));
}

static void
change_simd(IC *ic)
{
    int idx;
    imodule_t *imodp;
    IM_Context_t *imc = ic->imc;

    if (imc->inpinfo.cch_publish.uch)
    {
	printf(N_("query->%s\n"), (char *)(imc->inpinfo.cch_publish.s));
    }
    return;

    /* TODO : 記得刪掉 call_simd() */
    do {
	if ((imodp = OXIM_IMGetNext(imc->sinp_num+1, &idx)))
	    break;
    } while (idx != imc->sinp_num);

    if (idx != imc->sinp_num) {
        imc->sinp_num = idx;
        imc->s_imodp = imodp;
        xccore->default_im_sinmd = (inp_state_t)idx;
        xccore->oxim_mode &= ~OXIM_SHOW_CINPUT;
	call_simd(ic);
    }
}

/*----------------------------------------------------------------------------

	IM Protocol Handler.

----------------------------------------------------------------------------*/

static int 
xim_open_handler(XIMS ims, IMOpenStruct *call_data)
{
    DebugLog(2, ("XIM_OPEN\n"));
    //-----------------------------
/*    char buf[1024];
    sprintf(buf, "echo XIM_OPEN %d - %s >> /tmp/oximlog", call_data->connect_id, call_data->lang.name);
    system(buf);*/
    //-----------------------------

    if (call_data->connect_id > nencodings)
    {
	if (nencodings == 0)
	    im_enc = oxim_malloc((sizeof(int *) * call_data->connect_id)+sizeof(int*), True);
	else
	    im_enc = oxim_realloc(im_enc, (sizeof(int *) * call_data->connect_id)+sizeof(int*));
	nencodings = call_data->connect_id;
    }

    int i;
    for (i=0 ; i < xim_support_locales.nlocale ; i++)
    {
        if (strcasecmp(call_data->lang.name, xim_support_locales.locale[i])==0)
        {
	    im_enc[call_data->connect_id] = i;
            return True;
        }
    }

    return True;
}

static int 
xim_close_handler(XIMS ims, IMCloseStruct *call_data)
{
    DebugLog(2, ("XIM_CLOSE\n"));

    if (! (xccore->oxim_mode & OXIM_RUN_EXIT)) {
	ic_clean_all(call_data->connect_id, xccore);
    }
    return True;
}

static int
xim_create_ic_handler(XIMS ims, IMChangeICStruct *call_data, int *icid)
{
    int ret;

    if ((xccore->oxim_mode & OXIM_RUN_EXIT))
	return True;
    ret = ic_create(ims, call_data, xccore);
    *icid = (ret == True) ? call_data->icid : -1;
    DebugLog(2, ("XIM_CREATE_IC: icid=%d\n", *icid));
    return ret;
}

static int
xim_destroy_ic_handler(XIMS ims, IMDestroyICStruct *call_data, int *icid)
{
    *icid = call_data->icid;
    DebugLog(2, ("XIM_DESTORY_IC: icid=%d\n", *icid));

    if (! (xccore->oxim_mode & OXIM_RUN_EXIT))
	ic_destroy(*icid, xccore);
    return True;
}

static int 
xim_set_focus_handler(XIMS ims, IMChangeFocusStruct *call_data, int *icid) 
{
    IC *ic;

    *icid = call_data->icid;
    DebugLog(2, ("XIM_SET_IC_FOCUS: icid=%d\n", *icid));

    if ((xccore->oxim_mode & OXIM_RUN_EXIT))
	return True;
    if (! (ic = ic_find(call_data->icid)))
	return False;
/*    if(xccore->virtual_keyboard && !gui_keyboard_actived())
    {
       gui_show_keyboard();
    }*/

    if ((ic->ic_state & IC_FOCUS))
	return True;
/*
 *  This is special handler for OXIM_SINGLE_IMC mode. In this mode, all ICs
 *  share a single IMC. When the ICs change their focus status, it will have
 *  racing condition: for example: IC A focus out, and IC B focus in. It may
 *  by that IC B receive the focus-in event first, and then IC A receive the
 *  focus-out. This will lead to the un-correct ic->ic_state value. So here
 *  we only allow the focus-in IC to set its id to IMC, and IMC's focus state
 *  will change only when its icid is the same as the id of the current IC.
 *
 *  But for OXIM_SINGLE_IMC mode off, there will not have this problem. But
 *  this technique is still valid, since each IMC in each IC will have the
 *  same icid value as their corresponding IC's id.
 */
    ic->imc->icid = ic->id;
    if ((ic->imc->inp_state & IM_CINPUT)) {
	ic->imc->inp_state |= IM_XIMFOCUS;
    }
    if ((ic->imc->inp_state & IM_2BYTES)) {
	ic->imc->inp_state |= IM_2BFOCUS;
    }
    ic->ic_state |= IC_FOCUS;
    xccore->ic = ic;

    if ((ic->ic_state & IC_NEWIC)) {
	if ((xccore->oxim_mode & OXIM_RUN_IM_FOCUS) ||
	    (xccore->oxim_mode & OXIM_RUN_2B_FOCUS)) {
	    if (xim_connect(ic) == True) {
		if ((xccore->oxim_mode & OXIM_RUN_IM_FOCUS))
		    change_IM(ic, xccore->im_focus);
		if ((xccore->oxim_mode & OXIM_RUN_2B_FOCUS))
		    ic->imc->inp_state |= (IM_2BYTES | IM_2BFOCUS);
	    }
	}
        ic->ic_state &= ~(IC_NEWIC);
    }
    if ((xccore->oxim_mode & OXIM_SINGLE_IMC)) {
	if ((ic->imc->inp_state & IM_CINPUT) ||
	    (ic->imc->inp_state & IM_2BYTES))
	{
	    xim_connect(ic);
	}
	else
	    xim_disconnect(ic);
	ic->imc->ic_rec = &(ic->ic_rec);
    }

    int enc_id = im_enc[call_data->connect_id];
    setlocale(LC_ALL, xim_support_locales.locale[enc_id]);

    return True;
}

static int 
xim_unset_focus_handler(XIMS ims, IMChangeFocusStruct *call_data, int *icid)
{
    IC *ic;

    *icid = call_data->icid;
    DebugLog(2, ("XIM_UNSET_IC_FOCUS: icid=%d\n", *icid));

    if ((xccore->oxim_mode & OXIM_RUN_EXIT))
	return True;
    if (! (ic = ic_find(call_data->icid)))
	return False;
    //if(xccore->virtual_keyboard)
    {
       //if (ic_find_focus() == call_data->icid/* && gui_keyboard_actived()*/)
       {
           //gui_hide_keyboard();
       }
    }
    if (! (ic->ic_state & IC_FOCUS))
	return True;
    xccore->icp = ic;
    ic->ic_state &= ~(IC_FOCUS);

    setlocale(LC_CTYPE, xccore->lc_ctype);
//    setlocale(LC_ALL, xccore->lc_ctype);

    return True;
}

static int 
xim_trigger_handler(XIMS ims, IMTriggerNotifyStruct *call_data, int *icid)
{
    int major_code, minor_code;
    char *str;
    IC *ic;

    gui_hide_msgbox();
    
    *icid = call_data->icid;
    DebugLog(2, ("XIM_TRIGGER_NOTIFY: icid=%d\n", *icid));

    if ((xccore->oxim_mode & OXIM_RUN_EXIT))
	return True;
    if (! (ic = ic_find(call_data->icid)))
	return False;

    xccore->icp = xccore->ic;
    xccore->ic = ic;
    xccore->ic->ic_state |= IC_FOCUS;
    ic->imc->icid = ic->id;
    if (call_data->flag == 0) { 		/* on key */
        /* 
	 *  Here, the start of preediting is notified from
         *  IMlibrary, which is the only way to start preediting
         *  in case of Dynamic Event Flow, because ON key is
         *  mandatary for Dynamic Event Flow. 
	 */
	ic->ic_state |= IC_CONNECT;

	get_trigger_key(call_data->key_index/3, &major_code, &minor_code);
	switch (major_code) {
	case FKEY_ZHEN:				/* ctrl+space: default IM */
	    if (change_IM(ic, ic->imc->inp_num) == False)
	        xim_disconnect(ic);
	    break;
	case FKEY_2BSB:				/* shift+space: sb/2b */
	    ic->imc->inp_state |= (IM_2BYTES | IM_2BFOCUS);
	    if ((xccore->oxim_mode & OXIM_IM_FOCUS))
	        xccore->oxim_mode |= OXIM_RUN_2B_FOCUS;
	    break;
	case FKEY_CIRIM:			/* ctrl+shift: circuler + */
	    circular_change_IM(ic, 1);
	    if (! (ic->imc->inp_state & IM_CINPUT))
		xim_disconnect(ic);
	    /*if(gui_keyboard_actived())
	    {
		gui_hide_keyboard();
		gui_show_keyboard();
	    }*/
	    break;
	case FKEY_CIRRIM:			/* shift+ctrl: circuler - */
	    circular_change_IM(ic, -1);
	    if (! (ic->imc->inp_state & IM_CINPUT))
		xim_disconnect(ic);
	    break;
	case FKEY_IMN:				/* ctrl+alt+?: select IM */
	    if (change_IM(ic, oxim_get_IMIdxByKey(minor_code)) == False)
	        xim_disconnect(ic);
	    break;
	case FKEY_QPHRASE:
	    if ((str = oxim_qphrase_str(minor_code)))
	    {
		xim_commit_string(ic, str);
	    }
	    xim_disconnect(ic);
	    break;
	case FKEY_SYMBOL:
	    gui_switch_symbol();
	    break;
	case FKEY_KEYBOARD:
 	    gui_switch_keyboard();
	    //xim_disconnect(ic);
	    break;
	case FKEY_SKEYBOARD:
	    gui_show_keyboard();
	    //xim_disconnect(ic);
	    break;
	case FKEY_TEGAKI:
	    RunTegaki(0);
	    xim_disconnect(ic);
	    break;
	case FKEY_FILTER:
	    {
		IM_Context_t *imc;
		imc = ic->imc;

		change_filter(1, True, &ic->filter_current);
		if (!(imc->inp_state & IM_FILTER))
		{
		    imc->inp_state &= 0x0f;
		    imc->inp_state |= IM_FILTER;
		    
		    if(is_filter_default(&ic->filter_current))
			change_filter(1, False, &ic->filter_current);
		}
	    }
	    break;
	}
        return True;
    } 
    else 					/* never happens */
        return False;
}

#ifndef AltGrMask
#define AltGrMask (1L<<13)
#endif

static int
forward_keys_handler(IC *ic, keyinfo_t *keyinfo)
{
    unsigned int ret;
    int major_code, minor_code;
    IM_Context_t *imc;

    imc = ic->imc;
/*
 *  Keycode translation for different layout of keyboards.
 *
 *  In most places of the world, we use the "qwerty" layout of the keyboard.
 *  But in other places, e.g., France, they use the "azerty" layout of the
 *  keyboard. There are many differences between them. For example, the
 *  number keys KP_1 .... KP_9 for "qwerty" keyboard do not have the Shift
 *  ON, but for "azerty" they do. So we have to do the keyboard translation
 *  here, if necessary. The translation is just from other layout to the
 *  "standard" "qwerty" layout.
 *
 *  This translation code is provided by dupre <dupre@lifo.univ-orleans.fr>.
 *  He commented that to do the correct translation, we have to turn off
 *  the non-necessary Shift mask for some keys, and turn on Shift mask for
 *  the others. Sometimes the keycode of the "azerty" keyboard will come
 *  with the "AltGr" mask on. It should always be turned off. Note that the
 *  translation is performed only when Shift and AltGr are NOT ON at the
 *  same moment.
 */
    if(gui_selectmenu_actived())
    {
	char *pattern = "!@#$%^&*()";
	int i;
	for(i=0; pattern[i]; i++)
	{
	    if((int)pattern[i]==keyinfo->keysym)
	    {
		select_menu_selected(ic, keyinfo->keysym);
		return 1;
	    }
	}
	if((keyinfo->keysym == XK_Page_Up))
	{
	    select_menu_selectpage(ic, -1);
	    return 1;
	}
	if((keyinfo->keysym == XK_Page_Down))
	{
	    select_menu_selectpage(ic, 1);
	    return 1;
	}

	if( (keyinfo->keysym != XK_Shift_R) && (keyinfo->keysym != XK_Shift_L) &&
	    (keyinfo->keysym != XK_Page_Up) && (keyinfo->keysym != XK_Page_Down) )
	{
	    gui_hide_selectmenu();
	}
    }
    /*
    檢查是否按下 Shift-Enter鍵,
    若有就傳送 \n 給 xim
    */
    if ( keyinfo->keysym == XK_Return && keyinfo->keystate & ShiftMask )
    {
	xim_commit_string(ic, "\n");
	return 1;
    }

    if ((xccore->oxim_mode & OXIM_KEYBOARD_TRANS)) {
	if (! (keyinfo->keystate & ShiftMask &&
	       keyinfo->keystate & AltGrMask) &&
	    keyinfo->keysym > 32 && keyinfo->keysym < 127 &&
	    keyinfo->keystr_len) {
	    if (strstr("',-./0123456789;=[\\]`abcdefghijklmnopqrstuvwxyz",
			keyinfo->keystr) != NULL) {
		keyinfo->keystate &= ~ShiftMask;
		keyinfo->keystate &= ~AltGrMask;
	    }
	    else {
		keyinfo->keystate |=  ShiftMask;
		keyinfo->keystate &= ~AltGrMask;
	    }
	}
    }
/*	    if(gui_selectmenu_actived())
		if (! (keyinfo->keystate & ShiftMask))
		gui_hide_selectmenu();*/
/*
 *  Process the special key binding.
 */
    if (search_funckey(keyinfo->keysym, keyinfo->keystate,
		&major_code, &minor_code)) {
	char *str;
	switch (major_code) {
	case FKEY_ZHEN:
	    if(gui_selectmenu_actived())
		gui_hide_selectmenu();
            if ((imc->inp_state & IM_CINPUT))
                change_IM(ic, -1);
            else
                change_IM(ic, imc->inp_num);
	    break;
	case FKEY_SYMBOL:
	    gui_switch_symbol();
	    break;
	case FKEY_KEYBOARD:
 	    gui_switch_keyboard();
	    break;
	case FKEY_SKEYBOARD:
	    gui_show_keyboard();
	    break;
	case FKEY_TEGAKI:
	    RunTegaki(0);
// 	    xim_disconnect(ic);
	    break;
#if 0
	case FKEY_OUTSIMP:
	    if (!strcasecmp("UTF-8", nl_langinfo(CODESET)))
	    {
		if (!(imc->inp_state & IM_OUTSIMP))
		{
		    imc->inp_state &= 0x0f;
		    imc->inp_state |= IM_OUTSIMP;
		}
		else
		    imc->inp_state &= 0x0f;
	    }
	    break;

	case FKEY_OUTTRAD:
	    if (!strcasecmp("UTF-8", nl_langinfo(CODESET)))
	    {
		if (!(imc->inp_state & IM_OUTTRAD))
		{
		    imc->inp_state &= 0x0f;
		    imc->inp_state |= IM_OUTTRAD;
		}
		else
		    imc->inp_state &= 0x0f;
	    }
	    break;
#endif
	case FKEY_FILTER:
	    {
		change_filter(1, True, &ic->filter_current);
		if (!(imc->inp_state & IM_FILTER))
		{
		    imc->inp_state &= 0x0f;
		    imc->inp_state |= IM_FILTER;
		    
		    if(is_filter_default(&ic->filter_current))
			change_filter(1, False, &ic->filter_current);
		}
	    }
	    break;
	case FKEY_2BSB:
            if ((imc->inp_state & IM_2BYTES)) {
                imc->inp_state &= ~(IM_2BYTES | IM_2BFOCUS);
	        if ((xccore->oxim_mode & OXIM_IM_FOCUS))
	            xccore->oxim_mode &= ~OXIM_RUN_2B_FOCUS;
	    }
            else {
                imc->inp_state |= (IM_2BYTES | IM_2BFOCUS);
	        if ((xccore->oxim_mode & OXIM_IM_FOCUS))
	            xccore->oxim_mode |= OXIM_RUN_2B_FOCUS;
	    }
	    break;
	case FKEY_CIRIM:
            circular_change_IM(ic, 1);
	    /*if(gui_keyboard_actived())
	    {
		gui_hide_keyboard();
		gui_show_keyboard();
	    }*/
	    break;
	case FKEY_CIRRIM:
            circular_change_IM(ic, -1);
	    break;
	case FKEY_IMN:
            change_IM(ic, oxim_get_IMIdxByKey(minor_code));
	    break;
	case FKEY_CHREP:
	    xim_commit_string(ic, NULL);
	    break;
	case FKEY_SIMD: /* 查字根 */
            change_simd(ic);
	    break;
	case FKEY_IMFOCUS:
	    if ((xccore->oxim_mode & OXIM_IM_FOCUS)) {
		xccore->oxim_mode &= ~OXIM_IM_FOCUS;
		xccore->oxim_mode &= ~OXIM_RUN_IM_FOCUS;
		xccore->oxim_mode &= ~OXIM_RUN_2B_FOCUS;
	    }
	    else {
		xccore->oxim_mode |= OXIM_IM_FOCUS;
		if ((imc->inp_state & IM_CINPUT))
		    xccore->oxim_mode |= OXIM_RUN_IM_FOCUS;
		else
		    xccore->oxim_mode |= OXIM_RUN_2B_FOCUS;
		xccore->im_focus = imc->inp_num;
	    }
	    break;
	case FKEY_QPHRASE:
	    if ((str = oxim_qphrase_str(minor_code)))
	    {
                xim_commit_string(ic, str);
	    }
	    break;
	case FKEY_XCINSW:
	    gui_xcin_style_switch();
	    break;
	}
	if (! (imc->inp_state & IM_CINPUT) && ! (imc->inp_state & IM_2BYTES) && !(imc->inp_state & IM_OUTSIMP) && !(imc->inp_state & IM_OUTTRAD))
	    return 0;
	return 1;
    }

/*
 *  Forward key to the IM module.
 */
    if ((imc->inp_state & IM_CINPUT)) {
	imc->inpinfo.zh_ascii = ((imc->inp_state & IM_2BYTES)) ?
		(ubyte_t)1 : (ubyte_t)0;
        ret = imc->imodp->keystroke(imc->imodp->conf,&(imc->inpinfo), keyinfo);
	if (process_IMKEY(ic, keyinfo, ret) == True)
	    return 1;
    }
    if ((imc->inp_state & IM_2BYTES)) {
	char *cch;
	if ((keyinfo->keystate & Mod1Mask) != Mod1Mask &&
	    (keyinfo->keystate & ControlMask) != ControlMask &&
            (cch = fullchar_keystroke(&(imc->inpinfo), keyinfo->keysym, (imc->inp_state & IM_CINPUT)))) {
	    xim_commit_string(ic, cch);
	    return 1;
	}
    }

//     oxim_perr(OXIMMSG_EMPTY, ("==>%d====%d====\n"), keyinfo->keysym, XK_Return);
    /*
     * 變態設計 :-P
     *
     * Forward 程序執行到這裡，表示這個按鍵沒有被處理，準備轉送出去，
     * 但因為有些別的程序不知為何，又把這個按鍵 forward 回來(如 gedit)，
     * 形成互相 forward，無止無盡 :-(
     *
     * 只好在這裡加以處理，作法是檢視上一次 forward 與這一次的時間差
     * 精度為百萬分之一秒(沒有人可以打字那麼快吧!)，如果在一定的時間
     * 之內(1% 秒)，就視為重複 forward，此時就吃掉這個按鍵，不再 forward。
     */
    static unsigned int last_time = 0;
    if (keyinfo->keystr_len == 0)
    {
	struct timeval tp;
	unsigned int now_time, time_use;
	if (gettimeofday(&tp, NULL) == 0)
	{
	    now_time = (1000000 * tp.tv_sec) + tp.tv_usec;
	    time_use = now_time - last_time;
	    last_time = now_time;
	    if (time_use < 3000) /* 0.3% 秒 */
	    {
		return 1;
	    }
	}
    }

    return -1;
}

static int
xim_forward_handler(XIMS ims, IMForwardEventStruct *call_data, int *icid)
{
    IC *ic=NULL;
    int ret;
    XKeyEvent *kev;
    keyinfo_t keyinfo;

    *icid = call_data->icid;
    DebugLog(2, ("XIM_FORWARD_EVENT: icid=%d\n", *icid));

    if ((xccore->oxim_mode & OXIM_RUN_EXIT))
    {
	return True;
    }

    if (! (ic = ic_find(call_data->icid)))
    {
	return False;
    }
    kev = &call_data->event.xkey;
//printf("type=%d", kev->type);puts("");
    keyinfo.keytype = kev->type;
    keyinfo.keystate = kev->state;
    keyinfo.keystr_len = XLookupString(kev, keyinfo.keystr, 16, 
				&(keyinfo.keysym), NULL);
    keyinfo.keystr[(keyinfo.keystr_len >= 0) ? keyinfo.keystr_len : 0] = 0;
    //printf("Keysym=%x\n", keyinfo.keysym);

    if (keyinfo.keytype != KeyPress)
    {
	return True;
    }

    ret = forward_keys_handler(ic, &keyinfo);
    if (ret == 0)
    {
	xim_disconnect(ic);
    }
    else if (ret == -1)
    {
	IMForwardEvent(ims, (XPointer)call_data);
    }

    return True;
}

static int
xim_set_ic_values_handler(XIMS ims, IMChangeICStruct *call_data, int *icid)
{
    IC *ic;

    *icid = call_data->icid;
    DebugLog(2, ("XIM_SET_IC_VALUES: icid=%d\n", *icid));

    if ((xccore->oxim_mode & OXIM_RUN_EXIT))
	return True;

    if (! (ic = ic_find(call_data->icid)))
	return False;
    ic_set_values(ic, call_data, xccore);
    return True;
}

static int
xim_get_ic_values_handler(XIMS ims, IMChangeICStruct *call_data, int *icid)
{
    IC *ic;

    *icid = call_data->icid;
    DebugLog(2, ("XIM_GET_IC_VALUES: icid=%d\n", *icid));

    if ((xccore->oxim_mode & OXIM_RUN_EXIT))
	return True;

    if (! (ic = ic_find(call_data->icid)))
	return False;
    ic_get_values(ic, call_data, xccore);
    return True;
}

#define MAX_XIM_SYNC_CHECK	5

static int
xim_sync_reply_handler(XIMS ims, IMSyncXlibStruct *call_data, int *icid)
{
    static int n_check=0;

    *icid = call_data->icid;
/*
 *  This is the main controlling process to terminate oxim, which is a really
 *  involved and complicated process.
 *
 *  There are 2 ways to terminate oxim. 1. Use "kill" command in the shell.
 *  2. Click the "kill" button of the oxim window. In the OverTheSpot style,
 *  every action of the XIM client it will use XSetICValues() to send its
 *  spot location to oxim, and oxim *must* reply it. So when the termination
 *  occures, oxim have to ensure that it has replied all the XSetICValues()
 *  request to the current IC on focus, and then actually exits.
 *
 *  So, when terminating, oxim will send XIM_SYNC to the on-focus IC and wait
 *  for its response. This is the dummy event which is just giving the time
 *  for the XIM client to send its XSetICValues() request to oxim in time
 *  such that oxim can reply it. However, oxim cannot assume that the client
 *  will always send the XSetICValues() request everytime when it is going to
 *  exit or how many requests the client will send. So, we use the method
 *  that oxim tries to send several XIM_SYNC one by one, and expect a centain
 *  number of response from the client. In the amount of the response, the
 *  client may send its XSetICValues() request *in time* (if it needs) such
 *  that oxim can response to it.
 *
 *  For case 1 of termination, note that the "kill" signal is received by the
 *  sighandler() of the main program, but not in the X event loop. In this
 *  case we cannot send the XIM_SYNC in the sighandler(), because outside the
 *  X event loop the XIM_SYNC request will be queued somewhere and cannot be
 *  processed immediately. Hence in sighandler() XIM_SYNC we only set the
 *  special flag OXIM_RUN_KILL to inform oxim this circumstance. When backing
 *  the X event loop, oxim will check for this flag and send the XIM_SYNC
 *  request to the client.
 *
 *  For case 2 since it is already in the X event loop, so it can send the
 *  XIM_SYNC request directly (in gui_main*() when the oxim main window is
 *  being killed).
 *
 *  All the XIM_SYNC request is sent by xim_close() function. When it is
 *  called, it will check if the client window is really exist, and set the
 *  special flag OXIM_RUN_EXIT to denote the situation that oxim is exiting.
 *  Then in the xim_sync_reply_handler() it will decide when the oxim can
 *  actually exit, determined by the number of the XIM_SYNC reply. If oxim
 *  can actually exit, the flag OXIM_RUN_EXITALL is set.
 */
    if ((xccore->oxim_mode & OXIM_RUN_EXIT)) {
	n_check ++;
	DebugLog(2, ("XIM_SYNC_REPLY: icid=%d, n_check=%d\n", *icid, n_check));
	if (n_check < MAX_XIM_SYNC_CHECK) {
	    IC *ic = ic_find(call_data->icid);
	    xim_close(ic);
	}
	else
	    xccore->oxim_mode |= OXIM_RUN_EXITALL;
    }
    return True;
}


static int 
im_protocol_handler(XIMS ims, IMProtocol *call_data)
{
    int ret, icid=-1;
    
    switch (call_data->major_code) {
    case XIM_OPEN:      
        ret = xim_open_handler(ims, &(call_data->imopen));
	break;
    case XIM_CLOSE:     
        ret = xim_close_handler(ims, &(call_data->imclose));
	break;
    case XIM_CREATE_IC: 
        ret = xim_create_ic_handler(ims, &(call_data->changeic), &icid);
	break;
    case XIM_DESTROY_IC:
        ret = xim_destroy_ic_handler(ims, &(call_data->destroyic), &icid);
	break;
    case XIM_SET_IC_FOCUS: 
        ret = xim_set_focus_handler(ims, &(call_data->changefocus), &icid);
	break;
    case XIM_UNSET_IC_FOCUS: 
        ret = xim_unset_focus_handler(ims, &(call_data->changefocus), &icid);
	break;
    case XIM_TRIGGER_NOTIFY:
        ret = xim_trigger_handler(ims, &(call_data->triggernotify), &icid);
	break;
    case XIM_FORWARD_EVENT: 
        ret = xim_forward_handler(ims, &(call_data->forwardevent), &icid);
	break;
    case XIM_SET_IC_VALUES: 
        ret = xim_set_ic_values_handler(ims, &(call_data->changeic), &icid);
	break;
    case XIM_GET_IC_VALUES: 
        ret = xim_get_ic_values_handler(ims, &(call_data->changeic), &icid);
	break;
    case XIM_SYNC_REPLY:
	ret = xim_sync_reply_handler(ims, &(call_data->sync_xlib), &icid);
	break;
    case XIM_RESET_IC:
        DebugLog(2, ("XIM_RESET_IC\n"));
	ret = True;
	break;
/*
 *  Not implement yet.
 */
    case XIM_PREEDIT_START_REPLY:
        DebugLog(2, ("XIM_PREEDIT_START_REPLY\n"));
	ret = True;
	break;
    case XIM_PREEDIT_CARET_REPLY:
        DebugLog(2, ("XIM_PREEDIT_CARET_REPLY\n"));
	ret = True;
	break;
    default:
	DebugLog(2, ("XIM Unknown\n"));
	ret = False;
	break;
    }
    if (xccore->oxim_mode & OXIM_RUN_KILL) {
	xccore->oxim_mode &= ~OXIM_RUN_KILL;
	xim_close(xccore->ic);
    }
    gui_update_winlist();
    return ret;
}


/*----------------------------------------------------------------------------

	XIM Initialization.

----------------------------------------------------------------------------*/

#define XIM_TCP_PORT 9010

enum trans_type {
    TRANSPORT_X, 
    TRANSPORT_LOCAL, 
    TRANSPORT_TCP
};

/* Supported Encodings */
static XIMEncoding zhEncodings[] = {
    "COMPOUND_TEXT",
    "UTF8_STRING",
    NULL
};

static XIMStyle defaultStyles[] =
{
    XIMPreeditPosition|XIMStatusNothing,    /* OverTheSpot */
    XIMPreeditPosition|XIMStatusArea,    /* OverTheSpot */
    XIMPreeditPosition|XIMStatusNone,    /* OverTheSpot */
    XIMPreeditPosition|XIMStatusCallbacks,    /* OverTheSpot */

    XIMPreeditCallbacks|XIMStatusNothing,   /* OnTheSpotQT */
    XIMPreeditCallbacks|XIMStatusArea,
    XIMPreeditCallbacks|XIMStatusNone,
    XIMPreeditCallbacks|XIMStatusCallbacks, /* OnTheSpot */

    XIMPreeditNothing|XIMStatusNothing,     /* Root */
    XIMPreeditNothing|XIMStatusArea,
    XIMPreeditNothing|XIMStatusNone,
    XIMPreeditNothing|XIMStatusCallbacks,

    XIMPreeditArea|XIMStatusArea,	    /* OffTheSpot */
    XIMPreeditArea|XIMStatusNothing,
    XIMPreeditArea|XIMStatusNone,
    XIMPreeditArea|XIMStatusCallbacks
};

void 
xim_init(xccore_t *core)
{
    char transport[128], xim_name[128];
    unsigned int transport_type;
    XIMTriggerKeys on_keys;
    XIMEncodings encodings;
    int i;

    transport_type = TRANSPORT_X;
    xccore = core;
/*
    if (transport_type == TRANSPORT_LOCAL) {
        char hostname[64];
        char *address = "/tmp/.ximsock";
        gethostname(hostname, 64);
        sprintf(transport, "local/%s:%s", hostname, address);
    } 
    else if (transport_type == TRANSPORT_TCP) {
        char hostname[64];
        int port_number = XIM_TCP_PORT;
        gethostname(hostname, 64);
        sprintf(transport, "tcp/%s:%d", hostname, port_number);
    } 
    else 
*/
    strcpy(transport, "X/");
    check_funckey();
    xccore->input_styles.count_styles = (sizeof(defaultStyles) / sizeof(XIMStyle));
    xccore->input_styles.supported_styles = defaultStyles;

    encodings.count_encodings = sizeof(zhEncodings)/sizeof(XIMEncoding) - 1;
    encodings.supported_encodings = zhEncodings;
    make_trigger_keys(&on_keys);

    if (xccore->xim_name[0] == '\0') {
	strncpy(xim_name, "oxim", sizeof(xim_name));
    }
    else
	strncpy(xim_name, xccore->xim_name, sizeof(xim_name));

    /*------------------------*/
    load_support_locales();
    char *support_locales = oxim_malloc((xim_support_locales.nlocale * 2) + xim_support_locales.nbytes, True);
    for (i=0; i < xim_support_locales.nlocale; i++)
    {
	if (i)
	{
	    strcat(support_locales, ",");
	}
	strcat(support_locales, xim_support_locales.locale[i]);
    }
    /*-----------------------------*/

    ims = IMOpenIM(xccore->display,
                   IMServerWindow,    xccore->window,
                   IMModifiers,       "Xi18n",
                   IMServerName,      xim_name,
                   IMLocale,          (support_locales ? support_locales : xccore->lc_ctype),
                   IMServerTransport, transport,
                   IMInputStyles,     &(xccore->input_styles),
                   IMEncodingList,    &encodings,
                   IMProtocolHandler, im_protocol_handler,
                   IMFilterEventMask, KeyPressMask | KeyReleaseMask,
                   NULL);

    if (support_locales)
	free(support_locales);

    if (ims == 0)
	oxim_perr(OXIMMSG_ERROR,
		N_("IMOpenIM() with name \"%s\" transport \"%s\" failed.\n"),
		xim_name, transport);
    else {
	xccore->oxim_mode |= OXIM_RUN_INIT;
	xccore->oxim_mode |= OXIM_SHOW_CINPUT;
	oxim_perr(OXIMMSG_NORMAL,
		N_("XIM server \"%s\" transport \"%s\"\n"),
		xim_name, transport);
	oxim_perr(OXIMMSG_EMPTY, "\n");
	xim_set_trigger_keys();
    }
}

void xim_set_trigger_keys(void)
{
    XIMTriggerKeys on_keys;
    make_trigger_keys(&on_keys);
    IMSetIMValues(ims, IMOnKeysList, &on_keys, NULL);
}

void
xim_close(IC *ic)
{
    IMSyncXlibStruct pass_data;

    DebugLog(2, ("xim_close\n"));
    xccore->oxim_mode |= (OXIM_RUN_EXIT | OXIM_ICCHECK_OFF);
    if (ic && gui_check_window(ic->ic_rec.client_win) == True) {
	DebugLog(2, ("xim_close sent\n"));
	pass_data.major_code = XIM_SYNC;
	pass_data.minor_code = 0;
	pass_data.connect_id = ic->connect_id;
	pass_data.icid = ic->id;
	IMSyncXlib(ims, (XPointer)&pass_data);
	XSync(xccore->display, False);
    }
    else
	xccore->oxim_mode |= OXIM_RUN_EXITALL;
}

void
xim_terminate(void)
{
    if (ims)
	IMCloseIM(ims);
    if (xccore)
	XSync(xccore->display, False);
}
