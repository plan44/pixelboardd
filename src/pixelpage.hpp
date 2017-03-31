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

#ifndef __pixelboardd_pixelpage_hpp__
#define __pixelboardd_pixelpage_hpp__

#include "p44utils_common.hpp"
#include "jsonobject.hpp"

namespace p44 {

  class PixelPage;

  #define PAGE_NUMCOLS 10
  #define PAGE_NUMROWS 20

  #define PAGE_NUMPIXELS (PAGE_NUMCOLS*PAGE_NUMROWS)

  typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a; // alpha
  } PixelColor;


  enum {
    pagemode_controls1 = 0x01,
    pagemode_controls2 = 0x02,
    pagemode_startnow = 0x04
  } PageModeEnum;
  typedef uint8_t PageMode;

  /// Utilities
  uint8_t dimVal(uint8_t aVal, uint8_t aDim);
  PixelColor dimPixel(const PixelColor aPix, uint8_t aDim);
  void reduce(uint8_t &aByte, uint8_t aAmount, uint8_t aMin = 0);
  void increase(uint8_t &aByte, uint8_t aAmount, uint8_t aMax = 255);
  void overlayPixel(PixelColor &aPixel, PixelColor aOverlay);


  /// Is called from the page to deliver events to the application
  typedef boost::function<void (PixelPage &aPage, const string aInfo)> PixelPageInfoCB;

  typedef boost::function<void (JsonObjectPtr aResponse, ErrorPtr aError)> RequestDoneCB;


  class PixelPage : public P44Obj
  {

    string name;
    PixelPageInfoCB infoCallback;
    bool dirty;

  public :

    PixelPage(const string aName, PixelPageInfoCB aInfoCallback);

    virtual ~PixelPage();

    string getName() { return name; }

    /// start showing this page
    /// @param aMode in what mode to show the page (0x01=bottom, 0x02=top, 0x03=both)
    virtual void show(PageMode aMode) = 0;

    /// hide this page
    virtual void hide() = 0;

    /// calculate changes on the display, return true if any
    /// @return true if complete, false if step() would like to be called immediately again
    /// @note this is called on the active page at least once per mainloop cycle
    virtual bool step();

    /// handle key events
    /// @param aSide which side of the board (0=bottom, 1=top)
    /// @param aKeyNum key number 0..3 (on keypads: left==0...right==3)
    /// @return true if fully handled, false if next page should handle it as well
    virtual bool handleKey(int aSide, int aKeyNum);

    /// get key LED status
    /// @param aSide which side of the board (0=bottom, 1=top)
    /// @return bits 0..3 correspond to LEDs for key 0..3
    virtual uint8_t keyLedState(int aSide) { return 0; }

    /// handle API requests
    /// @param aRequest JSON request
    /// @param aRequestDoneCB must be called when the request has been executed, possibly passing back error or answer
    /// @return true if request will be handled by this page, false otherwise (other pages will be asked)
    virtual bool handleRequest(JsonObjectPtr aRequest, RequestDoneCB aRequestDoneCB);

    /// return if anything changed on the display since last call
    bool isDirty() { return dirty; };

    /// call when display is updated
    void updated() { dirty = false; };

    /// get color at X,Y
    /// @param aX PlayField X coordinate
    /// @param aY PlayField Y coordinate
    virtual PixelColor colorAt(int aX, int aY) = 0;

    /// true if coordinate is within display
    bool isWithin(int aX, int aY);

  protected:

    void makeDirty() { dirty = true; };

    // post info
    void postInfo(const string aInfo);

    JsonObjectPtr errorAnswer(ErrorPtr aError);

  };
  typedef boost::intrusive_ptr<PixelPage> PixelPagePtr;


} // namespace p44



#endif /* __pixelboardd_pixelpage_hpp__ */
