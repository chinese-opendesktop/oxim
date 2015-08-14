/****************************************************************************
** $Id: qoximinputcontext_x11.cpp,v 1.10 2006/06/07 08:18:16 firefly Exp $
**
** Implementation of QXIMInputContext class
**
** Copyright (C) 2000-2003 Trolltech AS.  All rights reserved.
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
** licenses for Unix/X11 may use this file in accordance with the Qt Commercial
** License Agreement provided with the Software.
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


#include "qoximinputcontext.h"
//Added by qt3to4:
#include <Q3CString>
#include <QEvent>
#include <QInputMethodEvent>
#include <QInputContext>
#include "oximtool.h"
#include "langinfo.h"

const int XKeyPress = KeyPress;
const int XKeyRelease = KeyRelease;
#undef KeyPress
#undef KeyRelease

#if !defined(QT_NO_IM)

//#include "qplatformdefs.h"
#include "qapplication.h"
#include "qwidget.h"
#include "qstring.h"
#include "q3ptrlist.h"
#include "q3intdict.h"
#include "qtextcodec.h"

#include <stdlib.h>
#include <limits.h>

#if !defined(QT_NO_XIM)

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>

// #define QT_XIM_DEBUG

// from qapplication_x11.cpp
static XIM	qt_xim = 0;
extern char    *qt_ximServer;
static bool isInitXIM = FALSE;
static Q3PtrList<QXIMInputContext> *ximContextList = 0;
#endif
extern int qt_ximComposingKeycode;


#if !defined(QT_NO_XIM)

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif // Q_C_CALLBACKS

#ifdef USE_X11R6_XIM
    static void xim_create_callback(XIM /*im*/,
				    XPointer /*client_data*/,
				    XPointer /*call_data*/)
    {
	// qDebug("xim_create_callback");
	QXIMInputContext::create_xim();
    }

    static void xim_destroy_callback(XIM /*im*/,
				     XPointer /*client_data*/,
				     XPointer /*call_data*/)
    {
	// qDebug("xim_destroy_callback");
	QXIMInputContext::close_xim();
	Display *dpy = QPaintDevice::x11AppDisplay();
	XRegisterIMInstantiateCallback(dpy, 0, 0, 0,
				       (XIMProc) xim_create_callback, 0);
    }

#endif // USE_X11R6_XIM

#if defined(Q_C_CALLBACKS)
}
#endif // Q_C_CALLBACKS

#endif // QT_NO_XIM

#ifndef QT_NO_XIM

#ifdef Q_C_CALLBACKS
extern "C" {
#endif // Q_C_CALLBACKS

    // These static functions should be rewritten as member of
    // QXIMInputContext

    static int xic_start_callback(XIC, XPointer client_data, XPointer) {
	QXIMInputContext *qic = (QXIMInputContext *) client_data;
	if (! qic) {
#ifdef QT_XIM_DEBUG
	    qDebug("compose start: no qic");
#endif // QT_XIM_DEBUG

	    return 0;
	}

	qic->resetClientState();
	qic->sendIMEvent( QEvent::InputMethod );  //strat

#ifdef QT_XIM_DEBUG
	qDebug("compose start");
#endif // QT_XIM_DEBUG

	return 0;
    }

    static int xic_draw_callback(XIC, XPointer client_data, XPointer call_data) {
	QXIMInputContext *qic = (QXIMInputContext *) client_data;
	if (! qic) {
#ifdef QT_XIM_DEBUG
	    qDebug("compose event: invalid compose event %p", qic);
#endif // QT_XIM_DEBUG

	    return 0;
	}

	bool send_imstart = FALSE;
	if( ! qic->isComposing() && qic->hasFocus() ) {
	    qic->resetClientState();
	    send_imstart = TRUE;
	} else if ( ! qic->isComposing() || ! qic->hasFocus() ) {
#ifdef QT_XIM_DEBUG
	    qDebug( "compose event: invalid compose event composing=%d hasFocus=%d",
		    qic->isComposing(), qic->hasFocus() );
#endif // QT_XIM_DEBUG

	    return 0;
	}

	if ( send_imstart )
	    qic->sendIMEvent( QEvent::InputMethod );  //start

	XIMPreeditDrawCallbackStruct *drawstruct =
	    (XIMPreeditDrawCallbackStruct *) call_data;
	XIMText *text = (XIMText *) drawstruct->text;
	int cursor = drawstruct->caret, sellen = 0;

	if ( ! drawstruct->caret && ! drawstruct->chg_first &&
	     ! drawstruct->chg_length && ! text ) {
	    if( qic->composingText.isEmpty() ) {
#ifdef QT_XIM_DEBUG
		qDebug( "compose emptied" );
#endif // QT_XIM_DEBUG
		// if the composition string has been emptied, we need
		// to send an IMEnd event
		qic->sendIMEvent( QEvent::InputMethod ); //end
		qic->resetClientState();
		// if the commit string has coming after here, IMStart
		// will be sent dynamically
	    }
	    return 0;
	}

	if (text) {
	    char *str = 0;
	    if (text->encoding_is_wchar) {
		int l = wcstombs(NULL, text->string.wide_char, text->length);
		if (l != -1) {
		    str = new char[l + 1];
		    wcstombs(str, text->string.wide_char, l);
		    str[l] = 0;
		}
	    } else
		str = text->string.multi_byte;

	    if (! str)
		return 0;

	    QString s = QString::fromUtf8(str);

	    if (text->encoding_is_wchar)
		delete [] str;

	    if (drawstruct->chg_length < 0)
		qic->composingText.replace(drawstruct->chg_first, UINT_MAX, s);
	    else
		qic->composingText.replace(drawstruct->chg_first, drawstruct->chg_length, s);

	    if ( qic->selectedChars.size() < qic->composingText.length() ) {
		// expand the selectedChars array if the compose string is longer
		uint from = qic->selectedChars.size();
		qic->selectedChars.resize( qic->composingText.length() );
		for ( uint x = from; from < qic->selectedChars.size(); ++x )
		    qic->selectedChars[x] = 0;
	    }

	    uint x;
	    bool *p = qic->selectedChars.data() + drawstruct->chg_first;
	    // determine if the changed chars are selected based on text->feedback
	    for ( x = 0; x < s.length(); ++x )
		*p++ = ( text->feedback ? ( text->feedback[x] & XIMReverse ) : 0 );

	    // figure out where the selection starts, and how long it is
	    p = qic->selectedChars.data();
	    bool started = FALSE;
	    for ( x = 0; x < qic->selectedChars.size(); ++x ) {
		if ( started ) {
		    if ( *p ) ++sellen;
		    else break;
		} else {
		    if ( *p ) {
			cursor = x;
			started = TRUE;
			sellen = 1;
		    }
		}
		++p;
	    }
	} else {
	    if (drawstruct->chg_length == 0)
		drawstruct->chg_length = -1;

	    qic->composingText.remove(drawstruct->chg_first, drawstruct->chg_length);
	    bool qt_compose_emptied = qic->composingText.isEmpty();
	    if ( qt_compose_emptied ) {
#ifdef QT_XIM_DEBUG
		qDebug( "compose emptied" );
#endif // QT_XIM_DEBUG
		// if the composition string has been emptied, we need
		// to send an IMEnd event
		qic->sendIMEvent( QEvent::InputMethod );  //end
		qic->resetClientState();
		// if the commit string has coming after here, IMStart
		// will be sent dynamically
		return 0;
	    }
	}

	qic->sendIMEvent( QEvent::InputMethod,
			  qic->composingText, cursor, sellen ); //compose

	return 0;
    }

    static int xic_done_callback(XIC, XPointer client_data, XPointer) {
	QXIMInputContext *qic = (QXIMInputContext *) client_data;
	if (! qic)
	    return 0;

	// Don't send IMEnd here. QXIMInputContext::x11FilterEvent()
	// handles IMEnd with commit string.
#if 0
	if ( qic->isComposing() )
	    qic->sendIMEvent( QEvent::InputMethodEnd );
	qic->resetClientState();
#endif

	return 0;
    }

#ifdef Q_C_CALLBACKS
}
#endif // Q_C_CALLBACKS

#endif // !QT_NO_XIM



QXIMInputContext::QXIMInputContext()
    : QInputContext(), ic(0)
{
    if(!isInitXIM)
	QXIMInputContext::init_xim();
}


void QXIMInputContext::setHolderWidget( QWidget *widget )
{
    if ( ! widget )
	return;

    QXIMInputContext::setHolderWidget( widget );

#if !defined(QT_NO_XIM)
    if (! qt_xim) {
	qWarning("QInputContext: no input method context available");
	return;
    }

    if (! widget->isTopLevel()) {
	qWarning("QInputContext: cannot create input context for non-toplevel widgets");
	return;
    }

    XVaNestedList preedit_attr = 0;
    XIMCallback startcallback, drawcallback, donecallback;

    startcallback.client_data = (XPointer) this;
    startcallback.callback = (XIMProc) xic_start_callback;
    drawcallback.client_data = (XPointer) this;
    drawcallback.callback = (XIMProc)xic_draw_callback;
    donecallback.client_data = (XPointer) this;
    donecallback.callback = (XIMProc) xic_done_callback;

    preedit_attr = XVaCreateNestedList(0,
		   XNPreeditStartCallback, &startcallback,
		   XNPreeditDrawCallback, &drawcallback,
		   XNPreeditDoneCallback, &donecallback,
			   (char *) 0);

    if (preedit_attr)
    {
	ic = XCreateIC(qt_xim,
		       XNInputStyle, XIMPreeditCallbacks|XIMStatusCallbacks,
		       XNClientWindow, widget->winId(),
		       XNPreeditAttributes, preedit_attr,
		       (char *) 0);
	XFree(preedit_attr);
    }
    else
    {
	ic = XCreateIC(qt_xim,
		       XNInputStyle, XIMPreeditCallbacks|XIMStatusCallbacks,
		       XNClientWindow, widget->winId(),
		       (char *) 0);
    }

    if (! ic)
	qFatal("Failed to create XIM input context!");

    setComposePosition(1,1);

    // when resetting the input context, preserve the input state
    (void) XSetICValues((XIC) ic, XNResetState, XIMPreserveState, (char *) 0);

    if( ! ximContextList )
	ximContextList = new Q3PtrList<QXIMInputContext>;
    ximContextList->append( this );
#endif // !QT_NO_XIM
}


QXIMInputContext::~QXIMInputContext()
{

#if !defined(QT_NO_XIM)
    if (qt_xim && ic)
	XDestroyIC((XIC) ic);

    if( ximContextList ) {
	ximContextList->remove( this );
	if(ximContextList->isEmpty()) {
	    // Calling XCloseIM gives a Purify FMR error
	    // XCloseIM( qt_xim );
	    // We prefer a less serious memory leak
	    if( qt_xim ) {
		qt_xim = 0;
		isInitXIM = FALSE;
	    }

	    delete ximContextList;
	    ximContextList = 0;
	}
    }
#endif // !QT_NO_XIM

    ic = 0;
}

void QXIMInputContext::init_xim()
{
#ifndef QT_NO_XIM
    if(!isInitXIM)
	isInitXIM = TRUE;

    bool codesetIsUtf8 = (QString::compare(nl_langinfo(CODESET), "UTF-8") == 0);
    if (!codesetIsUtf8)
    {
	setlocale(LC_CTYPE, "en_US.UTF-8");
    }
    qt_xim = 0;
//    QString ximServerName(qt_ximServer);
//    if (qt_ximServer)
//	ximServerName.prepend("@im=");
 //   else
//	ximServerName = "";

    if ( !XSupportsLocale() )
	qWarning("Qt: Locales not supported on X server");

#ifdef USE_X11R6_XIM
    else if ( XSetLocaleModifiers (ximServerName.ascii()) == 0 )
	qWarning( "Qt: Cannot set locale modifiers: %s",
		  ximServerName.ascii());
    else {
	Display *dpy = QPaintDevice::x11AppDisplay();
	XWindowAttributes attr; // XIM unselects all events on the root window
	XGetWindowAttributes( dpy, QPaintDevice::x11AppRootWindow(), &attr );
	XRegisterIMInstantiateCallback(dpy, 0, 0, 0,
				       (XIMProc) xim_create_callback, 0);
	XSelectInput( dpy, QPaintDevice::x11AppRootWindow(), attr.your_event_mask );
    }
#else // !USE_X11R6_XIM
    else if ( XSetLocaleModifiers ("") == 0 )
	qWarning("Qt: Cannot set locale modifiers");
    else
	QXIMInputContext::create_xim();
#endif // USE_X11R6_XIM
#endif // QT_NO_XIM
}


/*! \internal
  Creates the application input method.
 */
void QXIMInputContext::create_xim()
{
#ifndef QT_NO_XIM
    Display *appDpy = QPaintDevice::x11AppDisplay();
    qt_xim = XOpenIM( appDpy, 0, 0, 0 );
    if ( qt_xim ) {

#ifdef USE_X11R6_XIM
	XIMCallback destroy;
	destroy.callback = (XIMProc) xim_destroy_callback;
	destroy.client_data = 0;
	if ( XSetIMValues( qt_xim, XNDestroyCallback, &destroy, (char *) 0 ) != 0 )
	    qWarning( "Xlib doesn't support destroy callback");
	    XUnregisterIMInstantiateCallback(appDpy, 0, 0, 0,
					     (XIMProc) xim_create_callback, 0);
#endif // USE_X11R6_XIM

    }
#endif // QT_NO_XIM
}


/*! \internal
  Closes the application input method.
*/
void QXIMInputContext::close_xim()
{
#ifndef QT_NO_XIM
    QString errMsg( "QXIMInputContext::close_xim() has been called" );

    // Calling XCloseIM gives a Purify FMR error
    // XCloseIM( qt_xim );
    // We prefer a less serious memory leak

    qt_xim = 0;
    if( ximContextList ) {
	Q3PtrList<QXIMInputContext> contexts( *ximContextList );
	Q3PtrList<QXIMInputContext>::Iterator it = contexts.begin();
	while( it != contexts.end() ) {
	    (*it)->close( errMsg );
	    ++it;
	}
	// ximContextList will be deleted in ~QXIMInputContext
    }
#endif // QT_NO_XIM
}


bool QXIMInputContext::x11FilterEvent( QWidget *keywidget, XEvent *event )
{
#ifndef QT_NO_XIM
    int xkey_keycode = event->xkey.keycode;
    if ( XFilterEvent( event, keywidget->topLevelWidget()->winId() ) ) {
//	qt_ximComposingKeycode = xkey_keycode; // ### not documented in xlib

	// Cancel of the composition is realizable even if
	// follwing codes don't exist
#if 0
	if ( event->type != XKeyPress || ! (qt_xim_style & XIMPreeditCallbacks) )
	    return TRUE;

	/*
	 * The Solaris htt input method will transform a ClientMessage
	 * event into a filtered KeyPress event, in which case our
	 * keywidget is still zero.
	 */
	QETWidget *widget = (QETWidget*)QWidget::find( (WId)event->xany.window );
        if ( ! keywidget ) {
 	    keywidget = (QETWidget*)QWidget::keyboardGrabber();
	    if ( keywidget ) {
	        grabbed = TRUE;
	    } else {
	        if ( focus_widget )
		    keywidget = (QETWidget*)focus_widget;
	        if ( !keywidget ) {
		    if ( qApp->inPopupMode() ) // no focus widget, see if we have a popup
		        keywidget = (QETWidget*) qApp->activePopupWidget();
		    else if ( widget )
		        keywidget = (QETWidget*)widget->topLevelWidget();
	        }
	    }
        }

	/*
	  if the composition string has been emptied, we need to send
	  an IMEnd event.  however, we have no way to tell if the user
	  has cancelled input, or if the user has accepted the
	  composition.

	  so, we have to look for the next keypress and see if it is
	  the 'commit' key press (keycode == 0).  if it is, we deliver
	  an IMEnd event with the final text, otherwise we deliver an
	  IMEnd with empty text (meaning the user has cancelled the
	  input).
	*/
	if ( composing && focusWidget && qt_compose_emptied ) {
	    XEvent event2;
	    bool found = FALSE;
	    if ( XCheckTypedEvent( QPaintDevice::x11AppDisplay(),
				   XKeyPress, &event2 ) ) {
		if ( event2.xkey.keycode == 0 ) {
		    // found a key event with the 'commit' string
		    found = TRUE;
		    XPutBackEvent( QPaintDevice::x11AppDisplay(), &event2 );
		}
	    }

	    if ( !found ) {
		// no key event, so the user must have cancelled the composition
		QInputMethodEvent endevent( QEvent::InputMethod, QString::null, -1 ); //end
		QApplication::sendEvent( focusWidget, &endevent );

		focusWidget = 0;
	    }

	    qt_compose_emptied = FALSE;
	}
#endif
	return TRUE;
    } else if ( focusWidget() ) {
        if ( event->type == XKeyPress && event->xkey.keycode == 0 ) {
	    // input method has sent us a commit string
	    Q3CString data(513);
	    KeySym sym;    // unused
	    Status status; // unused
	    QString inputText;
	    int count = lookupString( &(event->xkey), data, &sym, &status );
	    if ( count > 0 )
	    {
		QTextCodec *qt_input_mapper = QTextCodec::codecForName("utf8");
	        inputText = qt_input_mapper->toUnicode( data, count );
	    }

	    if (!isComposing())
		sendIMEvent(QEvent::InputMethod); //start

	    sendIMEvent( QEvent::InputMethod, inputText ); //end
	    resetClientState();

	    return TRUE;
	}
    }
#endif // !QT_NO_XIM

    return FALSE;
}


void QXIMInputContext::sendIMEvent( QEvent::Type type, const QString &text,
				    int cursorPosition, int selLength )
{
    QXIMInputContext::sendIMEvent( type, text, cursorPosition, selLength );
    if ( type == QEvent::InputMethod ) //compose
	composingText = text;
}


void QXIMInputContext::reset()
{
#if !defined(QT_NO_XIM)
    if ( focusWidget() && isComposing() && ! composingText.isNull() ) {
#ifdef QT_XIM_DEBUG
	qDebug("QXIMInputContext::reset: composing - sending IMEnd (empty) to %p",
	       focusWidget() );
#endif // QT_XIM_DEBUG

	//QInputContext::reset();
	resetClientState();

	char *mb = XmbResetIC((XIC) ic);
	if (mb)
	    XFree(mb);
    }
#endif // !QT_NO_XIM
}


void QXIMInputContext::resetClientState()
{
#if !defined(QT_NO_XIM)
    composingText = QString::null;
    if ( selectedChars.size() < 128 )
	selectedChars.resize( 128 );
    selectedChars.fill( 0 );
#endif // !QT_NO_XIM
}


void QXIMInputContext::close( const QString &errMsg )
{
    qDebug( errMsg );
//    emit QInputContext::deletionRequested();
}


bool QXIMInputContext::hasFocus() const
{
    return ( focusWidget() != 0 );
}


void QXIMInputContext::setMicroFocus(int x, int y, int, int h, QFont *f)
{
    QWidget *widget = focusWidget();
    if ( qt_xim && widget ) {
	QPoint p( x, y );
	QPoint p2 = widget->mapTo( widget->topLevelWidget(), QPoint( 0, 0 ) );
	p = widget->topLevelWidget()->mapFromGlobal( p );
	setComposePosition(p.x(), p.y() + h);
    }

}

void QXIMInputContext::mouseHandler( int , QEvent::Type type,
				     Qt::ButtonState button,
				     Qt::ButtonState)
{

    if ( type == QEvent::MouseButtonPress ||
	 type == QEvent::MouseButtonDblClick )
    {
    }
}

void QXIMInputContext::setComposePosition(int x, int y)
{
puts("*******");
#if !defined(QT_NO_XIM)
puts("&&&&&&&&");
    if (qt_xim && ic) {
	XPoint point;
	point.x = x;
	point.y = y;

	XVaNestedList preedit_attr =
	    XVaCreateNestedList(0,
				XNSpotLocation, &point,
				(char *) 0);
	if (XSetICValues((XIC) ic, XNPreeditAttributes, preedit_attr, (char *) 0) != NULL)
	{
	    Display *display = QPaintDevice::x11AppDisplay();
	    Window w = 0;
	    int revert_to_return;
	    XGetInputFocus(display, &w, &revert_to_return);
	    Atom oxim_atom = XInternAtom(display, OXIM_ATOM, true);
	    if (w && oxim_atom)
	    {
		Window config_win = XGetSelectionOwner(display, oxim_atom);
		XClientMessageEvent event;
		event.type = ClientMessage;
		event.window = config_win;
		event.message_type = oxim_atom;
		event.format = 32;
		event.data.l[0] = OXIM_CMD_SET_LOCATION;
		event.data.l[1] = w;
		event.data.l[2] = x;
		event.data.l[3] = y;
		XSendEvent(display, config_win, False, 0, (XEvent *)&event);
	    }
	}
	XFree(preedit_attr);
    }
#endif // !QT_NO_XIM
}


int QXIMInputContext::lookupString(XKeyEvent *event, Q3CString &chars,
				KeySym *key, Status *status) const
{
    int count = 0;

#if !defined(QT_NO_XIM)
    if (qt_xim && ic) {
	count = XmbLookupString((XIC) ic, event, chars.data(),
				chars.size(), key, status);

	if ((*status) == XBufferOverflow ) {
	    chars.resize(count + 1);
	    count = XmbLookupString((XIC) ic, event, chars.data(),
				    chars.size(), key, status);
	}
    }

#endif // QT_NO_XIM

    return count;
}

void QXIMInputContext::setFocus()
{
#if !defined(QT_NO_XIM)
    if ( qt_xim && ic )
	XSetICFocus((XIC) ic);
#endif // !QT_NO_XIM
}

void QXIMInputContext::unsetFocus()
{
#if !defined(QT_NO_XIM)
    if (qt_xim && ic)
	XUnsetICFocus((XIC) ic);
#endif // !QT_NO_XIM
}

QString QXIMInputContext::identifierName()
{
    // the name should be "xim" rather than "XIM" to be consistent
    // with corresponding immodule of GTK+
    return "oxim";
}
#endif //QT_NO_IM
