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

#ifndef __pixelboardd_life_hpp__
#define __pixelboardd_life_hpp__

#include "pixelpage.hpp"
#include "textview.hpp"


namespace p44 {

  class LifePage : public PixelPage
  {
    typedef PixelPage inherited;

    uint32_t cells[PAGE_NUMPIXELS]; ///< internal representation

    uint8_t ledState[2];

    PageMode defaultMode;

    long generationTicket;
    long reviveTicket;

  public :

    MLMicroSeconds generationInterval;

    LifePage(PixelPageInfoCB aInfoCallback);

    /// show
    /// @param aMode in what mode to show the page (0x01=bottom, 0x02=top, 0x03=both)
    virtual void show(PageMode aMode) P44_OVERRIDE;

    /// hide
    virtual void hide() P44_OVERRIDE;

    /// calculate changes on the display, return true if any
    /// @return true if complete, false if step() would like to be called immediately again
    /// @note this is called on the active page at least once per mainloop cycle
    virtual bool step() P44_OVERRIDE;

    /// handle key events
    /// @param aSide which side of the board (0=bottom, 1=top)
    /// @param aKeyNum key number 0..3 (on keypads: left==0...right==3)
    /// @return true if fully handled, false if next page should handle it as well
    virtual bool handleKey(int aSide, int aKeyNum) P44_OVERRIDE;

    /// get key LED status
    /// @param aSide which side of the board (0=bottom, 1=top)
    /// @return bits 0..3 correspond to LEDs for key 0..3
    virtual uint8_t keyLedState(int aSide) P44_OVERRIDE;

    /// get color at X,Y
    /// @param aX PlayField X coordinate
    /// @param aY PlayField Y coordinate
    virtual PixelColor colorAt(int aX, int aY) P44_OVERRIDE;

  protected:

    void stop();
    void clear();
    void nextGeneration();
    void timeNext();
    void revive();
    int cellindex(int aX, int aY, bool aWrap);
    int calculateGeneration();
    void createRandomCells(int aMinCells, int aMaxCells);
    void placePattern(uint16_t aPatternNo, bool aWrap=true, int aCenterX=-1, int aCenterY=-1, int aOrientation=-1);
  };
  typedef boost::intrusive_ptr<LifePage> LifePagePtr;



} // namespace p44



#endif /* __pixelboardd_life_hpp__ */
