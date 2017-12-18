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

#include "imageview.hpp"

using namespace p44;

// MARK: ===== ImageView


ImageView::ImageView() :
  pngBuffer(NULL)
{
}


ImageView::~ImageView()
{
}


void ImageView::clear()
{
  inherited::clear();
  // init libpng image structure
  memset(&pngImage, 0, (sizeof pngImage));
  pngImage.version = PNG_IMAGE_VERSION;
  // free the buffer if allocated
  if (pngBuffer) {
    free(pngBuffer);
    pngBuffer = NULL;
  }
}


ErrorPtr ImageView::loadPNG(const string aPNGFileName)
{
  // clear any previous pattern (and make dirty)
  clear();
  // read image
  if (png_image_begin_read_from_file(&pngImage, aPNGFileName.c_str()) == 0) {
    // error
    return TextError::err("could not open PNG file %s", aPNGFileName.c_str());
  }
  else {
    // We only need the luminance
    pngImage.format = PNG_FORMAT_RGBA;
    // Now allocate enough memory to hold the image in this format; the
    // PNG_IMAGE_SIZE macro uses the information about the image (width,
    // height and format) stored in 'image'.
    pngBuffer = (png_bytep)malloc(PNG_IMAGE_SIZE(pngImage));
    LOG(LOG_INFO, "Image size in bytes = %d", PNG_IMAGE_SIZE(pngImage));
    LOG(LOG_INFO, "Image width = %d", pngImage.width);
    LOG(LOG_INFO, "Image height = %d", pngImage.height);
    LOG(LOG_INFO, "Image width*height = %d", pngImage.height*pngImage.width);
    contentSizeX = pngImage.width;
    contentSizeY = pngImage.height;
    if (pngBuffer==NULL) {
      return TextError::err("Could not allocate buffer for reading PNG file %s", aPNGFileName.c_str());
    }
    // now actually read the image
    if (png_image_finish_read(
      &pngImage,
      NULL, // background
      pngBuffer,
      0, // row_stride
      NULL //colormap
    ) == 0) {
      // error
      ErrorPtr err = TextError::err("Error reading PNG file %s: error: %s", aPNGFileName.c_str(), pngImage.message);
      clear(); // clear only after pngImage.message has been used
      return err;
    }
  }
  // image read ok
  makeDirty();
  return ErrorPtr();
}


PixelColor ImageView::contentColorAt(int aX, int aY)
{
  if (aX<0 || aX>=contentSizeX || aY<0 || aY>=contentSizeY) {
    return inherited::contentColorAt(aX, aY);
  }
  else {
    PixelColor pc;
    // get pixel infomration from image buffer
    uint8_t *pix = pngBuffer+((pngImage.height-1-aY)*pngImage.width+aX)*4;
    pc.r = *pix++;
    pc.g = *pix++;
    pc.b = *pix++;
    pc.a = *pix++;
    return pc;
  }
}

