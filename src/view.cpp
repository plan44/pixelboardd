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

#include "view.hpp"

using namespace p44;

// MARK: ===== View

View::View()
{
  setFrame(0, 0, 0, 0);
  // default to normal orientation
  contentOrientation = right;
  // default content size is same as view's
  offsetX = 0;
  offsetY = 0;
  contentSizeX = 0;
  contentSizeY = 0;
  backgroundColor = { .r=0, .g=0, .b=0, .a=0 }; // transparent background...
  alpha = 255; // but content pixels passed trough 1:1
  targetAlpha = -1; // not fading
}


View::~View()
{
  clear();
}


void View::clear()
{
  setContentSize(0, 0);
}


bool View::isInContentSize(int aX, int aY)
{
  return aX>=0 && aY>=0 && aX<contentSizeX && aY<contentSizeY;
}


void View::setFrame(int aOriginX, int aOriginY, int aSizeX, int aSizeY)
{
  originX = aOriginX;
  originY = aOriginY;
  dX = aSizeX,
  dY = aSizeY;
  makeDirty();
}


bool View::step()
{
  // check fading
  if (targetAlpha>=0) {
    double timeDone = (double)(MainLoop::now()-startTime)/fadeTime;
    if (timeDone<1) {
      // continue fading
      int currentAlpha = targetAlpha-(1-timeDone)*fadeDist;
      setAlpha(currentAlpha);
    }
    else {
      // target alpha reached
      setAlpha(targetAlpha);
      targetAlpha = -1;
      // call back
      if (fadeCompleteCB) {
        SimpleCB cb = fadeCompleteCB;
        fadeCompleteCB = NULL;
        cb();
      }
    }
  }
  return true; // completed
}


void View::setAlpha(int aAlpha)
{
  if (alpha!=aAlpha) {
    alpha = aAlpha;
    makeDirty();
  }
}


void View::stopFading()
{
  targetAlpha = -1;
  fadeCompleteCB = NULL; // did not run to end
}


void View::fadeTo(int aAlpha, MLMicroSeconds aWithIn, SimpleCB aCompletedCB)
{
  fadeDist = aAlpha-alpha;
  startTime = MainLoop::now();
  fadeTime = aWithIn;
  if (fadeTime<=0 || fadeDist==0) {
    // immediate
    setAlpha(aAlpha);
    targetAlpha = -1;
    if (aCompletedCB) aCompletedCB();
  }
  else {
    // start fading
    targetAlpha = aAlpha;
    fadeCompleteCB = aCompletedCB;
  }
}


void View::setFullFrameContent()
{
  setContentSize(dX, dY);
  setContentOffset(0, 0);
  setOrientation(View::right);
}




#define SHOW_ORIGIN 0

PixelColor View::colorAt(int aX, int aY)
{
  // default is background color
  PixelColor pc = backgroundColor;
  if (alpha==0) {
    pc.a = 0; // entire view is invisible
  }
  else {
    // calculate coordinate relative to the content's origin
    int x = aX-originX-offsetX;
    int y = aY-originY-offsetY;
    // translate into content coordinates
    if (contentOrientation & xy_swap) {
      swap(x, y);
    }
    if (contentOrientation & x_flip) {
      x = contentSizeX-x-1;
    }
    if (contentOrientation & y_flip) {
      y = contentSizeY-y-1;
    }
    // NOT limited to content size, content must restrict this!
    pc = contentColorAt(x, y);
    #if SHOW_ORIGIN
    if (x==0 && y==0) {
      return { .r=255, .g=0, .b=0, .a=255 };
    }
    else if (x==1 && y==0) {
      return { .r=0, .g=255, .b=0, .a=255 };
    }
    else if (x==0 && y==1) {
      return { .r=0, .g=0, .b=255, .a=255 };
    }
    #endif
    if (pc.a==0) {
      // background is where content is fully transparent
      pc = backgroundColor;
      // Note: view background does NOT shine through semi-transparent content pixels!
      //   But non-transparent content pixels directly are view pixels!
    }
    // factor in layer alpha
    if (alpha!=255) {
      pc.a = dimVal(pc.a, alpha);
    }
  }
  return pc;
}


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
    addToPixel(aPixel, aOverlay);
  }
  aPixel.a = 255; // result is never transparent
}


void p44::addToPixel(PixelColor &aPixel, PixelColor aIncrease)
{
  aPixel.r += aIncrease.r;
  aPixel.g += aIncrease.g;
  aPixel.b += aIncrease.b;
}




