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



