#include "gcin-imcontext-qt.h"
#include "gcin-common-qt.h"
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <cstdio>
//#include "gcin-im-client.h"
#include <QColor>
#include <QPalette>
#include <QInputMethodEvent>
#include <QTextCharFormat>

using namespace Qt;
static QWidget *focused_widget;

//typedef QInputMethodEvent::Attribute QAttribute;

QXIMInputContext::QXIMInputContext ()
{
  Display *display = QX11Info::display();
/*  if (!(gcin_ch = gcin_im_client_open(display))) {
    perror("cannot open gcin_ch");
    return;
  }*/
}

QXIMInputContext::~QXIMInputContext ()
{
  if (gcin_ch) {
    //gcin_im_client_close(gcin_ch);
    gcin_ch = NULL;
  }
}

QString QXIMInputContext::identifierName()
{
  return GCIN_IDENTIFIER_NAME;
}

void QXIMInputContext::mouseHandler (int offset, QMouseEvent *event)
{
}

void QXIMInputContext::widgetDestroyed (QWidget *widget)
{
}

void QXIMInputContext::reset ()
{
  if (gcin_ch) {
//    gcin_im_client_reset(gcin_ch);
    update_preedit();
  }
}

void QXIMInputContext::update_cursor(QWidget *fwidget)
{
/*
  gcin_im_client_set_window(gcin_ch, fwidget->winId());
  QRect rect = fwidget->inputMethodQuery (ImMicroFocus).toRect ();
  QPoint point (rect.x (), rect.y () + rect.height ());
  QPoint gxy = fwidget->mapToGlobal (point);
  if (gcin_ch) {
    Display *dpy = QX11Info::display();
    WId ow;
    int wx, wy;
    XTranslateCoordinates(dpy, fwidget->winId(), DefaultRootWindow(dpy),
    0,0,  &wx, &wy, &ow);

    gcin_im_client_set_cursor_location(gcin_ch, gxy.x()-wx, gxy.y()-wy);
  }*/
}

void QXIMInputContext::update_preedit()
{
    /*
  QList<QAttribute> preedit_attributes;
//  QString preedit_string;
  int preedit_cursor_position=0;
  int sub_comp_len;
  char *str=NULL;
  GCIN_PREEDIT_ATTR att[GCIN_PREEDIT_ATTR_MAX_N];
  int attN = gcin_im_client_get_preedit(gcin_ch, &str, att, &preedit_cursor_position, &sub_comp_len);

  if (gcin_ch) {
    int ret;
    gcin_im_client_set_flags(gcin_ch, FLAG_GCIN_client_handle_use_preedit, &ret);
  }

  preedit_attributes.push_back (QAttribute (QInputMethodEvent::Cursor, preedit_cursor_position, true, 0));

  const QWidget *focused_widget = qApp->focusWidget ();
  if (!focused_widget || !str || !*str) {
free_mem:
    free(str);
    return;
  }
  const QPalette &palette = focused_widget->palette ();
  if (&palette==NULL)
    goto free_mem;
  const QBrush &reversed_foreground = palette.base ();
  const QBrush &reversed_background = palette.text ();

#if DBG || 0
  printf("gtk_im_context_gcin_get_preedit_string attN:%d '%s'\n", attN, str);
#endif
  int i;
  for(i=0; i < attN; i++) {
    int ofs0 = att[i].ofs0;
    int len = att[i].ofs1 - att[i].ofs0;

    switch (att[i].flag) {
      case GCIN_PREEDIT_ATTR_FLAG_REVERSE:
          {
              QTextCharFormat text_format;
              text_format.setForeground (reversed_foreground);
              text_format.setBackground (reversed_background);
              QAttribute qt_attribute (QInputMethodEvent::TextFormat, ofs0, len, text_format);
              preedit_attributes.push_back (qt_attribute);
          }
          break;
      case GCIN_PREEDIT_ATTR_FLAG_UNDERLINE:
          {
              QTextCharFormat text_format;
              text_format.setProperty (QTextFormat::FontUnderline, true);
              QAttribute qt_attribute (QInputMethodEvent::TextFormat, ofs0, len, text_format);
              preedit_attributes.push_back (qt_attribute);
          }
    }
  }

  QInputMethodEvent im_event (QString::fromUtf8(str), preedit_attributes);
  sendEvent (im_event);
  free(str);
  */
}

void QXIMInputContext::update()
{
    /*
    QWidget *focused_widget = qApp->focusWidget ();
    if (focused_widget != NULL) {
        if (focused_widget == NULL) {
          if (gcin_ch)
            gcin_im_client_focus_in(gcin_ch);
        }

        update_cursor(focused_widget);
    }*/
}

QString QXIMInputContext::language ()
{
    return "";
}

void QXIMInputContext::setFocusWidget(QWidget *widget)
{
  if (!widget)
    return;
/*
  if (focused_widget != widget) {
#if 0
    if (focused_widget) {
      char *rstr;
      gcin_im_client_focus_out2(gcin_ch, &rstr);
      if (rstr) {
          QString inputText = QString::fromUtf8(rstr);
          QInputMethodEvent commit_event;
          commit_event.setCommitString (inputText);
          sendEvent (commit_event);

          QList<QAttribute> preedit_attributes;
          QInputMethodEvent im_event (QString::fromUtf8(""), preedit_attributes);
          sendEvent (im_event);
      }
    }
    focused_widget = widget;
#else
    gcin_im_client_focus_out(gcin_ch);
#endif
  }

  if (gcin_ch) {
    gcin_im_client_set_window(gcin_ch, widget->winId());
  }

  QInputContext::setFocusWidget (widget);
//  puts("setFocusWidget");
  if (gcin_ch)
    gcin_im_client_focus_in(gcin_ch);
    */
}

bool QXIMInputContext::x11FilterEvent (QWidget *widget, XEvent *event)
{
    /*
  KeySym keysym;
  char static_buffer[256];
  char *buffer = static_buffer;
  int buffer_size = sizeof(static_buffer) - 1;

  if (event->type != KeyPress && event->type != KeyRelease)
      return TRUE;

  XKeyEvent *keve = (XKeyEvent *) event;

  XLookupString (keve, buffer, buffer_size, &keysym, NULL);
  int result;
  char *rstr = NULL;
  unsigned int state = keve->state;


  if (event->type == KeyPress) {
      result = gcin_im_client_forward_key_press(gcin_ch,
        keysym, state, &rstr);

      if (rstr) {
          QString inputText = QString::fromUtf8(rstr);
          QInputMethodEvent commit_event;
          commit_event.setCommitString (inputText);
          sendEvent (commit_event);
      }
  } else {
     result = gcin_im_client_forward_key_release(gcin_ch,
       keysym, state, &rstr);
  }

  if (result)
    update_preedit();

  update_cursor(widget);

  if (rstr)
      free(rstr);

  return result;*/
  return 1;
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
