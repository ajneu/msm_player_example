// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "playergui.h"

#include <QtGui>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLabel>
#include <QApplication>
#include <chrono>

#include "button.h"


namespace {
// these custom events will be send to PlayerGui from the HW thread when HW action completes
class OpenedEvent : public QEvent {
public:
    OpenedEvent() : QEvent((QEvent::Type)2000) {}
    virtual ~OpenedEvent(){}
};
class ClosedEvent : public QEvent {
public:
    ClosedEvent() : QEvent((QEvent::Type)2001) {}
    virtual ~ClosedEvent(){}
};

}

PlayerGui::PlayerGui(QWidget *parent)
    : QWidget(parent),
      logic_(this,this),
      ioservice_(std::make_shared<boost::asio::io_service>()),
      work_(new boost::asio::io_service::work(*ioservice_)),
      thread_()
{
    auto service = ioservice_;
    thread_ = std::make_shared<std::thread>([service](){service->run();});

    display = new QLineEdit("");

    display->setReadOnly(true);
    display->setAlignment(Qt::AlignRight);
    display->setMaxLength(20);

    QFont font = display->font();
    font.setPointSize(font.pointSize() + 8);
    display->setFont(font);

    playButton = createButton(tr("Play"), SLOT(playClicked()));
    stopButton = createButton(tr("Stop"), SLOT(stopClicked()));
    pauseButton = createButton(tr("Pause"), SLOT(pauseClicked()));
    openCloseButton = createButton(tr("Open/Close"), SLOT(openCloseClicked()));
    previousSongButton = createButton(tr("Prev"), SLOT(previousSongClicked()));
    nextSongButton = createButton(tr("Next"), SLOT(nextSongClicked()));
    volumeUpButton = createButton(tr("Vol +"), SLOT(volumeUpButtonClicked()));
    volumeDownButton = createButton(tr("Vol -"), SLOT(volumeDownButtonClicked()));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    mainLayout->addWidget(display);
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(openCloseButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addWidget(pauseButton);
    buttonLayout->addWidget(playButton);
    buttonLayout->addWidget(previousSongButton);
    buttonLayout->addWidget(nextSongButton);
    buttonLayout->addWidget(volumeDownButton);
    buttonLayout->addWidget(volumeUpButton);
    mainLayout->addItem(buttonLayout);

    QHBoxLayout *hwSimuLayout = new QHBoxLayout;
    cdDetected = createButton(tr("CD Detected"), SLOT(cdDetectedClicked()));
    hwSimuLayout->addWidget(cdDetected);
    QRadioButton* goodDiskButton = new QRadioButton("Good Disk");
    goodDiskButton->setChecked(true);
    QRadioButton* badDiskButton = new QRadioButton("Bad Disk");
    diskInfoButtonGroup = new QButtonGroup;
    diskInfoButtonGroup->addButton(goodDiskButton);
    diskInfoButtonGroup->setId(goodDiskButton,0);
    diskInfoButtonGroup->addButton(badDiskButton);
    QVBoxLayout *diskButtonLayout = new QVBoxLayout;
    diskButtonLayout->addWidget(goodDiskButton);
    diskButtonLayout->addWidget(badDiskButton);

    hwSimuLayout->addItem(diskButtonLayout);

    hwActions = new QLineEdit(this);
    hwActions->setReadOnly(true);
    hwActions->setFixedWidth(200);
    hwActionsLabel = new QLabel("HW Actions:",this);
    hwActionsLabel->setBuddy(hwActions);
    hwSimuLayout->addWidget(hwActionsLabel);
    hwSimuLayout->addWidget(hwActions);

    mainLayout->addItem(hwSimuLayout);

    setLayout(mainLayout);

    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(timerDone()));
    timer->setSingleShot(true);
    setWindowTitle(tr("Player"));
    logic_.start();
    checkEnabledButtons();
}

void PlayerGui::playClicked()
{
    logic_.playButton();
    checkEnabledButtons();
}

void PlayerGui::stopClicked()
{
    logic_.stopButton();
    checkEnabledButtons();
}

void PlayerGui::pauseClicked()
{
    logic_.pauseButton();
    checkEnabledButtons();
}

void PlayerGui::openCloseClicked()
{
    logic_.openCloseButton();
    checkEnabledButtons();
}

void PlayerGui::cdDetectedClicked()
{
    DiskInfo newDisk((diskInfoButtonGroup->checkedId()==0));
    newDisk.songs.push_back("Let it be");
    newDisk.songs.push_back("Yellow Submarine");
    newDisk.songs.push_back("Yesterday");
    logic_.cdDetected(newDisk);
    checkEnabledButtons();
}
void PlayerGui::previousSongClicked()
{
    logic_.previousSongButton();
    checkEnabledButtons();
}
void PlayerGui::nextSongClicked()
{
    logic_.nextSongButton();
    checkEnabledButtons();
}
void PlayerGui::volumeUpButtonClicked()
{
    logic_.volumeUp();
    checkEnabledButtons();
}
void PlayerGui::volumeDownButtonClicked()
{
    logic_.volumeDown();
    checkEnabledButtons();
}
void PlayerGui::customEvent(QEvent *event)
{
    // if one of our custom vents, call the callback
    if ((event->type() == (QEvent::Type)2000) || (event->type() == (QEvent::Type)2001))
    {
        currentCallback();
        checkEnabledButtons();
    }
    QObject::customEvent(event);
}

void PlayerGui::startTimer(int intervalMs,std::function<void()> callback)
{
    currentCallback = callback;
    timer->start(intervalMs);
}
void PlayerGui::stopTimer()
{
    timer->stop();
}
void PlayerGui::timerDone()
{
    currentCallback();
    checkEnabledButtons();
}

Button *PlayerGui::createButton(const QString &text, const char *member)
{
    Button *button = new Button(text);
    connect(button, SIGNAL(clicked()), this, member);
    return button;
}

void PlayerGui::setDisplayText(QString const& text)
{
    display->setText(text);
}

void PlayerGui::openDrawer(std::function<void()> callback)
{
    // post job to the HW thread and wait for cutom event when done
    currentCallback = callback;
    ioservice_->post([this]()
                     {
                        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                        QApplication::postEvent(this,new OpenedEvent);
                     });
    hwActions->setText("open");
}
void PlayerGui::closeDrawer(std::function<void()> callback)
{
    // post job to the HW thread and wait for cutom event when done
    currentCallback = callback;
    ioservice_->post([this]()
                     {
                        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                        QApplication::postEvent(this,new ClosedEvent);
                     });
    hwActions->setText("close");
}
void PlayerGui::startPlaying()
{
    hwActions->setText("play");
}
void PlayerGui::stopPlaying()
{
    hwActions->setText("stop");
}
void PlayerGui::pausePlaying()
{
    hwActions->setText("pause");
}
void PlayerGui::nextSong()
{
    hwActions->setText("next song");
}
void PlayerGui::previousSong()
{
    hwActions->setText("previous song");
}
void PlayerGui::volumeUp()
{
    hwActions->setText("vol. up");
}
void PlayerGui::volumeDown()
{
    hwActions->setText("vol. down");
}
void PlayerGui::checkEnabledButtons()
{
    playButton->setEnabled(logic_.isPlayPossible());
    stopButton->setEnabled(logic_.isStopPossible());
    pauseButton->setEnabled(logic_.isPausePossible());
    nextSongButton->setEnabled(logic_.isNextSongPossible());
    previousSongButton->setEnabled(logic_.isPreviousSongPossible());
    volumeUpButton->setEnabled(logic_.isVolumeUpPossible());
    volumeDownButton->setEnabled(logic_.isVolumeDownPossible());
    if (logic_.isOpenPossible())
    {
        openCloseButton->setText("Open");
        openCloseButton->setEnabled(true);
    }
    else if (logic_.isClosePossible())
    {
        openCloseButton->setText("Close");
        openCloseButton->setEnabled(true);
    }
    else
    {
        openCloseButton->setEnabled(false);
    }
}

PlayerGui::~PlayerGui()
{
  work_.reset();
  thread_->detach();           // thread_->join(); // has different behaviour if timer is still running
}
