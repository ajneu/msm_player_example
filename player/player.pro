QT       += core
QT       += gui
QT       += widgets

CONFIG += c++11

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
LIBS += c:/xtoff/boost_1_54_0/stage/lib/libboost_thread-mgw47-mt-1_54.a
LIBS += c:/xtoff/boost_1_54_0/stage/lib/libboost_system-mgw47-mt-1_54.a
LIBS += -lws2_32
LIBS += -lmswsock
}else{
# linux gcc
#    sudo apt-get install libboost-all-dev
#INCLUDEPATH += "/usr/include"
QMAKE_CXXFLAGS += -std=c++11 -Wno-strict-aliasing

#LIBS += /usr/lib/x86_64-linux-gnu/libboost_thread.a
LIBS += -lboost_thread

#LIBS += /usr/lib/x86_64-linux-gnu/libboost_system.a
LIBS += -lboost_system
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

