#include "test-imcontext-qt.h"
#include "oximtool.h"
//#include "gcin-common-qt.h"
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <cstdio>
#include <Qt>
#include <QTextCodec>
//Added by qt3to4:
#include <Q3CString>

#include "q3ptrlist.h"
#include "langinfo.h"

#include <limits.h>

const int XKeyPress = KeyPress;
const int XKeyRelease = KeyRelease;
#undef KeyPress
#undef KeyRelease

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>

// #define QT_XIM_DEBUG
using namespace Qt;
// from qapplication_x11.cpp
static XIM	qt_xim = 0;
//extern char    *qt_ximServer;
static bool isInitXIM = FALSE;
static Q3PtrList<QXIMInputContext> *ximContextList = 0;



using namespace Qt;
//static QWidget *focused_widget;
//typedef QInputMethodEvent::Attribute QAttribute;


#if !defined(QT_NO_XIM)

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif // Q_C_CALLBACKS

#ifdef USE_X11R6_XIM
    static void xim_create_callback(XIM /*im*/,
				    XPointer /*client_data*/,
				    XPointer /*call_data*/)
    {
	qDebug("xim_create_callback");
	QXIMInputContext::create_xim();
    }

    static void xim_destroy_callback(XIM /*im*/,
				     XPointer /*client_data*/,
				     XPointer /*call_data*/)
    {
	qDebug("xim_destroy_callback");
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
	//if( ! qic->isComposing() && qic->hasFocus() ) {	// TODO: marked for test
	if( ! qic->isComposing() ) {
	    qic->resetClientState();
	    send_imstart = TRUE;
	//} else if ( ! qic->isComposing() || ! qic->hasFocus() ) {
	} else if ( ! qic->isComposing() ) {	// TODO: marked for test
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




QXIMInputContext::QXIMInputContext ()
    : QInputContext(), ic(0)
{
    if(!isInitXIM)
        QXIMInputContext::init_xim();
}

QXIMInputContext::~QXIMInputContext ()
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

QString QXIMInputContext::identifierName()
{
    // the name should be "xim" rather than "XIM" to be consistent
    // with corresponding immodule of GTK+
    return "oxim";
}

void QXIMInputContext::mouseHandler (int offset, QMouseEvent *event)
{
}

void QXIMInputContext::widgetDestroyed (QWidget *widget)
{
}

void QXIMInputContext::reset ()
{
#if !defined(QT_NO_XIM)
    if ( focusWidget() && isComposing() && ! composingText.isNull() ) {
#ifdef QT_XIM_DEBUG
	qDebug("QXIMInputContext::reset: composing - sending IMEnd (empty) to %p",
	       focusWidget() );
#endif // QT_XIM_DEBUG

	QInputContext::reset();
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

void QXIMInputContext::update_cursor(QWidget *fwidget)
{
}

void QXIMInputContext::update_preedit()
{
}

void QXIMInputContext::update()
{
}

QString QXIMInputContext::language ()
{
    return "";
}

void QXIMInputContext::setFocusWidget(QWidget *widget)
{
  if (!widget)
    return;
}

bool QXIMInputContext::x11FilterEvent (QWidget *keywidget, XEvent *event)
{
#ifndef QT_NO_XIM
    puts("QXIMInputContext::x11FilterEvent");
    int xkey_keycode = event->xkey.keycode;
    printf("keycode=%d\n\n", xkey_keycode);
    printf("winid=%d\n", keywidget->topLevelWidget()->winId());
    if ( XFilterEvent( event, keywidget->topLevelWidget()->winId() ) ) {
	//qt_ximComposingKeycode = xkey_keycode; // ### not documented in xlib

	// Cancel of the composition is realizable even if
	// follwing codes don't exist
	puts("bbb");
        return TRUE;
    } else if ( focusWidget() ) {
	puts("if ( focusWidget() ) {");
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
		sendIMEvent(QEvent::InputMethod);

	    sendIMEvent( QEvent::InputMethod, inputText );
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
    puts("sendIMEvent");
    //QInputContext::sendIMEvent( type, text, cursorPosition, selLength );
    if ( type == QEvent::InputMethod ) //compose
	composingText = text;
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


bool QXIMInputContext::filterEvent (const QEvent *event)
{
  return FALSE;
}

bool QXIMInputContext::isComposing() const
{
    /*
  char *str;
  GCIN_PREEDIT_ATTR att[GCIN_PREEDIT_ATTR_MAX_N];
  int preedit_cursor_position, sub_comp_len;
  gcin_im_client_get_preedit(gcin_ch, &str, att, &preedit_cursor_position, &sub_comp_len);
  bool is_compose = str[0]>0;
  free(str);

  return is_compose;*/
  return 1;
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
    //QString ximServerName(qt_ximServer);

/*    if (qt_ximServer)
	ximServerName.prepend("@im=");
   else
	ximServerName = "";*/

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
    {
	puts("hhh");
	QXIMInputContext::create_xim();
    }
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
		puts("create_xim");
#ifdef USE_X11R6_XIM
	XIMCallback destroy;
	destroy.callback = (XIMProc) xim_destroy_callback;
	destroy.client_data = 0;
	if ( XSetIMValues( qt_xim, XNDestroyCallback, &destroy, (char *) 0 ) != 0 )
	    puts("create_xim_ic");
	    qWarning( "Xlib doesn't support destroy callback");
	    XUnregisterIMInstantiateCallback(appDpy, 0, 0, 0,
					     (XIMProc) xim_create_callback, 0);
#endif // USE_X11R6_XIM

    }
    puts("aaasa");
#endif // QT_NO_XIM
}

#if 1
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
	    //(*it)->close( errMsg );
	    ++it;
	}
	// ximContextList will be deleted in ~QXIMInputContext
    }
#endif // QT_NO_XIM
}
#endif
