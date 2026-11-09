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
#include "qtknokfighterwidget.h"

#include <QBitmap>
#include <QFrame>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QProgressBar>
#include <QRandomGenerator>
#ifdef Q_OS_WASM
// Qt Multimedia is not available for WebAssembly:
// the web version plays the sounds with the browser's Audio objects
#include <emscripten/val.h>
#else
#include <QSoundEffect>
#endif
#include <QUrl>

#include <algorithm>

namespace {

const int JOOSTSTAATLINKS            =  1;
const int JOOSTSTAATRECHTS           =  2;
const int JOOSTLOOPTLINKS1           =  3;
const int JOOSTLOOPTLINKS2           =  4;
const int JOOSTLOOPTRECHTS1          =  5;
const int JOOSTLOOPTRECHTS2          =  6;
const int JOOSTSLAATLINKS1           =  7;
//const int JOOSTSLAATLINKS2           =  8;
const int JOOSTSCHOPTLANGZAAMLINKS   =  9;
const int JOOSTSCHOPTSNELLINKS       = 10;
const int JOOSTSLAATRECHTS1          = 11;
//const int JOOSTSLAATRECHTS2          = 12;
const int JOOSTSCHOPTLANGZAAMRECHTS  = 13;
const int JOOSTSCHOPTSNELRECHTS      = 14;
const int JOOSTSLAATLINKSLAAG        = 15;
const int JOOSTSLAATRECHTSLAAG       = 16;

const int RICHELSTAATLINKS           =   1;
const int RICHELSTAATRECHTS          =   2;
const int RICHELLOOPTLINKS1          =   3;
const int RICHELLOOPTLINKS2          =   4;
const int RICHELLOOPTRECHTS1         =   5;
const int RICHELLOOPTRECHTS2         =   6;
//HP
const int RICHELSLAATHOOGLINKS1      =   7;
//const int RICHELSLAATHOOGLINKS2      =   8;
const int RICHELSLAATHOOGRECHTS1     =   9;
//const int RICHELSLAATHOOGRECHTS2     =  10;
//LP
const int RICHELSLAATLAAGLINKS       =  11;
const int RICHELSLAATLAAGRECHTS      =  12;
//HK
const int RICHELSCHOPTHOOGLINKS      =  13;
const int RICHELSCHOPTHOOGRECHTS     =  14;
//LK
const int RICHELSCHOPTLAAGLINKS      =  15;
const int RICHELSCHOPTLAAGRECHTS     =  16;

const int VK_JLP = Qt::Key_A;
const int VK_JHP = Qt::Key_Q;
const int VK_JLK = Qt::Key_S;
const int VK_JHK = Qt::Key_W;

// VCL colours, without alpha
const QRgb clRed  = 0xFF0000;
const QRgb clLime = 0x00FF00;

const QStringList sound_names = {
  "Joostschophoog", "Joostschoplaag", "Joostslaathoog", "Joostslaatlaag",
  "Richelschopthoog", "Richelschoptlaag", "Richelslaathoog", "Richelslaatlaag"
};

int random(const int n) { return QRandomGenerator::global()->bounded(n); }

} // namespace

void Picture::load(const QString& name)
{
  m_image = QImage(":/images/" + name + ".png").convertToFormat(QImage::Format_RGB32);
  Q_ASSERT(!m_image.isNull());
  m_pixmap = QPixmap::fromImage(m_image);
  if (m_transparent)
  {
    // As a VCL TBitmap with TransparentMode = tmAuto:
    // the bottom-left pixel's colour is transparent
    const QColor transparent_color{m_image.pixel(0, m_image.height() - 1)};
    m_pixmap.setMask(m_pixmap.createMaskFromColor(transparent_color, Qt::MaskInColor));
  }
}

QRgb Picture::pixel(const int x, const int y) const
{
  if (!m_image.valid(x, y)) return 0xFFFFFFFF;
  return m_image.pixel(x, y) & 0xFFFFFF;
}

QtKnokfighterWidget::QtKnokfighterWidget(QWidget* parent)
  : QWidget(parent),
    m_joost(true),
    m_richel(true),
    m_wolk(true),
    m_screen(800, 600, QImage::Format_RGB32)
{
  setWindowTitle("Battle of honour");
  setFixedSize(852, 658);
  setFocusPolicy(Qt::StrongFocus);
  QPalette p = palette();
  p.setColor(QPalette::Window, Qt::black);
  setPalette(p);
  setAutoFillBackground(true);
  m_screen.fill(Qt::black);

  // The pictures stored in the .dfm file
  m_image_background.load("Background");
  m_startscherm.load("Startscherm");
  m_richel.load("RichelStaatRechts");
  m_richel.m_left = 8;
  m_richel.m_top = 240;
  m_joost.load("JoostStaatLinks");
  m_joost.m_left = 584;
  m_joost.m_top = 256;
  m_wolk.m_left = 288;
  m_wolk.m_top = 56;

#ifndef Q_OS_WASM
  for (const QString& name: sound_names)
  {
    QSoundEffect* const sound = new QSoundEffect(this);
    sound->setSource(QUrl("qrc:/sounds/" + name + ".wav"));
    m_sounds.insert(name, sound);
  }
#endif

  createPanels();

  connect(&m_timer_joost, &QTimer::timeout, this, &QtKnokfighterWidget::onTimerJoost);
  connect(&m_timer_richel, &QTimer::timeout, this, &QtKnokfighterWidget::onTimerRichel);
  connect(&m_richel_ai_timer, &QTimer::timeout, this, &QtKnokfighterWidget::onRichelAiTimer);
  connect(&m_wolk_timer, &QTimer::timeout, this, &QtKnokfighterWidget::onWolkTimer);

  // The timer settings of the .dfm file
  m_timer_joost.setInterval(1000); // Disabled
  m_timer_richel.start(1000);
  m_richel_ai_timer.start(800);
  m_wolk_timer.setInterval(350); // Disabled

  QTimer::singleShot(0, this, &QtKnokfighterWidget::onIdle);
  QTimer::singleShot(100, this, &QtKnokfighterWidget::focusInBrowser);
}

void QtKnokfighterWidget::focusInBrowser()
{
#ifdef Q_OS_WASM
  // In the web version, the keys only arrive if the window's focus helper
  // element has the browser's focus. Qt does not always give it the focus
  // (e.g. at startup, or after clicking on an already active window),
  // so do so here. This uses the web page structure of Qt 6.11
  using emscripten::val;
  const val container = val::global("document").call<val>("getElementById", std::string("qt-shadow-container"));
  if (container.isNull() || container["shadowRoot"].isNull()) return;
  const val helper = container["shadowRoot"].call<val>("querySelector", std::string(".qt-window-focus-helper"));
  if (helper.isNull()) return;
  helper.call<void>("focus");
#endif
}

void QtKnokfighterWidget::mousePressEvent(QMouseEvent* event)
{
  focusInBrowser();
  QWidget::mousePressEvent(event);
}

void QtKnokfighterWidget::createPanels()
{
  QFont label_font = font();
  label_font.setPixelSize(16);
  label_font.setBold(true);

  // Panel1: the health bars
  QFrame* const panel1 = new QFrame(this);
  panel1->setGeometry(3, 604, 798, 49);
  panel1->setFrameStyle(QFrame::Panel | QFrame::Raised);
  panel1->setAutoFillBackground(true);
  QPalette p1 = panel1->palette();
  p1.setColor(QPalette::Window, QColor(192, 192, 192)); //clSilver
  p1.setColor(QPalette::WindowText, Qt::black);
  panel1->setPalette(p1);

  QLabel* const label1 = new QLabel("Richel", panel1);
  label1->setFont(label_font);
  label1->move(8, 2);
  QLabel* const label2 = new QLabel("Joost", panel1);
  label2->setFont(label_font);
  label2->move(9, 23);

  m_richel_health_bar = new QProgressBar(panel1);
  m_richel_health_bar->setGeometry(64, 3, 729, 17);
  m_joost_health_bar = new QProgressBar(panel1);
  m_joost_health_bar->setGeometry(64, 27, 729, 17);
  for (QProgressBar* const bar: {m_richel_health_bar, m_joost_health_bar})
  {
    bar->setRange(0, 1000);
    bar->setValue(1000);
    bar->setTextVisible(false);
    bar->setFocusPolicy(Qt::NoFocus);
  }

  // Panel2: the vertical 'Knokfighter' title
  QFrame* const panel2 = new QFrame(this);
  panel2->setGeometry(804, 3, 43, 646);
  panel2->setFrameStyle(QFrame::Panel | QFrame::Raised);
  panel2->setAutoFillBackground(true);
  QPalette p2 = panel2->palette();
  p2.setColor(QPalette::Window, QColor(0, 128, 128)); //clTeal
  panel2->setPalette(p2);
  QLabel* const image1 = new QLabel(panel2);
  image1->setPixmap(QPixmap(":/images/Knokfighter.png"));
  image1->move(5, 5);
  image1->adjustSize();
}

void QtKnokfighterWidget::playSound(const QString& name)
{
#ifdef Q_OS_WASM
  // The .wav files are in the 'sounds' folder next to the web page
  using emscripten::val;
  static val playing_sound = val::null();
  if (!playing_sound.isNull()) playing_sound.call<void>("pause");
  playing_sound = val::global("Audio").new_(val(("sounds/" + name + ".wav").toStdString()));
  playing_sound.call<val>("play");
#else
  if (m_playing_sound) m_playing_sound->stop();
  m_playing_sound = m_sounds.value(name);
  if (m_playing_sound) m_playing_sound->play();
#endif
}

void QtKnokfighterWidget::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.drawImage(0, 0, m_screen);
}

void QtKnokfighterWidget::onIdle()
{
  // 'canvas' is the VCL ImageCanvas, 'screen' is FormMain->Canvas
  const auto draw = [](QImage& target, const Picture& picture)
  {
    if (picture.m_pixmap.isNull()) return;
    QPainter painter(&target);
    painter.drawPixmap(picture.m_left, picture.m_top, picture.m_pixmap);
  };
  const auto draw_on_screen = [this](const QImage& canvas)
  {
    QPainter painter(&m_screen);
    painter.drawImage(0, 0, canvas);
  };

  switch (m_program_state)
  {
    case ProgState::Running:
    {
      //TEKEN HET SCHERM
      //Zet het plaatje van de geheime canvas gelijk aan de achtergrond
      QImage canvas = m_image_background.m_image;
      //Teken Richel op de achtergrond
      draw(canvas, m_richel);
      //Teken Joost op de achtergrond
      draw(canvas, m_joost);
      //Teken het schadewolkje!
      draw(canvas, m_wolk);
      //Teken de geheime canvas op de Form
      draw_on_screen(canvas);
      break;
    }
    case ProgState::PreRunning:
    {
      //Zet het plaatje van de geheime canvas gelijk aan de achtergrond
      QImage canvas = m_image_background.m_image;
      //Zet Joost op de juiste plek
      m_joost.m_left = canvas.width() - m_joost.width() - 20;
      m_joost.m_top = canvas.height() - m_joost.height() - 20;
      //Zet Richel op de juiste plek
      m_richel.m_left = 20;
      m_richel.m_top = canvas.height() - m_richel.height() - 20;
      //Teken Joost op de achtergrond
      draw(canvas, m_joost);
      //Teken Richel op de achtergrond
      draw(canvas, m_richel);
      //Teken de geheime canvas op de Form
      draw_on_screen(canvas);
      m_joost.m_tag = JOOSTSTAATLINKS;
      m_richel.m_tag = RICHELSTAATRECHTS;
      m_program_state = ProgState::Running;
      break;
    }
    case ProgState::Initialized:
      break;
    case ProgState::UnInitialized:
      //Teken het startscherm op FormMain
      draw_on_screen(m_startscherm.m_image);
      m_program_state = ProgState::Initialized;
      break;
  }
  update();
}

void QtKnokfighterWidget::keyPressEvent(QKeyEvent* event)
{
  switch (m_program_state)
  {
    case ProgState::Running:
      //Het programma loopt. Welke toets wordt ingedrukt?
      switch (event->key())
      {
        case Qt::Key_Left:
          //Ha, links wordt ingedrukt. Hoe staat het met Joost nu?
          switch (m_joost.m_tag)
          {
            case JOOSTSTAATLINKS:
              //Joost staat en loopt nu naar links
              m_joost.load("JoostLooptLinks1");
              m_joost_shadow.load("JoostLooptLinks1Shadow");
              m_joost.m_left -= 10;
              m_joost.m_tag = JOOSTLOOPTLINKS1;
              m_timer_joost.start(250);
              break;
            case JOOSTSTAATRECHTS:
              //Joost kijkt naar rechts en gaat nu naar links kijken
              m_joost.load("JoostStaatLinks");
              m_joost_shadow.load("JoostStaatLinksShadow");
              m_joost.m_tag = JOOSTSTAATLINKS;
              break;
          }
          break;
        case Qt::Key_Right:
          //Ha, rechts wordt ingedrukt. Hoe staat het met Joost nu?
          switch (m_joost.m_tag)
          {
            case JOOSTSTAATLINKS:
              //Joost kijkt naar links en gaat nu naar rechts kijken
              //BUG (kept from the original VCL version): this loads the
              //facing-left picture, where it should load 'JoostStaatRechts'.
              //Joost faces right (his Tag says so), but is drawn facing left
              m_joost.load("JoostStaatLinks");
              m_joost_shadow.load("JoostStaatLinksShadow");
              m_joost.m_tag = JOOSTSTAATRECHTS;
              break;
            case JOOSTSTAATRECHTS:
              //Joost staat en loopt nu naar rechts
              m_joost.load("JoostLooptRechts1");
              m_joost_shadow.load("JoostLooptRechts1Shadow");
              m_joost.m_left += 10;
              m_joost.m_tag = JOOSTLOOPTRECHTS1;
              m_timer_joost.start(250);
              break;
          }
          break;
        case VK_JLP:
          //de Low Punch knop van Joost wordt ingedrukt. Hoe staat Joost erbij?
          switch (m_joost.m_tag)
          {
            case JOOSTSTAATLINKS:
              m_joost_bonus = 55;
              playSound("Joostslaatlaag");
              m_joost.load("JoostSlaatLinksLaag");
              m_joost_shadow.load("JoostSlaatLinksLaagShadow");
              m_joost.m_tag = JOOSTSLAATLINKSLAAG;
              m_timer_joost.start(500);
              break;
            case JOOSTSTAATRECHTS:
              m_joost_bonus = 55;
              playSound("Joostslaatlaag");
              m_joost.load("JoostSlaatRechtsLaag");
              m_joost_shadow.load("JoostSlaatRechtsLaagShadow");
              m_joost.m_tag = JOOSTSLAATRECHTSLAAG;
              m_timer_joost.start(500);
              break;
          }
          break;
        case VK_JHP:
          //de High Punch knop van Joost wordt ingedrukt. Hoe staat Joost erbij?
          switch (m_joost.m_tag)
          {
            case JOOSTSTAATLINKS:
              m_joost_bonus = 45;
              playSound("Joostslaathoog");
              m_joost.load("JoostSlaatLinks1");
              m_joost_shadow.load("JoostSlaatLinks1Shadow");
              m_joost.m_tag = JOOSTSLAATLINKS1;
              m_timer_joost.start(500);
              break;
            case JOOSTSTAATRECHTS:
              m_joost_bonus = 45;
              playSound("Joostslaathoog");
              m_joost.load("JoostSlaatRechts1");
              m_joost_shadow.load("JoostSlaatRechts1Shadow");
              m_joost.m_tag = JOOSTSLAATRECHTS1;
              m_timer_joost.start(500);
              break;
          }
          break;
        case VK_JLK:
          //de Low Kick knop van Joost wordt ingedrukt. Hoe staat Joost erbij?
          switch (m_joost.m_tag)
          {
            case JOOSTSTAATLINKS:
              m_joost_bonus = 40;
              playSound("Joostschoplaag");
              m_joost.load("JoostSchoptSnelLinks");
              m_joost_shadow.load("JoostSchoptSnelLinksShadow");
              m_joost.m_tag = JOOSTSCHOPTSNELLINKS;
              m_timer_joost.start(500);
              m_joost.m_top += 30;
              break;
            case JOOSTSTAATRECHTS:
              m_joost_bonus = 40;
              playSound("Joostschoplaag");
              m_joost.load("JoostSchoptSnelRechts");
              m_joost_shadow.load("JoostSchoptSnelRechtsShadow");
              m_joost.m_tag = JOOSTSCHOPTSNELRECHTS;
              m_timer_joost.start(500);
              m_joost.m_top += 30;
              break;
          }
          break;
        case VK_JHK:
          //de High Kick knop van Joost wordt ingedrukt. Hoe staat Joost erbij?
          switch (m_joost.m_tag)
          {
            case JOOSTSTAATLINKS:
              m_joost_bonus = 30;
              playSound("Joostschophoog");
              m_joost.m_top -= 80;
              m_joost.load("JoostSchoptLangzaamLinks");
              m_joost_shadow.load("JoostSchoptLangzaamLinksShadow");
              m_joost.m_tag = JOOSTSCHOPTLANGZAAMLINKS;
              m_timer_joost.start(1500);
              break;
            case JOOSTSTAATRECHTS:
              m_joost_bonus = 30;
              playSound("Joostschophoog");
              m_joost.m_top -= 80;
              m_joost.load("JoostSchoptLangzaamRechts");
              m_joost_shadow.load("JoostSchoptLangzaamRechtsShadow");
              m_joost.m_tag = JOOSTSCHOPTLANGZAAMRECHTS;
              m_timer_joost.start(1500);
              break;
          }
          break;
        default:
          break;
      }
      break;
    case ProgState::Initialized:
      m_program_state = ProgState::PreRunning;
      break;
    default:
      break;
  }
  onIdle();
}

void QtKnokfighterWidget::onTimerJoost()
{
  //Doet Joost Richel schade, zo ja hoeveel?
  int richel_damage = 0;
  m_richel_shadow.m_left = m_richel.m_left; //shadow achter figuur plaatsen
  m_richel_shadow.m_top = m_richel.m_top;
  m_joost_shadow.m_left = m_joost.m_left;
  m_joost_shadow.m_top = m_joost.m_top;
  //Joosts Shadows rode gedeelte doet schade:
  for (int a = 0; a < m_joost_shadow.width(); ++a)
  {
    for (int b = 0; b < m_joost_shadow.height(); ++b)
    {
      if (m_joost_shadow.pixel(a, b) == clRed)
      {
        const int c = a + m_joost_shadow.m_left - m_richel_shadow.m_left;
        const int d = b + m_joost_shadow.m_top - m_richel_shadow.m_top;
        if (m_richel_shadow.pixel(c, d) == clLime) ++richel_damage;
        //wolkje
        if (!m_wolk_timer.isActive())
        {
          m_wolk_timer.start();
          m_wolk.m_left = a + m_joost_shadow.m_left + 10;
          m_wolk.m_top = b + m_joost_shadow.m_top + 10;
        }
      }
    }
  }

  richel_damage = richel_damage / m_joost_bonus;
  m_richel_health = m_richel_health - richel_damage;
  m_richel_health_bar->setValue(std::clamp(m_richel_health, 0, 1000));

  switch (m_joost.m_tag)
  {
    case JOOSTLOOPTLINKS1:
      m_joost.load("JoostLooptLinks2");
      m_joost_shadow.load("JoostLooptLinks2Shadow");
      m_joost.m_tag = JOOSTLOOPTLINKS2;
      m_joost.m_left -= 15;
      break;
    case JOOSTLOOPTLINKS2:
      m_joost.load("JoostStaatLinks");
      m_joost_shadow.load("JoostStaatLinksShadow");
      m_joost.m_left -= 15;
      m_joost.m_tag = JOOSTSTAATLINKS;
      m_timer_joost.stop();
      break;
    case JOOSTLOOPTRECHTS1:
      m_joost.load("JoostLooptRechts2");
      m_joost_shadow.load("JoostLooptRechts2Shadow");
      m_joost.m_tag = JOOSTLOOPTRECHTS2;
      m_joost.m_left += 15;
      break;
    case JOOSTLOOPTRECHTS2:
      m_joost.load("JoostStaatRechts");
      m_joost_shadow.load("JoostStaatRechtsShadow");
      m_joost.m_tag = JOOSTSTAATRECHTS;
      m_joost.m_left += 15;
      m_timer_joost.stop();
      break;
    case JOOSTSLAATLINKS1:
    case JOOSTSLAATLINKSLAAG:
      m_joost.load("JoostStaatLinks");
      m_joost_shadow.load("JoostStaatLinksShadow");
      m_joost.m_tag = JOOSTSTAATLINKS;
      m_timer_joost.stop();
      break;
    case JOOSTSLAATRECHTS1:
    case JOOSTSLAATRECHTSLAAG:
      m_joost.load("JoostStaatRechts");
      m_joost_shadow.load("JoostStaatRechtsShadow");
      m_joost.m_tag = JOOSTSTAATRECHTS;
      m_timer_joost.stop();
      break;
    case JOOSTSCHOPTLANGZAAMLINKS:
      m_joost.m_top += 80;
      m_joost.load("JoostStaatLinks");
      m_joost_shadow.load("JoostStaatLinksShadow");
      m_joost.m_tag = JOOSTSTAATLINKS;
      m_timer_joost.stop();
      break;
    case JOOSTSCHOPTLANGZAAMRECHTS:
      m_joost.m_top += 80;
      m_joost.load("JoostStaatRechts");
      m_joost_shadow.load("JoostStaatRechtsShadow");
      m_joost.m_tag = JOOSTSTAATRECHTS;
      m_timer_joost.stop();
      break;
    case JOOSTSCHOPTSNELLINKS:
      m_joost.m_top -= 30;
      m_joost.load("JoostStaatLinks");
      m_joost_shadow.load("JoostStaatLinksShadow");
      m_joost.m_tag = JOOSTSTAATLINKS;
      m_timer_joost.stop();
      break;
    case JOOSTSCHOPTSNELRECHTS:
      m_joost.m_top -= 30;
      m_joost.load("JoostStaatRechts");
      m_joost_shadow.load("JoostStaatRechtsShadow");
      m_joost.m_tag = JOOSTSTAATRECHTS;
      m_timer_joost.stop();
      break;
  }
  onIdle();
}

void QtKnokfighterWidget::onTimerRichel()
{
  //Doet Richel Joost schade, zo ja hoeveel?
  int joost_damage = 0;
  m_richel_shadow.m_left = m_richel.m_left; //shadow achter figuur plaatsen
  m_richel_shadow.m_top = m_richel.m_top;
  m_joost_shadow.m_left = m_joost.m_left;
  m_joost_shadow.m_top = m_joost.m_top;
  //Richels Shadows rode gedeelte doet schade:
  for (int a = 0; a < m_richel_shadow.width(); ++a)
  {
    for (int b = 0; b < m_richel_shadow.height(); ++b)
    {
      if (m_richel_shadow.pixel(a, b) == clRed)
      {
        //raken ze elkander?
        const int c = a + m_richel_shadow.m_left - m_joost_shadow.m_left;
        const int d = b + m_richel_shadow.m_top - m_joost_shadow.m_top;
        if (m_joost_shadow.pixel(c, d) == clLime) ++joost_damage;
        if (!m_wolk_timer.isActive())
        {
          m_wolk_timer.start();
          m_wolk.m_left = a + m_richel_shadow.m_left + 10;
          m_wolk.m_top = b + m_richel_shadow.m_top + 10;
        }
      }
    }
  }

  joost_damage = joost_damage / m_richel_bonus;
  m_joost_health = m_joost_health - joost_damage;
  m_joost_health_bar->setValue(std::clamp(m_joost_health, 0, 1000));

  switch (m_richel.m_tag)
  {
    case RICHELLOOPTLINKS1:
      m_richel.load("RichelLooptLinks2");
      m_richel_shadow.load("RichelLooptLinks2Shadow");
      m_richel.m_tag = RICHELLOOPTLINKS2;
      m_richel.m_left -= 20;
      break;
    case RICHELLOOPTLINKS2:
      m_richel.load("RichelStaatLinks");
      m_richel_shadow.load("RichelStaatLinksShadow");
      m_richel.m_left -= 10;
      m_richel.m_tag = RICHELSTAATLINKS;
      m_timer_richel.stop();
      break;
    case RICHELLOOPTRECHTS1:
      m_richel.load("RichelLooptRechts2");
      m_richel_shadow.load("RichelLooptRechts2Shadow");
      m_richel.m_tag = RICHELLOOPTRECHTS2;
      m_richel.m_left += 20;
      break;
    case RICHELLOOPTRECHTS2:
      m_richel.load("RichelStaatRechts");
      m_richel_shadow.load("RichelStaatRechtsShadow");
      m_richel.m_left += 10;
      m_richel.m_tag = RICHELSTAATRECHTS;
      m_timer_richel.stop();
      break;
    case RICHELSLAATHOOGLINKS1:
    case RICHELSLAATLAAGLINKS:
    case RICHELSCHOPTHOOGLINKS:
      m_richel.load("RichelStaatLinks");
      m_richel_shadow.load("RichelStaatLinksShadow");
      m_richel.m_tag = RICHELSTAATLINKS;
      m_timer_richel.stop();
      break;
    case RICHELSLAATHOOGRECHTS1:
    case RICHELSLAATLAAGRECHTS:
    case RICHELSCHOPTHOOGRECHTS:
      m_richel.load("RichelStaatRechts");
      m_richel_shadow.load("RichelStaatRechtsShadow");
      m_richel.m_tag = RICHELSTAATRECHTS;
      m_timer_richel.stop();
      break;
    case RICHELSCHOPTLAAGLINKS:
      m_richel.m_top += 80;
      m_richel.load("RichelStaatLinks");
      m_richel_shadow.load("RichelStaatLinksShadow");
      m_richel.m_tag = RICHELSTAATLINKS;
      m_timer_richel.stop();
      break;
    case RICHELSCHOPTLAAGRECHTS:
      m_richel.m_top += 80;
      m_richel.load("RichelStaatRechts");
      m_richel_shadow.load("RichelStaatRechtsShadow");
      m_richel.m_tag = RICHELSTAATRECHTS;
      m_timer_richel.stop();
      break;
  }
  onIdle();
}

void QtKnokfighterWidget::onRichelAiTimer()
{
  //RICHEL'S AI
  if (m_richel.m_left - m_joost.m_left > 100)
  {
    //Ha, Richel moet naar links. Hoe staat het met Richel nu?
    switch (m_richel.m_tag)
    {
      case RICHELSTAATLINKS:
        //Richel staat en loopt nu naar links
        m_richel.load("RichelLooptLinks1");
        m_richel_shadow.load("RichelLooptLinks1Shadow");
        m_richel.m_left -= 10;
        m_richel.m_tag = RICHELLOOPTLINKS1;
        m_timer_richel.start(250);
        break;
      case RICHELSTAATRECHTS:
        //Richel kijkt naar rechts en gaat nu naar links kijken
        m_richel.load("RichelStaatLinks");
        m_richel_shadow.load("RichelStaatLinksShadow");
        m_richel.m_tag = RICHELSTAATLINKS;
        break;
    }
  }
  else if (m_joost.m_left - m_richel.m_left > 100) //naar rechts?
  {
    //naar rechts. Hoe staat het met Richel nu?
    switch (m_richel.m_tag)
    {
      case RICHELSTAATLINKS:
        //Richel kijkt naar links en gaat nu naar rechts kijken
        m_richel.load("RichelStaatRechts");
        m_richel_shadow.load("RichelStaatRechtsShadow");
        m_richel.m_tag = RICHELSTAATRECHTS;
        break;
      case RICHELSTAATRECHTS:
        //Richel staat en loopt nu naar rechts
        m_richel.load("RichelLooptRechts1");
        m_richel_shadow.load("RichelLooptRechts1Shadow");
        m_richel.m_left += 10;
        m_richel.m_tag = RICHELLOOPTRECHTS1;
        m_timer_richel.start(250);
        break;
    }
  }
  else if (m_richel.m_tag == RICHELSTAATRECHTS && m_richel.m_left - m_joost.m_left > 0)
  {
    //als richel de verkeerde kant opkijkt...
    m_richel.load("RichelStaatLinks");
    m_richel_shadow.load("RichelStaatLinksShadow");
    m_richel.m_tag = RICHELSTAATLINKS;
  }
  else if (m_richel.m_tag == RICHELSTAATLINKS && m_richel.m_left - m_joost.m_left < 0)
  {
    //als richel de verkeerde kant opkijkt...
    m_richel.load("RichelStaatRechts");
    m_richel_shadow.load("RichelStaatRechtsShadow");
    m_richel.m_tag = RICHELSTAATRECHTS;
  }
  else //lopen is niet noodzakelijk, richel moet een random vechtbeweging doen:
  {
    switch (random(6))
    {
      case 1: //een Low Punch. Hoe staat Richel erbij?
        switch (m_richel.m_tag)
        {
          case RICHELSTAATLINKS:
            m_richel_bonus = 30;
            playSound("Richelslaatlaag");
            m_richel.load("RichelSlaatLaagLinks");
            m_richel_shadow.load("RichelSlaatLaagLinksShadow");
            m_richel.m_tag = RICHELSLAATLAAGLINKS;
            m_timer_richel.start(500);
            break;
          case RICHELSTAATRECHTS:
            m_richel_bonus = 30;
            playSound("Richelslaatlaag");
            m_richel.load("RichelSlaatLaagRechts");
            m_richel_shadow.load("RichelSlaatLaagRechtsShadow");
            m_richel.m_tag = RICHELSLAATLAAGRECHTS;
            m_timer_richel.start(500);
            break;
        }
        break;
      case 2: //een High Punch. Hoe staat Richel erbij?
        switch (m_richel.m_tag)
        {
          case RICHELSTAATLINKS:
            m_richel_bonus = 30;
            playSound("Richelslaathoog");
            m_richel.load("RichelSlaatHoogLinks");
            m_richel_shadow.load("RichelSlaatHoogLinksShadow");
            m_richel.m_tag = RICHELSLAATHOOGLINKS1;
            m_timer_richel.start(500);
            break;
          case RICHELSTAATRECHTS:
            m_richel_bonus = 30;
            playSound("Richelslaathoog");
            m_richel.load("RichelSlaatHoogRechts");
            m_richel_shadow.load("RichelSlaatHoogRechtsShadow");
            m_richel.m_tag = RICHELSLAATHOOGRECHTS1;
            m_timer_richel.start(500);
            break;
        }
        break;
      case 3: //een Low Kick. Hoe staat Richel erbij?
        switch (m_richel.m_tag)
        {
          case RICHELSTAATLINKS:
            m_richel_bonus = 40;
            playSound("Richelschoptlaag");
            m_richel.load("RichelSchoptLaagLinks");
            m_richel_shadow.load("RichelSchoptLaagLinksShadow");
            m_richel.m_tag = RICHELSCHOPTLAAGLINKS;
            m_timer_richel.start(500);
            m_richel.m_top -= 80;
            break;
          case RICHELSTAATRECHTS:
            m_richel_bonus = 40;
            playSound("Richelschoptlaag");
            m_richel.load("RichelSchoptLaagRechts");
            m_richel_shadow.load("RichelSchoptLaagRechtsShadow");
            m_richel.m_tag = RICHELSCHOPTLAAGRECHTS;
            m_timer_richel.start(500);
            m_richel.m_top -= 80;
            break;
        }
        break;
      case 4: //een High Kick. Hoe staat Richel erbij?
        switch (m_richel.m_tag)
        {
          case RICHELSTAATLINKS:
            m_richel_bonus = 40;
            playSound("Richelschopthoog");
            m_richel.load("RichelSchoptHoogLinks");
            m_richel_shadow.load("RichelSchoptHoogLinksShadow");
            m_richel.m_tag = RICHELSCHOPTHOOGLINKS;
            m_timer_richel.start(500);
            break;
          case RICHELSTAATRECHTS:
            m_richel_bonus = 40;
            playSound("Richelschopthoog");
            m_richel.load("RichelSchoptHoogRechts");
            m_richel_shadow.load("RichelSchoptHoogRechtsShadow");
            m_richel.m_tag = RICHELSCHOPTHOOGRECHTS;
            m_timer_richel.start(500);
            break;
        }
        break;
    }
  }
  onIdle();
}

void QtKnokfighterWidget::gameOver(const QString& message)
{
  m_richel_ai_timer.stop();
  m_timer_joost.stop();
  m_timer_richel.stop();
  // Not QMessageBox::information, as a blocking dialog
  // does not work in the web (WebAssembly) version
  QMessageBox* const box = new QMessageBox(QMessageBox::Information, windowTitle(), message, QMessageBox::Ok, this);
  box->setAttribute(Qt::WA_DeleteOnClose);
  connect(box, &QMessageBox::finished, this, &QWidget::close);
  box->open();
}

void QtKnokfighterWidget::onWolkTimer()
{
  //tekst inladen
  switch (random(3))
  {
    case 0: m_wolk.load("Wolk3"); break;
    case 1: m_wolk.load("Wolk2"); break;
    case 2: m_wolk.load("Wolk1"); break;
  }

  //wolk verstoppen
  m_wolk.m_top = -500;
  m_wolk_timer.stop();

  //als het spel afgelopen is:
  if (m_richel_health_bar->value() < 1)
  {
    gameOver("Richel is dead! Evil is defeated!");
    return;
  }
  else if (m_joost_health_bar->value() < 1)
  {
    gameOver("Joost is defeated! All hope for mankind is lost!");
    return;
  }
  onIdle();
}
