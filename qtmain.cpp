/*
    KnokFighter, a simple fighting game
    Copyright (C) 2005  Richel Bilderbeek and Joost van den Bogaart

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <QApplication>

#include "qtknokfighterwidget.h"

int main(int argc, char* argv[])
{
  QApplication a(argc, argv);
  QApplication::setApplicationName("Knokfighter 1 - Battle of Honour");
  QtKnokfighterWidget w;
#ifdef Q_OS_WASM
  // In the browser, the game fills the web page, without a window frame
  w.setWindowFlags(Qt::FramelessWindowHint);
  w.move(0, 0);
#endif
  w.show();
  // Let the game receive the keys without clicking it first
  w.activateWindow();
  w.setFocus();
  return a.exec();
}
