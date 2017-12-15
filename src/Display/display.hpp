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

#ifndef __pixelboardd_display_hpp__
#define __pixelboardd_display_hpp__

#include "pixelpage.hpp"
#include "textview.hpp"
#include "imageview.hpp"
#include "viewstack.hpp"


namespace p44 {


  class DisplayPage : public PixelPage
  {
    typedef PixelPage inherited;

    TextViewPtr message;
    ImageViewPtr bgimage;
    string defaultMessage;
    MLMicroSeconds lastMessageShow;
    MLMicroSeconds autoMessageTimeout = 3*Minute;

  public :

    DisplayPage(PixelPageInfoCB aInfoCallback);

    /// start showing this page
    /// @param aMode in what mode to show the page (0x01=bottom, 0x02=top, 0x03=both)
    virtual void show(PageMode aMode) P44_OVERRIDE;

    /// hide this page
    virtual void hide() P44_OVERRIDE;

    /// calculate changes on the display, return true if any
    /// @return true if complete, false if step() would like to be called immediately again
    /// @note this is called on the active page at least once per mainloop cycle
    virtual bool step() P44_OVERRIDE;

    /// show PNG on DisplayPage
    ErrorPtr loadPNGBackground(const string aPNGFileName);

    /// set default message
    void setDefaultMessage(const string aMessage);

    /// handle key events
    /// @param aSide which side of the board (0=bottom, 1=top)
    /// @param aNewPressedKeys combined keycodes of keys newly detected pressed in this event.
    ///   Can be keycode_none for events signalling only released keys
    /// @param aCurrentPressed combined keycodes of keys currently pressed
    /// @return true if fully handled, false if next page should handle it as well
    virtual bool handleKey(int aSide, KeyCodes aNewPressedKeys, KeyCodes aCurrentPressed) P44_OVERRIDE;

    /// handle API requests
    /// @param aRequest JSON request
    /// @param aRequestDoneCB must be called when the request has been executed, possibly passing back error or answer
    /// @return true if request will be handled by this page, false otherwise (other pages will be asked)
    virtual bool handleRequest(JsonObjectPtr aRequest, RequestDoneCB aRequestDoneCB) P44_OVERRIDE;

  private:

    /// clear entire field
    void clear();

    void showMessage(const string aMessage);

  };
  typedef boost::intrusive_ptr<DisplayPage> DisplayPagePtr;




} // namespace p44



#endif /* __pixelboardd_display_hpp__ */
