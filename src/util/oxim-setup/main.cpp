#include <qapplication.h>
#include "oxim-setup.h"
#include "oximtool.h"
#include <X11/Xlib.h>

int main( int argc, char ** argv )
{
    QApplication oxim_setup(argc, argv);

    OXIM_Setup w;

    // 檢查 oxim-setup 是否正在執行?
    Window os_win;
    Atom myatom = XInternAtom(w.x11Display(), "OXIM_SETUP", False);
    os_win = XGetSelectionOwner(w.x11Display(), myatom);
    if (os_win != None)
    {
	exit(1);
    }
    XSetSelectionOwner(w.x11Display(), myatom, w.winId(), CurrentTime);

    w.IMListInit();
    w.show();

    oxim_setup.connect(&oxim_setup, SIGNAL( lastWindowClosed() ), &oxim_setup, SLOT( quit()));

    return oxim_setup.exec();
}
