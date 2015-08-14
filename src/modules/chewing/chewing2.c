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

#include <iconv.h>

/* the following keystate masks are defined by oxim */
#define CAPS_MASK (2)
#define CTRL_MASK (4)

#define OXIM_BYTE_NATIVE 	2
#define OXIM_BYTE_UTF8 		3

static int chewing_codeset;
static int KeyMap = 0; /* 0:標準注音鍵盤 */
static int CapsLockMode  = 0; /* 0:小寫, 1:大寫 */
static char *selKey_define = "1234567890";

void preconvert(char *input, char *output, int n_char);
wchar_t convert(wchar_t input);

static int bAddPhraseForward = 0;
static uch_t etymon_list[N_KEYCODE];

int MakeInpinfo(inpinfo_t *inpinfo, ChewingOutput *pgo);

int CallSetConfig(inpinfo_t *inpinfo, ChewingData *pgdata)
{
    ConfigData config;
    int i;

    config.selectAreaLen = 100;
    config.maxChiSymbolLen = 16;
    config.bAddPhraseForward = bAddPhraseForward;

    for (i = 0; i < 10;i++)
        config.selKey[i] = selKey_define[i];

    SetConfig(pgdata, &config);
    return 0;
}

static int
ChewingInit(void *conf, char *objname)
{
    ChewingConf *cf = (ChewingConf *)conf ;

    /* TODO : 這裡要改為偵測新酷音詞庫存放路徑 */
    char *prefix = CHEWING_DATA_DIR;

    /* 
     * Because libchewing uses BIG-5 encoding for all its structure
     * so we need to check if it is UTF-8 locale and do any conv
     */
    chewing_codeset = OXIM_BYTE_UTF8;

    settings_t *im_settings = oxim_get_im_settings(objname);
    if (!im_settings)
    {
        printf("Null setting: %s !\n", objname);
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
	if (KeyMap < KB_DEFAULT && KeyMap > KB_HANYU_PINYING)
	{
	    KeyMap = KB_DEFAULT;
	}
    }

    cf->kb_type = KeyMap;

    char *pho_key = "1qaz2wsxedcrfv5tgbyhnujm8ik,9ol.0p;/-7634";
    char *pho_name[41] = {"ㄅ","ㄆ","ㄇ","ㄈ","ㄉ","ㄊ","ㄋ","ㄌ","ㄍ","ㄎ","ㄏ","ㄐ","ㄑ","ㄒ","ㄓ","ㄔ","ㄕ","ㄖ","ㄗ","ㄘ","ㄙ","ㄧ","ㄨ","ㄩ","ㄚ","ㄛ","ㄜ","ㄝ","ㄞ","ㄟ","ㄠ","ㄡ","ㄢ","ㄣ","ㄤ","ㄥ","ㄦ","˙","ˊ","ˇ","ˋ"};
    int keylen = strlen(pho_key), i, idx;

     for (i = 0 ; i < keylen ; i++)
    {
	idx = oxim_key2code(pho_key[i]);
	strcpy((char *)etymon_list[idx].s, pho_name[i]);
    }

    /* Initialize Chewing */
    ReadTree(prefix);
    InitChar(prefix);
    InitDict(prefix);
    ReadHash(prefix);

    oxim_settings_destroy(im_settings);
    return True;
}

static int 
ChewingXimInit(void *conf, inpinfo_t *inpinfo)
{
    static char cchBuffer[MAX_PHONE_SEQ_LEN];
    ChewingConf *cf = (ChewingConf *) conf;
    int i;

    inpinfo->iccf = (ChewingData *) calloc(1, sizeof(ChewingData));
    inpinfo->etymon_list = (cf->kb_type == KB_HANYU_PINYING) ? NULL : etymon_list;

    InitChewing(inpinfo->iccf, cf);
    CallSetConfig(inpinfo, (ChewingData *) inpinfo->iccf);

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

void CommitString(inpinfo_t *inpinfo, ChewingOutput *pgo)
{
    int i ;
    char *str;
    char *output;
    bzero(inpinfo->cch, sizeof(char)*MAX_PHONE_SEQ_LEN);
    str = (char *) calloc(MAX_PHONE_SEQ_LEN,sizeof(char));
    output = (char *) calloc(MAX_PHONE_SEQ_LEN / 2 * chewing_codeset, sizeof(char));
    for (i = 0; i < pgo->nCommitStr; i++)
        strcat(str, (const char *) pgo->commitStr[i].s);
    preconvert(str, output, strlen(str));
    strcpy(inpinfo->cch, output);
    free(str);
    free(output);
}

static unsigned int 
ChewingXimEnd(void *conf, inpinfo_t *inpinfo)
{
    ChewingOutput gOut ;
    int rtn ;

    /* if preedit exists, commit the string */
    OnKeyEnter(inpinfo->iccf, &gOut);

    rtn = MakeInpinfo(inpinfo, &gOut);
    free(inpinfo->iccf);
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

void ShowChoose(inpinfo_t *inpinfo, ChoiceInfo *pci, ChewingOutput *pgo)
{
    int i,no,k,len, kk;
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
        output = (char *) calloc(
                        strlen(pci->totalChoiceStr[no]) * chewing_codeset + 1, 
                        sizeof(char));
        preconvert(pci->totalChoiceStr[no], output, strlen(pci->totalChoiceStr[no]));
        // for each char in the phrase, copy to mcch
        for (k = 0, kk = 0; output[k] != '\0'; k += chewing_codeset, kk++)
	{
	    memcpy(inpinfo->mcch[len++].s, &(output[k]), chewing_codeset) ;
	}
        free(output);
        // set grouping to the length of the phrase
        inpinfo->mcch_grouping[i+1] = kk;

	if (kk > 1)
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

void ShowText(inpinfo_t *inpinfo, ChewingOutput *pgo)
{
    int i;
    bzero(inpinfo->lcch, sizeof(uch_t)*MAX_PHONE_SEQ_LEN) ;
    for (i = 0; i < pgo->chiSymbolBufLen; i++)
    {
	pgo->chiSymbolBuf[i].wch = convert(pgo->chiSymbolBuf[i].wch);
	strcpy((char *)inpinfo->lcch[i].s, (char *)pgo->chiSymbolBuf[i].s);
    }
    inpinfo->n_lcch = pgo->chiSymbolBufLen ;
}

void ShowInterval(inpinfo_t *inpinfo, ChewingOutput *pgo)
{
    int i, k, begin, len, count, nGroup ;
    int label[MAX_PHONE_SEQ_LEN] ;

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

void ShowStateAndZuin(inpinfo_t *inpinfo, ChewingOutput *pgo)
{
    int i, a , len = 0;

    bzero( inpinfo->s_keystroke, sizeof(uch_t)*(3 + MAX_PHRASE_LEN));
    ChewingData *iccf = inpinfo->iccf;	

    if(pgo->bShowMsg)
    {
        for(i = 0; i < pgo->showMsgLen; i++)
	{
	    inpinfo->s_keystroke[i].uch = (uchar_t)convert(pgo->showMsg[i].wch);
	}
	inpinfo->keystroke_len = pgo->showMsgLen ;
    }
    else
    {
	// 漢語拼音 
	if (iccf->zuinData.kbtype == KB_HANYU_PINYING)
	{
	   for (i=0 ; i < PINYING_SIZE; i++)
	   {
		if (iccf->zuinData.pinYingData.keySeq[i] != '\0')
		{
		    inpinfo->s_keystroke[i].uch = iccf->zuinData.pinYingData.keySeq[i];
		}
	   } 
	   inpinfo->keystroke_len = strlen(iccf->zuinData.pinYingData.keySeq);
	}
	else
	{
	    for(i=0,a=0; i<ZUIN_SIZE; i++)
	    {
		if(pgo->zuinBuf[i].s[0] != '\0')
		{
		    inpinfo->s_keystroke[a++].uch = (uchar_t)convert(pgo->zuinBuf[i].wch);
		    ++ len;
		}
	    }
	    inpinfo->keystroke_len = len;
	}
    }
}

void SetCursor(inpinfo_t *inpinfo, ChewingOutput *pgo)
{
    inpinfo->edit_pos = pgo->chiSymbolCursor;
}

int MakeInpinfo(inpinfo_t *inpinfo, ChewingOutput *pgo)
{
    int rtnValue = 0 ;

    if(pgo->keystrokeRtn & KEYSTROKE_ABSORB)
        rtnValue |= IMKEY_ABSORB;
    if(pgo->keystrokeRtn & KEYSTROKE_IGNORE)
        rtnValue |= IMKEY_IGNORE;
    if(pgo->keystrokeRtn & KEYSTROKE_COMMIT) {
        rtnValue |= IMKEY_COMMIT;
        CommitString(inpinfo, pgo);
    }

    if(pgo->pci->nPage != 0) { // in selection mode
        ShowChoose(inpinfo,pgo->pci,pgo);
        inpinfo->guimode &= ~GUIMOD_LISTCHAR; 
    }
    else { // not in selection mode
        ShowText(inpinfo, pgo);
        ShowInterval(inpinfo, pgo);
        inpinfo->guimode |= GUIMOD_LISTCHAR;
    }
    ShowStateAndZuin(inpinfo, pgo);
    SetCursor(inpinfo, pgo);
    return rtnValue;
}

static unsigned int
ChewingKeystroke(void *conf, inpinfo_t *inpinfo, keyinfo_t *keyinfo)
{
    KeySym keysym = keyinfo->keysym;
    ChewingOutput gOut ;
    int rtn ;
    static KeySym last_key = 0;

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
    if( keyinfo->keystate & CAPS_MASK ) { // uppercase
            SetChiEngMode( inpinfo->iccf, SYMBOL_MODE);
    }
    else {  // lower case 
            SetChiEngMode( inpinfo->iccf, CHINESE_MODE);
    }

    // check no ctrl key was pressed
    if(keyinfo->keystate >= 0 && !(keyinfo->keystate & CTRL_MASK ) && !(keyinfo->keystate & ShiftMask) ) {
        switch(keysym) {
            case XK_Escape:
                OnKeyEsc(inpinfo->iccf, &gOut) ;
                inpinfo->n_mcch = 0;
                break ;
            case XK_Return:
                OnKeyEnter(inpinfo->iccf, &gOut) ;
                inpinfo->n_mcch = 0;
                break ;
            case XK_Delete:
                OnKeyDel(inpinfo->iccf, &gOut) ;
		break ;
            case XK_BackSpace:
                OnKeyBackspace(inpinfo->iccf, &gOut) ;
                break ;
            case XK_Up:
                OnKeyUp(inpinfo->iccf, &gOut);
                break ;
            case XK_Down:
                OnKeyDown(inpinfo->iccf, &gOut) ;
                break ;
            case XK_Left:
                OnKeyLeft(inpinfo->iccf, &gOut) ;
                break ;
            case XK_Right:
                OnKeyRight(inpinfo->iccf, &gOut) ;
                break ;
            case XK_Home:
                OnKeyHome(inpinfo->iccf, &gOut);
		break;
	    case XK_End:
                OnKeyEnd(inpinfo->iccf, &gOut);
                break;
            case XK_Tab:
                if (last_key == XK_Tab) // double click TAB
                    OnKeyDblTab(inpinfo->iccf, &gOut);
                else
                    OnKeyTab(inpinfo->iccf, &gOut);
                break;
            case XK_Caps_Lock:
                OnKeyCapslock(inpinfo->iccf, &gOut);
                break;
            case ' ': /* Space */
                OnKeySpace(inpinfo->iccf, &gOut);
                break;
            default:
                OnKeyDefault(inpinfo->iccf, keysym, &gOut);
                inpinfo->n_mcch = 0;
                break;
        }
    }
    else if (keyinfo->keystate & ShiftMask) {
	switch ( keysym ) {
	    case XK_Left:
		OnKeyShiftLeft(inpinfo->iccf, &gOut) ;
                break ;
            case XK_Right:
                OnKeyShiftRight(inpinfo->iccf, &gOut) ;
                break;
            default:
                OnKeyDefault(inpinfo->iccf, keysym, &gOut);
                inpinfo->n_mcch = 0;
            }
    }
    else if (keyinfo->keystate & CTRL_MASK) {  // Ctrl-key Mask
	// We need to fill the 'gOut' variable for output.
	    if (keysym <= '9' && keysym >= '0') 
		OnKeyCtrlNum(inpinfo->iccf,keysym,&gOut);
            else 
                OnKeyCtrlOption(inpinfo->iccf, keysym - 'a' + 1, &gOut);
    }

    last_key = keysym;
    rtn = MakeInpinfo(inpinfo, &gOut);
    return rtn ;
}

static int 
ChewingShowKeystroke(void *conf, simdinfo_t *simdinfo)
{
    simdinfo->s_keystroke = NULL;
    return False;
}                              

/* UTF-8 support */
void
preconvert(char *input, char *output, int n_char)
{
    const char *inptr = input;
    size_t inbytesleft = n_char;
    size_t outbytesleft = n_char / 2 * 3 + 1;

    char *outptr = output;
    iconv_t cd;

    cd = iconv_open("UTF-8", "BIG-5");
    iconv (cd, (char **)&inptr, &inbytesleft, &outptr, &outbytesleft);

    iconv_close(cd);
}

wchar_t
convert(wchar_t input)
{
    wch_t output;
    wch_t temp;

    output.wch = (wchar_t)0;
    temp.wch = input;
    const char *inptr = (char *)temp.s;
    size_t inbytesleft = 2;
    size_t outbytesleft = 3;
    char *outptr = (char *)output.s;
    iconv_t cd;
    cd = iconv_open("UTF-8", "BIG-5");
    iconv (cd, (char **)&inptr, &inbytesleft, &outptr, &outbytesleft);
    iconv_close(cd);
    return output.wch;
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
    sizeof(ChewingConf),		/* conf_size */
    ChewingInit,			/* init */
    ChewingXimInit,			/* xim_init */
    ChewingXimEnd,			/* xim_end */
    ChewingKeystroke,			/* keystroke */
    ChewingShowKeystroke,		/* show_keystroke */
    ChewingTerminate			/* terminate */
};
