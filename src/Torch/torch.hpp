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

#ifndef __pixelboardd_torch_hpp__
#define __pixelboardd_torch_hpp__

#include "pixelpage.hpp"

#ifdef TORCH

namespace p44 {


  class TorchPage : public PixelPage
  {
    typedef PixelPage inherited;

  public :

    TorchPage(PixelPageInfoCB aInfoCallback);

    /// start showing this page
    /// @param aTwoSided expect two-sided usage of the page
    virtual void show(bool aTwoSided) P44_OVERRIDE;

    /// stop showing this page
    virtual void hide() P44_OVERRIDE;

    /// stop to next
    virtual bool step() P44_OVERRIDE;

    /// get color at X,Y
    /// @param aX PlayField X coordinate
    /// @param aY PlayField Y coordinate
    virtual PixelColor colorAt(int aX, int aY) P44_OVERRIDE;

    /// show Message on Torch
    ErrorPtr message(const string aMessage);

  private:

    /// clear entire display
    void clear();

  };
  typedef boost::intrusive_ptr<DisplayPage> DisplayPagePtr;




} // namespace p44

#endif // TORCH


#endif /* __pixelboardd_display_hpp__ */
