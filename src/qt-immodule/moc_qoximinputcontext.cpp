/****************************************************************************
** QXIMInputContext meta object code from reading C++ file 'qoximinputcontext.h'
**
** Created: Tue Jun 6 14:21:48 2006
**      by: The Qt MOC ($Id: moc_qoximinputcontext.cpp,v 1.1 2006/06/06 06:49:31 firefly Exp $)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "qoximinputcontext.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 26)
#error "This file was generated using the moc from 3.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *QXIMInputContext::className() const
{
    return "QXIMInputContext";
}

QMetaObject *QXIMInputContext::metaObj = 0;
static QMetaObjectCleanUp cleanUp_QXIMInputContext( "QXIMInputContext", &QXIMInputContext::staticMetaObject );

#ifndef QT_NO_TRANSLATION
QString QXIMInputContext::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "QXIMInputContext", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString QXIMInputContext::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "QXIMInputContext", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* QXIMInputContext::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QInputContext::staticMetaObject();
    metaObj = QMetaObject::new_metaobject(
	"QXIMInputContext", parentObject,
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_QXIMInputContext.setMetaObject( metaObj );
    return metaObj;
}

void* QXIMInputContext::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "QXIMInputContext" ) )
	return this;
    return QInputContext::qt_cast( clname );
}

bool QXIMInputContext::qt_invoke( int _id, QUObject* _o )
{
    return QInputContext::qt_invoke(_id,_o);
}

bool QXIMInputContext::qt_emit( int _id, QUObject* _o )
{
    return QInputContext::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool QXIMInputContext::qt_property( int id, int f, QVariant* v)
{
    return QInputContext::qt_property( id, f, v);
}

bool QXIMInputContext::qt_static_property( QObject* , int , int , QVariant* ){ return FALSE; }
#endif // QT_NO_PROPERTIES
