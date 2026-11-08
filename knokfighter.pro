# Qt version of Knokfighter.
# The original C++Builder/VCL version is ProjectKnokfighter.bpr
#
# Build with:
#
#   qmake knokfighter.pro
#   make           # nmake or jom when using MSVC
TEMPLATE = app
TARGET = knokfighter
QT += core gui widgets multimedia
CONFIG += c++17

SOURCES += \
  qtmain.cpp \
  qtknokfighterwidget.cpp

HEADERS += \
  qtknokfighterwidget.h

RESOURCES += knokfighter.qrc
