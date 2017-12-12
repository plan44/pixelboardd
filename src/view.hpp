//
//  Copyright (c) 2016-2017 plan44.ch / Lukas Zeller, Zurich, Switzerland
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

#ifndef __pixelboardd_view_hpp__
#define __pixelboardd_view_hpp__

#include "p44utils_common.hpp"

namespace p44 {


  typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a; // alpha
  } PixelColor;

  /// Utilities
  uint8_t dimVal(uint8_t aVal, uint8_t aDim);
  PixelColor dimPixel(const PixelColor aPix, uint8_t aDim);
  void reduce(uint8_t &aByte, uint8_t aAmount, uint8_t aMin = 0);
  void increase(uint8_t &aByte, uint8_t aAmount, uint8_t aMax = 255);
  void overlayPixel(PixelColor &aPixel, PixelColor aOverlay);

  class View : public P44Obj
  {

    bool dirty;

    // outer frame
    int originX;
    int originY;
    int dX;
    int dY;

    // alpha (opacity)
    uint8_t alpha;

    // background
    PixelColor backgroundColor;

  public:

    // Orientation
    enum {
      // basic transformation flags
      xy_swap = 0x01,
      x_flip = 0x02,
      y_flip = 0x04,
      // directions of X axis
      right = 0, /// untransformed X goes left to right, Y goes up
      down = xy_swap+x_flip, /// X goes down, Y goes right
      left = x_flip+y_flip, /// X goes left, Y goes down
      up = xy_swap+y_flip, /// X goes down, Y goes right
    };
    typedef uint8_t Orientation;

  protected:

    // content
    int offsetX; ///< content X offset (in view coordinates)
    int offsetY; ///< content Y offset (in view coordinates)
    Orientation contentOrientation; ///< orientation of content in view (defines content->view coordinate transformation)
    int contentSizeX; ///< X size of content (in content coordinates)
    int contentSizeY; ///< Y size of content (in content coordinates)

    /// get content pixel color
    /// @param aX content X coordinate
    /// @param aY content Y coordinate
    /// @note aX and aY are NOT guaranteed to be within actual content as defined by contentSizeX/Y
    ///   implementation must check this!
    virtual PixelColor contentColorAt(int aX, int aY) { return backgroundColor; }

    /// initialize for use
    void init();

    /// set dirty - to be called by step() implementation when the view needs to be redisplayed
    void makeDirty() { dirty = true; };

  public :

    /// create view
    View();

    virtual ~View();

    /// set the frame within the board coordinate system
    /// @param aOriginX origin X on pixelboard
    /// @param aOriginY origin Y on pixelboard
    /// @param aSizeX the X width of the view
    /// @param aSizeY the Y width of the view
    void setFrame(int aOriginX, int aOriginY, int aSizeX, int aSizeY);

    /// set the view's background color
    /// @param aBackGroundColor color of pixels not covered by content
    void setBackGroundColor(PixelColor aBackGroundColor) { backgroundColor = aBackGroundColor; makeDirty(); };

    void setAlpha(int aAlpha) { alpha = aAlpha; makeDirty(); }

    /// @param aOrientation the orientation of the content
    void setOrientation(Orientation aOrientation) { contentOrientation = aOrientation; makeDirty(); }

    /// set content offset
    void setContentOffset(int aOffsetX, int aOffsetY) { offsetX = aOffsetX; offsetY = aOffsetY; makeDirty(); };

    /// set content size
    void setContentSize(int aSizeX, int aSizeY) { contentSizeX = aSizeX; contentSizeY = aSizeY; makeDirty(); };


    /// calculate changes on the display, return true if any
    /// @return true if complete, false if step() would like to be called immediately again
    /// @note this is called on the active page at least once per mainloop cycle
    virtual bool step() = 0;

    /// return if anything changed on the display since last call
    bool isDirty() { return dirty; };

    /// call when display is updated
    void updated() { dirty = false; };

    /// get color at X,Y
    /// @param aX PlayField X coordinate
    /// @param aY PlayField Y coordinate
    PixelColor colorAt(int aX, int aY);

  };
  typedef boost::intrusive_ptr<View> ViewPtr;

} // namespace p44



#endif /* __pixelboardd_view_hpp__ */
