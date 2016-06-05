TEMPLATE = lib
CONFIG += plugin c++11 debug
QT += qml

PLUGIN_IMPORT_PATH = org/kimmoli/firmata
TARGET  = qmlfirmataplugin

SOURCES += \
    plugin.cpp \
    src/firmata.cpp \
    src/backends/backend.cpp \
    src/backends/serialport.cpp \
    src/pins/pin.cpp \
    src/pins/digitalpin.cpp \
    src/pins/pwmpin.cpp \
    src/pins/analogpin.cpp \
    src/pins/servo.cpp \
    src/pins/encoder.cpp \
    src/pins/i2c.cpp

HEADERS += \
    plugin.h \
    src/utils.h \
    src/firmata.h \
    src/backends/backend.h \
    src/backends/serialport.h \
    src/pins/pin.h \
    src/pins/digitalpin.h \
    src/pins/pwmpin.h \
    src/pins/analogpin.h \
    src/pins/servo.h \
    src/pins/encoder.h \
    src/pins/i2c.h

target.path = $$[QT_INSTALL_QML]/$$PLUGIN_IMPORT_PATH
INSTALLS += target

qmldir.files += $$_PRO_FILE_PWD_/qmldir
qmldir.path +=  $$target.path
INSTALLS += qmldir

OTHER_FILES += \
    rpm/qfirmata.spec \
    rpm/qmlfirmataplugin.spec

