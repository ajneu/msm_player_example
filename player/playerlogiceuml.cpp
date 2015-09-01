// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
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
    struct play_impl : msm::front::euml::euml_event<play_impl> {};
    // an event convertible from any other event (used for transitions originating from an exit pseudo state)
    struct stop_impl : msm::front::euml::euml_event<stop_impl>
    {
        stop_impl(){}
        template <class T>
        stop_impl(T const&){}
    };
    struct pause_impl : msm::front::euml::euml_event<pause_impl>{};
    struct open_close_impl : msm::front::euml::euml_event<open_close_impl>{};
    // an event with data
    struct cd_detected_impl : msm::front::euml::euml_event<cd_detected_impl>
    {
        cd_detected_impl():info_(0){} //not used
        cd_detected_impl(DiskInfo const& info):info_(&info){}
        DiskInfo const* info_;
    };
    struct opened_impl : msm::front::euml::euml_event<opened_impl>{};
    struct closed_impl : msm::front::euml::euml_event<closed_impl>{};
    struct next_song_impl : msm::front::euml::euml_event<next_song_impl>{};
    struct prev_song_impl : msm::front::euml::euml_event<prev_song_impl>{};
    struct volume_up_impl : msm::front::euml::euml_event<volume_up_impl>{};
    struct volume_down_impl : msm::front::euml::euml_event<volume_down_impl>{};
    struct volume_timeout_impl : msm::front::euml::euml_event<volume_timeout_impl>{};
    struct update_song_impl : msm::front::euml::euml_event<update_song_impl>{};

    // define some dummy instances for use in the transition table
    // it is also possible to default-construct them instead:
    // struct play {};
    // inside the table: play()
    play_impl play;
    stop_impl stop;
    pause_impl pausing;
    open_close_impl open_close;
    cd_detected_impl cd_detected;
    opened_impl opened;
    closed_impl closed;
    next_song_impl next_song;
    prev_song_impl prev_song;
    volume_down_impl volume_down;
    volume_up_impl volume_up;
    volume_timeout_impl volume_timeout;
    update_song_impl update_song;

    //flags. Allow us to be more flexible than asking in which state the FSM is.
    struct PlayPossible{};
    struct PausePossible{};
    struct StopPossible{};
    struct OpenPossible{};
    struct ClosePossible{};
    struct PrevNextPossible{};

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
    struct Init_impl : public msm::front::state<> , public msm::front::euml::euml_state<Init_impl>
    {
    };
    struct Empty_impl : public msm::front::state<> , public msm::front::euml::euml_state<Empty_impl>
    {
        typedef mpl::vector1<OpenPossible>      flag_list;
        typedef mpl::vector1<play_impl>         deferred_events;
    };
    struct Open_impl : public msm::front::state<> , public msm::front::euml::euml_state<Open_impl>
    {
        typedef mpl::vector2<ClosePossible,PlayPossible>      flag_list;
        template <class Event,class FSM>
        void on_entry(Event const& ,FSM& fsm) {fsm.display_->setDisplayText("Open");}
    };
    struct Opening_impl : public msm::front::state<> , public msm::front::euml::euml_state<Opening_impl>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm)
        {
            fsm.display_->setDisplayText("Opening");
        }
    };
    struct Closing_impl : public msm::front::state<> , public msm::front::euml::euml_state<Closing_impl>
    {
        typedef mpl::vector1<PlayPossible>      flag_list;
        typedef mpl::vector1<play_impl>         deferred_events;
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm)
        {
            fsm.display_->setDisplayText("Closing");
            // start a longer action and pass the desired callback for when action done
            fsm.hardware_->closeDrawer(Callback<FSM,closed_impl>(&fsm));
        }
    };
    struct Stopped_impl : public msm::front::state<> , public msm::front::euml::euml_state<Stopped_impl>
    {
        typedef mpl::vector2<PlayPossible,OpenPossible>      flag_list;

        template <class Event,class FSM>
        void on_entry(Event const& ,FSM& fsm) {fsm.display_->setDisplayText("Stopped");}
    };
    struct Running_impl : public msm::front::state<> , public msm::front::euml::euml_state<Running_impl>
    {
        // flags are searched recursively
        typedef mpl::vector1<PrevNextPossible>      flag_list;
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm)
        {
            // we have the right to update display only if this fsm is active
            if (fsm.active_)
            {
                fsm.display_->setDisplayText(QString(fsm.songs_[fsm.currentSong_].c_str()));
            }
        }
    };
    struct NoVolumeChange_impl : public msm::front::state<> , public msm::front::euml::euml_state<NoVolumeChange_impl>
    {
    };
    struct VolumeChange_impl : public msm::front::state<> , public msm::front::euml::euml_state<VolumeChange_impl>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm)
        {
            // we have the right to update display only if this fsm is active
            if (fsm.active_)
            {
                QString vol = QString("Volume:%1").arg(fsm.volume_);
                fsm.display_->setDisplayText(vol);
                // start a timer and when expired, call callback
                fsm.hardware_->startTimer(2000,Callback<FSM,volume_timeout_impl>(&fsm));
            }
        }
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& fsm)
        {
            fsm.hardware_->stopTimer();
        }
    };
    struct PseudoExit_impl : public msm::front::exit_pseudo_state<stop_impl>,
                             public msm::front::euml::euml_state<PseudoExit_impl>
    {
    };
    // Playing substate instances for nicer table
    Running_impl const Running;
    PseudoExit_impl const PseudoExit;
    NoVolumeChange_impl const NoVolumeChange;
    VolumeChange_impl const VolumeChange;

    // transition actions
    BOOST_MSM_EUML_ACTION(first_song)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const&,FSM& fsm,SourceState& ,TargetState& )
        {
            return (fsm.currentSong_ == 0);
        }
    };
    BOOST_MSM_EUML_ACTION(last_song)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const&,FSM& fsm,SourceState& ,TargetState& )
        {
            return (fsm.songs_.empty() || fsm.currentSong_ >= fsm.songs_.size()-1);
        }
    };
    BOOST_MSM_EUML_ACTION(inc_song)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            ++fsm.currentSong_;
            fsm.hardware_->nextSong();
        }
    };
    BOOST_MSM_EUML_ACTION(dec_song)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            --fsm.currentSong_;
            fsm.hardware_->previousSong();
        }
    };
    BOOST_MSM_EUML_ACTION(inc_volume)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            if (fsm.volume_ < std::numeric_limits<std::size_t>::max())
            {
                ++fsm.volume_;
                fsm.hardware_->volumeUp();
            }
        }
    };
    BOOST_MSM_EUML_ACTION(dec_volume)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            if (fsm.volume_ > 0)
            {
                --fsm.volume_;
                fsm.hardware_->volumeDown();
            }
        }
    };
    // submachine front-end
    struct Playing_ : public msm::front::state_machine_def<Playing_>
    {
        Playing_(IDisplay* display, IHardware* hardware):
            currentSong_(0),volume_(0),display_(display),hardware_(hardware){}
        typedef mpl::vector3<StopPossible,OpenPossible,PausePossible>      flag_list;

        //submachines also have entry/exit conditions
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm) {active_=true;fsm.hardware_->startPlaying();}

        // we leave the state with another event than pause => we want to start from beginning
        template <class Event,class FSM>
        typename boost::disable_if<typename boost::is_same<Event,pause_impl>::type,void>::type
        on_exit(Event const&,FSM& ){currentSong_ = 0;active_=false;}

        // we leave the state with pause => we want to continue where we left
        template <class Event,class FSM>
        typename boost::enable_if<typename boost::is_same<Event,pause_impl>::type,void>::type
        on_exit(Event const&,FSM& ){active_=false;}

        // initial states (2 regions => 2 initial states)
        typedef boost::mpl::vector2<Running_impl,NoVolumeChange_impl> initial_state;
        // Playing has a transition table
        BOOST_MSM_EUML_DECLARE_TRANSITION_TABLE((
        //  +------------------------------------------------------------------------------+
             Running        + next_song [last_song]                        == PseudoExit   ,
             Running        + next_song [!last_song]  / inc_song           == Running      ,
             Running        + prev_song [!first_song] / dec_song           == Running      ,
             Running        + prev_song [first_song]                       == PseudoExit   ,
             Running        + update_song                                  == Running      ,
             NoVolumeChange + volume_up   / inc_volume                     == VolumeChange ,
             NoVolumeChange + volume_down / dec_volume                     == VolumeChange ,
             // ignore timeout events
             NoVolumeChange + volume_timeout                                               ,
             VolumeChange   + volume_up   / inc_volume                     == VolumeChange ,
             VolumeChange   + volume_down / dec_volume                     == VolumeChange ,
             VolumeChange   + volume_timeout / process_(update_song)       == NoVolumeChange
        //  +------------------------------------------------------------------------------+
        ),transition_table )
        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& , FSM&,int state)
        {
            std::cout << "no transition from state " << state << " on event " << typeid(Event).name() << std::endl;
        }
        Playing_():currentSong_(0),volume_(0),active_(false){}
        size_t currentSong_;
        DiskInfo::Songs songs_;
        size_t volume_;
        IDisplay* display_;
        IHardware* hardware_;
        bool active_;
    };
    // backend => this is now usable in a transition table like any other state
    typedef boost::msm::back::state_machine<Playing_> Playing_impl;
    // player substates
    struct Paused_impl : public msm::front::state<> , public msm::front::euml::euml_state<Paused_impl>
    {
        typedef mpl::vector3<PlayPossible,OpenPossible,StopPossible>      flag_list;
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm) {fsm.display_->setDisplayText("Paused");}
    };
    //to make the transition table more readable
    Empty_impl const Empty;
    Closing_impl const Closing;
    Open_impl const Open;
    Opening_impl const Opening;
    Stopped_impl const Stopped;
    Playing_impl const Playing;
    Paused_impl const Paused;
    Init_impl const Init;
    // transition actions
    BOOST_MSM_EUML_ACTION(first_message)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.display_->setDisplayText("Insert Disk");
        }
    };
    BOOST_MSM_EUML_ACTION(open_drawer)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            // start a longer action and pass the desired callback for when action done
            fsm.hardware_->openDrawer(Callback<FSM,opened_impl>(&fsm));
        }
    };
    BOOST_MSM_EUML_ACTION(close_drawer)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.display_->setDisplayText("Checking Disk");
        }
    };
    BOOST_MSM_EUML_ACTION(wrong_disk)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.display_->setDisplayText("Wrong Disk");
        }
    };
    BOOST_MSM_EUML_ACTION(stop_playback)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.hardware_->stopPlaying();
        }
    };
    BOOST_MSM_EUML_ACTION(pause_playback)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.hardware_->pausePlaying();
        }
    };
    // transition guards
    BOOST_MSM_EUML_ACTION(good_disk)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const& evt,FSM&,SourceState& ,TargetState& )
        {
            return evt.info_->goodDisk;
        }
    };
    BOOST_MSM_EUML_ACTION(store_cd_info)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& evt, FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.template get_state<Playing_impl&>().songs_ = evt.info_->songs;
        }
    };
    // front-end: define the FSM structure
    struct StateMachine_ : public msm::front::state_machine_def<StateMachine_>
    {
        // constructor, automatically forwarded from back-end
        StateMachine_(IDisplay* display, IHardware* hardware):display_(display),hardware_(hardware){}

        // the initial state of the player SM. Must be defined
        typedef Init_impl initial_state;

        // Transition table for player
        BOOST_MSM_EUML_DECLARE_TRANSITION_TABLE((
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
          Empty   + cd_detected [good_disk]
                                / (store_cd_info,stop_playback)     == Stopped               ,
          Empty   + cd_detected [!good_disk] / wrong_disk                                    ,
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

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& , FSM&,int state)
        {
            std::cout << "no transition from state " << state << " on event " << typeid(Event).name() << std::endl;
        }
        IDisplay* display_;
        IHardware* hardware_;
    };
}
// just inherit from back-end and this structure can be forward-declared in the header file
// for shorter compile-time
struct PlayerLogic::StateMachine : public msm::back::state_machine<StateMachine_>
{
    StateMachine(IDisplay* display, IHardware* hardware)
        // the first part of the constructor replaces Playing with our own instance
        // the second part just forwards arguments
        : msm::back::state_machine<StateMachine_>(msm::back::states_ << Playing_impl(display,hardware),display,hardware)
    {
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
    fsm_->process_event(cd_detected_impl(info));
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
    return fsm_->is_flag_active<StopPossible>();
}
bool PlayerLogic::isPlayPossible()const
{
    return fsm_->is_flag_active<PlayPossible>();
}
bool PlayerLogic::isPausePossible()const
{
    return fsm_->is_flag_active<PausePossible>();
}
bool PlayerLogic::isOpenPossible()const
{
    return fsm_->is_flag_active<OpenPossible>();
}
bool PlayerLogic::isClosePossible()const
{
    return fsm_->is_flag_active<ClosePossible>();
}
bool PlayerLogic::isNextSongPossible()const
{
    return fsm_->is_flag_active<PrevNextPossible>();
}
bool PlayerLogic::isPreviousSongPossible()const
{
    return fsm_->is_flag_active<PrevNextPossible>();
}
bool PlayerLogic::isVolumeUpPossible()const
{
    return fsm_->is_flag_active<PrevNextPossible>();
}
bool PlayerLogic::isVolumeDownPossible()const
{
    return fsm_->is_flag_active<PrevNextPossible>();
}
