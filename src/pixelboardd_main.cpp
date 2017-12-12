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

// Pages
#include "blocks.hpp"
#include "display.hpp"
#include "life.hpp"

using namespace p44;

#define DEFAULT_LOGLEVEL LOG_NOTICE


typedef std::map<string, PixelPagePtr> PagesMap;

class PixelBoardD : public CmdLineApp
{
  typedef CmdLineApp inherited;

  LEDChainCommPtr display;
  bool upsideDown;

  ButtonInputPtr lower_left;
  ButtonInputPtr lower_right;
  ButtonInputPtr lower_turn;
  ButtonInputPtr lower_drop;
  ButtonInputPtr upper_left;
  ButtonInputPtr upper_right;
  ButtonInputPtr upper_turn;
  ButtonInputPtr upper_drop;
  ButtonInputPtr pause;

  I2CDevicePtr touchDevL;
  I2CDevicePtr touchDevH;
  I2CDevicePtr keyLedDevL;
  I2CDevicePtr keyLedDevH;
  DigitalIoPtr touchDetect;
  uint8_t touchState[2];

  // API Server
  SocketCommPtr apiServer;

  // The display pages
  string defaultPageName;
  PageMode defaultMode;
  PagesMap pages;
  PixelPagePtr currentPage; ///< the current page

  // pages
  DisplayPagePtr displayPage;
  BlocksPagePtr blocksPage;
  LifePagePtr lifePage;

  MLMicroSeconds starttime;


public:

  PixelBoardD() :
    starttime(MainLoop::now()),
    upsideDown(false),
    defaultMode(pagemode_controls1)
  {
  }

  virtual int main(int argc, char **argv)
  {
    const char *usageText =
      "Usage: %1$s [options]\n";
    const CmdLineOptionDescriptor options[] = {
      { 'd', "ledchain",       true,  "device;set device to send LED chain data to" },
      { 0  , "touchsel",       true,  "pinspec;touchboard selection signal" },
      { 0  , "touchdetect",    true,  "pinspec;touchboard touch detect signal" },
      { 0  , "touchreset",     true,  "pinspec;touchboard reset signal" },
      { 0  , "i2cbuslow",      true,  "busno;i2c bus connected to the lower touchboard" },
      { 0  , "i2cbushigh",     true,  "busno;i2c bus connected to the higher touchboard" },
      { 'u', "upsidedown",     false, "use board upside down" },
      { 0  , "consolekeys",    false, "allow controlling via console keys" },
      { 0  , "notouch",        false, "disable touch pad checking" },
      { 0  , "jsonapiport",    true,  "port;server port number for JSON API (default=none)" },
      { 0  , "jsonapinonlocal",false, "allow JSON API from non-local clients" },
      { 0  , "defaultpage",    true,  "display page;default page to show after start and after other page ends" },
      { 0  , "defaultmode",    true,  "mode;defines default page mode: 1=normal, 2=reversed, 3=twosided" },
      { 0  , "image",          true,  "filename;image to show by default on display page" },
      { 0  , "message",        true,  "message;text to show from time to time on display page" },
      { 'l', "loglevel",       true,  "level;set max level of log message detail to show on stdout" },
      { 0  , "errlevel",       true,  "level;set max level for log messages to go to stderr as well" },
      { 0  , "dontlogerrors",  false, "don't duplicate error messages (see --errlevel) on stdout" },
      { 0  , "deltatstamps",   false, "show timestamp delta between log lines" },
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
      SETDELTATIME(getOption("deltatstamps"));

      // create the LED chain
      upsideDown = getOption("upsidedown");
      string leddev = "/tmp/ledchainsim";
      getStringOption("ledchain", leddev);
      display = LEDChainCommPtr(new LEDChainComm(LEDChainComm::ledtype_ws281x, leddev, 200, 20, upsideDown, true, true, !upsideDown));

      // - start API server and wait for things to happen
      string apiport;
      if (getStringOption("jsonapiport", apiport)) {
        apiServer = SocketCommPtr(new SocketComm(MainLoop::currentMainLoop()));
        apiServer->setConnectionParams(NULL, apiport.c_str(), SOCK_STREAM, AF_INET);
        apiServer->setAllowNonlocalConnections(getOption("jsonapinonlocal"));
        apiServer->startServer(boost::bind(&PixelBoardD::apiConnectionHandler, this, _1), 3);
      }

      getStringOption("defaultpage", defaultPageName);
      int m = defaultMode;
      getIntOption("defaultmode", m);
      defaultMode = m;

      // add pages
      // - display
      displayPage = DisplayPagePtr(new DisplayPage(boost::bind(&PixelBoardD::pageInfoHandler, this, _1, _2)));
      string s;
      if (getStringOption("image", s)) {
        displayPage->loadPNGBackground(s);
      }
      if (getStringOption("message", s)) {
        displayPage->setDefaultMessage(s);
      }
      // - blocks
      blocksPage = BlocksPagePtr(new BlocksPage(boost::bind(&PixelBoardD::pageInfoHandler, this, _1, _2)));
      // - life
      lifePage = LifePagePtr(new LifePage(boost::bind(&PixelBoardD::pageInfoHandler, this, _1, _2)));


      if (getOption("consolekeys")) {
        // create the console keys
        // - lower
        lower_left = ButtonInputPtr(new ButtonInput("simpin.left:j"));
        lower_left->setButtonHandler(boost::bind(&PixelBoardD::keyHandler, this, 0, keycode_left, keycode_none), false);
        lower_right = ButtonInputPtr(new ButtonInput("simpin.right:l"));
        lower_right->setButtonHandler(boost::bind(&PixelBoardD::keyHandler, this, 0, keycode_right, keycode_none), false);
        lower_turn = ButtonInputPtr(new ButtonInput("simpin.turn:k"));
        lower_turn->setButtonHandler(boost::bind(&PixelBoardD::keyHandler, this, 0, keycode_middleleft, keycode_none), false);
        lower_drop = ButtonInputPtr(new ButtonInput("simpin.drop:m"));
        lower_drop->setButtonHandler(boost::bind(&PixelBoardD::keyHandler, this, 0, keycode_middleright, keycode_none), false);
        // - upper
        upper_left = ButtonInputPtr(new ButtonInput("simpin.left:a"));
        upper_left->setButtonHandler(boost::bind(&PixelBoardD::keyHandler, this, 1, keycode_left, keycode_none), false);
        upper_right = ButtonInputPtr(new ButtonInput("simpin.right:d"));
        upper_right->setButtonHandler(boost::bind(&PixelBoardD::keyHandler, this, 1, keycode_right, keycode_none), false);
        upper_turn = ButtonInputPtr(new ButtonInput("simpin.turn:s"));
        upper_turn->setButtonHandler(boost::bind(&PixelBoardD::keyHandler, this, 1, keycode_middleleft, keycode_none), false);
        upper_drop = ButtonInputPtr(new ButtonInput("simpin.drop:x"));
        upper_drop->setButtonHandler(boost::bind(&PixelBoardD::keyHandler, this, 1, keycode_middleright, keycode_none), false);
        // - special pause combination on lower keys
        pause = ButtonInputPtr(new ButtonInput("simpin.pause:p"));
        pause->setButtonHandler(boost::bind(&PixelBoardD::keyHandler, this, 0, keycode_outer, keycode_outer), false);
      }

      // create the touch pad controls
      if (!getOption("notouch")) {
        string touchdetectname = "/gpio.19";
        string touchresetname = "gpio.15";
        getStringOption("touchdetect", touchdetectname);
        getStringOption("touchreset", touchresetname);
        int i2cBusL = 1;
        int i2cBusH = 2;
        getIntOption("i2cbuslow", i2cBusL);
        getIntOption("i2cbushigh", i2cBusH);
        // prepare access to the touch chip
        touchDevL = I2CManager::sharedManager().getDevice(i2cBusL, "generic@1B");
        touchDevH = I2CManager::sharedManager().getDevice(i2cBusH, "generic@1B");
        touchDetect = DigitalIoPtr(new DigitalIo(touchdetectname.c_str(), false, false));
        uint8_t id, sta;
        touchDevL->getBus().SMBusReadByte(touchDevL.get(), 0, id);
        touchDevL->getBus().SMBusReadByte(touchDevL.get(), 0, sta);
        LOG(LOG_NOTICE,"Device ID = 0x%02X, keystatus = 0x%02X", id, sta);
        touchState[0] = 0;
        touchState[1] = 0;
        // prepare access to the key LEDs
        keyLedDevL = I2CManager::sharedManager().getDevice(i2cBusL, "generic@20");
        keyLedDevH = I2CManager::sharedManager().getDevice(i2cBusH, "generic@20");
      }

    } // if !terminated
    // app now ready to run (or cleanup when already terminated)
    return run();
  }


  void gotoPage(const string aPageName, PageMode aMode)
  {
    PagesMap::iterator pos = pages.find(aPageName);
    if (pos!=pages.end()) {
      // hide current page
      if (currentPage) {
        currentPage->hide();
      }
      // start new one
      currentPage = pos->second;
      currentPage->show(aMode);
    }
    updateDisplay();
  }



  virtual void pageInfoHandler(PixelPage &aPage, const string aInfo)
  {
    LOG(LOG_INFO, "Page '%s' sends info '%s'", aPage.getName().c_str(), aInfo.c_str());
    if (aInfo=="register") {
      pages[aPage.getName()] = PixelPagePtr(&aPage);
    }
    else if (aInfo=="unregister") {
      PagesMap::iterator pos = pages.find(aPage.getName());
      if (pos!=pages.end()) {
        if (currentPage==pos->second) {
          currentPage = NULL;
        }
        pages.erase(pos);
      }
    }
    else if (aInfo=="quit") {
      // back to default page
      gotoPage(defaultPageName, defaultMode);
    }
    else if (aInfo=="go0") {
      gotoPage("blocks", defaultMode);
    }
    else if (aInfo=="go3") {
      gotoPage("life", defaultMode);
    }
  }



  virtual void initialize()
  {
    srand((unsigned)MainLoop::currentMainLoop().now()*4223);
    display->begin();
    display->show();
    MainLoop::currentMainLoop().executeOnce(boost::bind(&PixelBoardD::step, this, _1));
    MainLoop::currentMainLoop().executeOnce(boost::bind(&PixelBoardD::gotoPage, this, defaultPageName, defaultMode), 2*Second);
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
        bool upload = false;
        bool action = (method!="GET");
        // check for uploads
        string uploadedfile;
        if (aRequest->get("uploadedfile", o)) {
          uploadedfile = o->stringValue();
          upload = true;
          action = false; // other params are in the URI, not the POSTed upload
        }
        if (action) {
          // JSON data is in the request
          data = aRequest->get("data");
        }
        else {
          // URI params is the JSON to process
          data = aRequest->get("uri_params");
          if (data) action = true; // GET, but with query_params: treat like PUT/POST with data
          if (upload) {
            // move that into the request
            data->add("uploadedfile", JsonObject::newString(uploadedfile));
          }
        }
        // request elements now: uri and data
        if (processRequest(uri, data, action, boost::bind(&PixelBoardD::requestHandled, this, aConnection, _1, _2))) {
          // done, callback will send response and close connection
          return;
        }
        // request cannot be processed, return error
        LOG(LOG_ERR,"Invalid JSON request");
        aError = WebError::webErr(404, "No handler found for request to %s", uri.c_str());
      }
      else {
        LOG(LOG_ERR,"Invalid JSON request");
        aError = WebError::webErr(415, "Invalid JSON request format");
      }
    }
    // return error
    requestHandled(aConnection, JsonObjectPtr(), aError);
  }


  void requestHandled(JsonCommPtr aConnection, JsonObjectPtr aResponse, ErrorPtr aError)
  {
    if (!aResponse) {
      aResponse = JsonObject::newObj(); // empty response
    }
    if (!Error::isOK(aError)) {
      aResponse->add("Error", JsonObject::newString(aError->description()));
    }
    LOG(LOG_INFO,"API answer: %s", aResponse->c_strValue());
    aConnection->sendMessage(aResponse);
    aConnection->closeAfterSend();
  }


  bool processRequest(string aUri, JsonObjectPtr aData, bool aIsAction, RequestDoneCB aRequestDoneCB)
  {
    ErrorPtr err;
    JsonObjectPtr o;
    if (aUri=="player1" || aUri=="player2") {
      int side = aUri=="player2" ? 1 : 0;
      if (aIsAction) {
        if (aData->get("key", o)) {
          string key = o->stringValue();
          // Note: assume keys are already released when event is reported
          if (key=="left")
            keyHandler(side, keycode_left, keycode_none);
          else if (key=="right")
            keyHandler(side, keycode_right, keycode_none);
          else if (key=="turn")
            keyHandler(side, keycode_middleleft, keycode_none);
          else if (key=="drop")
            keyHandler(side, keycode_middleright, keycode_none);
        }
      }
      aRequestDoneCB(JsonObjectPtr(), ErrorPtr());
      return true;
    }
    else if (aUri=="board") {
      if (aIsAction) {
        PageMode mode = pagemode_controls1; // default to bottom controls
        if (aData->get("mode", o))
          mode = o->int32Value();
        if (aData->get("page", o)) {
          string page = o->stringValue();
          gotoPage(page, mode);
        }
      }
      aRequestDoneCB(JsonObjectPtr(), ErrorPtr());
      return true;
    }
    else if (aUri=="page") {
      // ask each page
      for (PagesMap::iterator pos = pages.begin(); pos!=pages.end(); ++pos) {
        if (pos->second->handleRequest(aData, aRequestDoneCB)) {
          // request will be handled by this page, done for now
          return true;
        }
      }
    }
    else if (aUri=="/") {
      string uploadedfile;
      string cmd;
      if (aData->get("uploadedfile", o))
        uploadedfile = o->stringValue();
      if (aData->get("cmd", o))
        cmd = o->stringValue();
      if (cmd=="imageupload" && displayPage) {
        ErrorPtr err = displayPage->loadPNGBackground(uploadedfile);
        gotoPage("display", false);
        updateDisplay();
        aRequestDoneCB(JsonObjectPtr(), err);
        return true;
      }
    }
    return false;
  }


  ErrorPtr processUpload(string aUri, JsonObjectPtr aData, const string aUploadedFile)
  {
    ErrorPtr err;

    string cmd;
    JsonObjectPtr o;
    if (aData->get("cmd", o)) {
      cmd = o->stringValue();
      if (cmd=="imageupload") {
        displayPage->loadPNGBackground(aUploadedFile);
        gotoPage("display", false);
        updateDisplay();
      }
      else {
        err = WebError::webErr(500, "Unknown upload cmd '%s'", cmd.c_str());
      }
    }
    return err;
  }


  void setLeds(int aSide, uint8_t aLedMask)
  {
    if (aSide==1)
      keyLedDevH->getBus().SMBusWriteByte(keyLedDevH.get(), 0x14, aLedMask);
    else
      keyLedDevH->getBus().SMBusWriteByte(keyLedDevL.get(), 0x14, aLedMask);
  }


  void checkInputs()
  {
    uint8_t newState;
    for (int side=0; side<2; side++) {
      I2CDevicePtr td = side==1 ? touchDevH : touchDevL;
      I2CDevicePtr kd = side==1 ? keyLedDevH : keyLedDevL;
      if (td) {
        td->getBus().SMBusReadByte(td.get(), 3, newState); // get key state
        // clear those that were set in last call already
        uint8_t triggers;
        triggers = newState & ~touchState[side];
        LOG(triggers ? LOG_INFO : LOG_DEBUG, "checkInputs: side=%d, touchState=0x%02X, newState=0x%02X, triggers=0x%02X", side, touchState[side], newState, triggers);
        int reportedside = upsideDown ? 1-side : side; // report sides reversed when board is (physically) used upside down.
        KeyCodes newcodes = (KeyCodes)((triggers>>1) & 0x0F);
        KeyCodes pressed = (KeyCodes)((newState>>1) & 0x0F);
        if (touchState[side] != newState) {
          // report all changes, even if no new pressed key, just released ones
          keyHandler(reportedside, newcodes, pressed);
          touchState[side] = newState;
        }
        // update LEDs
        uint8_t leds = currentPage ? currentPage->keyLedState(reportedside) : 0;
        uint8_t ledmask = 0x1E & (~(leds<<1) & 0x1E);
        kd->getBus().SMBusWriteByte(kd.get(), 0x14, ledmask);
      }
    }
    updateDisplay();
  }




  void updateDisplay()
  {
    if (currentPage && currentPage->isDirty()) {
      for (int x=0; x<10; x++) {
        for (int y=0; y<20; y++) {
          PixelColor p = currentPage->colorAt(x, y);
          display->setColorXY(x, y, p.r, p.g, p.b);
        }
      }
      display->show();
      currentPage->updated();
    }
  }



  void step(MLTimer &aTimer)
  {
    bool completed = true;
    if (currentPage) {
      completed = currentPage->step();
    }
    checkInputs();
    updateDisplay();
    MainLoop::currentMainLoop().retriggerTimer(aTimer, 10*MilliSecond);
  }


  void keyHandler(int aSide, KeyCodes aNewPressedKeys, KeyCodes aCurrentPressed)
  {
    LOG(LOG_INFO, "Posting key changes from side %d : New: %c%c%c%c - Pressed %c%c%c%c",
      aSide,
      aNewPressedKeys & keycode_left ? 'L' : '-',
      aNewPressedKeys & keycode_middleleft ? 'T' : '-',
      aNewPressedKeys & keycode_middleright ? 'D' : '-',
      aNewPressedKeys & keycode_right ? 'R' : '-',
      aCurrentPressed & keycode_left ? 'L' : '-',
      aCurrentPressed & keycode_middleleft ? 'T' : '-',
      aCurrentPressed & keycode_middleright ? 'D' : '-',
      aCurrentPressed & keycode_right ? 'R' : '-'
    );
    if (currentPage) {
      bool handled = currentPage->handleKey(aSide, aNewPressedKeys, aCurrentPressed);
      if (!handled && currentPage) {
        // probably another page is now active, let it handle the key as well
        currentPage->handleKey(aSide, aNewPressedKeys, aCurrentPressed);
      }
    }
    updateDisplay();
  }


};





int main(int argc, char **argv)
{
  // prevent debug output before application.main scans command line
  SETLOGLEVEL(LOG_EMERG);
  SETERRLEVEL(LOG_EMERG, false); // messages, if any, go to stderr
  // create app with current mainloop
  static PixelBoardD application;
  // pass control
  return application.main(argc, argv);
}
