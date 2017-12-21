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

#include "display.hpp"
#include "application.hpp"

using namespace p44;


// MARK: ===== DisplayPage


DisplayPage::DisplayPage(PixelPageInfoCB aInfoCallback) :
  inherited("display", aInfoCallback),
  lastMessageShow(Never)
{
  clear();
  // user defined content
  message = TextViewPtr(new TextView(2, 0, 20, View::down));
  bgimage = ImageViewPtr(new ImageView());
  sizeViewToPage(bgimage);
  // help screen
  infoView = ImageViewPtr(new ImageView());
  infoView->loadPNG(Application::sharedApplication()->resourcePath("images/main.png"));
  ViewStackPtr stack = ViewStackPtr(new ViewStack());
  stack->pushView(bgimage);
  stack->pushView(message);
  stack->pushView(infoView);
  setView(stack);
}


void DisplayPage::show(PageMode aMode)
{
  makeDirty();
  infoFlash(0);
  // showMessage(defaultMessage);
}


bool DisplayPage::infoFlash(int aSide)
{
  bool wasVisible = infoView->getAlpha()>0;
  infoView->setOrientation(aSide ? View::left : View::right);
  infoView->show();
  infoView->fadeTo(0, 15*Second);
  return wasVisible;
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
  }
  return inherited::step();
}



void DisplayPage::clear()
{
  if (message) message->clear();
  if (bgimage) bgimage->clear();
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


bool DisplayPage::handleKey(int aSide, KeyCodes aNewPressedKeys, KeyCodes aCurrentPressed)
{
  // every keypress revives info
  if (infoFlash(aSide)) {
    // info was already on display
    if (aNewPressedKeys & keycode_left) {
      postInfo("go0");
    }
    else if (aNewPressedKeys & keycode_middleleft) {
      postInfo("go1");
    }
    else if (aNewPressedKeys & keycode_middleright) {
      postInfo("go2");
    }
    if (aNewPressedKeys & keycode_right) {
      postInfo("go3");
    }
  }
  return true; // done
}


ErrorPtr DisplayPage::loadPNGBackground(const string aPNGFileName)
{
  return bgimage->loadPNG(aPNGFileName);
}
