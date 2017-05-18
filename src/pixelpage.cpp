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

#include "pixelpage.hpp"

using namespace p44;

// MARK: ===== Utilities

uint8_t p44::dimVal(uint8_t aVal, uint8_t aDim)
{
  return ((uint16_t)aVal*(aDim+1))>>8;
}


PixelColor p44::dimPixel(const PixelColor aPix, uint8_t aDim)
{
  PixelColor pix;
  pix.r = dimVal(aPix.r, aDim);
  pix.g = dimVal(aPix.g, aDim);
  pix.b = dimVal(aPix.b, aDim);
  pix.a = aPix.a;
  return pix;
}


void p44::reduce(uint8_t &aByte, uint8_t aAmount, uint8_t aMin)
{
  int r = aByte-aAmount;
  if (r<aMin)
    aByte = aMin;
  else
    aByte = (uint8_t)r;
}


void p44::increase(uint8_t &aByte, uint8_t aAmount, uint8_t aMax)
{
  int r = aByte+aAmount;
  if (r>aMax)
    aByte = aMax;
  else
    aByte = (uint8_t)r;
}


void p44::overlayPixel(PixelColor &aPixel, PixelColor aOverlay)
{
  if (aOverlay.a==255) {
    aPixel = aOverlay;
  }
  else {
    // mix in
    // - reduce original by alpha of overlay
    aPixel = dimPixel(aPixel, 255-aOverlay.a);
    // - reduce overlay by its own alpha
    aOverlay = dimPixel(aOverlay, aOverlay.a);
    // - add in
    aPixel.r += aOverlay.r;
    aPixel.g += aOverlay.g;
    aPixel.b += aOverlay.b;
  }
  aPixel.a = 255; // result is never transparent
}





// MARK: ===== PixelPage


PixelPage::PixelPage(const string aName, PixelPageInfoCB aInfoCallback) :
  name(aName),
  infoCallback(aInfoCallback)
{
  postInfo("register");
}


PixelPage::~PixelPage()
{
  postInfo("unregister");
}


bool PixelPage::step()
{
  // dummy in base class
  return true; // everything done
}


bool PixelPage::handleKey(int aSide, KeyCodes aNewPressedKeys, KeyCodes aCurrentPressed)
{
  // by default, any key quits current page
  postInfo("quit");
  return true; // and that's all
}


bool PixelPage::handleRequest(JsonObjectPtr aRequest, RequestDoneCB aRequestDoneCB)
{
  return false; // page does not handle the request
}


JsonObjectPtr PixelPage::errorAnswer(ErrorPtr aError)
{
  JsonObjectPtr ans;
  if (Error::isOK(aError)) {
    ans = JsonObject::newObj();
    ans->add("error", JsonObject::newString(aError->description()));
  }
  return ans;
}


void PixelPage::postInfo(const string aInfo)
{
  if (infoCallback) infoCallback(*this, aInfo);
}


bool PixelPage::isWithin(int aX, int aY)
{
  if (aX<0 || aX>=PAGE_NUMCOLS) return false;
  if (aY<0 || aY>=PAGE_NUMROWS) return false;
  return true; // within
}



