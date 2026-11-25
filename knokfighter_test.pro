# Tests for the game, with code coverage measured in CI
# Run with: ./knokfighter_test

# Qt version of Knokfighter.
# The original C++Builder/VCL version is ProjectKnokfighter.bpr
#
# Build with:
#
#   qmake knokfighter.pro
#   make           # nmake or jom when using MSVC
TEMPLATE = app
TARGET = knokfighter_test
QT += core gui widgets

# Qt Multimedia is not available for WebAssembly,
# the web version uses the browser to play sounds
!wasm {
  QT += multimedia
  RESOURCES += knokfighter_sounds.qrc
}
CONFIG += c++17

SOURCES += \
  knokfighter_test.cpp \
  qtknokfighterwidget.cpp

HEADERS += \
  qtknokfighterwidget.h

RESOURCES += knokfighter.qrc

QT += testlib
CONFIG += console
CONFIG -= app_bundle
