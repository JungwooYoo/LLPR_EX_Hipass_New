#-------------------------------------------------
#
# Project created by QtCreator 2019-11-07T18:09:16
#
#-------------------------------------------------


QT       += core gui
QT       += sql xml serialport  network
QT       += ftp

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


TARGET = LLPR_EX_HIPASS
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#sdw  /usr/lib/x86_64-linux-gnu/libQt5Xml.so:-1: error: undefined reference to `qt_version_tag@Qt_5.9'
DEFINES += QT_NO_VERSION_TAGGING

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    cameraconfigdlg.cpp \
    apsgdlg.cpp \
    recognitiondlg.cpp \
    logdlg.cpp \
    centerdlg.cpp \
    configdlg.cpp \
    databasedlg.cpp \
    liccomm.cpp \
    ccucomm.cpp \
    tgmcucomm.cpp \
    controlboarddlg.cpp \
    config.cpp \
    spinview.cpp \
    my_qlabel.cpp \
    recogthread.cpp \
    commonvalues.cpp \
    mainthread.cpp \
    centerclient.cpp \
    ipsocket.cpp \
    autoiris.cpp \
    syslogger.cpp \
    dbmng.cpp

HEADERS += \
    dataclass.h \
    commonvalues.h \
    mainwindow.h \
    cameraconfigdlg.h \
    apsgdlg.h \
    recognitiondlg.h \
    logdlg.h \
    centerdlg.h \
    configdlg.h \
    databasedlg.h \
    liccomm.h \
    ccucomm.h \
    tgmcucomm.h \
    controlboarddlg.h \
    config.h \
    spinview.h \
    my_qlabel.h \
    lib/ilpr_nano_1.5.2.2/iplpr.h \
    lib/alpr_nano_v3.6.5_lt/ALPR_api.h \
    lib/alpr_nano_v3.6.5_lt/LPResult.h \
    recogthread.h \
    lib/alpr_nano_v3.6.5_lt/LPDataInfo.h \
    mainthread.h \
    centerclient.h \
    ipsocket.h \
    autoiris.h \
    syslogger.h \
    dbmng.h


FORMS += \
        mainwindow.ui \
    cameraconfigdlg.ui \
    apsgdlg.ui \
    recognitiondlg.ui \
    logdlg.ui \
    centerdlg.ui \
    configdlg.ui \
    databasedlg.ui \
    controlboarddlg.ui

CONFIG += c++11
CUDA_PATH = /usr/local/cuda
NVCC = $$CUDA_PATH/bin/nvcc -ccbin $(filter-out g++ )


#intstall spinnaker-1.26.0.31-Ubuntu18.04-amd64-pkg.tar.gz
INCLUDEPATH += /usr/include/spinnaker
LIBS += -L/usr/lib -lSpinnaker

#recognition
CONFIG += link_pkgconfig
PKGCONFIG += opencv4

INCLUDEPATH += \
    /usr/include/x86_64-linux-gnu \
    $$CUDA_PATH/include \
    /usr/include/libdrm \
    $$PWD/lib/ilpr_nano_1.5.2.2 \
    $$PWD/lib/alpr_nano_v3.6.5_lt

LIBS += -L$$PWD/lib/ilpr_nano_1.5.2.2 -llpr -L$$PWD/lib/alpr_nano_v3.6.5_lt -lALPR3
LIBS +=  \
        -L$$PWD \
        -L/usr/local/lib \
        -L/usr/lib/x86_64-linux-gnu \
        -L/usr/lib \
        -L$$CUDA_PATH/lib64 \
        -lpthread -lv4l2 -lEGL -lGLESv2 -lX11\
        -lcuda -lcudart -lcudnn \
        -lnvinfer -lnvparsers \
        -lopencv_highgui -lopencv_imgproc -lopencv_core -lopencv_video -lopencv_videoio -lopencv_imgcodecs \
        -lboost_system -lboost_regex -ldl

#QMAKE_CXXFLAGS += -rpath


