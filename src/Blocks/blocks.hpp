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

#ifndef __pixelboardd_blocks_hpp__
#define __pixelboardd_blocks_hpp__

#include "pixelpage.hpp"
#include "textview.hpp"


namespace p44 {


  class BlocksPage;

  typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
  } BlocksColor;


  typedef enum {
    block_line,
    block_sqare,
    block_l,
    block_l_reverse,
    block_t,
    block_squiggly,
    block_squiggly_reverse,
    numBlockTypes
  } BlockType;

  typedef uint8_t ColorCode;


  class Block : public P44Obj
  {

    BlocksPage &playField; ///< the playfield
    ColorCode colorCode; ///< the color code of the piece
    int x; ///< X coordinate of the (rotation) center of the piece
    int y; ///< Y coordinate of the (rotation) center of the piece
    int orientation; ///< 0=normal, 1..3=90 degree rotation steps clockwise
    bool shown; ///< set when piece is currently shown on playfield
    BlockType blockType; ///< the block type

  public:

    Block(BlocksPage &aPlayField, BlockType aBlockType, ColorCode aColorCode = 0);

    /// remove from playfield
    void remove();

    /// show on playfield
    void show();

    /// switch colors to dimmed palette
    void dim();

    /// move block
    bool move(int aDx, int aDy, int aRot, bool aOpenAtBottom);

    /// (re)position piece on playfield, returns true if possible, false if not
    /// @param aX playfield X coordinate of where piece should be positioned
    /// @param aY playfield Y coordinate of where piece should be positioned
    /// @param aOrientation new orientation
    /// @param aOpenAtBottom if set, playfield is considered open at the bottom
    /// @return true if positioning could be carried out (was possible without collision)
    bool position(int aX, int aY, int aOrientation, bool aOpenAtBottom);

    /// get max extents of block around its center
    void getExtents(int aOrientation, int &aMinDx, int &aMaxDx, int &aMinDy, int &aMaxDy);


  protected:

    /// iterate over pixels of this piece
    /// @param aPixelIndex enumerate pixels from 0..numPixels
    /// @param aDx returns x coordinate relative to piece's rotation center
    /// @param aDy returns y coordinate relative to piece's rotation center
    /// @return false when piece has no more pixels
    bool getPixel(int aPixelIndex, int &aDx, int &aDy);

  private:

    /// transform relative to absolute pixel position
    /// @param aPixelIndex enumerate pixels from 0..numPixels
    /// @param aCenterX playfield X coordinate of piece's center
    /// @param aCenterY playfield Y coordinate of piece's center
    /// @param aOrientation new orientation
    /// @param aX returns pixel's playfield X coordinate (no playfield bounds checking, can be negative)
    /// @param aY returns pixel's playfield Y coordinate (no playfield bounds checking, can be negative)
    /// @return false when piece has no more pixels
    bool getPositionedPixel(int aPixelIndex, int aCenterX, int aCenterY, int aOrientation, int &aX, int &aY);

  };
  typedef boost::intrusive_ptr<Block> BlockPtr;



  class BlockRunner
  {
    friend class BlocksPage;

    BlockPtr block;
    MLMicroSeconds lastStep;
    MLMicroSeconds stepInterval;
    bool movingUp;
    bool dropping; ///< set when block starts dropping
    int droppedsteps; ///< how many steps block has dropped so far
  };


  class BlocksPage : public PixelPage
  {
    enum {
      game_ready,
      game_running,
      game_over
    } gameState;

    typedef PixelPage inherited;
    friend class Block;

    ColorCode colorCodes[PAGE_NUMPIXELS]; ///< internal representation

    BlockRunner activeBlocks[2]; ///< the max 2 active blocks (top and bottom)
    int score[2]; ///< the score for the players
    int level; ///< the level

    long rowKillTicket;
    long stateChangeTicket;

    uint8_t ledState[2];

    PageMode playModeAccumulator;
    PageMode defaultMode;

    TextViewPtr scoretext;

  public :

    MLMicroSeconds stepInterval;
    MLMicroSeconds dropStepInterval;
    MLMicroSeconds rowKillDelay;


    BlocksPage(PixelPageInfoCB aInfoCallback);

    /// show
    /// @param aMode in what mode to show the page (0x01=bottom, 0x02=top, 0x03=both)
    virtual void show(PageMode aMode) P44_OVERRIDE;

    /// hide
    virtual void hide() P44_OVERRIDE;

    /// calculate changes on the display, return true if any
    /// @return true if complete, false if step() would like to be called immediately again
    /// @note this is called on the active page at least once per mainloop cycle
    virtual bool step() P44_OVERRIDE;

    /// handle key events
    /// @param aSide which side of the board (0=bottom, 1=top)
    /// @param aKeyNum key number 0..3 (on keypads: left==0...right==3)
    /// @return true if fully handled, false if next page should handle it as well
    virtual bool handleKey(int aSide, int aKeyNum) P44_OVERRIDE;

    /// get key LED status
    /// @param aSide which side of the board (0=bottom, 1=top)
    /// @return bits 0..3 correspond to LEDs for key 0..3
    virtual uint8_t keyLedState(int aSide) P44_OVERRIDE;

    /// get color at X,Y
    /// @param aX PlayField X coordinate
    /// @param aY PlayField Y coordinate
    virtual PixelColor colorAt(int aX, int aY) P44_OVERRIDE;

  protected:

    /// get color at X,Y
    /// @param aX PlayField X coordinate
    /// @param aY PlayField Y coordinate
    ColorCode colorCodeAt(int aX, int aY);

    /// set color at X,Y
    /// @param aX PlayField X coordinate
    /// @param aY PlayField Y coordinate
    void setColorCodeAt(ColorCode aColorCode, int aX, int aY);

    /// check if X,Y is within (or ouside above/below) playfield
    /// @param aX PlayField X coordinate
    /// @param aY PlayField Y coordinate
    /// @param aOpenAtBottom if set, playfield is considered open at the bottom
    bool isWithinPlayfield(int aX, int aY, bool aOpen = false, bool aOpenAtBottom = false);

    /// launch a block into the playfield
    /// @param aBlockType the type of block
    /// @param aColumn the column where to launch (will be adjusted left or right in case block extends playfield)
    /// @param aOrientation initial orientation
    /// @param aBottom if set, launch a block from bottom up
    bool launchBlock(BlockType aBlockType, int aColumn, int aOrientation, bool aBottom);

    /// launch new random block
    bool launchRandomBlock(bool aBottom);
    
    /// move block
    void moveBlock(int aDx, int aRot, bool aLower);

    /// change speed of block
    void dropBlock(bool aLower);

  private:

    void clear();
    void stop();
    void makeReady(bool aWithAutostart);
    void startAccTimeout();
    void startGame(PageMode aMode); // 0x01=single player normal, 0x02=single player reversed, 0x03=dual player
    void gameOver();
    void removeRow(int aY, bool aBlockFromBottom, int aRemovedRows);
    void checkRows(bool aBlockFromBottom, int aRemovedRows);


  };
  typedef boost::intrusive_ptr<BlocksPage> BlocksPagePtr;




} // namespace p44



#endif /* __pixelboardd_blocks_hpp__ */
