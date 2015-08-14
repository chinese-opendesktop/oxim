/****************************************************************************
** $Id: qoximinputcontextplugin.cpp,v 1.1.1.1 2005/12/05 15:42:21 firefly Exp $
**
** Implementation of QXIMInputContextPlugin class
**
** Copyright (C) 2004 immodule for Qt Project.  All rights reserved.
**
** This file is written to contribute to Trolltech AS under their own
** licence. You may use this file under your Qt license. Following
** description is copied from their original file headers. Contact
** immodule-qt@freedesktop.org if any conditions of this licensing are
** not clear to you.
**
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

//#ifndef QT_NO_IM
#include <Qt>
#include <QInputContextPlugin>

#include "qoximinputcontext.h"

//#include "test-imcontext-qt.h"

#include "qoximinputcontextplugin.h"
//#include <qinputcontextplugin.h>

using namespace Qt;

/* Implementations */
//QXIMInputContext::
#include <stdio.h>
QXIMInputContextPlugin::QXIMInputContextPlugin()
{
    puts("asasa");
}

QXIMInputContextPlugin::~QXIMInputContextPlugin()
{
}

QStringList QXIMInputContextPlugin::keys() const
{
    QStringList identifiers;
    identifiers.push_back ("oxim");
    return identifiers;

}

QInputContext *QXIMInputContextPlugin::create( const QString &key )
{
    if (key.toLower () != "oxim") {
        return NULL;
    } else {
        //return NULL;
        return new QXIMInputContext();
    }

}

QStringList QXIMInputContextPlugin::languages( const QString &key )
{
    QStringList a;
    a << "zh_TW" << "zh_HK" << "zh_CN";
    return a;
}

QString QXIMInputContextPlugin::displayName( const QString & )
{
    return "OXIM";
}

QString QXIMInputContextPlugin::description( const QString & )
{
    return QString::fromUtf8 ("Qt immodule plugin for gcin");
}


//Q_EXPORT_PLUGIN( QXIMInputContextPlugin )
Q_EXPORT_PLUGIN2( QXIMInputContextPlugin, QXIMInputContextPlugin )

//#endif
