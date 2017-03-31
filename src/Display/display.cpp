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

#include "display.hpp"

using namespace p44;


// MARK: ===== DisplayPage


DisplayPage::DisplayPage(PixelPageInfoCB aInfoCallback) :
  inherited("display", aInfoCallback),
  pngBuffer(NULL),
  lastMessageShow(Never)
{
  clear();
  message = TextViewPtr(new TextView(7, 0, 20, 1));
}


void DisplayPage::show(PageMode aMode)
{
  makeDirty();
  showMessage(defaultMessage);
}


void DisplayPage::hide()
{

}


bool DisplayPage::step()
{
  if (message) {
    if (lastMessageShow+autoMessageTimeout<MainLoop::now() && defaultMessage.size()>0) {
      showMessage(defaultMessage);
    }
    message->step();
    if (message->isDirty()) makeDirty();
  }
  return true;
}



void DisplayPage::clear()
{
  for (int i=0; i<PAGE_NUMPIXELS; ++i) {
    pixels[i].r = 0;
    pixels[i].g = 0;
    pixels[i].b = 0;
    pixels[i].a = 0;
  }
  // init libpng image structure
  memset(&pngImage, 0, (sizeof pngImage));
  pngImage.version = PNG_IMAGE_VERSION;
  // free the buffer if allocated
  if (pngBuffer) {
    free(pngBuffer);
    pngBuffer = NULL;
  }
}


PixelColor DisplayPage::colorAt(int aX, int aY)
{
  PixelColor pix = bgColorAt(aX, aY);
  PixelColor ovl = pixels[aY*PAGE_NUMCOLS+aX];
  overlayPixel(pix, ovl);
  if (message) {
    ovl = message->colorAt(aX, aY);
    overlayPixel(pix, ovl);
  }
  return pix;
}


void DisplayPage::setColorAt(int aX, int aY, uint8_t r, uint8_t g, uint8_t b)
{
  if (!isWithin(aX, aY)) return;
  pixels[aY*PAGE_NUMCOLS+aX] = { r, g, b, 255 };
}




bool DisplayPage::handleRequest(JsonObjectPtr aRequest, RequestDoneCB aRequestDoneCB)
{
  JsonObjectPtr o;
  if (aRequest->get("message", o)) {
    showMessage(o->stringValue());
    if (aRequestDoneCB) aRequestDoneCB(JsonObjectPtr(), ErrorPtr());
    return true;
  }
  else if (aRequest->get("textcolor", o)) {
    // webcolor
    string webcolor = o->stringValue();
    uint32_t col;
    if (sscanf(webcolor.c_str(),"%x",&col)==1) {
      PixelColor p;
      if (webcolor.size()<7)
        p.a = 0xFF;
      else
        p.a = (col>>24) & 0xFF;
      p.r = (col>>16) & 0xFF;
      p.g = (col>>8) & 0xFF;
      p.b = col & 0xFF;
      message->setTextColor(p);
    }
    if (aRequestDoneCB) aRequestDoneCB(JsonObjectPtr(), ErrorPtr());
    return true;
  }
  return false; // page does not handle the request
}


void DisplayPage::showMessage(const string aMessage)
{
  message->setText(aMessage);
  lastMessageShow = MainLoop::now();
}


void DisplayPage::setDefaultMessage(const string aMessage)
{
  defaultMessage = aMessage;
}


bool DisplayPage::handleKey(int aSide, int aKeyNum)
{
  inherited::handleKey(aSide, aKeyNum);
  return false; // let next page handle the keys again
}


ErrorPtr DisplayPage::loadPNGBackground(const string aPNGFileName)
{
  // clear any previous pattern
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
    LOG(LOG_INFO, "Image size in bytes = %d\n", PNG_IMAGE_SIZE(pngImage));
    LOG(LOG_INFO, "Image width = %d\n", pngImage.width);
    LOG(LOG_INFO, "Image height = %d\n", pngImage.height);
    LOG(LOG_INFO, "Image width*height = %d\n", pngImage.height*pngImage.width);
    if (pngBuffer==NULL) {
      return TextError::err("Could not allocate buffer for reading PNG file %s", aPNGFileName.c_str());
    }
    // now actually red the image
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


PixelColor DisplayPage::bgColorAt(int aX, int aY)
{
  // FIXME
  aY = 19-aY;

  PixelColor p;
  if (
    pngBuffer == NULL ||
    aX<0 || aX>=pngImage.width ||
    aY<0 || aY>=pngImage.height
  ) {
    p.r = 0;
    p.g = 0;
    p.b = 0;
    p.a = 0;
    return p; // black
  }
  // get pixel infomration
  uint8_t *pix = pngBuffer+(aY*pngImage.width+aX)*4;
  p.r = *pix++;
  p.g = *pix++;
  p.b = *pix++;
  p.a = *pix++;
  return p;
}

