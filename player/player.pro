QT       += core
QT       += gui
QT       += widgets

#use this for full eUML implementation (looong compile)
#CONFIG += fulleuml
#CONFIG += euml
CONFIG += func

HEADERS       = button.h \
    playergui.h \
    playerlogic.h \
    ihardware.h \
    idisplay.h

fulleuml {
SOURCES       = button.cpp \
                main.cpp \
    playergui.cpp \
    ihardware.cpp \
    playerlogicfulleuml.cpp \
    idisplay.cpp
} euml {
SOURCES       = button.cpp \
                main.cpp \
    playergui.cpp \
    ihardware.cpp \
    playerlogiceuml.cpp \
    idisplay.cpp
} func {
SOURCES       = button.cpp \
                main.cpp \
    playergui.cpp \
    ihardware.cpp \
    playerlogic.cpp \
    idisplay.cpp
}
win32{
INCLUDEPATH += "c:/xtoff/boost_1_54_0"
QMAKE_CXXFLAGS += -std=c++11 -Wno-strict-aliasing
LIBS += c:/xtoff/boost_1_54_0/stage/lib/libboost_thread-mgw47-mt-1_54.a
LIBS += c:/xtoff/boost_1_54_0/stage/lib/libboost_system-mgw47-mt-1_54.a
LIBS += -lws2_32
LIBS += -lmswsock
}else{
# linux gcc
INCLUDEPATH += /11.4/home/hyc/i4trunk/packages/boost/boost_i4/
}

# install
target.path = .\player
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS player.pro
sources.path = .\player
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000C602
    include($$PWD/../../symbianpkgrules.pri)
}
maemo5: include($$PWD/../../maemo5pkgrules.pri)

