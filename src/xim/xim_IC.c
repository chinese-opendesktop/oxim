/******************************************************************
 
  Copyright 1994, 1995 by Sun Microsystems, Inc.
  Copyright 1993, 1994 by Hewlett-Packard Company
 
Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of Sun Microsystems, Inc.
and Hewlett-Packard not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.
Sun Microsystems, Inc. and Hewlett-Packard make no representations about
the suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.
 
SUN MICROSYSTEMS INC. AND HEWLETT-PACKARD COMPANY DISCLAIMS ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
SUN MICROSYSTEMS, INC. AND HEWLETT-PACKARD COMPANY BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 
  Author: Hidetoshi Tajima(tajima@Eng.Sun.COM) Sun Microsystems, Inc.
 
******************************************************************/
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <X11/Xlib.h>
#include "IMdkit.h"
#include "Xi18n.h"
#include "oximtool.h"
#include "oxim.h"

/*----------------------------------------------------------------------------

	Basic IMC handling functions.

----------------------------------------------------------------------------*/

static IM_Context_t *imc_list = (IM_Context_t *)NULL;
static IM_Context_t *imc_free = (IM_Context_t *)NULL;

static IM_Context_t *
new_IMC(int icid, int single_imc)
{
    IM_Context_t *imc;
    int create=0;

    if (! single_imc) {
	if (imc_free != NULL) {
            imc = imc_free;
            imc_free = imc_free->next;
	    bzero(imc, sizeof(IM_Context_t));
	}
	else
	    imc = (IM_Context_t *)oxim_malloc(sizeof(IM_Context_t), 1);
	if (imc_list)
	    imc_list->prev = imc;
	imc->next = imc_list;
	imc_list  = imc;
	create = 1;
    }
    else {
	if (! imc_list) {
	    imc = (IM_Context_t *)oxim_malloc(sizeof(IM_Context_t), 1);
	    imc_list = imc;
	    create = 1;
	}
	else
	    imc = imc_list;
    }

    if (create) {
	imc->id = icid;
	imc->sinmd_keystroke = oxim_malloc(10*sizeof(uch_t), 1);
	imc->skey_size = 10;
	imc->cch = oxim_malloc((UCH_SIZE+1)*sizeof(char), 1);
	imc->cch_size = UCH_SIZE + 1;
    }
    imc->icid = icid;
    return imc;
}

static void
delete_IMC(IM_Context_t *imc)
{
    IM_Context_t *prev, *next;

    prev = imc->prev;
    next = imc->next;

    if (prev != NULL)
	prev->next = next;
    else
	imc_list = next;
    if (next != NULL)
	next->prev = prev;

    imc->next = imc_free;
    imc_free  = imc;
    if (imc->cch)
	free(imc->cch);
    if (imc->sinmd_keystroke)
	free(imc->sinmd_keystroke);
}

IM_Context_t *
imc_find(int imid)
{
    IM_Context_t *imc = imc_list;

    while (imc) {
	if (imc->id == imid)
	    return imc;
	imc = imc->next;
    }
    return NULL;
}

void imc_reset(void)
{
    IM_Context_t *imc = imc_list;
    inp_state_t inp_num = oxim_get_Default_IM();
    while (imc)
    {
	imc->inp_state &= ~(IM_2BYTES|IM_2BFOCUS|IM_CINPUT);
	imc->inp_num = imc->sinp_num = inp_num;
	imc->imodp = imc->s_imodp = NULL;
	imc = imc->next;
    }
}

/*----------------------------------------------------------------------------

	New IC, Delete IC, and Find IC functions.

----------------------------------------------------------------------------*/

static IC *ic_list = (IC *)NULL;
static IC *ic_free = (IC *)NULL;

static IC *
new_IC(int single_imc)
{
    static CARD16 icid = 0;
    CARD16 new_icid;
    IC *rec;

    if (ic_free != NULL) {
        rec = ic_free;
        ic_free = ic_free->next;
	new_icid = rec->id;
	bzero(rec, sizeof(IC));
    } 
    else {
        rec = (IC *)oxim_malloc(sizeof(IC), 1);
	icid ++;
	new_icid = icid;
    }
    rec->id  = new_icid;
    rec->imc = new_IMC(new_icid, single_imc);

    rec->next = ic_list;
    ic_list   = rec;
    return rec;
}

static void 
delete_IC(IC *ic, IC *last, xccore_t *xccore)
{
    int clear = ((xccore->oxim_mode & OXIM_SINGLE_IMC)) ? 0 : 1;
    ic_rec_t *ic_rec = &(ic->ic_rec);
/* 
 *  The IC is eventually being deleted, so don't process any IMKEY send back.
 */
    call_xim_end(ic, 1, clear);

    if (last != NULL)
	last->next = ic->next;
    else
	ic_list = ic->next;
    ic->next = ic_free;
    ic_free = ic;
    if (xccore->ic == ic)
	xccore->ic = NULL;
    if (xccore->icp == ic)
	xccore->icp = NULL;

#ifdef XIM_COMPLETE
    if (ic_rec->resource_name)
	free(ic_rec->resource_name);
    if (ic_rec->resource_class)
	free(ic_rec->resource_class);
#endif

    if (clear)
	delete_IMC(ic->imc);
}

IC *
ic_find(CARD16 icid)
{
    IC *ic = ic_list;

    while (ic != NULL) {
        if (ic->id == icid)
            return ic;
        ic = ic->next;
    }
    return NULL;
}

int
ic_find_focus()
{
    IC *ic = ic_list;
    while (ic != NULL)
    {
       if (ic->ic_state & IC_FOCUS)
           return ic->id;
       ic = ic->next;
    }
    return -1;
}

void disconnect_all_ic(void)
{
    IC *ic = ic_list;
    while (ic)
    {
	xim_disconnect(ic);
	ic = ic->next;
    }
}

void ic_set_location(Window w, int x, int y)
{
    IC *ic = ic_list;

    while (ic != NULL) {
        if ((unsigned int)ic->ic_rec.client_win == w || (unsigned int)ic->ic_rec.focus_win == w)
	{
	    ic->ic_rec.pre_attr.spot_location.x = x;
	    ic->ic_rec.pre_attr.spot_location.y = y;
	    ic->has_set_spot = True;
	    return;
	}
        ic = ic->next;
    }
}

/*----------------------------------------------------------------------------

	ic_get_values(): Get IC values from oxim.

----------------------------------------------------------------------------*/
static match(char *attr, XICAttribute *attr_list)
{
    return (strcmp(attr, attr_list->name) == 0) ? True : False;
}

void
ic_get_values(IC *ic, IMChangeICStruct *call_data, xccore_t *xccore)
{
    XICAttribute *ic_attr  = call_data->ic_attr;
    XICAttribute *pre_attr = call_data->preedit_attr;
#ifdef XIM_COMPLETE
    XICAttribute *sts_attr = call_data->status_attr;
#endif
    ic_rec_t	 *ic_rec   = &(ic->ic_rec);
    register int  i;

    for (i=0; i < (int)call_data->ic_attr_num; i++, ic_attr++) {
	if (! ic_attr->name)
	    continue;
	DebugLog(3, ("ic_get: ic_attr: %s\n", ic_attr->name));
        if (match (XNFilterEvents, ic_attr)) {
            ic_attr->value = (void *)oxim_malloc(sizeof(CARD32), 0);
            ic_attr->value_length = sizeof(CARD32);
	    *(CARD32*)ic_attr->value = KeyPressMask;
        }
	else if (match (XNInputStyle, ic_attr)) {
	    ic_attr->value = (void *)oxim_malloc(sizeof(INT32), 0);
	    ic_attr->value_length = sizeof(INT32);
	    *(INT32*)ic_attr->value = ic_rec->input_style;
	}
	else if (match (XNSeparatorofNestedList, ic_attr)) {
	    ic_attr->value = (void *)oxim_malloc(sizeof(CARD16), 0);
	    ic_attr->value_length = sizeof(CARD16);
	    *(CARD16*)ic_attr->value = 0;
	}
	else if (match (XNPreeditState, ic_attr)) {
	    /* some java applications need XNPreeditState attribute in
	    * IC attribute instead of Preedit attributes
	    * so we support XNPreeditState attr here */
	    ic_attr->value = (void *)malloc(sizeof(XIMPreeditState));
	    if ((ic->imc->inp_state & IM_CINPUT) ||
		(ic->imc->inp_state & IM_2BYTES))
		*(XIMPreeditState *)ic_attr->value = XIMPreeditEnable;
	    else
		*(XIMPreeditState *)ic_attr->value = XIMPreeditDisable;
	    ic_attr->value_length = sizeof(XIMPreeditState);
	}
	else {
            oxim_perr(OXIMMSG_WARNING, 
		N_("ic_get: unknown IC attr: %s\n"), ic_attr->name);
	}
    }

    for (i=0; i < (int)call_data->preedit_attr_num; i++, pre_attr++) {
	if (! pre_attr->name)
	    continue;
	DebugLog(3, ("ic_get: pre_attr: %s\n", pre_attr->name));
	if (match (XNArea, pre_attr)) {
	    pre_attr->value = (void *)oxim_malloc(sizeof(XRectangle), 0);
	    *(XRectangle*)pre_attr->value = ic_rec->pre_attr.area;
	    pre_attr->value_length = sizeof(XRectangle);
	}
	else if (match (XNSpotLocation, pre_attr)) {
	    pre_attr->value = (void *)oxim_malloc(sizeof(XPoint), 0);
	    *(XPoint*)pre_attr->value = ic_rec->pre_attr.spot_location;
	    pre_attr->value_length = sizeof(XPoint);
        } 
	else if (match (XNPreeditState, pre_attr)) {
	    pre_attr->value = (void *)oxim_malloc(sizeof(XIMPreeditState), 0);
	    pre_attr->value_length = sizeof(XIMPreeditState);
	    if ((ic->imc->inp_state & IM_CINPUT) ||
		(ic->imc->inp_state & IM_2BYTES))
		*(XIMPreeditState *)pre_attr->value = XIMPreeditEnable;
	    else
		*(XIMPreeditState *)pre_attr->value = XIMPreeditDisable;
	}
	else if (match (XNLineSpace, pre_attr)) {
            pre_attr->value = (void *)oxim_malloc(sizeof(long), 0);
            *(long*)pre_attr->value = ic_rec->pre_attr.line_space;
            pre_attr->value_length = sizeof(long);
        }
	else if (match (XNAreaNeeded, pre_attr)) {
	    pre_attr->value = (void *)oxim_malloc(sizeof(XRectangle), 0);
	    *(XRectangle*)pre_attr->value = ic_rec->pre_attr.area_needed;
	    pre_attr->value_length = sizeof(XRectangle);
        } 
	else {
            oxim_perr(OXIMMSG_WARNING, 
		N_("ic_get: unknown IC pre_attr: %s\n"), pre_attr->name);
	}
    }

#ifdef XIM_COMPLETE
    for (i = 0; i < (int)call_data->status_attr_num; i++, sts_attr++) {
	if (! sts_attr->name)
	    continue;
	DebugLog(3, ("ic_get: sts_attr: %s\n", sts_attr->name));
        if (match (XNArea, sts_attr)) {
            sts_attr->value = (void *)oxim_malloc(sizeof(XRectangle), 0);
            *(XRectangle*)sts_attr->value = ic_rec->sts_attr.area;
            sts_attr->value_length = sizeof(XRectangle);
        } 
	else if (match (XNAreaNeeded, sts_attr)) {
            sts_attr->value = (void *)oxim_malloc(sizeof(XRectangle), 0);
            *(XRectangle*)sts_attr->value = ic_rec->sts_attr.area_needed;
            sts_attr->value_length = sizeof(XRectangle);
        } 
	else if (match (XNForeground, sts_attr)) {
            sts_attr->value = (void *)oxim_malloc(sizeof(long), 0);
            *(long*)sts_attr->value = ic_rec->sts_attr.foreground;
            sts_attr->value_length = sizeof(long);
        } 
	else if (match (XNBackground, sts_attr)) {
            sts_attr->value = (void *)oxim_malloc(sizeof(long), 0);
            *(long*)sts_attr->value = ic_rec->sts_attr.background;
            sts_attr->value_length = sizeof(long);
        } 
	else if (match (XNLineSpace, sts_attr)) {
            sts_attr->value = (void *)oxim_malloc(sizeof(long), 0);
            *(long*)sts_attr->value = ic_rec->sts_attr.line_space;
            sts_attr->value_length = sizeof(long);
        }
	else {
            oxim_perr(OXIMMSG_WARNING, 
		N_("ic_get: unknown IC sts_attr: %s\n"), sts_attr->name);
	}
    }
#endif
    return;
}


/*----------------------------------------------------------------------------

	ic_set_values(): Set IC values into oxim.

	Here we should carefully check if the value is really changed or
	not, and update it in oxim only when it is really changed.

----------------------------------------------------------------------------*/
#define checkset_ic_val(flag, type, attr, val) 				\
    if (ic_rec->ic_value_set & flag) {					\
	if (memcmp(&(val), (attr)->value, sizeof(type)) != 0) {		\
	    val = *(type *)(attr)->value;				\
	    ic_rec->ic_value_update |= flag;				\
	}								\
    }									\
    else {								\
	val = *(type *)(attr)->value;					\
	ic_rec->ic_value_update |= flag;				\
	ic_rec->ic_value_set |= flag;					\
    }

#define checkset_ic_str(flag, attr, str)				\
    if ((ic_rec->ic_value_set & flag) && str) {				\
	if (strcmp(str, (attr)->value) != 0) {				\
	    free(str);							\
	    str = (char *)strdup((attr)->value);			\
	    ic_rec->ic_value_update |= flag;				\
	}								\
    }									\
    else {								\
	str = (char *)strdup((attr)->value);				\
	ic_rec->ic_value_update |= flag;				\
	ic_rec->ic_value_set |= flag;					\
    }

void
ic_set_values(IC *ic, IMChangeICStruct *call_data, xccore_t *xccore)
/*  For details, see Xlib Ref, Chap 11.6  */
{
    XICAttribute *ic_attr  = call_data->ic_attr;
    XICAttribute *pre_attr = call_data->preedit_attr;
#ifdef XIM_COMPLETE
    XICAttribute *sts_attr = call_data->status_attr;
#endif
    ic_rec_t	 *ic_rec   = &(ic->ic_rec);
    register int  i;

    for (i=0; i < (int)(call_data->ic_attr_num); i++, ic_attr++) {
	if (! ic_attr->name && ! ic_attr->value)
	    continue;
	DebugLog(3, ("ic_set: ic_attr: %s\n", ic_attr->name));
        if (match (XNInputStyle, ic_attr)) {
	    int j;
	    checkset_ic_val(CLIENT_SETIC_INPSTY, INT32, ic_attr,
			    ic_rec->input_style);
	    for (j=0; j < xccore->input_styles.count_styles &&
		      ic_rec->input_style != 
			xccore->input_styles.supported_styles[j]; j++);
	    if (j >= xccore->input_styles.count_styles) {
                oxim_perr(OXIMMSG_WARNING, 
		     N_("client input style not enabled(%x), set to \"Root\".\n"), ic_rec->input_style);
		ic_rec->input_style = XIMPreeditNothing|XIMStatusNothing;
	    }
        }
	else if (match (XNClientWindow, ic_attr)) {
	    checkset_ic_val(CLIENT_SETIC_CLIENTW, Window, ic_attr,
			    ic_rec->client_win);
	}
        else if (match (XNFocusWindow, ic_attr)) {
	    checkset_ic_val(CLIENT_SETIC_FOCUSW, Window, ic_attr,
			    ic_rec->focus_win);
	}
#ifdef XIM_COMPLETE
	else if (match (XNResourceName, ic_attr))
	    ic_rec->resource_name = (char *)strdup((char *)ic_attr->value);
	else if (match (XNResourceClass, ic_attr))
	    ic_rec->resource_class = (char *)strdup((char *)ic_attr->value);
#endif
        else
            oxim_perr(OXIMMSG_WARNING, 
		N_("ic_set: unknown IC attr: %s\n"), ic_attr->name);
    }
        
    for (i=0; i < (int)(call_data->preedit_attr_num); i++, pre_attr++) {
	if (! pre_attr->name && ! pre_attr->value)
	    continue;
	DebugLog(3, ("ic_set: pre_attr: %s\n", pre_attr->name));
        if (match (XNArea, pre_attr)) {
	    checkset_ic_val(CLIENT_SETIC_PRE_AREA, XRectangle, pre_attr,
			    ic_rec->pre_attr.area);
	}
        else if (match (XNSpotLocation, pre_attr)) {
	    checkset_ic_val(CLIENT_SETIC_PRE_SPOTLOC, XPoint, pre_attr,
			    ic_rec->pre_attr.spot_location);
	    ic->has_set_spot = True;
	}
        else if (match (XNForeground, pre_attr)) {
	    checkset_ic_val(CLIENT_SETIC_PRE_FGCOLOR, CARD32, pre_attr,
			    ic_rec->pre_attr.foreground);
	}
        else if (match (XNBackground, pre_attr)) {
	    checkset_ic_val(CLIENT_SETIC_PRE_BGCOLOR, CARD32, pre_attr,
			    ic_rec->pre_attr.background);
	}
        else if (match (XNFontSet, pre_attr))
	{
            /* do nothing */
	}
        else if (match (XNLineSpace, pre_attr))
            ic_rec->pre_attr.line_space = *(CARD32 *)pre_attr->value;
	else if (match (XNPreeditState, pre_attr)) {
	    XIMPreeditState preedit_state;
	    preedit_state = *(XIMPreeditState *)pre_attr->value;
	    if (preedit_state == XIMPreeditDisable) {
		if ((ic->imc->inp_state & IM_CINPUT) ||
		    (ic->imc->inp_state & IM_2BYTES)) {
		    change_IM(ic, -1);
		    xim_disconnect(ic);
		}
	    }
	    else if (preedit_state == XIMPreeditEnable) {
		if (! (ic->imc->inp_state & IM_CINPUT) &&
		    ! (ic->imc->inp_state & IM_2BYTES)) {
		    if (change_IM(ic, ic->imc->inp_num) == True)
			xim_connect(ic);
		}
	    }
	    else if (preedit_state == XIMPreeditUnKnown)
		DebugLog(3, ("ic_set: preedit_state = XIMPreeditUnKnown.\n"));
	}

        else if (match (XNAreaNeeded, pre_attr))
            ic_rec->pre_attr.area_needed = *(XRectangle *)pre_attr->value;
        else if (match (XNColormap, pre_attr))
            ic_rec->pre_attr.cmap = *(Colormap *)pre_attr->value;
        else if (match (XNStdColormap, pre_attr))
            ic_rec->pre_attr.cmap = *(Colormap *)pre_attr->value;
        else if (match (XNBackgroundPixmap, pre_attr))
            ic_rec->pre_attr.bg_pixmap = *(Pixmap *)pre_attr->value;
        else if (match (XNCursor, pre_attr))
            ic_rec->pre_attr.cursor = *(Cursor *)pre_attr->value;
        else
            oxim_perr(OXIMMSG_WARNING, 
		N_("ic_set: unknown IC pre_attr: %s\n"), pre_attr->name);
    }

#ifdef XIM_COMPLETE
    for (i=0; i < (int)(call_data->status_attr_num); i++, sts_attr++) {
	if (! sts_attr->name && ! sts_attr->value)
	    continue;
	DebugLog(3, ("ic_set: sts_attr: %s\n", sts_attr->name));
        if (match (XNArea, sts_attr))
            ic_rec->sts_attr.area = *(XRectangle *)sts_attr->value;
        else if (match (XNAreaNeeded, sts_attr))
            ic_rec->sts_attr.area_needed = *(XRectangle *)sts_attr->value;
        else if (match (XNColormap, sts_attr))
            ic_rec->sts_attr.cmap = *(Colormap *)sts_attr->value;
        else if (match (XNStdColormap, sts_attr))
            ic_rec->sts_attr.cmap = *(Colormap *)sts_attr->value;
        else if (match (XNForeground, sts_attr))
            ic_rec->sts_attr.foreground = *(CARD32 *)sts_attr->value;
        else if (match (XNBackground, sts_attr))
            ic_rec->sts_attr.background = *(CARD32 *)sts_attr->value;
        else if (match (XNBackgroundPixmap, sts_attr))
            ic_rec->sts_attr.bg_pixmap = *(Pixmap *)sts_attr->value;
	else if (match (XNLineSpace, sts_attr))
            ic_rec->sts_attr.line_space= *(CARD32 *)sts_attr->value;
        else if (match (XNCursor, sts_attr))
            ic_rec->sts_attr.cursor = *(Cursor *)sts_attr->value;
	else if (match (XNFontSet, pre_attr))
	{
	    /* do nothing */
	}
        else
            oxim_perr(OXIMMSG_WARNING, 
		N_("ic_set: unknown IC sts_attr: %s\n"), sts_attr->name);
    }
#endif
/*
 *  Extra setting/checking after some IC values set.
 */
    if (ic_rec->ic_value_update & CLIENT_SETIC_INPSTY) {
	Window win = (Window)0;

	if (ic_rec->ic_value_set & CLIENT_SETIC_FOCUSW)
	    win = ic_rec->focus_win;
	else if (ic_rec->ic_value_set & CLIENT_SETIC_CLIENTW)
	    win = ic_rec->client_win;
	if (win) {
	    ic_rec->ic_value_update &= ~CLIENT_SETIC_INPSTY;
	    if ((xccore->oxim_mode & OXIM_RUN_INIT) && xccore->ic==NULL &&
		gui_check_input_focus(xccore, win) == True) {
/*
 *  Get the first input-focus IC window, to keep xccore->ic != NULL in
 *  the beginning. This is required when oxim is started in rxvt or other
 *  XIM terminal. Because in this case oxim does not receive the XSetICFocus,
 *  so it does not know that this XIM client is on focus now. If we kill
 *  oxim immediately in this situation, the XIM client may crash because
 *  oxim will not send XSync event to it. So we must get the initial input
 *  focus window here.
 */
		xccore->ic = ic;
		xccore->oxim_mode &= ~OXIM_RUN_INIT;
	    }
	}
    }
}

int 
ic_create(XIMS ims, IMChangeICStruct *call_data, xccore_t *xccore)
{
    IC *ic;
    int single_imc = (xccore->oxim_mode & OXIM_SINGLE_IMC);
 
    if (! (ic = new_IC(single_imc)))
        return False;

    ic->connect_id = call_data->connect_id;
    call_data->icid = ic->id;

    if (! single_imc)
	ic->imc->ic_rec = &(ic->ic_rec);
    if ((xccore->oxim_mode & OXIM_IM_FOCUS))
	ic->imc->inp_num = xccore->im_focus;
    else
	ic->imc->inp_num = oxim_get_Default_IM();
    ic->ic_state |= IC_NEWIC;
    ic->imc->inpinfo.imid = (int)(ic->imc->id);

    ic_set_values(ic, call_data, xccore);
    return True;
}

int 
ic_destroy(int icid, xccore_t *xccore)
{
    IC *ic, *last=NULL;

    for (ic=ic_list; ic!=NULL; last=ic, ic=ic->next) {
	if (ic->id == icid) {
	    delete_IC(ic, last, xccore);
	    return  True;
	}
    }
    return False;
}

int
ic_clean_all(CARD16 connect_id, xccore_t *xccore)
{
    IC *ic=ic_list, *last=NULL;
    int clean_count=0;

    while (ic != NULL) {
        if (ic->connect_id == connect_id) {
	    delete_IC(ic, last, xccore);
	    ic = (last) ? last->next : ic_list;
	    clean_count ++;
	}
	else {
	    last = ic;
	    ic = ic->next;
        }
    }
    return (clean_count) ? True : False;
}
#if 0
/*---------------------------------------------------------------------------

        Garbage Collection

---------------------------------------------------------------------------*/

#ifdef DEBUG
#define TIMECHECK_STEP    10
#define IC_IDLE_TIME      20
#else
#define TIMECHECK_STEP    300
#define IC_IDLE_TIME      600
#endif

void
check_ic_exist(int icid, xccore_t *xccore)
{
    static time_t last_check;
    IC *ic = ic_list, *last = NULL, *ref_ic;
    time_t current_time;
    int delete;

    if (icid == -1 || (ref_ic = ic_find(icid)) == NULL)
	return;
    current_time = time(NULL);
    if (current_time - last_check <= TIMECHECK_STEP)
	return;

    DebugLog(1, ("Begin check: current time = %d, last check = %d\n", 
		(int)current_time, (int)last_check));

    while (ic != NULL) {
	DebugLog(3, ("IC: id=%d, focus_w=0x%x, client_w=0x%x%s\n",
		ic->id, (unsigned int)ic->ic_rec.focus_win, 
		(unsigned int)ic->ic_rec.client_win, 
		(ic==ref_ic) ? ", (ref)." : "."));
	delete = 0;

	if (ic == ref_ic)
	    ic->exec_time = current_time;
        else if (ic->ic_rec.focus_win && 
		 ic->ic_rec.focus_win == ref_ic->ic_rec.focus_win)
	/* each IC should has its distinct window */
	    delete = 1;
	else if (current_time - ic->exec_time > IC_IDLE_TIME &&
		 ic->ic_rec.client_win != 0) {
	    DebugLog(3, ("Check IC: id=%d, window=0x%x, exec_time=%d.\n", 
		    ic->id, (unsigned int)ic->ic_rec.client_win, 
		    (int)ic->exec_time));
	    ic->exec_time = current_time;
	    if (gui_check_window(ic->ic_rec.client_win) != True)
		delete = 1;
        }

	if (delete) {
	    DebugLog(3, ("Delete IC: id=%d, window=0x%x, exec_time=%d.\n", 
		    ic->id, (unsigned int)ic->ic_rec.client_win, 
		    (int)ic->exec_time));
	    delete_IC(ic, last, xccore);
	    ic = (last) ? last->next : ic_list;
	}
	else {
	    last = ic;
	    ic = ic->next;
	}
    }
    last_check = current_time;
}
#endif
