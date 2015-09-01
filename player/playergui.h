// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef CALCULATOR_H
#define CALCULATOR_H
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

#include <QWidget>
#include <idisplay.h>
#include <ihardware.h>
#include <playerlogic.h>



QT_BEGIN_NAMESPACE
class QLineEdit;
class QButtonGroup;
class QTimer;
class QLabel;
class QLineEdit;
QT_END_NAMESPACE
class Button;

//! [0]
class PlayerGui : public QWidget, public IDisplay, public IHardware
{
    Q_OBJECT

public:
    PlayerGui(QWidget *parent = 0);

private slots:
    void playClicked();
    void stopClicked();
    void pauseClicked();
    void previousSongClicked();
    void nextSongClicked();
    void volumeUpButtonClicked();
    void volumeDownButtonClicked();
    void openCloseClicked();
    void timerDone();
    void cdDetectedClicked();
    //void startOpenCloseTimer(int intervalMs);

private:
    Button *createButton(const QString &text, const char *member);
    // from IDisplay
    virtual void setDisplayText(QString const& text);
    // from IHardware
    virtual void openDrawer(boost::function<void()> callback);
    virtual void closeDrawer(boost::function<void()> callback);
    virtual void startPlaying();
    virtual void stopPlaying();
    virtual void pausePlaying();
    virtual void nextSong();
    virtual void previousSong();
    virtual void volumeUp();
    virtual void volumeDown();
    virtual void startTimer(int intervalMs,boost::function<void()> callback);
    virtual void stopTimer();

    void checkEnabledButtons();
    virtual void customEvent(QEvent* event);

    QLineEdit *display;
    Button *playButton;
    Button *stopButton;
    Button *pauseButton;
    Button *openCloseButton;
    Button *cdDetected;
    Button *nextSongButton;
    Button *previousSongButton;
    Button *volumeUpButton;
    Button *volumeDownButton;
    QButtonGroup* diskInfoButtonGroup;
    QTimer* timer;
    QLabel* hwActionsLabel;
    QLineEdit* hwActions;

    PlayerLogic logic_;
    // our asio worker thread
    // is used to simulate hardware
    boost::shared_ptr<boost::asio::io_service > ioservice_;
    boost::shared_ptr<boost::asio::io_service::work> work_;
    boost::shared_ptr<boost::thread> thread_;


    boost::function<void()> currentCallback;

};


#endif
