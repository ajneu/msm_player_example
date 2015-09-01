// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
//#define FUSION_MAX_VECTOR_SIZE 20
//#define FUSION_MAX_MAP_SIZE 20
//#define FUSION_MAX_SET_SIZE 20
// we have more than the default 20 transitions, so we need to require more from Boost.MPL
#include "boost/mpl/vector/vector30.hpp"
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
// back-end
#include <boost/msm/back/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/euml/euml.hpp>
#include "playerlogic.h"

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace std;
using namespace boost::msm::front::euml;

namespace
{
    // events
    BOOST_MSM_EUML_EVENT(play)
    BOOST_MSM_EUML_EVENT(pausing)
    BOOST_MSM_EUML_EVENT(open_close)
    BOOST_MSM_EUML_EVENT(opened)
    BOOST_MSM_EUML_EVENT(closed)
    BOOST_MSM_EUML_EVENT(next_song)
    BOOST_MSM_EUML_EVENT(prev_song)
    BOOST_MSM_EUML_EVENT(volume_up)
    BOOST_MSM_EUML_EVENT(volume_down)
    BOOST_MSM_EUML_EVENT(volume_timeout)
    BOOST_MSM_EUML_EVENT(update_song)
    // an event convertible from any other event (used for transitions originating from an exit pseudo state)
    struct stop_impl : msm::front::euml::euml_event<stop_impl>
    {
        stop_impl(){}
        template <class T>
        stop_impl(T const&){}
    };
    // dummy instance for table
    stop_impl stop;
    // attributes used later on
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(DiskInfo,disc_info)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(IDisplay*,display_)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(IHardware*,hardware_)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(size_t,currentSong_)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(DiskInfo::Songs,songs_)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(size_t,volume_)
    BOOST_MSM_EUML_DECLARE_ATTRIBUTE(bool,active_)
    // an event with data
    BOOST_MSM_EUML_ATTRIBUTES((attributes_ << disc_info ), cd_detected_attributes)
    BOOST_MSM_EUML_EVENT_WITH_ATTRIBUTES(cd_detected,cd_detected_attributes)

    //flags. Allow us to be more flexible than asking in which state the FSM is.
    BOOST_MSM_EUML_FLAG(PlayPossible)
    BOOST_MSM_EUML_FLAG(PausePossible)
    BOOST_MSM_EUML_FLAG(StopPossible)
    BOOST_MSM_EUML_FLAG(OpenPossible)
    BOOST_MSM_EUML_FLAG(ClosePossible)
    BOOST_MSM_EUML_FLAG(PrevNextPossible)

    // wrapper for methods
    BOOST_MSM_EUML_METHOD(IsGood_ , isGood , isGood_ , bool , bool )
    BOOST_MSM_EUML_METHOD(GetSongs_ , getSongs , getSongs_ , DiskInfo::Songs& , DiskInfo::Songs& )

    //helper for callbacks
    template <class FSM,class EVENT>
    struct Callback
    {
        Callback(FSM* fsm):fsm_(fsm){}
        void operator()()const
        {
            fsm_->process_event(evt_);
        }
    private:
        FSM* fsm_;
        EVENT evt_;
    };
    // The list of FSM states
    BOOST_MSM_EUML_STATE(( ),Init)
    BOOST_MSM_EUML_STATE((no_action,no_action,attributes_ << no_attributes_,
                          // Flag: OpenPossible, deferred: play
                          configure_<< OpenPossible << play ),Empty)

    BOOST_MSM_EUML_ACTION(OpenEntry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM& fsm,STATE& ){fsm.get_attribute(display_)->setDisplayText("Open");}
    };
    BOOST_MSM_EUML_STATE((OpenEntry,no_action,attributes_ << no_attributes_,
                          // Flags: ClosePossible,PlayPossible
                          configure_<< ClosePossible << PlayPossible ),Open)

    BOOST_MSM_EUML_ACTION(OpeningEntry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM& fsm,STATE& )
        {
            fsm.get_attribute(display_)->setDisplayText("Opening");            
        }
    };
    BOOST_MSM_EUML_STATE((OpeningEntry,no_action),Opening)

    BOOST_MSM_EUML_ACTION(ClosingEntry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM& fsm,STATE& )
        {
            fsm.get_attribute(display_)->setDisplayText("Closing");
            // start a longer action and pass the desired callback for when action done
            fsm.get_attribute(hardware_)->closeDrawer(Callback<FSM,BOOST_MSM_EUML_EVENT_NAME(closed)>(&fsm));
        }
    };
    BOOST_MSM_EUML_STATE((ClosingEntry,no_action,attributes_ << no_attributes_,
                          configure_<< PlayPossible << play ),Closing)

    BOOST_MSM_EUML_ACTION(StoppedEntry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM& fsm,STATE& )
        {
            fsm.get_attribute(display_)->setDisplayText("Stopped");
        }
    };
    BOOST_MSM_EUML_STATE((StoppedEntry,no_action,attributes_ << no_attributes_,
                          configure_<< PlayPossible << OpenPossible),Stopped)

    BOOST_MSM_EUML_ACTION(RunningEntry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM& fsm,STATE& )
        {
            // we have the right to update display only if this fsm is active
            if (fsm.get_attribute(active_))
            {
                fsm.get_attribute(display_)->setDisplayText(
                            QString(fsm.get_attribute(songs_)[fsm.get_attribute(currentSong_)].c_str()));
            }
        }
    };
    BOOST_MSM_EUML_STATE((RunningEntry,no_action,attributes_ << no_attributes_,
                          configure_<< PrevNextPossible),Running)
    BOOST_MSM_EUML_STATE((),NoVolumeChange)
    BOOST_MSM_EUML_ACTION(VolumeChangeEntry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM& fsm,STATE& )
        {
            // we have the right to update display only if this fsm is active
            if (fsm.get_attribute(active_))
            {
                QString vol = QString("Volume:%1").arg(fsm.get_attribute(volume_));
                fsm.get_attribute(display_)->setDisplayText(vol);
                // start a timer and when expired, call callback
                fsm.get_attribute(hardware_)->startTimer(2000,Callback<FSM,BOOST_MSM_EUML_EVENT_NAME(volume_timeout)>(&fsm));
            }
        }
    };
    BOOST_MSM_EUML_STATE((VolumeChangeEntry,no_action),VolumeChange)
    BOOST_MSM_EUML_EXIT_STATE(( stop ),PseudoExit)


    // transition actions
    BOOST_MSM_EUML_ACTION(last_song)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const&,FSM& fsm,SourceState& ,TargetState& )
        {
            return (fsm.get_attribute(songs_).empty() ||
                    fsm.get_attribute(currentSong_) >= fsm.get_attribute(songs_).size()-1);
        }
    };
    BOOST_MSM_EUML_ACTION(inc_song)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            ++fsm.get_attribute(currentSong_);
            fsm.get_attribute(hardware_)->nextSong();
        }
    };
    BOOST_MSM_EUML_ACTION(dec_song)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            --fsm.get_attribute(currentSong_);
            fsm.get_attribute(hardware_)->previousSong();
        }
    };
    BOOST_MSM_EUML_ACTION(inc_volume)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            if (fsm.get_attribute(volume_) < std::numeric_limits<std::size_t>::max())
            {
                ++fsm.get_attribute(volume_);
                fsm.get_attribute(hardware_)->volumeUp();
            }
        }
    };
    BOOST_MSM_EUML_ACTION(dec_volume)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.get_attribute(hardware_)->volumeDown();
        }
    };
    BOOST_MSM_EUML_ACTION(Log_No_Transition)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& )
        {
            std::cout << "no transition" << " on event " << typeid(Event).name() << std::endl;
        }
    };
    // submachine front-end
    BOOST_MSM_EUML_ACTION(PlayingEntry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM&,STATE& fsm)
        {
            fsm.get_attribute(active_) = true;
            fsm.get_attribute(hardware_)->startPlaying();
        }
    };
    BOOST_MSM_EUML_ACTION(PlayingExit)
    {
        // we leave the state with another event than pause => we want to start from beginning
        template <class Event,class FSM,class STATE>
        typename boost::disable_if<typename boost::is_same<Event,BOOST_MSM_EUML_EVENT_NAME(pausing)>::type,void>::type
        operator()(Event const&,FSM& ,STATE& fsm)
        {
            fsm.get_attribute(currentSong_)=0;
            fsm.get_attribute(active_) = false;
        }
        // we leave the state with pause => we want to continue where we left
        template <class Event,class FSM,class STATE>
        typename boost::enable_if<typename boost::is_same<Event,BOOST_MSM_EUML_EVENT_NAME(pausing)>::type,void>::type
        operator()(Event const&,FSM&,STATE& fsm)
        {
            fsm.get_attribute(active_) = false;
        }
    };
    // Playing has a transition table
    BOOST_MSM_EUML_TRANSITION_TABLE((
    //  +------------------------------------------------------------------------------+
         Running        + next_song [last_song]                        == PseudoExit   ,
         Running        + next_song [!last_song]  / inc_song           == Running      ,
         Running        + prev_song [!(fsm_(currentSong_)==Size_t_<0>())]
                                     / dec_song                        == Running      ,
         Running        + prev_song [fsm_(currentSong_)==Size_t_<0>()] == PseudoExit   ,
         Running        + update_song                                  == Running      ,
         NoVolumeChange + volume_up   / inc_volume                     == VolumeChange ,
         NoVolumeChange + volume_down /
             if_then_(fsm_(volume_) > Size_t_<0>(),
                      (--fsm_(volume_),dec_volume))                    == VolumeChange ,
         // ignore timeout events
         NoVolumeChange + volume_timeout                                               ,
         VolumeChange   + volume_up   / inc_volume                     == VolumeChange ,
         VolumeChange   + volume_down /
              if_then_(fsm_(volume_) > Size_t_<0>(),
                       (--fsm_(volume_),dec_volume))                   == VolumeChange ,
         VolumeChange   + volume_timeout / process_(update_song)       == NoVolumeChange
    //  +------------------------------------------------------------------------------+
    ),playing_transition_table )
    BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( playing_transition_table, //STT
                                        init_ << Running << NoVolumeChange, // Init State
                                        PlayingEntry, // Entry
                                        PlayingExit, // Exit
                                        attributes_ << currentSong_ << songs_
                                                    << volume_ << display_ << hardware_ << active_, // Attributes
                                        configure_  << StopPossible << OpenPossible << PausePossible, // flags
                                        Log_No_Transition // no_transition handler
                                        ),
                                      Playing_) //subfsm name
    // backend => this is now usable in a transition table like any other state
    typedef boost::msm::back::state_machine<Playing_> Playing_impl;
    Playing_impl const Playing;

    BOOST_MSM_EUML_ACTION(PausedEntry)
    {
        template <class Event,class FSM,class STATE>
        void operator()(Event const&,FSM& fsm,STATE& ){fsm.get_attribute(display_)->setDisplayText("Paused");}
    };
    BOOST_MSM_EUML_STATE((PausedEntry,no_action,attributes_ << no_attributes_,
                          configure_<< StopPossible << OpenPossible << PlayPossible),Paused)

    // transition actions
    BOOST_MSM_EUML_ACTION(first_message)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.get_attribute(display_)->setDisplayText("Insert Disk");
        }
    };
    BOOST_MSM_EUML_ACTION(open_drawer)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            // start a longer action and pass the desired callback for when action done
            fsm.get_attribute(hardware_)->openDrawer(Callback<FSM,BOOST_MSM_EUML_EVENT_NAME(opened)>(&fsm));
        }
    };
    BOOST_MSM_EUML_ACTION(close_drawer)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.get_attribute(display_)->setDisplayText("Checking Disk");
        }
    };
    BOOST_MSM_EUML_ACTION(wrong_disk)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.get_attribute(display_)->setDisplayText("Wrong Disk");
        }
    };
    BOOST_MSM_EUML_ACTION(stop_playback)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.get_attribute(hardware_)->stopPlaying();
        }
    };
    BOOST_MSM_EUML_ACTION(pause_playback)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.get_attribute(hardware_)->pausePlaying();
        }
    };
    // front-end: define the FSM structure
    // Transition table for player
    BOOST_MSM_EUML_TRANSITION_TABLE((
      // an anonymous transition (without event trigger)
      Init                  / first_message                     == Empty                 ,
      //  +------------------------------------------------------------------------------+
      Stopped + play                                            == Playing               ,
      Stopped + open_close  / open_drawer                       == Opening               ,
      Opening + opened                                          == Open                  ,
      //  +------------------------------------------------------------------------------+
      Open    + open_close                                      == Closing               ,
      // it is possible to process events from a transition
      // note that we have an internal transition
      Open    + play        / (process_(open_close),process_(play))                      ,
      // ignore timer events
      Open    + opened                                                                   ,
      //  +------------------------------------------------------------------------------+
      Closing + closed      / close_drawer                      == Empty                 ,
      //  +------------------------------------------------------------------------------+
      Empty   + open_close  / open_drawer                       == Opening               ,
      // conflicting transitions solved by mutually exclusive guard conditions
      Empty   + cd_detected [isGood_(event_(disc_info))]
              / (attribute_(substate_(Playing,fsm_),songs_)
                 =getSongs_(event_(disc_info)),stop_playback)   == Stopped               ,
      Empty   + cd_detected [!isGood_(event_(disc_info))] / wrong_disk                   ,
      // ignore timer events
      Empty   + closed                                                                   ,
      //  +------------------------------------------------------------------------------+
      Playing + stop        / stop_playback                     == Stopped               ,
      Playing + pausing     / pause_playback                    == Paused                ,
      // several actions called in this order
      Playing + open_close  / (stop_playback,open_drawer)       == Opening               ,
      // transition originating from a pseudo exit
      exit_pt_(Playing,PseudoExit) + stop / stop_playback       == Stopped               ,
      //  +------------------------------------------------------------------------------+
      Paused  + play                                            == Playing               ,
      Paused  + stop        / stop_playback                     == Stopped               ,
      Paused  + open_close  / (stop_playback,open_drawer)       == Opening
      //  +------------------------------------------------------------------------------+
      ),transition_table)
    // create a state machine "on the fly"
    BOOST_MSM_EUML_DECLARE_STATE_MACHINE(( transition_table, //STT
                                        init_ << Init, // Init State
                                        no_action, // Entry
                                        no_action, // Exit
                                        attributes_ << display_ << hardware_, // Attributes
                                        configure_ << no_configure_, // configuration
                                        Log_No_Transition // no_transition handler
                                        ),
                                      StateMachine_) //fsm name

}
// just inherit from back-end and this structure can be forward-declared in the header file
// for shorter compile-time
struct PlayerLogic::StateMachine : public msm::back::state_machine<StateMachine_>
{
    StateMachine(IDisplay* display, IHardware* hardware)
        // the first part of the constructor replaces Playing with our own instance
        // the second part just forwards arguments
        : msm::back::state_machine<StateMachine_>()
    {
        get_attribute(display_) = display;
        get_attribute(hardware_) = hardware;
        get_state<Playing_impl&>().get_attribute(display_) = display;
        get_state<Playing_impl&>().get_attribute(hardware_) = hardware;
    }
};
PlayerLogic::PlayerLogic(IDisplay* display, IHardware* hardware)
    : fsm_(new PlayerLogic::StateMachine(display,hardware))
{
}
// start the state machine (first call of on_entry)
void PlayerLogic::start()
{
    fsm_->start();
}
// the public interfaces simply forwards events to the state machine
void PlayerLogic::playButton()
{
    fsm_->process_event(play);
}
void PlayerLogic::pauseButton()
{
    fsm_->process_event(pausing);
}
void PlayerLogic::stopButton()
{
    fsm_->process_event(stop);
}
void PlayerLogic::openCloseButton()
{
    fsm_->process_event(open_close);
}
void PlayerLogic::nextSongButton()
{
    fsm_->process_event(next_song);
}
void PlayerLogic::previousSongButton()
{
    fsm_->process_event(prev_song);
}
void PlayerLogic::cdDetected(DiskInfo const& info)
{
    fsm_->process_event(cd_detected(info));
}
void PlayerLogic::volumeUp()
{
    fsm_->process_event(volume_up);
}
void PlayerLogic::volumeDown()
{
    fsm_->process_event(volume_down);
}
bool PlayerLogic::isStopPossible()const
{
    return fsm_->is_flag_active<BOOST_MSM_EUML_FLAG_NAME(StopPossible)>();
}
bool PlayerLogic::isPlayPossible()const
{
    return fsm_->is_flag_active<BOOST_MSM_EUML_FLAG_NAME(PlayPossible)>();
}
bool PlayerLogic::isPausePossible()const
{
    return fsm_->is_flag_active<BOOST_MSM_EUML_FLAG_NAME(PausePossible)>();
}
bool PlayerLogic::isOpenPossible()const
{
    return fsm_->is_flag_active<BOOST_MSM_EUML_FLAG_NAME(OpenPossible)>();
}
bool PlayerLogic::isClosePossible()const
{
    return fsm_->is_flag_active<BOOST_MSM_EUML_FLAG_NAME(ClosePossible)>();
}
bool PlayerLogic::isNextSongPossible()const
{
    return fsm_->is_flag_active<BOOST_MSM_EUML_FLAG_NAME(PrevNextPossible)>();
}
bool PlayerLogic::isPreviousSongPossible()const
{
    return fsm_->is_flag_active<BOOST_MSM_EUML_FLAG_NAME(PrevNextPossible)>();
}
bool PlayerLogic::isVolumeUpPossible()const
{
    return fsm_->is_flag_active<BOOST_MSM_EUML_FLAG_NAME(PrevNextPossible)>();
}
bool PlayerLogic::isVolumeDownPossible()const
{
    return fsm_->is_flag_active<BOOST_MSM_EUML_FLAG_NAME(PrevNextPossible)>();
}
