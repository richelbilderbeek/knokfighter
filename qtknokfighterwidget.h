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
// Qt port of the C++Builder/VCL version (UnitFormKnokfighterMain.*).
// The game logic is kept as close to the original as possible.
#ifndef QTKNOKFIGHTERWIDGET_H
#define QTKNOKFIGHTERWIDGET_H

#include <QHash>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QTimer>
#include <QWidget>

class QProgressBar;
class QSoundEffect;

/// The equivalent of a VCL TImage with AutoSize = true:
/// a picture at a position, with a Tag
struct Picture
{
  explicit Picture(bool transparent = false) : m_transparent{transparent} {}

  /// Load a picture from the resources, e.g. 'JoostStaatLinks'
  void load(const QString& name);

  int width() const { return m_image.width(); }
  int height() const { return m_image.height(); }

  /// The colour of a pixel, without alpha (as a VCL TColor would be)
  /// Returns 0xFFFFFFFF when out of range
  QRgb pixel(int x, int y) const;

  bool m_transparent;
  QImage m_image;
  QPixmap m_pixmap; //With the transparency mask applied
  int m_left = 0;
  int m_top = 0;
  int m_tag = 0;
};

class QtKnokfighterWidget : public QWidget
{
  Q_OBJECT

public:
  explicit QtKnokfighterWidget(QWidget* parent = nullptr);

protected:
  void keyPressEvent(QKeyEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

private:
  enum class ProgState { UnInitialized, Initialized, PreRunning, Running };

  ProgState m_program_state = ProgState::UnInitialized;

  int m_joost_health = 1000;
  int m_richel_health = 1000;
  int m_richel_bonus = 100; //de damagebonus van een kick, hoger is slechter...
  int m_joost_bonus = 100;

  Picture m_image_background;
  Picture m_startscherm;
  Picture m_joost;
  Picture m_richel;
  Picture m_joost_shadow;
  Picture m_richel_shadow;
  Picture m_wolk;

  /// What is drawn on the form, i.e. the VCL FormMain->Canvas
  QImage m_screen;

  QTimer m_timer_joost;
  QTimer m_timer_richel;
  QTimer m_richel_ai_timer;
  QTimer m_wolk_timer;

  QProgressBar* m_richel_health_bar;
  QProgressBar* m_joost_health_bar;

  // Unused in the web version
  QHash<QString, QSoundEffect*> m_sounds;
  QSoundEffect* m_playing_sound = nullptr;

  void createPanels();

  /// Web version only: give the keyboard focus to the game's web page element
  void focusInBrowser();

  /// Like the Win32 PlaySound with SND_ASYNC: stops the sound being played
  void playSound(const QString& name);

  /// The VCL OnIdle event: called after each event is processed
  void onIdle();

  void onTimerJoost();
  void onTimerRichel();
  void onRichelAiTimer();
  void onWolkTimer();

  /// Show the message and close the game
  void gameOver(const QString& message);
};

#endif // QTKNOKFIGHTERWIDGET_H
