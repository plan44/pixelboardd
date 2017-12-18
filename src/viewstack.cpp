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

#include "viewstack.hpp"

using namespace p44;


// MARK: ===== ViewStack

ViewStack::ViewStack()
{
}


ViewStack::~ViewStack()
{
}


void ViewStack::clear()
{
  viewStack.clear();
  inherited::clear();
}


void ViewStack::pushView(ViewPtr aView)
{
  viewStack.push_back(aView);
  makeDirty();
}


void ViewStack::popView()
{
  viewStack.pop_back();
  makeDirty();
}


void ViewStack::removeView(ViewPtr aView)
{
  for (ViewsList::iterator pos = viewStack.begin(); pos!=viewStack.end(); ++pos) {
    if ((*pos)==aView) {
      viewStack.erase(pos);
      break;
    }
  }
}


bool ViewStack::step()
{
  bool complete = inherited::step();
  for (ViewsList::iterator pos = viewStack.begin(); pos!=viewStack.end(); ++pos) {
    if (!(*pos)->step()) {
      complete = false;
    }
  }
  return complete;
}


bool ViewStack::isDirty()
{
  if (inherited::isDirty()) return true; // dirty anyway
  for (ViewsList::iterator pos = viewStack.begin(); pos!=viewStack.end(); ++pos) {
    if ((*pos)->isDirty())
      return true; // subview is dirty -> stack is dirty
  }
  return false;
}


void ViewStack::updated()
{
  inherited::updated();
  for (ViewsList::iterator pos = viewStack.begin(); pos!=viewStack.end(); ++pos) {
    (*pos)->updated();
  }
}


PixelColor ViewStack::contentColorAt(int aX, int aY)
{
  // default is the viewstack's background color
  if (alpha==0) {
    return transparent; // entire viewstack is invisible
  }
  else {
    // consult views in stack
    PixelColor pc = black;
    PixelColor lc;
    uint8_t seethrough = 255; // first layer is directly visible, not yet obscured
    for (ViewsList::reverse_iterator pos = viewStack.rbegin(); pos!=viewStack.rend(); ++pos) {
      ViewPtr layer = *pos;
      if (layer->alpha==0) continue; // shortcut: skip fully transparent layers
      lc = layer->colorAt(aX, aY);
      if (lc.a==0) continue; // skip layer with fully transparent pixel
      // not-fully-transparent pixel
      // - scale down to current budget left
      lc.a = dimVal(lc.a, seethrough);
      lc = dimPixel(lc, lc.a);
      addToPixel(pc, lc);
      seethrough -= lc.a;
      if (seethrough<=0) break; // nothing more to see though
    } // collect from all layers
    if (seethrough>0) {
      // rest is background
      lc.a = dimVal(backgroundColor.a, seethrough);
      lc = dimPixel(backgroundColor, lc.a);
      addToPixel(pc, lc);
    }
    // factor in alpha of entire viewstack
    if (alpha!=255) {
      pc.a = dimVal(pc.a, alpha);
    }
    return pc;
  }
}
