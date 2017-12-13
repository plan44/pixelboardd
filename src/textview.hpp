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

#ifndef __pixelboardd_textview_hpp__
#define __pixelboardd_textview_hpp__

#include "p44utils_common.hpp"

#include "view.hpp"

namespace p44 {

  class TextView : public View
  {
    typedef View inherited;

    // text parameters
    bool scrolling; // set if text should scroll
    int text_intensity; // intensity of last column of text (where text appears)
    int cycles_per_px;
    int text_repeats; // text displays until faded down to almost zero
    int fade_per_repeat; // how much to fade down per repeat
    uint8_t fade_base; // crossfading base brightness level
    bool mirrorText;
    PixelColor textColor;
    MLMicroSeconds textStepTime = 0.02*Second;

    // text rendering
    string text; ///< internal representation of text
    uint8_t *textPixels;
    int textPixelOffset;
    int textCycleCount;
    int repeatCount;
    MLMicroSeconds lastTextStep;

  public :

    TextView(int aOriginX, int aOriginY, int aWidth, int aOrientation=View::right);

    virtual ~TextView();

    virtual void clear();

    /// calculate changes on the display, return true if any
    /// @return true if complete, false if step() would like to be called immediately again
    /// @note this is called on the active page at least once per mainloop cycle
    virtual bool step();

    /// set new text
    void setText(const string aText, bool aScrolling = true);

    /// set new text color
    void setTextColor(PixelColor aTextColor);

  protected:

    /// get content color at X,Y
    virtual PixelColor contentColorAt(int aX, int aY);

  private:

    void crossFade(uint8_t aFader, uint8_t aValue, uint8_t &aOutputA, uint8_t &aOutputB);

  };
  typedef boost::intrusive_ptr<TextView> TextViewPtr;


} // namespace p44



#endif /* __pixelboardd_textview_hpp__ */
