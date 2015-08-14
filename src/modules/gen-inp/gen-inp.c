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
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "oximtool.h"
#include "module.h"
#include "gen-inp.h"

static char *bopomofo[] = {"ㄅㄆㄇㄈㄉㄊㄋㄌㄍㄎㄏㄐㄑㄒㄓㄔㄕㄖㄗㄘㄙ", /* 聲母 */
		      "ㄧㄨㄩ",
		      "ㄚㄛㄜㄝㄞㄟㄠㄡㄢㄣㄤㄥㄦ" /* 韻母 */};

static int fillpage(gen_inp_conf_t *cf, inpinfo_t *inpinfo,
         gen_inp_iccf_t *iccf, byte_t dir);
/*----------------------------------------------------------------------------

        gen_inp_init()

----------------------------------------------------------------------------*/

static int
loadtab(gen_inp_conf_t *cf)
{
    int n, nn, ret=1;
    char modID[MODULE_ID_SIZE];
    int size;

    size = sizeof(char) * MODULE_ID_SIZE;
    cf->word_start_pos = size;
    if (gzread(cf->zfp , modID, size) != size || strcmp(modID, "gencin"))
    {
	oxim_perr(OXIMMSG_WARNING, N_("gen_inp: %s: invalid tab file.\n"),cf->tabfn);
	return False;
    }
    size = sizeof(cintab_head_t);
    cf->word_start_pos += size;
    if (gzread(cf->zfp, &(cf->header), size) != size)
    {
	oxim_perr(OXIMMSG_WARNING, N_("gen_inp: %s: reading error.\n"), cf->tabfn);
	return False;
    }
    if (strcmp(GENCIN_VERSION, cf->header.version) > 0) {
	oxim_perr(OXIMMSG_WARNING, N_("gen_inp: %s: invalid version.\n"), cf->tabfn);
	return False;
    }

    n  = nn = cf->header.n_icode;
    cf->ic1 = oxim_malloc(sizeof(icode_t) * n, 0);
    cf->ichar = oxim_malloc(sizeof(ichar_t) * nn, 0);
    int n_size = sizeof(icode_t) * n;
    cf->word_start_pos += n_size;
    int nn_size = sizeof(ichar_t) * nn;
    cf->word_start_pos += nn_size;
    if (!n || !nn ||
	gzread(cf->zfp, cf->ic1, n_size) != n_size ||
	gzread(cf->zfp, cf->ichar, nn_size) != nn_size )
    {
	if (n)
	{
	    free(cf->ic1);
	    cf->ic1 = NULL;
	}
	if (nn)
	{
	    free(cf->ichar);
	    cf->ichar = NULL;
	}
	ret = False;
    }

    if (ret && cf->header.icode_mode == ICODE_MODE2)
    {
	cf->word_start_pos += n_size;
        cf->ic2 = oxim_malloc(n_size, 0);
	if (gzread(cf->zfp, cf->ic2, n_size) != n_size)
	{
	    if (n)
	    {
		free(cf->ic2);
		cf->ic2 = NULL;
	    }
	    ret = False;
	}
    }

    if (! ret) {
	oxim_perr(OXIMMSG_WARNING, N_("gen_inp: %s: reading error.\n"), cf->tabfn);
	return False;
    }
    else
	return True;
}

static void
gen_inp_resource(gen_inp_conf_t *cf, char *objname)
{
    settings_t *im_settings = oxim_get_im_settings(objname);
    if (!im_settings)
    {
	printf("Null setting: %s !\n", objname);
	return;
    }

    int boolean;

    if (oxim_setting_GetBoolean(im_settings, "AutoCompose", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_AUTOCOMPOSE, boolean);

    if (oxim_setting_GetBoolean(im_settings, "AutoUpChar", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_AUTOUPCHAR, boolean);

    if (oxim_setting_GetBoolean(im_settings, "AutoFullUp", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_AUTOFULLUP, boolean);

    if (oxim_setting_GetBoolean(im_settings, "SpaceAutoUp", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_SPACEAUTOUP, boolean);

    if (oxim_setting_GetBoolean(im_settings, "SelectKeyShift", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_SELKEYSHIFT, boolean);

    if (oxim_setting_GetBoolean(im_settings, "SpaceIgnore", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_SPACEIGNOR, boolean);

    if (oxim_setting_GetBoolean(im_settings, "SpaceReset", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_SPACERESET, boolean);

    if (oxim_setting_GetBoolean(im_settings, "AutoReset", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_AUTORESET, boolean);

    if (oxim_setting_GetBoolean(im_settings, "WildEnable", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_WILDON, boolean);

    if (oxim_setting_GetBoolean(im_settings, "EndKey", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_ENDKEY, boolean);


    char *string;
    if (oxim_setting_GetString(im_settings, "DisableSelectList", &string))
    {
	if (cf->disable_sel_list)
	    free(cf->disable_sel_list);

	if (strcasecmp("NONE", string) == 0)
	    cf->disable_sel_list = NULL;
	else
	    cf->disable_sel_list = strdup(string);
    }

    oxim_settings_destroy(im_settings);
}

static int
gen_inp_init(void *conf, char *objname)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf, cfd;
    char *s, value[128], truefn[256];
    char *sub_path = "tables";
    int ret;

    bzero(&cfd, sizeof(gen_inp_conf_t));
    cfd.mode = DEFAULT_INP_MODE;
    cf->modesc = 0;
    gen_inp_resource(&cfd, objname);
/*
 *  Resource setup.
 */
    if ((cfd.mode & INP_MODE_AUTOCOMPOSE))
	cf->mode |= INP_MODE_AUTOCOMPOSE;
    if ((cfd.mode & INP_MODE_AUTOUPCHAR)) {
	cf->mode |= INP_MODE_AUTOUPCHAR;
	if ((cfd.mode & INP_MODE_SPACEAUTOUP))
	    cf->mode |= INP_MODE_SPACEAUTOUP;
	if ((cfd.mode & INP_MODE_SELKEYSHIFT))
	    cf->mode |= INP_MODE_SELKEYSHIFT;
    }
    if ((cfd.mode & INP_MODE_AUTOFULLUP)) {
	cf->mode |= INP_MODE_AUTOFULLUP;
	if ((cfd.mode & INP_MODE_SPACEIGNOR))
	    cf->mode |= INP_MODE_SPACEIGNOR;
    }
    if ((cfd.mode & INP_MODE_AUTORESET))
	cf->mode |= INP_MODE_AUTORESET;
    else if ((cfd.mode & INP_MODE_SPACERESET))
	cf->mode |= INP_MODE_SPACERESET;
    if ((cfd.mode & INP_MODE_WILDON))
	cf->mode |= INP_MODE_WILDON;
    cf->modesc = cfd.modesc;
    cf->disable_sel_list = cfd.disable_sel_list;
/*
 *  Read the IM tab file.
 */
    sprintf(value, "%s.tab", objname);
    if (oxim_check_datafile(value, sub_path, truefn, 256) == True) {
	cf->tabfn = (char *)strdup(truefn);
	if ((cf->zfp = gzopen(truefn, "rb")) == NULL)
	    return False;
        ret = loadtab(cf);
	if (!ret)
	{
	    gzclose(cf->zfp);
	    cf->zfp = NULL;
	}
    }
    else
	return False;

    if (cf->header.n_endkey) {
	if ((cfd.mode & INP_MODE_ENDKEY))
	    cf->mode |= INP_MODE_ENDKEY;
    }

    return ret;
}


/*----------------------------------------------------------------------------

        gen_inp_xim_init()

----------------------------------------------------------------------------*/

static int
gen_inp_xim_init(void *conf, inpinfo_t *inpinfo)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf;
    gen_inp_iccf_t *iccf;
    int i;

    inpinfo->iccf = oxim_malloc(sizeof(gen_inp_iccf_t), True);
    iccf = (gen_inp_iccf_t *)inpinfo->iccf;

    inpinfo->etymon_list = cf->header.keyname;
    inpinfo->guimode = 0;

    inpinfo->keystroke_len = 0;
    inpinfo->s_keystroke = oxim_malloc((INP_CODE_LENGTH+1)*sizeof(uch_t), True);
    inpinfo->suggest_skeystroke =
		oxim_malloc((INP_CODE_LENGTH+1)*sizeof(uch_t), True);

    if (! (cf->mode & INP_MODE_SELKEYSHIFT)) {
	inpinfo->n_selkey = cf->header.n_selkey;
	inpinfo->s_selkey = oxim_malloc(inpinfo->n_selkey*sizeof(uch_t), True);
	for (i=0; i<SELECT_KEY_LENGTH && i<cf->header.n_selkey; i++)
	    inpinfo->s_selkey[i].s[0] = cf->header.selkey[i];
    }
    else {
	inpinfo->n_selkey = cf->header.n_selkey+1;
	inpinfo->s_selkey = oxim_malloc(inpinfo->n_selkey*sizeof(uch_t), True);
	for (i=0; i<SELECT_KEY_LENGTH && i<cf->header.n_selkey; i++)
	    inpinfo->s_selkey[i+1].s[0] = cf->header.selkey[i];
    }
    inpinfo->n_mcch = 0;
    i = inpinfo->n_selkey;
    inpinfo->mcch = NULL;
    inpinfo->mcch_grouping = oxim_malloc((i+1) * sizeof(uint_t), True);
    inpinfo->mcch_pgstate = MCCH_ONEPG;

    inpinfo->n_lcch = 0;
    inpinfo->lcch = NULL;
    inpinfo->lcch_grouping = NULL;
    inpinfo->cch_publish.uch = 0;

    return  True;
}

static unsigned int
gen_inp_xim_end(void *conf, inpinfo_t *inpinfo)
{
    gen_inp_iccf_t *iccf = (gen_inp_iccf_t *)inpinfo->iccf;

    if (iccf->n_mcch_list)
    {
	free(iccf->mcch_list);
	free(iccf->mcch_list_grouping);
    }

    if (iccf->n_mkey_list)
	free(iccf->mkey_list);
    free(inpinfo->iccf);
    free(inpinfo->s_keystroke);
    free(inpinfo->suggest_skeystroke);
    free(inpinfo->s_selkey);
    free(inpinfo->mcch_grouping);
    inpinfo->iccf = NULL;
    inpinfo->s_keystroke = NULL;
    inpinfo->suggest_skeystroke = NULL;
    inpinfo->s_selkey = NULL;
    inpinfo->mcch = NULL;
    inpinfo->mcch_grouping = NULL;
    return IMKEY_ABSORB;
}


/*----------------------------------------------------------------------------

	gen_inp_keystroke()

----------------------------------------------------------------------------*/

static unsigned int
return_wrong(gen_inp_conf_t *cf)
{
    return IMKEY_ABSORB;
}

static unsigned int
return_correct(gen_inp_conf_t *cf)
{
    return IMKEY_ABSORB;
}

static void
reset_keystroke(inpinfo_t *inpinfo, gen_inp_iccf_t *iccf)
{
    inpinfo->s_keystroke[0].uch = 0;
    inpinfo->keystroke_len = 0;
    inpinfo->n_mcch = 0;
    iccf->keystroke[0] = '\0';
    iccf->mode = 0;
    inpinfo->mcch_pgstate = MCCH_ONEPG;
    if (iccf->n_mcch_list)
    {
	free(iccf->mcch_list);
	free(iccf->mcch_list_grouping);
	iccf->n_mcch_list = 0;
    }

    if (iccf->n_mkey_list)
    {
	free(iccf->mkey_list);
	iccf->n_mkey_list = 0;
    }
}


/*------------------------------------------------------------------------*/

static int
cmp_icvalue(icode_t *ic1, icode_t *ic2, unsigned int idx,
	    icode_t icode1, icode_t icode2, int mode)
{
    if (ic1[idx] > icode1)
	return 1;
    else if (ic1[idx] < icode1)
	return -1;
    else {
	if (! mode)
	    return 0;
	else if (ic2[idx] > icode2)
	    return 1;
	else if (ic2[idx] < icode2)
	    return -1;
	else
	    return 0;
    }
}

static int 
bsearch_char(icode_t *ic1, icode_t *ic2, icode_t icode1, icode_t icode2,
		int size, int mode, int wild, int *end_idx)
{
    int head, middle, end, ret;

    head   = 0;
    middle = size / 2;
    end    = size;
    while ((ret=cmp_icvalue(ic1, ic2, middle, icode1, icode2, mode))) {
        if (ret > 0)
            end = middle;
        else
            head = middle + 1;
        middle = (end + head) / 2;
        if (middle == head && middle == end)
            break;
    }
    if (ret == 0) {
	int last_idx = middle;
	/* 找該組字的一筆索引 */
	while(middle > 0 &&
	      ! cmp_icvalue(ic1, ic2, middle-1, icode1, icode2, mode)) 
	{
	    middle --;
	}

	/* 找最後一筆索引 */
	if (end_idx)
	{
	    while(last_idx > 0 &&
		! cmp_icvalue(ic1, ic2, last_idx+1, icode1, icode2, mode))
	    {
		last_idx++;
	    }
	    *end_idx = last_idx;
	}
	return middle;
    }
    else
	return (wild) ? middle : -1;
}

/*---------------------------------------------------------------------
找詞
---------------------------------------------------------------------*/
static char *get_word(gen_inp_conf_t *cf, unsigned int ichar, unsigned int *ret_len)
{
    unsigned int offset = ichar & ~WORD_MASK;
    if (gzseek(cf->zfp, cf->word_start_pos + offset, SEEK_SET) >= 0)
    {
	unsigned short word_len = 0;
	if (gzread(cf->zfp, &word_len, sizeof(unsigned short)) >= 0)
	{
	    char *word = oxim_malloc(word_len + 1, True);
	    if (gzread(cf->zfp, word, word_len) == word_len)
	    {
		*ret_len = word_len;
		return word;
	    }
	    free(word);
	}
    }
    *ret_len = 0;
    return NULL;
}

static int
pick_cch_wild(gen_inp_conf_t *cf, gen_inp_iccf_t *iccf, int *head, byte_t dir,
		inpinfo_t *inpinfo, unsigned int *n_mcch)
{
    unsigned int i, size, klist[2];
    int n_klist, ks_size, r=0, idx, n_igrp=0;
    char *ks;
    unsigned int start = *head;
    unsigned int mcch_size = inpinfo->n_selkey;
    int have_word = False;

    size = cf->header.n_icode;
    ks_size = cf->header.n_max_keystroke + 1;
    ks = oxim_malloc(ks_size, 0);
    n_klist = (cf->header.icode_mode == ICODE_MODE1) ? 1 : 2;
    dir = (dir > 0) ? (byte_t)1 : (byte_t)-1;

    if (iccf->n_mkey_list)
    {
	free(iccf->mkey_list);
    }
    iccf->mkey_list = oxim_malloc(mcch_size*sizeof(int), False);

    if (iccf->n_mcch_list)
    {
	free(iccf->mcch_list);
	free(iccf->mcch_list_grouping);
    }

    iccf->mcch_list = oxim_malloc(mcch_size*sizeof(uch_t), False);
    iccf->mcch_list_grouping = oxim_malloc(mcch_size*sizeof(word_group_t), False);

    for (i=0, idx = *head; idx>=0 && idx<size && i<=mcch_size; idx+=dir)
    {
	klist[0] = cf->ic1[idx];
	if (cf->header.icode_mode == ICODE_MODE2)
	    klist[1] = cf->ic2[idx];
	oxim_codes2keys(klist, n_klist, ks, ks_size);

	if (strcmp_wild(iccf->keystroke, ks) == 0)
	{
	    if (i < mcch_size)
	    {
		iccf->mcch_list_grouping[n_igrp].n_idx = i;
		if (cf->ichar[idx] & WORD_MASK)
		{
		    have_word = True;
		    unsigned int ret_len;
		    char *ret_word = get_word(cf, cf->ichar[idx], &ret_len);
		    if (ret_word)
		    {
			unsigned int ucs4;
			int nbytes;
			char *p = ret_word;
			unsigned int utf8_len = 0;
			while (ret_len && (nbytes = oxim_utf8_to_ucs4(p, &ucs4, ret_len)) > 0)
			{
			    utf8_len ++;
			    if (utf8_len > 1)
			    {
				mcch_size ++;
				iccf->mcch_list = oxim_realloc(iccf->mcch_list, mcch_size*sizeof(uch_t));
			    }	
			    iccf->mcch_list[i].uch = 0;
			    memcpy((char *)iccf->mcch_list[i].s, p, nbytes);
			    i ++;
			    p += nbytes;
			    ret_len -= nbytes;
			}
		    	iccf->mcch_list_grouping[n_igrp].n_word = utf8_len;
			n_igrp ++;
			free(ret_word);
		    }
		}
		else if (ccode_to_char(cf->ichar[idx], (char *)iccf->mcch_list[i].s))
		{
		    iccf->mkey_list[n_igrp] = idx;
		    iccf->mcch_list_grouping[n_igrp].n_word = 1;
		    i ++;
		    n_igrp ++;
		}
		*head = idx;
	    }
	    else
	    {
		r = 1;
	    }
	}
    }
    free(ks);

    iccf->n_mkey_list = n_igrp;
    iccf->n_mcch_list = n_igrp;
    *n_mcch = i;

    if (i)
    {
	inpinfo->n_mcch = n_igrp;
	inpinfo->mcch = iccf->mcch_list;
	for (i=0 ; i < n_igrp ; i++)
	{
	    inpinfo->mcch_grouping[i+1] = iccf->mcch_list_grouping[i].n_word;
	}
	inpinfo->mcch_grouping[0] = have_word ? n_igrp : 0;
    }
    else
    {
	inpinfo->n_mcch = 0;
	inpinfo->mcch_grouping[0] = 0;
	free(iccf->mcch_list);
	free(iccf->mcch_list_grouping);
	free(iccf->mkey_list);
	*head = 0;
    }
    return r;
}

static int
match_keystroke_wild(gen_inp_conf_t *cf, 
		       inpinfo_t *inpinfo, gen_inp_iccf_t *iccf)
{
    icode_t icode[2];
    unsigned int md, n_mcch;
    int idx;
    char *s1, *s2, *s, tmpch;

    md = (cf->header.icode_mode == ICODE_MODE2) ? 1 : 0;
    icode[0] = icode[1] = 0;

    /*
     *  Search for the first char.
     */
    s1 = strchr(iccf->keystroke, '*');
    s2 = strchr(iccf->keystroke, '?');
    if (s1 && s2)
	s = (s1 < s2) ? s1 : s2;
    else if (s1)
	s = s1;
    else
	s = s2;
    tmpch = *s;
    *s = '\0';

    oxim_keys2codes(icode, 2, iccf->keystroke);
    idx = bsearch_char(cf->ic1, cf->ic2, icode[0], icode[1], 
			cf->header.n_icode, md, 1, NULL);
    *s = tmpch;
    iccf->mcch_hidx = idx;		/* refer to head index of cf->icidx */

    /*
     *  Pick up the remaining chars;
     */
    if (pick_cch_wild(cf, iccf, &idx, 1, inpinfo, &n_mcch) == 0)
    {
        inpinfo->mcch_pgstate = MCCH_ONEPG;
    }
    else 
    {
	inpinfo->mcch_pgstate = MCCH_BEGIN;
    }

    iccf->mcch_eidx = idx;		/* refer to end index of cf->icidx */
    return (n_mcch) ? 1 : 0;
}

static int
match_keystroke_normal(gen_inp_conf_t *cf, 
		       inpinfo_t *inpinfo, gen_inp_iccf_t *iccf)
{
    icode_t icode[2];
    unsigned int size, md, n_ich=0, n_igrp=0, mcch_size;
    int idx, end_idx;
    uch_t *mcch = NULL;
    word_group_t *mcch_grouping = NULL;

    size = cf->header.n_icode;
    md = (cf->header.icode_mode == ICODE_MODE2) ? 1 : 0;
    icode[0] = icode[1] = 0;

    /*
     *  Search for the first char.
     */
    oxim_keys2codes(icode, 2, iccf->keystroke);
    if ((idx = bsearch_char(cf->ic1, cf->ic2, 
		icode[0], icode[1], size, md, 0, &end_idx)) == -1)
	return 0;

    /*
     *  Search for all the chars with the same keystroke.
     */
    mcch_size = end_idx - idx + 1; /* 總共幾筆紀錄 */
    /* 預先配置記憶體 */
    mcch = oxim_malloc(mcch_size*sizeof(uch_t), False);
    mcch_grouping = oxim_malloc(mcch_size*sizeof(word_group_t), False);

    while(idx <= end_idx)
    {
    	mcch_grouping[n_igrp].n_idx = n_ich;
	if (cf->ichar[idx] & WORD_MASK)
	{
	    unsigned int ret_len;
	    char *ret_word = get_word(cf, cf->ichar[idx], &ret_len);
	    if (ret_word)
	    {
		unsigned int ucs4;
		int nbytes;
		char *p = ret_word;
		unsigned int utf8_len = 0;
		while (ret_len && (nbytes = oxim_utf8_to_ucs4(p, &ucs4, ret_len)) > 0)
		{
		    utf8_len ++;
		    if (utf8_len > 1)
		    {
			mcch_size ++;
			mcch = oxim_realloc(mcch, mcch_size*sizeof(uch_t));
		    }	
		    mcch[n_ich].uch = 0;
		    memcpy((char *)mcch[n_ich].s, p, nbytes);
		    n_ich ++;
		    p += nbytes;
		    ret_len -= nbytes;
		}
	    	mcch_grouping[n_igrp].n_word = utf8_len;
		n_igrp ++;
		free(ret_word);
	    }
	}
	else if (ccode_to_char(cf->ichar[idx], (char *)mcch[n_ich].s))
	{
	    mcch_grouping[n_igrp].n_word = 1;
	    n_ich ++;
	    n_igrp ++;
	}
        idx ++;
    }

    if (iccf->n_mcch_list)
    {
	free(iccf->mcch_list);
	free(iccf->mcch_list_grouping);
    }

    iccf->mcch_list = mcch;
    iccf->mcch_list_grouping = mcch_grouping;
    iccf->n_mcch_list = n_igrp;
    iccf->mcch_hidx = 0;	/* refer to index of iccf->mcch_list */

    fillpage(cf, inpinfo, iccf, 0);
    return 1;
}

static int 
match_keystroke(gen_inp_conf_t *cf, inpinfo_t *inpinfo, gen_inp_iccf_t *iccf)
/* return: 1: success, 0: false */
{
    int ret;

    inpinfo->n_mcch = 0;
    if (! (iccf->mode & INPINFO_MODE_INWILD))
	ret = match_keystroke_normal(cf, inpinfo, iccf);
    else
	ret = match_keystroke_wild(cf, inpinfo, iccf);

    if (inpinfo->n_mcch > 1 && (iccf->mode & INPINFO_MODE_SPACE))
	iccf->mode &= ~(INPINFO_MODE_SPACE);

    return ret;
}


/*------------------------------------------------------------------------*/

static void
get_correct_skeystroke(gen_inp_conf_t *cf,
		inpinfo_t *inpinfo, gen_inp_iccf_t *iccf, int idx, uch_t *cch)
{
    unsigned int i=0, j, klist[2];
    int n_klist, ks_size, keycode;
    char *ks;

    if (idx < iccf->n_mkey_list)
    {
	i = iccf->mkey_list[idx];
    }
    else
    {
	inpinfo->suggest_skeystroke[0].uch = 0;
	return;
    }
    ks_size = cf->header.n_max_keystroke + 1;
    ks = oxim_malloc(ks_size, 0);
    n_klist = (cf->header.icode_mode == ICODE_MODE1) ? 1 : 2;

    klist[0] = cf->ic1[i];
    if (cf->header.icode_mode == ICODE_MODE2)
	klist[1] = cf->ic2[i];
    oxim_codes2keys(klist, n_klist, ks, ks_size);
    if (strcmp_wild(iccf->keystroke, ks) == 0)
    {
	for (j=0; ks[j] != '\0'; j++)
	{
	    keycode = oxim_key2code(ks[j]);
	    inpinfo->suggest_skeystroke[j].uch =
			cf->header.keyname[keycode].uch;
	}
	inpinfo->suggest_skeystroke[j].uch = 0;
    }
    else
	inpinfo->suggest_skeystroke[0].uch = 0;
    free(ks);
}

static void
commit_char(gen_inp_conf_t *cf, inpinfo_t *inpinfo, gen_inp_iccf_t *iccf, int idx)
{
    static char cch_s[1024];
    char *s=NULL;
    int i;

    unsigned int grp_idx = (iccf->mode & INPINFO_MODE_INWILD) ? idx : iccf->mcch_hidx + idx;
    word_group_t wg = iccf->mcch_list_grouping[grp_idx];
    uch_t *mcch = iccf->mcch_list + wg.n_idx;
    uch_t *cch = mcch;
    ushort_t cch_len = wg.n_word;

    cch_s[0] = '\0';
    while (cch_len--)
    {
	strcat(cch_s, (char *)mcch->s);
	mcch ++;
    }

    inpinfo->cch = cch_s;

    /* 只有單一字元才需要提示字根 */
    if (wg.n_word == 1)
    {
	if (! (s = strchr(iccf->keystroke,'*')) &&
		! (s = strchr(iccf->keystroke,'?')))
	{
	    for (i=0; i<=inpinfo->keystroke_len; i++)
	    inpinfo->suggest_skeystroke[i].uch = inpinfo->s_keystroke[i].uch;
	}
	else
	{
	    get_correct_skeystroke(cf, inpinfo, iccf, idx, cch);
	}
    }
    else
    {
	inpinfo->suggest_skeystroke[0].uch = 0;
    }

    inpinfo->keystroke_len = 0;
    inpinfo->s_keystroke[0].uch = 0;
    inpinfo->n_mcch = 0;
    inpinfo->cch_publish.uch = cch->uch;
    inpinfo->mcch_pgstate = MCCH_ONEPG;

    iccf->mode &= ~(INPINFO_MODE_MCCH);
    iccf->mode &= ~(INPINFO_MODE_INWILD);
    inpinfo->guimode &= ~(GUIMOD_SELKEYSPOT);
}

static unsigned int
commit_keystroke(gen_inp_conf_t *cf, inpinfo_t *inpinfo, gen_inp_iccf_t *iccf)
/* return: the IMKEY state */
{
    if (match_keystroke(cf, inpinfo, iccf))
    {
        if (inpinfo->n_mcch == 1)
	{
            commit_char(cf, inpinfo, iccf, 0);
            return IMKEY_COMMIT;
        }
        else
	{
            iccf->mode |= INPINFO_MODE_MCCH;
	    inpinfo->guimode |= GUIMOD_SELKEYSPOT;
            return return_correct(cf);
        }
    }
    else
    {
	if ((cf->mode & INP_MODE_AUTORESET))
	{
	    reset_keystroke(inpinfo, iccf);
	}
	else
	{
	    iccf->mode |= INPINFO_MODE_WRONG;
	}
        return return_wrong(cf);
    }
}


/*------------------------------------------------------------------------*/

static int
mcch_choosech(gen_inp_conf_t *cf, 
	      inpinfo_t *inpinfo, gen_inp_iccf_t *iccf, int idx)
{
    int min;

    if (inpinfo->n_mcch == 0 && ! match_keystroke(cf, inpinfo, iccf))
    {
        return 0;
    }

    if (idx < 0)		/* Always select the first. */
    {
	idx = 0;
    }
    else
    {
        if ((cf->mode & INP_MODE_SELKEYSHIFT))
	{
	    idx ++;
	}
        min = (inpinfo->n_selkey > inpinfo->n_mcch) ? 
                inpinfo->n_mcch : inpinfo->n_selkey;
        if (idx >= min)
	{
            return 0;
	}
    }

    commit_char(cf, inpinfo, iccf, idx);
    reset_keystroke(inpinfo, iccf);

    return 1;
}

static int
fillpage(gen_inp_conf_t *cf, inpinfo_t *inpinfo, 
	 gen_inp_iccf_t *iccf, byte_t dir)
{
    int i, j, n_pg=inpinfo->n_selkey;

    if (! (iccf->mode & INPINFO_MODE_INWILD))
    {
        int total = iccf->n_mcch_list;

        switch (dir)
	{
        case 0:
	    iccf->mcch_hidx = 0;
	    break;
        case 1:
	    if (iccf->mcch_hidx + n_pg < total)
	        iccf->mcch_hidx += n_pg;
	    else
	        return 0;
	    break;
        case -1:
	    if (iccf->mcch_hidx - n_pg >= 0)
	        iccf->mcch_hidx -= n_pg;
	    else
	        return 0;
	    break;
        }
	/* */
	uch_t *mcch = iccf->mcch_list + iccf->mcch_list_grouping[iccf->mcch_hidx].n_idx;
	j = total - iccf->mcch_hidx;
	i = (j >= n_pg) ? n_pg : j;

	inpinfo->mcch_grouping[0] = 0;
	for (j=0; j < i ; j++)
	{
	    inpinfo->mcch_grouping[j+1] = iccf->mcch_list_grouping[iccf->mcch_hidx+j].n_word;
	    if (iccf->mcch_list_grouping[iccf->mcch_hidx+j].n_word > 1)
	    {
		inpinfo->mcch_grouping[0] = i;
	    }
	}

	inpinfo->mcch = mcch;

        if (iccf->mcch_hidx == 0)
            inpinfo->mcch_pgstate = (i < total) ? MCCH_BEGIN : MCCH_ONEPG;
        else if (total - iccf->mcch_hidx > n_pg)
	    inpinfo->mcch_pgstate = MCCH_MIDDLE;
        else
	    inpinfo->mcch_pgstate = MCCH_END;

        inpinfo->n_mcch = i;
    }
    else
    {
	int r=0, n_mcch, hidx, eidx; 

        switch (dir) {
        case 0:
	    return 0;
        case 1:
	    if (inpinfo->mcch_pgstate == MCCH_BEGIN ||
	        inpinfo->mcch_pgstate == MCCH_MIDDLE) {
		hidx = eidx = iccf->mcch_eidx + 1;
        	r = pick_cch_wild(cf, iccf, &eidx, 1,
			inpinfo, (unsigned int *)&n_mcch);
	    }
	    else
		return 0;
	    break;
        case -1:
	    if (inpinfo->mcch_pgstate == MCCH_END ||
	        inpinfo->mcch_pgstate == MCCH_MIDDLE)
	    {
		hidx = eidx = iccf->mcch_hidx - 1;
        	r = pick_cch_wild(cf, iccf, &hidx, -1,
	        	inpinfo, (unsigned int *)&n_mcch);
	    }
	    else
	    {
		return 0;
	    }
	    break;
        }

        if (r == 0)
	{
	    if (dir == 1)
                inpinfo->mcch_pgstate = MCCH_END;
	    else
                inpinfo->mcch_pgstate = MCCH_BEGIN;
	}
        else
	{
	    inpinfo->mcch_pgstate = MCCH_MIDDLE;
	}

	iccf->mcch_hidx = hidx;
	iccf->mcch_eidx = eidx;
    }

    return 1;
}

static int
mcch_nextpage(gen_inp_conf_t *cf, 
	      inpinfo_t *inpinfo, gen_inp_iccf_t *iccf, char key)
{
    int ret=0;

    switch (inpinfo->mcch_pgstate) {
    case MCCH_ONEPG:
	switch (key) {
	case ' ':
	    if ((cf->mode & INP_MODE_AUTOUPCHAR))
                ret = (mcch_choosech(cf, inpinfo, iccf, -1)) ?
                	IMKEY_COMMIT : return_wrong(cf);
            else
                ret = return_correct(cf);
	    break;
	case '<':
	case '>':
	    ret = return_correct(cf);
	    break;
	default:
	    ret = return_wrong(cf);
	    break;
	}
	break;

    case MCCH_END:
	switch (key) {
	case ' ':
	case '>':
	    ret = (fillpage(cf, inpinfo, iccf, 0)) ? 
			IMKEY_ABSORB : return_wrong(cf);
	    break;
	case '<':
	    ret = (fillpage(cf, inpinfo, iccf, -1)) ?
			IMKEY_ABSORB : return_wrong(cf);
	    break;
	default:
	    ret = return_wrong(cf);
	    break;
	}
	break;

    case MCCH_BEGIN:
	switch (key) {
	case ' ':
	case '>':
	    ret = (fillpage(cf, inpinfo, iccf, 1)) ? 
			IMKEY_ABSORB : return_wrong(cf);
	    break;
	case '<':
	    ret = return_correct(cf);
	    break;
	default:
	    ret = return_wrong(cf);
	    break;
	}
	break;

    default:
	switch (key) {
	case ' ':
	case '>':
	    ret = (fillpage(cf, inpinfo, iccf, 1)) ? 
			IMKEY_ABSORB : return_wrong(cf);
	    break;
	case '<':
	    ret = (fillpage(cf, inpinfo, iccf, -1)) ?
			IMKEY_ABSORB : return_wrong(cf);
	    break;
	default:
	    ret = return_wrong(cf);
	    break;
	}
	break;
    }
    return ret;
}

/*------------------------------------------------------------------------*/

static unsigned int
modifier_escape(gen_inp_conf_t *cf, int class)
{
    unsigned int ret=0;

    switch (class) {
    case QPHR_SHIFT:
	if ((cf->modesc & QPHR_SHIFT))
	    ret |= IMKEY_SHIFTPHR;
	ret |= IMKEY_SHIFTESC;
	break;
    case QPHR_CTRL:
	if ((cf->modesc & QPHR_CTRL))
	    ret |= IMKEY_CTRLPHR;
	break;
    case QPHR_ALT:
	if ((cf->modesc & QPHR_ALT))
	    ret |= IMKEY_ALTPHR;
	break;
    case QPHR_FALLBACK:
	if ((cf->modesc & QPHR_FALLBACK))
	    ret |= IMKEY_FALLBACKPHR;
	break;
    }
    return ret;
}

static int
get_bopomofo_index(uch_t uch)
{
    int idx;
    int uch_len = strlen((char *)uch.s);
    int isbopomofo = False;
    for (idx = 0; idx < 3 && uch_len; idx++)
    {
	if (strstr(bopomofo[idx], (char *)uch.s))
	    return idx;
    }
    return -1;
}

static unsigned int
gen_inp_keystroke(void *conf, inpinfo_t *inpinfo, keyinfo_t *keyinfo)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf;
    gen_inp_iccf_t *iccf = (gen_inp_iccf_t *)inpinfo->iccf;
    KeySym keysym = keyinfo->keysym;
    char *keystr = keyinfo->keystr;
    int len, max_len;
    char sp_ignore=0, inp_wrong=0;

    len = inpinfo->keystroke_len;
    max_len = cf->header.n_max_keystroke;
    if ((iccf->mode & INPINFO_MODE_SPACE)) {
        sp_ignore = 1;
        iccf->mode &= ~(INPINFO_MODE_SPACE);
    }
    if ((iccf->mode & INPINFO_MODE_WRONG)) {
	inp_wrong = 1;
	iccf->mode &= ~(INPINFO_MODE_WRONG);
    }

    if ((keysym == XK_BackSpace || keysym == XK_Delete) && len) {
        iccf->keystroke[len-1] = '\0';
        inpinfo->s_keystroke[len-1].uch = (uchar_t)0;
        inpinfo->keystroke_len --;
	inpinfo->n_mcch = 0;
	inpinfo->cch_publish.uch = (uchar_t)0;
	inpinfo->mcch_pgstate = MCCH_ONEPG;
	inpinfo->guimode &= ~(GUIMOD_SELKEYSPOT);
	iccf->mode = 0;
	if ((cf->mode & INP_MODE_WILDON) &&
	    (strchr(iccf->keystroke, '*') || strchr(iccf->keystroke, '?')))
	    iccf->mode |= INPINFO_MODE_INWILD;
	if (len-1 > 0 && (cf->mode & INP_MODE_AUTOCOMPOSE))
	    match_keystroke(cf, inpinfo, iccf);
        return IMKEY_ABSORB;
    }

    else if (keysym == XK_Escape && len) {
	reset_keystroke(inpinfo, iccf);
        inpinfo->cch_publish.uch = (uchar_t)0;
	inpinfo->mcch_pgstate = MCCH_ONEPG;
	inpinfo->guimode &= ~(GUIMOD_SELKEYSPOT);
        return IMKEY_ABSORB;
    }

    else if (keysym == XK_space) {
        inpinfo->cch_publish.uch = (uchar_t)0;

	if ((cf->mode & INP_MODE_SPACEAUTOUP) && 
/* avoid wild mode auto up */
	    (! (iccf->mode & INPINFO_MODE_INWILD) ||
	       (iccf->mode & INPINFO_MODE_MCCH)) &&
/* end of wild mod */
	    (inpinfo->n_mcch > 1 || inpinfo->mcch_pgstate != MCCH_ONEPG)) {
            if ((mcch_choosech(cf, inpinfo, iccf, -1)))
                return IMKEY_COMMIT;
            else {
                if ((cf->mode & INP_MODE_AUTORESET))
                    reset_keystroke(inpinfo, iccf);
                else
                    iccf->mode |= INPINFO_MODE_WRONG;
                return return_wrong(cf);
            }
	}
	else if ((iccf->mode & INPINFO_MODE_MCCH))
	    return mcch_nextpage(cf, inpinfo, iccf, ' ');
        else if ((cf->mode & INP_MODE_SPACERESET) && inp_wrong) {
            reset_keystroke(inpinfo, iccf);
            return IMKEY_ABSORB;
        }
        else if (sp_ignore)
            return IMKEY_ABSORB;
	else if (inpinfo->keystroke_len)
	    return commit_keystroke(cf, inpinfo, iccf);
    }

    else if ((keysym >= XK_KP_0 && keysym <= XK_KP_9) || 
	     (keysym >= XK_KP_Multiply && keysym <= XK_KP_Divide))
	return IMKEY_IGNORE;

    else if ((keysym == XK_Left || keysym == XK_Right) &&
		(inpinfo->guimode & GUIMOD_SELKEYSPOT))
    {
	char pg_key = (keysym == XK_Left) ? '<' : '>';
	return mcch_nextpage(cf, inpinfo, iccf, pg_key);
    }

    else if (keyinfo->keystr_len == 1) {
        unsigned int ret=IMKEY_ABSORB, ret1=0;
	int selkey_idx, endkey_pressed=0;
        char keycode, *s;
        uch_t uch;

        inpinfo->cch_publish.uch = (uchar_t)0;
	keycode = oxim_key2code(keystr[0]);
	uch.uch = cf->header.keyname[(int)keycode].uch;

	selkey_idx = ((s = strchr(cf->header.selkey, keystr[0]))) ? 
		(int)(s - cf->header.selkey) : -1;
	if (cf->header.n_endkey && 
	    strchr(cf->header.endkey, iccf->keystroke[len-1]))
	    endkey_pressed = 1;

	if (len && selkey_idx != -1 && (endkey_pressed || ! uch.uch)) {
	    /* Don't enter the multi-cch selection, but selkey pressed. */
	    if (len==1 && cf->disable_sel_list && 
		strchr(cf->disable_sel_list, iccf->keystroke[len-1]))
		uch.s[0] = keystr[0];
	    else
		return (mcch_choosech(cf, inpinfo, iccf, selkey_idx)) ?
			IMKEY_COMMIT : return_wrong(cf);
	}
	else if ((keystr[0]=='<' || keystr[0]=='>') &&
		 (inpinfo->guimode & GUIMOD_SELKEYSPOT))
	    return mcch_nextpage(cf, inpinfo, iccf, keystr[0]);
	else if ((iccf->mode & INPINFO_MODE_MCCH)) {
	    /* Enter the multi-cch selection. */
	    if (selkey_idx != -1)
		return (mcch_choosech(cf, inpinfo, iccf, selkey_idx)) ?
			IMKEY_COMMIT : return_wrong(cf);
	    else if ((cf->mode & INP_MODE_AUTOUPCHAR)) {
		if (! mcch_choosech(cf, inpinfo, iccf, -1))
		    return return_wrong(cf);
		ret |= IMKEY_COMMIT;
	    }
	    else
		return return_wrong(cf);
	}

	/* 檢查所輸入的字根是否為注音符號 */
	int bopomofo_idx = get_bopomofo_index(uch);
	int isappend = True;
	int stroke_idx;

	/* 注音符號要特別處理 */
	if (bopomofo_idx >= 0)
	{
	    for (stroke_idx=0 ;stroke_idx < inpinfo->keystroke_len;stroke_idx++)
	    {
		/* 首先，注音符號不可能有重複 */
		if (strcmp((char *)uch.s, (char *)inpinfo->s_keystroke[stroke_idx].s) == 0)
		    return IMKEY_ABSORB;

		/* 字根性質相同嗎?是的話，直接取代 */
		if (get_bopomofo_index(inpinfo->s_keystroke[stroke_idx]) == bopomofo_idx)
		{
		    isappend = False;
		    break;
		}

	    }
	}

	/* The previous cch might be committed, so len might be 0 */
	len = inpinfo->keystroke_len;

	if ((keyinfo->keystate & ShiftMask)) {
	    if (! (cf->mode & INP_MODE_WILDON) || 
		(keystr[0] != '*' && keystr[0] != '?'))
		return (ret | modifier_escape(cf, QPHR_SHIFT));
	    else
	        iccf->mode |= INPINFO_MODE_INWILD;
	}
	else if ((keyinfo->keystate & ControlMask) &&
		 (ret1 = modifier_escape(cf, QPHR_CTRL)))
	    return (ret | ret1);
	else if ((keyinfo->keystate & Mod1Mask) &&
		 (ret1 = modifier_escape(cf, QPHR_ALT)))
	    return (ret | ret1);
	else if (! uch.uch)
	    return (ret | IMKEY_IGNORE);
        else if (len >= max_len)
            return return_wrong(cf);

	if (isappend)
	{
            iccf->keystroke[len] = keystr[0];
            iccf->keystroke[len+1] = '\0';
	    if (keystr[0] == '*' || keystr[0] == '?')
	    {
	        inpinfo->s_keystroke[len].s[0] = keystr[0];
	        inpinfo->s_keystroke[len].s[1] = ' ';
	    }
	    else
	    {
                inpinfo->s_keystroke[len].uch = uch.uch;
	    }
            inpinfo->s_keystroke[len+1].uch = (uchar_t)0;
            inpinfo->keystroke_len ++;
            len ++;
	}
	else
	{
	    iccf->keystroke[stroke_idx] = keystr[0];
	    inpinfo->s_keystroke[stroke_idx].uch = uch.uch;
	}

	/* 如果是注音輸入法的話，要依據注音符號規則排列 */
	if (bopomofo_idx >= 0)
	{
	    int idx;
	    int keystroke_len = inpinfo->keystroke_len;
	    int sort_tbl[4] = {-1, -1, -1, -1};
	    /* 兩個字根以上才需要檢查 */
	    if (keystroke_len > 1)
	    {
		/* 取得每個注音符號的性質位置*/
		for (idx=0 ; idx < keystroke_len ; idx++)
		{
		    int b_idx = get_bopomofo_index(inpinfo->s_keystroke[idx]);
		    sort_tbl[idx] = b_idx < 0 ? 9 : b_idx;
		}
		for (idx=0 ; idx < keystroke_len-1 ; idx++)
		{
		    int x;
		    for (x=idx+1 ; x < keystroke_len ; x++)
		    {
			if (sort_tbl[idx] > sort_tbl[x])
			{
			    int tmp = sort_tbl[idx];
			    sort_tbl[idx] = sort_tbl[x];
			    sort_tbl[x] = tmp;

			    uch_t tmp_uch = inpinfo->s_keystroke[idx];
			    inpinfo->s_keystroke[idx] = inpinfo->s_keystroke[x];
			    inpinfo->s_keystroke[x] = tmp_uch;

			    char tmp_ch = iccf->keystroke[idx];
			    iccf->keystroke[idx] = iccf->keystroke[x];
			    iccf->keystroke[x] = tmp_ch;
			}
		    }
		}
	    }
	}

	if ((cf->mode & INP_MODE_SPACEIGNOR) && len == max_len)
	    iccf->mode |= INPINFO_MODE_SPACE;
	if ((cf->mode & INP_MODE_ENDKEY) && len>1 &&
		 (s=strchr(cf->header.endkey, keystr[0])))
	    return commit_keystroke(cf, inpinfo, iccf);
	else if ((cf->mode & INP_MODE_AUTOFULLUP) && len == max_len)
	    return commit_keystroke(cf, inpinfo, iccf);
	else if ((cf->mode & INP_MODE_AUTOCOMPOSE))
	    match_keystroke(cf, inpinfo, iccf);

	return ret;
    }

    return IMKEY_IGNORE;
}

/*----------------------------------------------------------------------------

	gen_inp_show_keystroke()

----------------------------------------------------------------------------*/

static int 
gen_inp_show_keystroke(void *conf, simdinfo_t *simdinfo)
{
#if 0
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf;
    int i, idx;
    uchar_t tmp;
    char *k, keystroke[INP_CODE_LENGTH+1];
    static uch_t keystroke_list[INP_CODE_LENGTH+1];

    if (cf->header.icode_mode == ICODE_MODE1)
	oxim_codes2keys(&(cf->ic1[idx]), 1, keystroke, SELECT_KEY_LENGTH+1);
    else if (cf->header.icode_mode == ICODE_MODE2) {
        unsigned int klist[2];

	klist[0] = cf->ic1[idx];
	klist[1] = cf->ic2[idx];
	oxim_codes2keys(klist, 2, keystroke, SELECT_KEY_LENGTH+1);
    }
    for (i=0, k=keystroke; i<INP_CODE_LENGTH && *k; i++, k++) {
	idx = oxim_key2code(*k);
	if ((tmp = cf->header.keyname[idx].uch))
	    keystroke_list[i].uch = tmp;
	else {
	    keystroke_list[i].uch = (uchar_t)0;
	    keystroke_list[i].s[0] = '?';
	}
    }
    keystroke_list[i].uch = (uchar_t)0;
    simdinfo->s_keystroke = keystroke_list;

    return (i) ? True : False;
#else
    simdinfo->s_keystroke = NULL;
    return False;
#endif
}

static int
gen_inp_terminate(void *conf)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf;

    if (cf->tabfn)
    {
	free(cf->tabfn);
	cf->tabfn = NULL;
    }

    if (cf->ic1)
    {
	free(cf->ic1);
	cf->ic1 = NULL;
    }
    if (cf->ic2)
    {
	free(cf->ic2);
	cf->ic2 = NULL;
    }
    if (cf->ichar)
    {
	free(cf->ichar);
	cf->ichar = NULL;
    }
    if (cf->zfp)
    {
	gzclose(cf->zfp);
	cf->zfp = NULL;
    }
    return True;
}
/*----------------------------------------------------------------------------

        Definition of general input method module (template).

----------------------------------------------------------------------------*/

static char *gen_inp_valid_objname[] = { "*", NULL };

static char gen_inp_comments[] = N_(
	"This is the general input method module. It is the most generic\n"
	"module which can read the IM table (\".tab\" file) and perform\n"
	"the correct operations of the specific IM. It also can accept a\n"
	"lot of options from the \"oximrc\" file to detailly control the\n"
	"behavior of the IM. We hope that this module could match most of\n"
	"your Chinese input requirements.\n\n"
	"This module is free software, as part of oxim system.\n");

module_t module_ptr = {
    { MTYPE_IM,					/* module_type */
      "gen_inp",				/* name */
      MODULE_VERSION,				/* version */
      gen_inp_comments },			/* comments */
    gen_inp_valid_objname,			/* valid_objname */
    sizeof(gen_inp_conf_t),			/* conf_size */
    gen_inp_init,				/* init */
    gen_inp_xim_init,				/* xim_init */
    gen_inp_xim_end,				/* xim_end */
    gen_inp_keystroke,				/* keystroke */
    gen_inp_show_keystroke,			/* show_keystroke */
    gen_inp_terminate				/* terminate */
};
