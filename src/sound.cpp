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

#include "sound.hpp"

#include <signal.h>

using namespace p44;

// MARK: ===== SoundChannel

#define VOLUME_CMD "/usr/bin/amixer"
#define WAVPLAY_CMD "/usr/bin/aplay"
#define MODPLAY_CMD "/usr/bin/hxcmodplayer"


SoundChannel::SoundChannel(const string aDeviceAndVolumeName) :
  deviceAndVolumeName(aDeviceAndVolumeName),
  playerPID(-1),
  numPlaying(0)
{
}


SoundChannel::~SoundChannel()
{
  stop();
}


void SoundChannel::setVolume(int aVolume)
{
  #if defined(__APPLE__)
  const char *path = "/bin/echo";
  #else
  const char *path = VOLUME_CMD;
  #endif
  // amixer -D <devicename> sset <controlname> <value>
  const char *cmd[] = {
    path,
    "sset",
    deviceAndVolumeName.c_str(),
    string_format("%d%%", aVolume).c_str(),
    NULL
  };
  MainLoop::currentMainLoop().fork_and_execve(
    boost::bind(&SoundChannel::volumeUpdated, this, _1),
    path, (char **)cmd
  );
}


void SoundChannel::volumeUpdated(ErrorPtr aError)
{
  if (!Error::isOK(aError)) {
    LOG(LOG_ERR, "Volume update failed: %s", aError->description().c_str());
  }
  LOG(LOG_INFO, "Volume updated");
}


void SoundChannel::stop()
{
  if (playerPID>=0) {
    kill(playerPID, SIGTERM);
    LOG(LOG_INFO, "Sound player process %d killed", playerPID);
    playerPID = -1; // mark killed (even if it isn't yet)
  }
}


bool SoundChannel::playing()
{
  return numPlaying>0;
}



void SoundChannel::play(const string aSoundFile)
{
  // aplay -D <devicename> <wavfilename>
  // hxcmodplayer -D <devicename> <modfilename>
  const char *path;
  #if defined(__APPLE__)
  size_t sz = aSoundFile.size();
  if (aSoundFile.substr(sz-4)==".mod") {
    path = "/bin/echo";
  }
  else {
    path = "/usr/bin/afplay";
  }
  const char *cmd[] = {
    path,
    aSoundFile.c_str(),
    NULL
  };
  #else
  size_t sz = aSoundFile.size();
  if (aSoundFile.substr(sz-4)==".mod") {
    path = MODPLAY_CMD;
  }
  else {
    path = WAVPLAY_CMD;
  }
  const char *cmd[] = {
    path,
    "-D",
    deviceAndVolumeName.c_str(),
    aSoundFile.c_str(),
    NULL
  };
  #endif
  playerPID = MainLoop::currentMainLoop().fork_and_execve(
    boost::bind(&SoundChannel::soundPlayed, this, _1),
    path, (char **)cmd
  );
  numPlaying++;
}


void SoundChannel::soundPlayed(ErrorPtr aError)
{
  if (!Error::isOK(aError)) {
    LOG(LOG_ERR, "Playing sound failed: %s", aError->description().c_str());
  }
  numPlaying--;
  LOG(LOG_INFO, "sound played, still playing=%d", numPlaying);
  if (numPlaying<=0) {
    playerPID = -1;
    numPlaying = 0;
  }
}



