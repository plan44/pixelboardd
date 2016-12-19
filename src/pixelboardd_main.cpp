//
//  Copyright (c) 2016 plan44.ch / Lukas Zeller, Zurich, Switzerland
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

#include "application.hpp"

#include "ledchaincomm.hpp"
#include "digitalio.hpp"

#include "blocks.hpp"

using namespace p44;

#define MAINLOOP_CYCLE_TIME_uS 10000 // 10mS
#define DEFAULT_LOGLEVEL LOG_NOTICE


/// Main program for plan44.ch P44-DSB-DEH in form of the "vdcd" daemon)
class PixelBoardD : public CmdLineApp
{
  typedef CmdLineApp inherited;

  LEDChainCommPtr display;

  PlayFieldPtr playfield;

  ButtonInputPtr lower_left;
  ButtonInputPtr lower_right;
  ButtonInputPtr lower_turn;
  ButtonInputPtr lower_drop;


public:

  PixelBoardD()
  {
  }

  virtual int main(int argc, char **argv)
  {
    const char *usageText =
      "Usage: %1$s [options]\n";
    const CmdLineOptionDescriptor options[] = {
      { 0  , "ledchain",      true,  "device;set device to send LED chain data to" },
      { 'l', "loglevel",      true,  "level;set max level of log message detail to show on stdout" },
      { 0  , "errlevel",      true,  "level;set max level for log messages to go to stderr as well" },
      { 0  , "dontlogerrors", false, "don't duplicate error messages (see --errlevel) on stdout" },
      { 'h', "help",          false, "show this text" },
      { 0, NULL } // list terminator
    };

    // parse the command line, exits when syntax errors occur
    setCommandDescriptors(usageText, options);
    parseCommandLine(argc, argv);

    if ((numOptions()<1) || numArguments()>0) {
      // show usage
      showUsage();
      terminateApp(EXIT_SUCCESS);
    }

    // build objects only if not terminated early
    if (!isTerminated()) {
      int loglevel = DEFAULT_LOGLEVEL;
      getIntOption("loglevel", loglevel);
      SETLOGLEVEL(loglevel);
      int errlevel = LOG_ERR; // testing by default only reports to stdout
      getIntOption("errlevel", errlevel);
      SETERRLEVEL(errlevel, !getOption("dontlogerrors"));

      // create the LED chain
      string leddev = "/tmp/ledchainsim";
      getStringOption("ledchain", leddev);
      display = LEDChainCommPtr(new LEDChainComm(LEDChainComm::ledtype_ws281x, leddev, 200, 20, false, true, true));

      // create the controls
      lower_left = ButtonInputPtr(new ButtonInput("simpin.left:j"));
      lower_left->setButtonHandler(boost::bind(&PixelBoardD::controlHandler, this, false, 1, 0, false), false); // left
      lower_right = ButtonInputPtr(new ButtonInput("simpin.right:l"));
      lower_right->setButtonHandler(boost::bind(&PixelBoardD::controlHandler, this, false, -1, 0, false), false); // right
      lower_turn = ButtonInputPtr(new ButtonInput("simpin.turn:k"));
      lower_turn->setButtonHandler(boost::bind(&PixelBoardD::controlHandler, this, false, 0, 1, false), false); // turn
      lower_drop = ButtonInputPtr(new ButtonInput("simpin.drop:m"));
      lower_drop->setButtonHandler(boost::bind(&PixelBoardD::controlHandler, this, false, 0, 0, true), false); // drop


    } // if !terminated
    // app now ready to run (or cleanup when already terminated)
    return run();
  }


  virtual void initialize()
  {
    display->begin();
    display->show();
    playfield = PlayFieldPtr(new PlayField);
    playfield->launchRandomBlock(false);
    refreshPlayfield();
    MainLoop::currentMainLoop().registerIdleHandler(this, boost::bind(&PixelBoardD::step, this));
  }


  void refreshPlayfield()
  {
    for (int x=0; x<10; x++) {
      for (int y=0; y<20; y++) {
        int cc = playfield->colorAt(x, y);
        display->setColorXY(x, y, cc & 0x01 ? 0xFF : 0, cc & 0x02 ? 0xFF : 0, cc & 0x04 ? 0xFF : 0);
      }
    }
    display->show();
  }



  bool step()
  {
    if (playfield->step()) {
      refreshPlayfield();
    }
    return true;
  }


  void controlHandler(bool aLower, int aMovement, int aRotation, bool aDrop)
  {
    if (aMovement!=0 || aRotation!=0) {
      playfield->moveBlock(aMovement, aRotation, aLower);
    }
    if (aDrop) {
      playfield->dropBlock(aLower);
    }
    step();
  }
  


};





int main(int argc, char **argv)
{
  // prevent debug output before application.main scans command line
  SETLOGLEVEL(LOG_EMERG);
  SETERRLEVEL(LOG_EMERG, false); // messages, if any, go to stderr
  // create the mainloop
  MainLoop::currentMainLoop().setLoopCycleTime(MAINLOOP_CYCLE_TIME_uS);
  // create app with current mainloop
  static PixelBoardD application;
  // pass control
  return application.main(argc, argv);
}
