TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_off release

LIBS	+= -L../../lib/.libs -loxim

INCLUDEPATH	+= ../../include

SOURCES	+= main.cpp

FORMS	= oxim-setup.ui \
	globolsetting.ui \
	gencin.ui \
	chewing.ui \
	installim.ui

IMAGES	= images/cd01.png \
	images/edit_add.png \
	images/edit_remove.png \
	images/connect.png

QT += qt3support






unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}

