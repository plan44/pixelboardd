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

using namespace p44;

#define MAINLOOP_CYCLE_TIME_uS 10000 // 10mS
#define DEFAULT_LOGLEVEL LOG_NOTICE


typedef std::map<string, PixelPagePtr> PagesMap;

/// Main program for plan44.ch P44-DSB-DEH in form of the "vdcd" daemon)
class PixelBoardD : public CmdLineApp
{
  typedef CmdLineApp inherited;

  LEDChainCommPtr display;
  bool upsideDown;

  ButtonInputPtr lower_left;
  ButtonInputPtr lower_right;
  ButtonInputPtr lower_turn;
  ButtonInputPtr lower_drop;

  I2CDevicePtr touchDev;
  I2CDevicePtr keyLedDev;
  DigitalIoPtr touchSel;
  DigitalIoPtr touchDetect;
  uint8_t touchState[2];

  // API Server
  SocketCommPtr apiServer;

  // The display pages
  string defaultPageName;
  bool defaultTwoSided;
  PagesMap pages;
  PixelPagePtr currentPage; ///< the current page

  // pages
  DisplayPagePtr displayPage;
  BlocksPagePtr blocksPage;

  MLMicroSeconds starttime;


public:

  PixelBoardD() :
    starttime(MainLoop::now()),
    upsideDown(false)
  {
  }

  virtual int main(int argc, char **argv)
  {
    const char *usageText =
      "Usage: %1$s [options]\n";
    const CmdLineOptionDescriptor options[] = {
      { 'd', "ledchain",       true,  "device;set device to send LED chain data to" },
      { 'u', "upsidedown",     false, "use board upside down" },
      { 0  , "consolekeys",    false, "allow controlling via console keys" },
      { 0  , "notouch",        false, "disable touch pad checking" },
      { 0  , "jsonapiport",    true,  "port;server port number for JSON API (default=none)" },
      { 0  , "jsonapinonlocal",false, "allow JSON API from non-local clients" },
      { 0  , "defaultpage",    true,  "display page;default page to show after start and after other page ends" },
      { 0  , "twosided",       false, "defines if default page should run two-sided" },
      { 0  , "image",          true,  "filename;image to show by default on display page" },
      { 0  , "message",        true,  "message;text to show from time to time on display page" },
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
      upsideDown = getOption("upsidedown");
      string leddev = "/tmp/ledchainsim";
      getStringOption("ledchain", leddev);
      display = LEDChainCommPtr(new LEDChainComm(LEDChainComm::ledtype_ws281x, leddev, 200, 20, upsideDown, true, true, upsideDown));

      // - start API server and wait for things to happen
      string apiport;
      if (getStringOption("jsonapiport", apiport)) {
        apiServer = SocketCommPtr(new SocketComm(MainLoop::currentMainLoop()));
        apiServer->setConnectionParams(NULL, apiport.c_str(), SOCK_STREAM, AF_INET);
        apiServer->setAllowNonlocalConnections(getOption("jsonapinonlocal"));
        apiServer->startServer(boost::bind(&PixelBoardD::apiConnectionHandler, this, _1), 3);
      }

      getStringOption("defaultpage", defaultPageName);
      defaultTwoSided = getOption("twosided");

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


      if (getOption("consolekeys")) {
        // create the console keys
        lower_left = ButtonInputPtr(new ButtonInput("simpin.left:j"));
        lower_left->setButtonHandler(boost::bind(&PixelBoardD::keyHandler, this, 0, 0), false); // left
        lower_right = ButtonInputPtr(new ButtonInput("simpin.right:l"));
        lower_right->setButtonHandler(boost::bind(&PixelBoardD::keyHandler, this, 0, 3), false); // right
        lower_turn = ButtonInputPtr(new ButtonInput("simpin.turn:k"));
        lower_turn->setButtonHandler(boost::bind(&PixelBoardD::keyHandler, this, 0, 1), false); // turn
        lower_drop = ButtonInputPtr(new ButtonInput("simpin.drop:m"));
        lower_drop->setButtonHandler(boost::bind(&PixelBoardD::keyHandler, this, 0, 2), false); // drop
      }

      // create the touch pad controls
      if (!getOption("notouch")) {
        // prepare access to the touch chip
        touchDev = I2CManager::sharedManager().getDevice(0, "generic@1B");
        touchSel = DigitalIoPtr(new DigitalIo("gpio.7", true, false));
        touchDetect = DigitalIoPtr(new DigitalIo("/gpio.15", false, false));
        uint8_t id, sta;
        touchDev->getBus().SMBusReadByte(touchDev.get(), 0, id);
        touchDev->getBus().SMBusReadByte(touchDev.get(), 0, sta);
        LOG(LOG_NOTICE,"Device ID = 0x%02X, keystatus = 0x%02X", id, sta);
        touchState[0] = 0;
        touchState[1] = 0;
        // prepare access to the key LEDs
        keyLedDev = I2CManager::sharedManager().getDevice(0, "generic@20");
      }

    } // if !terminated
    // app now ready to run (or cleanup when already terminated)
    return run();
  }


  void gotoPage(const string aPageName, bool aTwoSided)
  {
    PagesMap::iterator pos = pages.find(aPageName);
    if (pos!=pages.end()) {
      // hide current page
      if (currentPage) {
        currentPage->hide();
      }
      // start new one
      currentPage = pos->second;
      currentPage->show(aTwoSided);
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
      if (aPage.getName()=="display") {
        gotoPage("blocks", defaultTwoSided);
      }
      else {
        // back to default page
        gotoPage(defaultPageName, defaultTwoSided);
      }
    }
  }



  virtual void initialize()
  {
    srand((unsigned)MainLoop::currentMainLoop().now()*4223);
    display->begin();
    display->show();
    MainLoop::currentMainLoop().registerIdleHandler(this, boost::bind(&PixelBoardD::step, this));
    MainLoop::currentMainLoop().executeOnce(boost::bind(&PixelBoardD::gotoPage, this, defaultPageName, defaultTwoSided), 2*Second);
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
          if (key=="left")
            keyHandler(side, 0); // left
          else if (key=="right")
            keyHandler(side, 3); // right
          else if (key=="turn")
            keyHandler(side, 1); // turn
          else if (key=="drop")
            keyHandler(side, 2); // drop
        }
      }
      aRequestDoneCB(JsonObjectPtr(), ErrorPtr());
      return true;
    }
    else if (aUri=="board") {
      if (aIsAction) {
        bool twoSided = false;
        if (aData->get("twosided", o))
          twoSided = o->boolValue();
        if (aData->get("page", o)) {
          string page = o->stringValue();
          gotoPage(page, twoSided);
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
    touchSel->set(aSide==1); // select side
    keyLedDev->getBus().SMBusReadByte(keyLedDev.get(), 0x14, aLedMask);
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
        int reportedside = upsideDown ? 1-side : side; // report sides reversed when board is (physically) used upside down.
        if (triggers & 0x02) keyHandler(reportedside, 0); // left
        else if (triggers & 0x10) keyHandler(reportedside, 3); // right
        else if (triggers & 0x04) keyHandler(reportedside, 1); // turn
        else if (triggers & 0x08) keyHandler(reportedside, 2); // drop
        // update LEDs
        uint8_t leds = currentPage ? currentPage->keyLedState(reportedside) : 0;
        uint8_t ledmask = 0x1E & (~(leds<<1) & 0x1E);
        keyLedDev->getBus().SMBusWriteByte(keyLedDev.get(), 0x14, ledmask);
      }
      updateDisplay();
    }
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



  bool step()
  {
    bool completed = true;
    if (currentPage) {
      completed = currentPage->step();
    }
    checkInputs();
    updateDisplay();
    return completed;
  }


  void keyHandler(int aSide, int aKeyNum)
  {
    if (currentPage) {
      bool handled = currentPage->handleKey(aSide, aKeyNum);
      if (!handled && currentPage) {
        // probably another page is now active, let it handle the key as well
        currentPage->handleKey(aSide, aKeyNum);
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
  // create the mainloop
  MainLoop::currentMainLoop().setLoopCycleTime(MAINLOOP_CYCLE_TIME_uS);
  // create app with current mainloop
  static PixelBoardD application;
  // pass control
  return application.main(argc, argv);
}
