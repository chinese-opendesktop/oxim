#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define SIZEOF_LONG_LONG 8
#include "oximtool.h"
#include "module.h"

#include "imi_win.h"

#include <sunpinyin.h>
//#include "imi_view.h"
//#include "imi_uiobjects.h"

CWinHandler::CWinHandler(CIMIView* pv, inpinfo_t *inp_info) : mp_view(pv),
        CIMIWinHandler()
{
    m_iconv = iconv_open("UTF-8", TWCHAR_ICONV_NAME);
    inpinfo = inp_info;
    m_buf[0] = '\0';
    m_commit[0] = '\0';
}

CWinHandler::~CWinHandler()
{
    iconv_close(m_iconv);
}

CIMIView *
CWinHandler::getView()
{
    return mp_view;
}

/*void
CWinHandler::updateStatus(int key, int value)
{
    switch (key) {
    case STATUS_ID_CN:          switchCN( value != 0 ); break;
    case STATUS_ID_FULLPUNC:    switchFullPunc( value != 0 ); break;
    case STATUS_ID_FULLSYMBOL:  switchFullSimbol( value != 0 ); break;
    }
}
*/

void
CWinHandler::commit(const TWCHAR* wstr)
{
    wstring cand_str;
    
    cand_str = wstr;
    TIConvSrcPtr src = (TIConvSrcPtr)(cand_str.c_str());
    size_t srclen = (cand_str.size()+1)*sizeof(TWCHAR);
    char * dst = m_commit;
    size_t dstlen = sizeof(m_commit) - 1;
    iconv(m_iconv, &src, &srclen, &dst, &dstlen);
#ifdef DEBUG
    printf("commit: %s\n", m_commit);
#endif
}

void
CWinHandler::updatePreedit(const IPreeditString* ppd)
{
    int i;
    TIConvSrcPtr src = (TIConvSrcPtr)(ppd->string());
    size_t srclen = (ppd->size()+1)*sizeof(TWCHAR);
    char * dst = m_buf;
    size_t dstlen = sizeof(m_buf)-1;
    iconv(m_iconv, &src, &srclen, &dst, &dstlen);
    
    for(i=0; i<strlen(m_buf); i++)
    {
	inpinfo->s_keystroke[i].uch = (uchar_t)0;
	inpinfo->s_keystroke[i].s[0] = (char)m_buf[i];
    }
    inpinfo->s_keystroke[i].uch = (uchar_t)0;
    inpinfo->keystroke_len = i;
}

void
CWinHandler::updateCandidates(const ICandidateList* pcl)
{
#ifdef DEBUG
    for (int i=0, sz=pcl->size(); i < sz; ++i) {
    const TWCHAR* pcand = pcl->candiString(i);
    if (pcand != NULL) {
	printf("%c. ", '1' + i);
	print_wide(pcand);
	printf("\n");
    }
    }
#endif
    wstring cand_str;
    inpinfo->mcch_grouping[0] = 0;
    int len = 0;

    for (int i=0, sz=pcl->size(); i < sz; ++i) {
        const TWCHAR* pcand = pcl->candiString(i);
        if (pcand == NULL) break;
	
        cand_str = pcand;
	TIConvSrcPtr src = (TIConvSrcPtr)(cand_str.c_str());
	size_t srclen = (cand_str.size()+1)*sizeof(TWCHAR);
	char * dst = m_buf;
	size_t dstlen = sizeof(m_buf) - 1;
	iconv(m_iconv, &src, &srclen, &dst, &dstlen);
	
	char *output = m_buf;
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

// 	if (nchars > 1)
	    inpinfo->mcch_grouping[0] ++; 
   
    }
    
    int pageSize = inpinfo->n_selkey;
    // set pgstate
    if( pcl->total() < pageSize || 0 == pcl->total()) 
    {
        inpinfo->mcch_pgstate = MCCH_ONEPG;
    }
    else 
    {
	int nPage = round(pcl->total()/pageSize) + 1;
	int pageNo;
	
	pageNo = 0 == pcl->first() ? 1 : pcl->first()/pageSize + 1;

        if( pageNo == 1 )
            inpinfo->mcch_pgstate = MCCH_BEGIN;
        else if( pageNo == nPage/* - 1*/)
            inpinfo->mcch_pgstate = MCCH_END;
        else
            inpinfo->mcch_pgstate = MCCH_MIDDLE;
    }

    inpinfo->n_mcch = len ;
}

char *
CWinHandler::getBuffer()
{
    return m_commit;
}

void
CWinHandler::clearBuffer()
{
    m_commit[0] = '\0';
}
