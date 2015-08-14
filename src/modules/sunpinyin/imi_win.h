#ifndef IMI_WIN_H
#define IMI_WIN_H

//#include "portability.h"

// #include <X11/Xlib.h>
// #include <gtk/gtk.h>
// #include <gdk/gdk.h>
// #include <gdk/gdkkeysyms.h>
#include "module.h"
#include "imi_winHandler.h"

class CWinHandler : public CIMIWinHandler
{
public:
    CWinHandler(CIMIView* pv, inpinfo_t *inp_info);

    char * getBuffer();
    void clearBuffer();
    CIMIView * getView();
    
    /* Inherited methods implementation */
    /*@{*/
    virtual ~CWinHandler();

    /** commit a string, normally the converted result */
    virtual void commit(const TWCHAR* wstr);

    /** Update window's preedit area using a GTK widget. */
    virtual void updatePreedit(const IPreeditString* ppd);

    /** Update window's candidates area using a GTK widget. */
    virtual void updateCandidates(const ICandidateList* pcl);

    /** Update status of current session using a GTK buttons. */
//     virtual void  updateStatus(int key, int value);
    
    /*@}*/
    
protected:
    iconv_t             m_iconv;

    /*@{*/
    CIMIView           *mp_view;

    /*@}*/

private:
    char m_buf[512];
    char m_commit[512];
    inpinfo_t *inpinfo;
};

#endif
