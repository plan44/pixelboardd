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

#ifndef __pixelboardd_imageview_hpp__
#define __pixelboardd_imageview_hpp__

#include "p44utils_common.hpp"

#include "view.hpp"

#include <png.h>

namespace p44 {

  class ImageView : public View
  {
    typedef View inherited;

    png_image pngImage; /// The control structure used by libpng
    png_bytep pngBuffer; /// byte buffer

  public :

    ImageView();

    virtual ~ImageView();

    /// clear image
    void clear();

    /// load PNG image
    ErrorPtr loadPNG(const string aPNGFileName);

  protected:

    /// get content color at X,Y
    virtual PixelColor contentColorAt(int aX, int aY);

  };
  typedef boost::intrusive_ptr<ImageView> ImageViewPtr;


} // namespace p44



#endif /* __pixelboardd_imageview_hpp__ */
