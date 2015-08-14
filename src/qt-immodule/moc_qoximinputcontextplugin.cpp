/****************************************************************************
** QXIMInputContextPlugin meta object code from reading C++ file 'qoximinputcontextplugin.h'
**
** Created: Tue Jun 6 14:21:48 2006
**      by: The Qt MOC ($Id: moc_qoximinputcontextplugin.cpp,v 1.1 2006/06/06 06:49:31 firefly Exp $)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "qoximinputcontextplugin.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *QXIMInputContextPlugin::className() const
{
    return "QXIMInputContextPlugin";
}

QMetaObject *QXIMInputContextPlugin::metaObj = 0;
static QMetaObjectCleanUp cleanUp_QXIMInputContextPlugin( "QXIMInputContextPlugin", &QXIMInputContextPlugin::staticMetaObject );

#ifndef QT_NO_TRANSLATION
QString QXIMInputContextPlugin::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "QXIMInputContextPlugin", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString QXIMInputContextPlugin::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "QXIMInputContextPlugin", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* QXIMInputContextPlugin::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QInputContextPlugin::staticMetaObject();
    metaObj = QMetaObject::new_metaobject(
	"QXIMInputContextPlugin", parentObject,
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_QXIMInputContextPlugin.setMetaObject( metaObj );
    return metaObj;
}

void* QXIMInputContextPlugin::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "QXIMInputContextPlugin" ) )
	return this;
    return QInputContextPlugin::qt_cast( clname );
}

bool QXIMInputContextPlugin::qt_invoke( int _id, QUObject* _o )
{
    return QInputContextPlugin::qt_invoke(_id,_o);
}

bool QXIMInputContextPlugin::qt_emit( int _id, QUObject* _o )
{
    return QInputContextPlugin::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool QXIMInputContextPlugin::qt_property( int id, int f, QVariant* v)
{
    return QInputContextPlugin::qt_property( id, f, v);
}

bool QXIMInputContextPlugin::qt_static_property( QObject* , int , int , QVariant* ){ return FALSE; }
#endif // QT_NO_PROPERTIES
