//
//  Copyright (c) 2017 plan44.ch / Lukas Zeller, Zurich, Switzerland
//
//  Author: Lukas Zeller <luz@plan44.ch>
//
//  This file is part of pixelboardd.
//
//  pixelboardd is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  pixelboardd is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with pixelboardd. If not, see <http://www.gnu.org/licenses/>.
//

#ifndef __pixelboardd_sound_hpp__
#define __pixelboardd_sound_hpp__

#include "p44utils_common.hpp"

namespace p44 {

  class SoundChannel : public P44Obj
  {

    string deviceAndVolumeName;
    pid_t playerPID;
    int numPlaying;
    int volume;

  public :

    /// create sound channel
    /// @param aDeviceAndVolumeName the ALSA device *and* simple volume control name for the channel
    SoundChannel(const string aDeviceAndVolumeName);

    virtual ~SoundChannel();

    /// set volume
    /// @param aVolume audio volume of this sound channel, 0..100%
    void setVolume(int aVolume);

    /// get volume
    /// @return current audio volume of this sound channel, 0..100%
    int getVolume();

    /// stop currently playing sound
    /// @note currently, only most recently played sound can be stopped
    void stop();

    /// play the given sound file
    /// @param aSoundFile the sound file to play
    void play(const string aSoundFile);

    /// check if something is playing in this channel
    bool playing();

  private:

    void volumeUpdated(ErrorPtr aError);
    void soundPlayed(ErrorPtr aError);

  };
  typedef boost::intrusive_ptr<SoundChannel> SoundChannelPtr;

} // namespace p44



#endif /* __pixelboardd_sound_hpp__ */
