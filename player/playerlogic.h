// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef PLAYERLOGIC_H
#define PLAYERLOGIC_H

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

#include <idisplay.h>
#include <ihardware.h>

struct DiskInfo
{
    typedef std::vector<std::string> Songs;

    DiskInfo():goodDisk(true){}
    DiskInfo(bool good):goodDisk(good){}

    bool isGood(){return goodDisk;}
    Songs& getSongs(){return songs;}

    bool goodDisk;
    Songs songs;
};

class PlayerLogic
{
public:
    PlayerLogic(IDisplay* display, IHardware* hardware);
    // event methods
    void start();
    void playButton();
    void pauseButton();
    void stopButton();
    void openCloseButton();
    void nextSongButton();
    void previousSongButton();
    void cdDetected(DiskInfo const&);
    void volumeUp();
    void volumeDown();

    bool isStopPossible()const;
    bool isPlayPossible()const;
    bool isPausePossible()const;
    bool isOpenPossible()const;
    bool isClosePossible()const;
    bool isNextSongPossible()const;
    bool isPreviousSongPossible()const;
    bool isVolumeUpPossible()const;
    bool isVolumeDownPossible()const;

private:
    struct StateMachine;
    boost::shared_ptr<StateMachine> fsm_;
};

#endif // PLAYERLOGIC_H
