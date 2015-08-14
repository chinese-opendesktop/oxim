//Added by qt3to4:
#include <QKeyEvent>
#include <QEvent>
#include <Q3CString>
/****************************************************************************
** $Id: qoximinputcontext.h,v 1.6 2006/07/26 07:04:45 firefly Exp $
**
** Definition of QXIMInputContext
**
** Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the input method module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef QXIMINPUTCONTEXT_H
#define QXIMINPUTCONTEXT_H


//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//
//

#if !defined(Q_NO_IM)

#include "QtCore/qglobal.h"
#include <QtGui/qinputcontext.h>
//#include <qcstring.h>

class QKeyEvent;
class QWidget;
class QFont;
class QString;


#ifdef Q_WS_X11
#include "q3memarray.h"

#include "QtGui/qwindowdefs.h"
//#include <private/qt_x11_p.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>

#endif

class QXIMInputContext : public QInputContext
{
    Q_OBJECT
public:
#ifdef Q_WS_X11
    QXIMInputContext();
    ~QXIMInputContext();

    QString identifierName();
//    QString language();

    bool x11FilterEvent( QWidget *keywidget, XEvent *event );
    void reset();

    void setFocus();
    void unsetFocus();
    void setMicroFocus( int x, int y, int w, int h, QFont *f = 0);
    void mouseHandler( int x, QEvent::Type type,
		       Qt::ButtonState button, Qt::ButtonState state );
//    bool isPreeditRelocationEnabled();

    void setHolderWidget( QWidget *widget );

    bool hasFocus() const;
    void resetClientState();
    void close( const QString &errMsg );

    void sendIMEvent( QEvent::Type type,
		      const QString &text = QString::null,
		      int cursorPosition = -1, int selLength = 0 );

    static void init_xim();
    static void create_xim();
    static void close_xim();

    void *ic;
    QString composingText;
    Q3MemArray<bool> selectedChars;

protected:
//    QCString _language;

private:
    void setComposePosition(int, int);
    int lookupString(XKeyEvent *, Q3CString &, KeySym *, Status *) const;

#endif // Q_WS_X11
};


#endif //Q_NO_IM

#endif // QXIMINPUTCONTEXT_H
