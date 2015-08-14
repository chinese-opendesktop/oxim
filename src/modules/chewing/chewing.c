/*
 * Bridge interface between libchewing and oxim
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <chewing/chewing.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "oximtool.h"
#include "module.h"

/* the following keystate masks are defined by oxim */
#define CAPS_MASK (2)
#define CTRL_MASK (4)

static int KeyMap = 0; /* 0:標準注音鍵盤 */
static int CapsLockMode  = 0; /* 0:小寫, 1:大寫 */
static char *selKey_define = "1234567890";
static uch_t etymon_list[N_KEYCODE];

// 新酷音 0.3.091 把結構內的名稱改了，只好用偽造的代替
typedef struct {
        int selectAreaLen; // 0.3.091 為 int candPerPage; orz
        int maxChiSymbolLen;
        int selKey[ MAX_SELKEY ];
        int bAddPhraseForward;
        int bSpaceAsSelection;
        int bEscCleanAllBuf;
        /** @brief
            HSU_SELKEY_TYPE1 = asdfjkl789,
            HSU_SELKEY_TYPE2 = asdfzxcv89.
         */
        int hsuSelKeyType;
} artChewingConfigData;


int MakeInpinfo(inpinfo_t *inpinfo);

int CallSetConfig(inpinfo_t *inpinfo, ChewingContext *ctx)
{
    artChewingConfigData config;
    int i;

    config.selectAreaLen = 0xffff;
    config.maxChiSymbolLen = 20;
    config.bSpaceAsSelection = True;

    for (i = 0; i < 10;i++)
    {
        config.selKey[i] = selKey_define[i];
    }

    ChewingConfigData *cc = (artChewingConfigData *)&config;
    chewing_Configure(ctx, cc);
    return 0;
}

static int
ChewingInit(void *context, char *objname)
{
    ChewingContext *ctx = (ChewingContext *)context ;

    settings_t *im_settings = oxim_get_im_settings(objname);
    if (!im_settings)
    {
        printf("沒有 %s 的設定!\n", objname);
        return False;
    }

    int SelectionKeys;
    if (oxim_setting_GetInteger(im_settings, "SelectionKeys", &SelectionKeys))
    {
	switch (SelectionKeys)
	{
	    case 1:
		selKey_define = "qwertyuiop";
		break;
	    case 2:
		selKey_define = "asdfghjkl;";
		break;
	    case 3:
		selKey_define = "zxcvbnm,./";
		break;
	}
    }

    if (!oxim_setting_GetInteger(im_settings, "CapsLockMode", &CapsLockMode))
	CapsLockMode = 0;

    if (oxim_setting_GetInteger(im_settings, "KeyMap", &KeyMap))
    {
	if (KeyMap < KB_DEFAULT && KeyMap > KB_HANYU_PINYIN)
	{
	    KeyMap = KB_DEFAULT;
	}
    }

    char *pho_key = "1qaz2wsxedcrfv5tgbyhnujm8ik,9ol.0p;/-7634";
    char *pho_name[41] = {"ㄅ","ㄆ","ㄇ","ㄈ","ㄉ","ㄊ","ㄋ","ㄌ","ㄍ","ㄎ","ㄏ","ㄐ","ㄑ","ㄒ","ㄓ","ㄔ","ㄕ","ㄖ","ㄗ","ㄘ","ㄙ","ㄧ","ㄨ","ㄩ","ㄚ","ㄛ","ㄜ","ㄝ","ㄞ","ㄟ","ㄠ","ㄡ","ㄢ","ㄣ","ㄤ","ㄥ","ㄦ","˙","ˊ","ˇ","ˋ"};
    int keylen = strlen(pho_key), i, idx;

     for (i = 0 ; i < keylen ; i++)
    {
	idx = oxim_key2code(pho_key[i]);
	strcpy((char *)etymon_list[idx].s, pho_name[i]);
    }

    /* Initialize Chewing */
    chewing_Init(CHEWING_DATA_DIR, NULL);

    oxim_settings_destroy(im_settings);
    return True;
}

static int 
ChewingXimInit(void *conf, inpinfo_t *inpinfo)
{
    static char cchBuffer[MAX_PHONE_SEQ_LEN * MAX_UTF8_SIZE + 1];
    ChewingContext *ctx = (ChewingContext *) conf;
    int i;

    inpinfo->iccf = chewing_new();

    CallSetConfig(inpinfo, (ChewingContext *) inpinfo->iccf);

    chewing_set_KBType( inpinfo->iccf, KeyMap );
    inpinfo->etymon_list = (KeyMap == KB_HANYU_PINYIN) ? NULL : etymon_list;

    inpinfo->lcch = (uch_t *) calloc(MAX_PHONE_SEQ_LEN, sizeof(uch_t));
    inpinfo->lcch_grouping = (ubyte_t *) calloc(MAX_PHONE_SEQ_LEN, sizeof(ubyte_t));
    inpinfo->cch = cchBuffer;

    inpinfo->guimode = GUIMOD_LISTCHAR | GUIMOD_SELKEYSPOT;
    inpinfo->keystroke_len = 0;
    inpinfo->s_keystroke = (uch_t *) calloc(3 + MAX_PHRASE_LEN, sizeof(uch_t));

    inpinfo->mcch = (uch_t *) calloc(MAX_CHOICE_BUF, sizeof(uch_t));
    inpinfo->mcch_grouping = calloc( MAX_SELKEY, sizeof(uint_t));
    inpinfo->n_mcch = 0;

    inpinfo->n_lcch = 0;
    inpinfo->edit_pos = 0;
    inpinfo->cch_publish.uch = (uchar_t) 0;

    inpinfo->n_selkey = 10;
    inpinfo->s_selkey = (uch_t *) calloc(MAX_SELKEY, sizeof(uch_t));

    for (i = 0; i < 10; i++)
    {
	inpinfo->s_selkey[i].uch = (uchar_t) 0;
	inpinfo->s_selkey[i].s[0] = selKey_define[i];
    }
    return True;
}

void CommitString(inpinfo_t *inpinfo)
{
    int i ;
    ChewingOutput *pgo = ((ChewingContext *) inpinfo->iccf)->output;

    inpinfo->cch[0] = '\0';
    for (i = 0; i < pgo->nCommitStr; i++)
    {
        strcat(inpinfo->cch, (const char *) pgo->commitStr[i].s);
    }
}

static unsigned int 
ChewingXimEnd(void *conf, inpinfo_t *inpinfo)
{
    int rtn ;

    /* if preedit exists, commit the string */
    chewing_handle_Enter(inpinfo->iccf);

    rtn = MakeInpinfo(inpinfo);
    chewing_free((ChewingContext*) inpinfo->iccf);
    free(inpinfo->s_keystroke);
    free(inpinfo->lcch);
    free(inpinfo->mcch);
    free(inpinfo->mcch_grouping);
    free(inpinfo->s_selkey);

    inpinfo->iccf = NULL;
    inpinfo->s_keystroke = NULL;
    inpinfo->lcch = NULL;
    inpinfo->mcch = NULL;
    inpinfo->mcch_grouping = NULL;
    inpinfo->s_selkey = NULL;

    return rtn ;
}      

void ShowChoose(inpinfo_t *inpinfo, ChoiceInfo *pci)
{
    int i,no,len;
    char *output;

    // for new oxim, there is no need to modify the lcch buffer
    // instead, we put phrase to choose in mcch
    no = pci->pageNo * pci->nChoicePerPage;
    len = 0;

    inpinfo->mcch_grouping[0] = 0;
    for (i = 0;i < pci->nChoicePerPage; no++,i++)
    {
        // in the last page, no will exceed nTotalChoice
        if( no >= pci->nTotalChoice ) 
            break;

	output = pci->totalChoiceStr[no];

	unsigned int ucs4, olen = strlen(output);
	int nbytes, nchars = 0;
        /* for each char in the phrase, copy to mcch */
	while (olen && (nbytes = oxim_utf8_to_ucs4(output, &ucs4, olen)) > 0)
	{
	    inpinfo->mcch[len].uch = (uchar_t)0;
	    memcpy(inpinfo->mcch[len++].s, output, nbytes);
	    nchars ++;
	    output += nbytes;
	    olen -= nbytes;
	}

        /* set grouping to the length of the phrase */
        inpinfo->mcch_grouping[i+1] = nchars;

	if (nchars > 1)
	    inpinfo->mcch_grouping[0] ++; 
    }

    // set pgstate according to pci->pageNo & pci->nPage
    if( pci->nPage == 1) {
        inpinfo->mcch_pgstate = MCCH_ONEPG;
    }
    else {
        if( pci->pageNo == 0 )
            inpinfo->mcch_pgstate = MCCH_BEGIN;
        else if( pci->pageNo == pci->nPage - 1)
            inpinfo->mcch_pgstate = MCCH_END;
        else
            inpinfo->mcch_pgstate = MCCH_MIDDLE;
    }

    inpinfo->n_mcch = len ;
}

void ShowText(inpinfo_t *inpinfo)
{
    int i;
    ChewingOutput *pgo = ((ChewingContext *) inpinfo->iccf)->output;
    bzero(inpinfo->lcch, sizeof(uch_t)*MAX_PHONE_SEQ_LEN) ;
    for (i = 0; i < pgo->chiSymbolBufLen; i++)
    {
	strcpy((char *)inpinfo->lcch[i].s, (char *)pgo->chiSymbolBuf[i].s);
    }
    inpinfo->n_lcch = pgo->chiSymbolBufLen ;
}

void ShowInterval(inpinfo_t *inpinfo)
{
    int i, k, begin, len, count, nGroup ;
    int label[MAX_PHONE_SEQ_LEN] ;
    ChewingOutput *pgo = ((ChewingContext *) inpinfo->iccf)->output;

    if( pgo->chiSymbolBufLen == 0) {
        inpinfo->lcch_grouping[0] = 0 ;
        return ;
    }

    // set label
    for(count=0; count<pgo->chiSymbolBufLen; count++)
    {
        label[count] = count;
    }

    for(i=0; i<pgo->nDispInterval; i++)
    {
        len = pgo->dispInterval[i].to - pgo->dispInterval[i].from ;
        if( len > 1)
	{
            for(k=pgo->dispInterval[i].from; k<pgo->dispInterval[i].to; k++)
	    {
                label[k] = count ;
	    }
	    count++ ;
	}
    }

        // begin to set lcch_grouping by the label
        nGroup = 0 ;
        begin = 0 ;
        for(i=1; i<pgo->chiSymbolBufLen; i++) {
                if( label[i] != label[begin] ) {
                        inpinfo->lcch_grouping[++nGroup] = ( i - begin ) ;
                        begin = i ;
                }
        }
        inpinfo->lcch_grouping[++nGroup] = ( i - begin ) ;
        inpinfo->lcch_grouping[0] = nGroup ;
}

void ShowStateAndZuin(inpinfo_t *inpinfo)
{
    int i, a , len = 0;
    ChewingOutput *pgo = ((ChewingContext *) inpinfo->iccf)->output;

    bzero( inpinfo->s_keystroke, sizeof(uch_t)*(3 + MAX_PHRASE_LEN));
    ChewingContext *iccf = inpinfo->iccf;	

    if(pgo->bShowMsg)
    {
        for(i = 0; i < pgo->showMsgLen; i++)
	{
	    inpinfo->s_keystroke[i].uch = (uchar_t)(pgo->showMsg[i].wch);
	}
	inpinfo->keystroke_len = pgo->showMsgLen ;
    }
    else
    {
	// 漢語拼音 
	if (iccf->data->zuinData.kbtype == KB_HANYU_PINYIN)
	{
	   for (i=0 ; i < PINYIN_SIZE; i++)
	   {
		if (iccf->data->zuinData.pinYinData.keySeq[i] != '\0')
		{
		    inpinfo->s_keystroke[i].uch = iccf->data->zuinData.pinYinData.keySeq[i];
		}
	   } 
	   inpinfo->keystroke_len = strlen(iccf->data->zuinData.pinYinData.keySeq);
	}
	else
	{
	    for(i=0,a=0; i<ZUIN_SIZE; i++)
	    {
		if(pgo->zuinBuf[i].s[0] != '\0')
		{
		    inpinfo->s_keystroke[a++].uch = (uchar_t)(pgo->zuinBuf[i].wch);
		    ++ len;
		}
	    }
	    inpinfo->keystroke_len = len;
	}
    }
}

void SetCursor(inpinfo_t *inpinfo)
{
    ChewingOutput *pgo = ((ChewingContext *) inpinfo->iccf)->output;
    inpinfo->edit_pos = pgo->chiSymbolCursor;
}

int MakeInpinfo(inpinfo_t *inpinfo)
{
    ChewingOutput *pgo = ((ChewingContext *) inpinfo->iccf)->output;
    int rtnValue = 0 ;

    if(pgo->keystrokeRtn & KEYSTROKE_ABSORB)
        rtnValue |= IMKEY_ABSORB;
    if(pgo->keystrokeRtn & KEYSTROKE_IGNORE)
        rtnValue |= IMKEY_IGNORE;
    if(pgo->keystrokeRtn & KEYSTROKE_COMMIT) {
        rtnValue |= IMKEY_COMMIT;
        CommitString(inpinfo);
    }

    if(pgo->pci->nPage != 0) { // in selection mode
        ShowChoose(inpinfo,pgo->pci);
        inpinfo->guimode &= ~GUIMOD_LISTCHAR; 
    }
    else { // not in selection mode
        ShowText(inpinfo);
        ShowInterval(inpinfo);
        inpinfo->guimode |= GUIMOD_LISTCHAR;
    }
    ShowStateAndZuin(inpinfo);
    SetCursor(inpinfo);
    return rtnValue;
}

static unsigned int
ChewingKeystroke(void *conf, inpinfo_t *inpinfo, keyinfo_t *keyinfo)
{
    KeySym keysym = keyinfo->keysym;
    int rtn ;

    /*
     * CapsLock 時，輸出小寫，加上 Shift 才輸出大寫。
     */
    if (CapsLockMode == 0 && keyinfo->keystate & LockMask)
    {
	if (keyinfo->keystate & ShiftMask)
	{
	    keysym = toupper(keysym);
	}
	else
	{
	    keysym = tolower(keysym);
	}
    }

    // set Chinese / English mode by keystate
    if( keyinfo->keystate & CAPS_MASK )
    {
            chewing_set_ChiEngMode( inpinfo->iccf, SYMBOL_MODE);
    }
    else
    { 
            chewing_set_ChiEngMode( inpinfo->iccf, CHINESE_MODE);
    }

    // check no ctrl key was pressed
    if(keyinfo->keystate >= 0 && !(keyinfo->keystate & CTRL_MASK ) && !(keyinfo->keystate & ShiftMask) ) {
        switch(keysym) {
            case XK_Escape:
                chewing_handle_Esc(inpinfo->iccf) ;
                inpinfo->n_mcch = 0;
                break ;
            case XK_Return:
                chewing_handle_Enter(inpinfo->iccf) ;
                inpinfo->n_mcch = 0;
                break ;
            case XK_Delete:
                chewing_handle_Del(inpinfo->iccf) ;
		break ;
            case XK_BackSpace:
                chewing_handle_Backspace(inpinfo->iccf) ;
                break ;
            case XK_Up:
                chewing_handle_Up(inpinfo->iccf);
                break ;
            case XK_Down:
                chewing_handle_Down(inpinfo->iccf) ;
                break ;
            case XK_Left:
                chewing_handle_Left(inpinfo->iccf) ;
                break ;
            case XK_Right:
                chewing_handle_Right(inpinfo->iccf) ;
                break ;
            case XK_Home:
                chewing_handle_Home(inpinfo->iccf);
		break;
	    case XK_End:
                chewing_handle_End(inpinfo->iccf);
                break;
            case XK_Tab:
                chewing_handle_Tab(inpinfo->iccf);
                break;
            case XK_Caps_Lock:
                chewing_handle_Capslock(inpinfo->iccf);
                break;
            case ' ': /* Space */
                chewing_handle_Space(inpinfo->iccf);
                break;
	    case XK_Insert:
	    case XK_Page_Up:
	    case XK_Page_Down:
		return IMKEY_IGNORE;
            default:
                chewing_handle_Default(inpinfo->iccf, (char) keysym );
                inpinfo->n_mcch = 0;
                break;
        }
    }
    else if (keyinfo->keystate & ShiftMask) {
	switch ( keysym ) {
	    case XK_Left:
		chewing_handle_ShiftLeft(inpinfo->iccf) ;
                break ;
            case XK_Right:
                chewing_handle_ShiftRight(inpinfo->iccf) ;
                break;
            case ' ':
                chewing_handle_ShiftSpace(inpinfo->iccf) ;
                break;
	    case XK_Insert:
	    case XK_Delete:
	    case XK_Home:
	    case XK_End:
	    case XK_Page_Up:
	    case XK_Page_Down:
	    case XK_Up:
	    case XK_Down:
		return IMKEY_IGNORE;
            default:
		chewing_handle_Default(inpinfo->iccf, (char) keysym);
		inpinfo->n_mcch = 0;
		break;
	}
    }
    else if (keyinfo->keystate & CTRL_MASK) {  // Ctrl-key Mask
	    if (keysym <= '9' && keysym >= '0') 
	    {
		chewing_handle_CtrlNum(inpinfo->iccf, (char) keysym);
	    }
	    else
	    {
		return IMKEY_IGNORE;
	    }
    }

    rtn = MakeInpinfo(inpinfo);
    return rtn ;
}

static int 
ChewingShowKeystroke(void *conf, simdinfo_t *simdinfo)
{
    simdinfo->s_keystroke = NULL;
    return False;
}                              

static int ChewingTerminate(void *conf)
{
    return True;
}

static char zh_chewing_comments[] = N_( 
    "Chewing : a smart phonetic input method module for OXIM.\n"
    "By Lu-chuan Kung <lckung@iis.sinica.edu.tw>,\n"
    "Kang-pen Chen <kpchen@iis.sinica.edu.tw>, and others.\n");

static char *zh_chewing_valid_objname[] = { "chewing", NULL };

module_t module_ptr = {
    { 
        MTYPE_IM,
        "Chewing",			/* name */
        MODULE_VERSION,			/* version */
        zh_chewing_comments 
    },					/* comments */
    zh_chewing_valid_objname,		/* valid_objname */
    sizeof(ChewingConfigData),		/* conf_size */
    ChewingInit,			/* init */
    ChewingXimInit,			/* xim_init */
    ChewingXimEnd,			/* xim_end */
    ChewingKeystroke,			/* keystroke */
    ChewingShowKeystroke,		/* show_keystroke */
    ChewingTerminate			/* terminate */
};
