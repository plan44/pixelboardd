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
#include "i2c.hpp"
#include "jsoncomm.hpp"


#include "blocks.hpp"
#include "display.hpp"

using namespace p44;

#define MAINLOOP_CYCLE_TIME_uS 10000 // 10mS
#define DEFAULT_LOGLEVEL LOG_NOTICE


typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} ColorDef;

static const ColorDef colorDefs[33] = {
  // falling top down
  {   0,   0,   0 }, // none
  { 255,   0,   0 }, // line
  { 100, 255,   0 }, // square
  {   0, 100, 255 }, // L
  {   0, 255, 100 }, // L reverse
  {   0,   0, 255 }, // T
  { 100,   0, 255 }, // squiggly
  { 255,   0, 100 }, // squiggly reversed
  // rising bottom up
  {   0,   0,   0 }, // none
  { 255,   0,   0 }, // line
  { 100, 255,   0 }, // square
  {   0, 100, 255 }, // L
  {   0, 255, 100 }, // L reverse
  {   0,   0, 255 }, // T
  { 100,   0, 255 }, // squiggly
  { 255,   0, 100 }, // squiggly reversed
  // DIMMED: falling top down
  {   0,   0,   0 }, // none
  { 255,   0,   0 }, // line
  { 100, 255,   0 }, // square
  {   0, 100, 255 }, // L
  {   0, 255, 100 }, // L reverse
  {   0,   0, 255 }, // T
  { 100,   0, 255 }, // squiggly
  { 255,   0, 100 }, // squiggly reversed
  // DIMMED: rising bottom up
  {   0,   0,   0 }, // none
  { 255,   0,   0 }, // line
  { 100, 255,   0 }, // square
  {   0, 100, 255 }, // L
  {   0, 255, 100 }, // L reverse
  {   0,   0, 255 }, // T
  { 100,   0, 255 }, // squiggly
  { 255,   0, 100 }, // squiggly reversed
  // Row kill flash
  { 255, 255, 255 }
};



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

  I2CDevicePtr touchDev;
  DigitalIoPtr touchSel;
  DigitalIoPtr touchDetect;
  uint8_t touchState[2];

  // API Server
  SocketCommPtr apiServer;

  // Q&D
  DisplayPtr dsp;
  int mode; // 0=display, 1=blocks
  MLMicroSeconds starttime;

public:

  PixelBoardD() :
    mode(0),
    starttime(MainLoop::now())
  {
  }

  virtual int main(int argc, char **argv)
  {
    const char *usageText =
      "Usage: %1$s [options]\n";
    const CmdLineOptionDescriptor options[] = {
      { 0  , "ledchain",       true,  "device;set device to send LED chain data to" },
      { 0  , "consolekeys",    false, "allow controlling via console keys" },
      { 0  , "notouch",        false, "disable touch pad checking" },
      { 0  , "jsonapiport",    true,  "port;server port number for JSON API (default=none)" },
      { 0  , "jsonapinonlocal",false, "allow JSON API from non-local clients" },
      { 0  , "two",            false, "cooperative two-player game" },
      { 0  , "display",        true,  "filename;show image" },
      { 'l', "loglevel",       true,  "level;set max level of log message detail to show on stdout" },
      { 0  , "errlevel",       true,  "level;set max level for log messages to go to stderr as well" },
      { 0  , "dontlogerrors",  false, "don't duplicate error messages (see --errlevel) on stdout" },
      { 'h', "help",           false, "show this text" },
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

      // - start API server and wait for things to happen
      string apiport;
      if (getStringOption("jsonapiport", apiport)) {
        apiServer = SocketCommPtr(new SocketComm(MainLoop::currentMainLoop()));
        apiServer->setConnectionParams(NULL, apiport.c_str(), SOCK_STREAM, AF_INET);
        apiServer->setAllowNonlocalConnections(getOption("jsonapinonlocal"));
        apiServer->startServer(boost::bind(&PixelBoardD::apiConnectionHandler, this, _1), 3);
      }

      string bgimage;
      if (getStringOption("display", bgimage)) {
        dsp = DisplayPtr(new Display());
        dsp->loadPNGBackground(bgimage);
      }

      playfield = PlayFieldPtr(new PlayField);

      if (getOption("consolekeys")) {
        // create the console keys
        lower_left = ButtonInputPtr(new ButtonInput("simpin.left:j"));
        lower_left->setButtonHandler(boost::bind(&PixelBoardD::controlHandler, this, false, 1, 0, false), false); // left
        lower_right = ButtonInputPtr(new ButtonInput("simpin.right:l"));
        lower_right->setButtonHandler(boost::bind(&PixelBoardD::controlHandler, this, false, -1, 0, false), false); // right
        lower_turn = ButtonInputPtr(new ButtonInput("simpin.turn:k"));
        lower_turn->setButtonHandler(boost::bind(&PixelBoardD::controlHandler, this, false, 0, 1, false), false); // turn
        lower_drop = ButtonInputPtr(new ButtonInput("simpin.drop:m"));
        lower_drop->setButtonHandler(boost::bind(&PixelBoardD::controlHandler, this, false, 0, 0, true), false); // drop
      }

      // create the touch pad controls
      if (!getOption("notouch")) {
        touchDev = I2CManager::sharedManager().getDevice(0, "generic@1B");
        touchSel = DigitalIoPtr(new DigitalIo("gpio.7", true, false));
        touchDetect = DigitalIoPtr(new DigitalIo("/gpio.15", false, false));
        uint8_t id, sta;
        touchDev->getBus().SMBusReadByte(touchDev.get(), 0, id);
        touchDev->getBus().SMBusReadByte(touchDev.get(), 0, sta);
        LOG(LOG_NOTICE,"Device ID = 0x%02X, keystatus = 0x%02X", id, sta);
        touchState[0] = 0;
        touchState[1] = 0;
      }

    } // if !terminated
    // app now ready to run (or cleanup when already terminated)
    return run();
  }


  virtual void initialize()
  {
    srand((unsigned)MainLoop::currentMainLoop().now()*4223);
    display->begin();
    display->show();
    MainLoop::currentMainLoop().registerIdleHandler(this, boost::bind(&PixelBoardD::step, this));
    MainLoop::currentMainLoop().executeOnce(boost::bind(&PixelBoardD::refreshDsp, this), 2*Second);
  }




  void play(bool aTwoPlayer)
  {
    playfield->restart(aTwoPlayer);
    refreshPlayfield();
  }



  SocketCommPtr apiConnectionHandler(SocketCommPtr aServerSocketComm)
  {
    JsonCommPtr conn = JsonCommPtr(new JsonComm(MainLoop::currentMainLoop()));
    conn->setMessageHandler(boost::bind(&PixelBoardD::apiRequestHandler, this, conn, _1, _2));
    conn->setClearHandlersAtClose(); // close must break retain cycles so this object won't cause a mem leak
    return conn;
  }


  void apiRequestHandler(JsonCommPtr aConnection, ErrorPtr aError, JsonObjectPtr aRequest)
  {
    ErrorPtr err;
    JsonObjectPtr answer = JsonObject::newObj();
    // Decode mg44-style request (HTTP wrapped in JSON)
    if (Error::isOK(aError)) {
      LOG(LOG_INFO,"API request: %s", aRequest->c_strValue());
      JsonObjectPtr o;
      o = aRequest->get("method");
      if (o) {
        string method = o->stringValue();
        string uri;
        o = aRequest->get("uri");
        if (o) uri = o->stringValue();
        JsonObjectPtr data;
        bool action = (method!="GET");
        if (action) {
          data = aRequest->get("data");
        }
        else {
          data = aRequest->get("uri_params");
          if (data) action = true; // GET, but with query_params: treat like PUT/POST with data
        }
        // request elements now: uri and data
        JsonObjectPtr r = processRequest(uri, data, action);
        if (r) answer->add("result", r);
      }
    }
    else {
      LOG(LOG_ERR,"Invalid JSON request");
      answer->add("Error", JsonObject::newString(aError->description()));
    }
    LOG(LOG_INFO,"API answer: %s", answer->c_strValue());
    err = aConnection->sendMessage(answer);
    aConnection->closeAfterSend();
  }


  JsonObjectPtr processRequest(string aUri, JsonObjectPtr aData, bool aIsAction)
  {
    ErrorPtr err;
    JsonObjectPtr o;
    if (aUri=="player1" || aUri=="player2") {
      bool lower = aUri=="player2";
      if (aIsAction) {
        if (aData->get("key", o)) {
          string key = o->stringValue();
          if (key=="left")
            controlHandler(lower, 1, 0, false); // left
          else if (key=="right")
            controlHandler(lower, -1, 0, false); // right
          else if (key=="turn")
            controlHandler(lower, 0, 1, false); // turn
          else if (key=="drop")
            controlHandler(lower, 0, 0, true); // drop
        }
      }
    }
    else if (aUri=="board") {
      if (aIsAction) {
        bool twoSided = false;
        if (aData->get("twosided", o))
          twoSided = o->boolValue();
        if (aData->get("page", o)) {
          string page = o->stringValue();
          showPage(page, twoSided);
        }
      }
    }
    else {
      err = WebError::webErr(500, "Unknown URI '%s'", aUri.c_str());
    }
    // return error or ok
    if (Error::isOK(err))
      return JsonObjectPtr(); // ok
    else {
      JsonObjectPtr errorJson = JsonObject::newObj();
      errorJson->add("error", JsonObject::newString(err->description()));
      return errorJson;
    }
  }


  void showPage(const string page, bool aTwoSided)
  {
    // FIXME: Q&D now
    if (page=="blocks") {
      mode=1;
      play(aTwoSided);
    }
  }




  void checkInputs()
  {
    if (touchDev) {
      uint8_t newState;
      for (int side=0; side<2; side++) {
        touchSel->set(side==1); // select side
        touchDev->getBus().SMBusReadByte(touchDev.get(), 3, newState); // get key state
        // clear those that were set in last call already
        uint8_t triggers;
        triggers = newState & ~touchState[side];
        LOG(triggers ? LOG_INFO : LOG_DEBUG, "checkInputs: side=%d, touchState=0x%02X, newState=0x%02X, triggers=0x%02X", side, touchState[side], newState, triggers);
        touchState[side] = newState;
        if (triggers & 0x02) controlHandler(side==1, 1, 0, false); // left
        else if (triggers & 0x10) controlHandler(side==1, -1, 0, false); // right
        else if (triggers & 0x04) controlHandler(side==1, 0, 1, false); // turn
        else if (triggers & 0x08) controlHandler(side==1, 0, 0, true); // drop
      }
    }
  }


  void refreshPlayfield()
  {
    for (int x=0; x<10; x++) {
      for (int y=0; y<20; y++) {
        uint8_t cc = playfield->colorAt(x, y);
        int r,g,b;
        const ColorDef *cdef = &colorDefs[cc];
        r = cdef->r;
        g = cdef->g;
        b = cdef->b;
        if (cc>=16 && cc<32) {
          r = r*3/4;
          g = g*3/4;
          b = b*3/4;
        }
        display->setColorXY(x, y, r, g, b);
      }
    }
    display->show();
  }


  void refreshDsp()
  {
    for (int x=0; x<10; x++) {
      for (int y=0; y<20; y++) {
        RGBWebColor p = dsp->colorAt(x, y);
        display->setColorXY(x, y, p.r, p.g, p.b);
      }
    }
    display->show();
  }



  bool step()
  {
    if (mode!=0) {
      if (playfield->step()) {
        refreshPlayfield();
      }
    }
    checkInputs();
    return true; // completed for this cycle (10mS)
  }


  void controlHandler(bool aLower, int aMovement, int aRotation, bool aDrop)
  {
    if (mode==0 && starttime+15*Second<=MainLoop::now()) {
      mode=1;
      play(getOption("two"));
    }
    else {
      // left/right is always seen from bottom end, so needs to be swapped if aLower
      if (aLower) aMovement = -aMovement;
      if (aMovement!=0 || aRotation!=0) {
        playfield->moveBlock(aMovement, aRotation, aLower);
      }
      if (aDrop) {
        playfield->dropBlock(aLower);
      }
      if (playfield->isDirty()) {
        refreshPlayfield();
      }
    }
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
