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

  const PixelColor transparent = { .r=0, .g=0, .b=0, .a=0 };
  const PixelColor black = { .r=0, .g=0, .b=0, .a=255 };

  /// Utilities
  uint8_t dimVal(uint8_t aVal, uint8_t aDim);
  PixelColor dimPixel(const PixelColor aPix, uint8_t aDim);
  void reduce(uint8_t &aByte, uint8_t aAmount, uint8_t aMin = 0);
  void increase(uint8_t &aByte, uint8_t aAmount, uint8_t aMax = 255);
  void addToPixel(PixelColor &aPixel, PixelColor aIncrease);
  void overlayPixel(PixelColor &aPixel, PixelColor aOverlay);

  class View : public P44Obj
  {
    friend class ViewStack;

    bool dirty;

    /// fading
    int targetAlpha; ///< alpha to reach at end of fading, -1 = not fading
    int fadeDist; ///< amount to fade
    MLMicroSeconds startTime; ///< time when fading has started
    MLMicroSeconds fadeTime; ///< how long fading takes
    SimpleCB fadeCompleteCB; ///< fade complete

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

    // outer frame
    int originX;
    int originY;
    int dX;
    int dY;

    // alpha (opacity)
    uint8_t alpha;

    // background
    PixelColor backgroundColor;

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

    /// helper for implementations: check if aX/aY within set content size
    bool isInContentSize(int aX, int aY);

    /// set dirty - to be called by step() implementation when the view needs to be redisplayed
    void makeDirty() { dirty = true; };

  public :

    /// create view
    View();

    virtual ~View();

    /// clear contents of this view
    /// @note base class just resets content size to zero, subclasses might NOT want to do that
    ///   and thus choose NOT to call inherited.
    virtual void clear();

    /// set the frame within the board coordinate system
    /// @param aOriginX origin X on pixelboard
    /// @param aOriginY origin Y on pixelboard
    /// @param aSizeX the X width of the view
    /// @param aSizeY the Y width of the view
    virtual void setFrame(int aOriginX, int aOriginY, int aSizeX, int aSizeY);

    /// set the view's background color
    /// @param aBackGroundColor color of pixels not covered by content
    void setBackGroundColor(PixelColor aBackGroundColor) { backgroundColor = aBackGroundColor; makeDirty(); };

    /// set view's alpha
    /// @param aAlpha 0=fully transparent, 255=fully opaque
    void setAlpha(int aAlpha);

    /// get current alpha
    /// @return current alpha value 0=fully transparent=invisible, 255=fully opaque
    uint8_t getAlpha() { return alpha; };

    /// hide (set alpha to 0)
    void hide() { setAlpha(0); };

    /// show (set alpha to max)
    void show() { setAlpha(255); };

    /// set visible (hide or show)
    /// @param aVisible true to show, false to hide
    void setVisible(bool aVisible) { if (aVisible) show(); else hide(); };

    /// fade alpha
    /// @param aAlpha 0=fully transparent, 255=fully opaque
    /// @param aWithIn time from now when specified aAlpha should be reached
    /// @param aCompletedCB is called when fade is complete
    void fadeTo(int aAlpha, MLMicroSeconds aWithIn, SimpleCB aCompletedCB = NULL);

    /// stop ongoing fading
    /// @note: completed callback will not be called
    void stopFading();

    /// @param aOrientation the orientation of the content
    void setOrientation(Orientation aOrientation) { contentOrientation = aOrientation; makeDirty(); }

    /// set content offset
    void setContentOffset(int aOffsetX, int aOffsetY) { offsetX = aOffsetX; offsetY = aOffsetY; makeDirty(); };

    /// set content size
    void setContentSize(int aSizeX, int aSizeY) { contentSizeX = aSizeX; contentSizeY = aSizeY; makeDirty(); };

    /// set content size to full frame content with same coordinates
    void setFullFrameContent();

    /// calculate changes on the display, return true if any
    /// @return true if complete, false if step() would like to be called immediately again
    /// @note this is called on the active page at least once per mainloop cycle
    virtual bool step();

    /// return if anything changed on the display since last call
    virtual bool isDirty() { return dirty; };

    /// call when display is updated
    virtual void updated() { dirty = false; };

    /// get color at X,Y
    /// @param aX PlayField X coordinate
    /// @param aY PlayField Y coordinate
    virtual PixelColor colorAt(int aX, int aY);

  };
  typedef boost::intrusive_ptr<View> ViewPtr;

} // namespace p44



#endif /* __pixelboardd_view_hpp__ */
