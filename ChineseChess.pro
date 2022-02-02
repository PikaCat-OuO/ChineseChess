QT       += core gui network # testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QMAKE_CXXFLAGS += -std=gnu++2b -march=native -mtune=native -fopenmp
QMAKE_CXXFLAGS_RELEASE += -Ofast -flto
# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    src/GUI/dialog.h \
    src/board/bitboard.h \
    src/board/chessboard.h \
    src/chessengine.h \
    src/global.h \
    src/move/historymove.h \
    src/move/move.h \
    src/move/valuedcapmove.h \
    src/move/valuedmove.h \
    src/search/searchcapturemachine.h \
    src/search/searchinstance.h \
    src/search/searchmachine.h \
    src/search/searchquiescencemachine.h \
    src/table/hashtable.h \
    src/table/historytable.h \
    src/table/killertable.h \
    src/table/pregen.h \
#    test/perfttest.h

SOURCES += \
    src/GUI/dialog.cpp \
    src/board/bitboard.cpp \
    src/board/chessboard.cpp \
    src/chessengine.cpp \
    src/main.cpp \
    src/move/historymove.cpp \
    src/move/move.cpp \
    src/move/valuedcapmove.cpp \
    src/move/valuedmove.cpp \
    src/search/searchcapturemachine.cpp \
    src/search/searchinstance.cpp \
    src/search/searchmachine.cpp \
    src/search/searchquiescencemachine.cpp \
    src/table/hashtable.cpp \
    src/table/historytable.cpp \
    src/table/killertable.cpp \
    src/table/pregen.cpp \
#    test/perfttest.cpp

INCLUDEPATH += src \
    src/GUI \
    src/board \
    src/search \
    src/table \
    src/move \
#    test

LIBS += -fopenmp

FORMS += \
    src\GUI\dialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc

RC_FILE = \
    app.rc
