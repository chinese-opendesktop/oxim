#include <QApplication>
#include <QEvent>
#include <QFont>
#include <QInputContext>
#include <QInputMethodEvent>
#include <QObject>
#include <QPoint>
#include <QWidget>
#include <QX11Info>

#include <Q3CString>
#include <Q3MemArray>

#include <X11/Xlib.h>
#include <X11/keysym.h>

//struct GCIN_client_handle_S;

class QXIMInputContext: public QInputContext {
    public:
        QXIMInputContext ();
        ~QXIMInputContext ();
        bool x11FilterEvent (QWidget *widget, XEvent *event);
        bool filterEvent (const QEvent *event);
        void update();
        QString identifierName();
        QString language();
        void mouseHandler (int offset, QMouseEvent *event);
        void setFocusWidget (QWidget *widget);
        void widgetDestroyed (QWidget *widget);
        void reset ();
        //GCIN_client_handle_S *gcin_ch;
        bool isComposing() const;
        void update_cursor(QWidget *);
        void update_preedit();
        
    static void init_xim();
    static void create_xim();
    static void close_xim();
    void *ic;
    QString composingText;
    Q3MemArray<bool> selectedChars;
    void resetClientState();
    void sendIMEvent( QEvent::Type type,
		      const QString &text = QString::null,
		      int cursorPosition = -1, int selLength = 0 );

private:
    //void setComposePosition(int, int);
    int lookupString(XKeyEvent *, Q3CString &, KeySym *, Status *) const;

};
