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

#include "blocks.hpp"

using namespace p44;


// MARK: ===== Blocks definitions

static const int maxPixelsPerPiece = 4; // Tetrominoes

typedef struct {
  int dx;
  int dy;
} Point;

typedef struct {
  ColorCode color;
  int numPixels;
  Point pixelOffsets[maxPixelsPerPiece];
} BlockDef;


static const BlockDef blockDefs[numBlockTypes] = {
  { 1, 4, { {-2,0},{-1,0},{0,0},{1,0} }}, // line
  { 2, 4, { {0,0},{1,0},{1,1},{0,1} }}, // square
  { 3, 4, { {0,-1},{1,-1},{0,0},{0,1} }}, // L
  { 4, 4, { {0,-1},{1,-1},{1,0},{1,1} }}, // L reversed
  { 5, 4, { {-1,0},{0,0},{1,0},{0,1} }}, // T
  { 6, 4, { {0,-1},{0,0},{1,0},{1,1} }}, // squiggly
  { 7, 4, { {1,-1},{1,0},{0,0},{0,1} }}, // squiggly reversed
};


typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} ColorDef;


static const ColorDef colorDefs[33] = {
  // falling top down
  {   0,   0,   0 }, // none
  { 255,   0,   0 }, // line
  { 255, 255,   0 }, // square
  {   0, 160, 255 }, // L
  {   0, 255, 160 }, // L reverse
  {   0,   0, 255 }, // T
  { 160,   0, 255 }, // squiggly
  { 255,   0, 160 }, // squiggly reversed
  // rising bottom up
  {   0,   0,   0 }, // none
  { 255,   0,   0 }, // line
  { 255, 255,   0 }, // square
  {   0, 160, 255 }, // L
  {   0, 255, 160 }, // L reverse
  {   0,   0, 255 }, // T
  { 160,   0, 255 }, // squiggly
  { 255,   0, 160 }, // squiggly reversed
  // DIMMED: falling top down
  {   0,   0,   0 }, // none
  { 255,   0,   0 }, // line
  { 255, 255,   0 }, // square
  {   0, 160, 255 }, // L
  {   0, 255, 160 }, // L reverse
  {   0,   0, 255 }, // T
  { 160,   0, 255 }, // squiggly
  { 255,   0, 160 }, // squiggly reversed
  // DIMMED: rising bottom up
  {   0,   0,   0 }, // none
  { 255,   0,   0 }, // line
  { 255, 255,   0 }, // square
  {   0, 160, 255 }, // L
  {   0, 255, 160 }, // L reverse
  {   0,   0, 255 }, // T
  { 160,   0, 255 }, // squiggly
  { 255,   0, 160 }, // squiggly reversed
  // Row kill flash
  { 255, 255, 255 }
};




// MARK: ===== Block

Block::Block(BlocksPage &aPlayField, BlockType aBlockType, ColorCode aColorCode) :
  playField(aPlayField),
  blockType(aBlockType),
  colorCode(aColorCode),
  shown(false),
  x(0),
  y(0),
  orientation(0)
{
  if (colorCode==0) {
    // use defaul color code from table
    colorCode = blockDefs[blockType].color;
  }
}


bool Block::getPixel(int aPixelIndex, int &aDx, int &aDy)
{
  const BlockDef *bd = &blockDefs[blockType];
  if (aPixelIndex<0 || aPixelIndex>=bd->numPixels) return false;
  aDx = bd->pixelOffsets[aPixelIndex].dx;
  aDy = bd->pixelOffsets[aPixelIndex].dy;
  return true;
}


void Block::getExtents(int aOrientation, int &aMinDx, int &aMaxDx, int &aMinDy, int &aMaxDy)
{
  aMinDx=0;
  aMaxDx=0;
  aMinDy=0;
  aMaxDy=0;
  int idx=0;
  int dx,dy;
  while (getPositionedPixel(idx, 0, 0, aOrientation, dx, dy)) {
    if (dx<aMinDx) aMinDx = dx;
    else if (dx>aMaxDx) aMaxDx = dx;
    if (dy<aMinDy) aMinDy = dy;
    else if (dy>aMaxDy) aMaxDy = dy;
    idx++;
  }
}




bool Block::getPositionedPixel(int aPixelIndex, int aCenterX, int aCenterY, int aOrientation, int &aX, int &aY)
{
  int dx,dy;
  if (!getPixel(aPixelIndex, dx, dy)) return false;
  switch (aOrientation) {
    default: aX=aCenterX+dx; aY=aCenterY+dy; break;
    case 1: aX=aCenterX+dy; aY=aCenterY-dx; break;
    case 2: aX=aCenterX-dx; aY=aCenterY-dy; break;
    case 3: aX=aCenterX-dy; aY=aCenterY+dx; break;
  }
  return true;
}


void Block::remove()
{
  int px,py;
  int idx=0;
  if (shown) {
    shown = false;
    while (getPositionedPixel(idx, x, y, orientation, px, py)) {
      playField.setColorCodeAt(0, px, py);
      idx++;
    }
  }
}


void Block::show()
{
  int px,py;
  int idx=0;
  if (!shown) {
    shown = true;
    while (getPositionedPixel(idx, x, y, orientation, px, py)) {
      playField.setColorCodeAt(colorCode, px, py);
      idx++;
    }
  }
}


void Block::dim()
{
  int px,py;
  int idx=0;
  shown = true;
  while (getPositionedPixel(idx, x, y, orientation, px, py)) {
    playField.setColorCodeAt(colorCode | 0x10, px, py);
    idx++;
  }
}



bool Block::position(int aX, int aY, int aOrientation, bool aOpenAtBottom)
{
  int px,py;
  int idx=0;
  bool wasShown = shown;
  // unshow it
  remove();
  // check that new position is free
  while (getPositionedPixel(idx, aX, aY, aOrientation, px, py)) {
    if (!playField.isWithinPlayfield(px, py, true, aOpenAtBottom) || playField.colorCodeAt(px, py)!=0) {
      // collision or not within field
      if (wasShown) {
        // show again at previous position
        show();
      }
      // signal new position is not possible
      return false;
    }
    idx++;
  }
  // can be positioned this way, do it now
  x = aX;
  y = aY;
  orientation = aOrientation;
  // show in new place
  show();
  // signal block at new position
  return true;
}


bool Block::move(int aDx, int aDy, int aRot, bool aOpenAtBottom)
{
  int o = (orientation + aRot) % 4;
  int nx = x+aDx;
  int ny = y+aDy;
  return position(nx, ny, o, aOpenAtBottom);
}



// MARK: ===== BlocksPage


BlocksPage::BlocksPage(PixelPageInfoCB aInfoCallback) :
  inherited("blocks", aInfoCallback),
  stepInterval(0.7*Second),
  dropStepInterval(0.05*Second),
  rowKillDelay(0.15*Second),
  defaultMode(0x01),
  gameState(game_ready),
  playModeAccumulator(0),
  rowKillTicket(0),
  stateChangeTicket(0)
{
  stop();
  scoretext = TextViewPtr(new TextView(7, 0, 20, 1));
  scoretext->setTextColor({255, 128, 0, 255});
}


void BlocksPage::hide()
{
  stop();
}


void BlocksPage::clear()
{
  stop();
  for (int i=0; i<PAGE_NUMPIXELS; ++i) {
    colorCodes[i] = 0;
  }
  makeDirty();
}


void BlocksPage::stop()
{
  MainLoop::currentMainLoop().cancelExecutionTicket(rowKillTicket);
  MainLoop::currentMainLoop().cancelExecutionTicket(stateChangeTicket);
  // remove block if any is still running
  for (int bi=0; bi<2; bi++) {
    // clear runners
    BlockRunner *b = &activeBlocks[bi];
    if (b->block) b->block->dim(); // dim it, no longer live
    b->block = NULL; // forget it
    ledState[bi] = 0; // LEDs off
  }
}



void BlocksPage::show(PageMode aMode)
{
  defaultMode = aMode;
  // make ready
  makeReady(true);
}


void BlocksPage::makeReady(bool aWithAutostart)
{
  gameState = game_ready;
  playModeAccumulator = 0;
  // show start keys
  ledState[0] = 0x06; // Turn and Drop on
  ledState[1] = 0x06; // Turn and Drop on
  if (aWithAutostart) {
    // auto-start default game in 15 secs
    MainLoop::currentMainLoop().executeTicketOnce(
      stateChangeTicket,
      boost::bind(&BlocksPage::startGame, this, defaultMode),
      (defaultMode & pagemode_startnow) ? 1*Second : 15*Second
    );
  }
  else {
    // quit after a while
    MainLoop::currentMainLoop().executeTicketOnce(stateChangeTicket,boost::bind(&BlocksPage::postInfo, this, "quit"), 42*Second);
  }
  makeDirty();
}


void BlocksPage::startGame(PageMode aMode)
{
  MainLoop::currentMainLoop().cancelExecutionTicket(stateChangeTicket);
  stop();
  clear();
  level = 0;
  score[0] = 0;
  score[1] = 0;
  ledState[0] = 0;
  ledState[1] = 0;
  gameState = game_running;
  if ((aMode & pagemode_controls1) || aMode==0) {
    // normal side
    launchRandomBlock(false);
    ledState[0] = 0x0F; // all 4 keys on
  }
  if (aMode & pagemode_controls2) {
    // other side
    launchRandomBlock(true);
    ledState[1] = 0x0F; // all 4 keys on
  }
}


void BlocksPage::gameOver()
{
  gameState = game_over;
  MainLoop::currentMainLoop().executeTicketOnce(stateChangeTicket,boost::bind(&BlocksPage::makeReady, this, false), 5*Second);
  ledState[0] = 0; // LEDs off
  ledState[1] = 0; // LEDs off
  scoretext->setText(string_format("%d", score[0]+score[1]), true);
  makeDirty();
}


PixelColor BlocksPage::colorAt(int aX, int aY)
{
  uint8_t cc = colorCodeAt(aX, aY);
  PixelColor pix;
  const ColorDef *cdef = &colorDefs[cc];
  pix.r = cdef->r;
  pix.g = cdef->g;
  pix.b = cdef->b;
  if (gameState!=game_running) {
    pix = dimPixel(pix, 128);
    // overlay text
    PixelColor ovl = scoretext->colorAt(aX, aY);
    overlayPixel(pix, ovl);
  }
  else if (cc>=16 && cc<32) {
    pix = dimPixel(pix, 188);
  }
  return pix;
}


bool BlocksPage::isWithinPlayfield(int aX, int aY, bool aOpen, bool aOpenAtBottom)
{
  if (aX<0 || aX>=PAGE_NUMCOLS) return false; // may not extend sidewards
  if ((aOpenAtBottom || !aOpen) && aY>=PAGE_NUMROWS) return false; // may not extend above playfield
  if ((!aOpenAtBottom || !aOpen) && aY<0) return false; // may not extend below playfield
  return true; // within
}


ColorCode BlocksPage::colorCodeAt(int aX, int aY)
{
  if (!isWithin(aX, aY)) return 0;
  return colorCodes[aY*PAGE_NUMCOLS+aX];
}


void BlocksPage::setColorCodeAt(ColorCode aColorCode, int aX, int aY)
{
  if (!isWithin(aX, aY)) return;
  colorCodes[aY*PAGE_NUMCOLS+aX] = aColorCode;
}


uint8_t BlocksPage::keyLedState(int aSide)
{
  return ledState[aSide];
}


bool BlocksPage::handleKey(int aSide, int aKeyNum)
{
  if (gameState==game_ready) {
    // left or right restarts, drop exits
    if (aKeyNum==2) {
      postInfo("quit");
    }
    else if (aKeyNum==1) {
      // add to acculumator
      playModeAccumulator |= aSide==1 ? pagemode_controls2 : pagemode_controls1;
      ledState[aSide==1 ? 1 : 0] = 0x0F; // immediate feedback: all 4 keys on
      // but start with a little delay so other player can also join
      MainLoop::currentMainLoop().executeTicketOnce(stateChangeTicket,boost::bind(&BlocksPage::startAccTimeout, this), 2*Second);
    }
  }
  else if (gameState==game_running) {
    int movement = 0;
    int rotation = 0;
    bool drop = false;
    switch (aKeyNum) {
      case 0 : movement = -1; break; // left
      case 1 : rotation = 1; break; // turn
      case 2 : drop = true; break; // drop
      case 3 : movement = 1; break; // right
    }
    // movement X is right edge, so need to swap for bottom end keys
    if (aSide==0) movement = -movement;
    if (movement!=0 || rotation!=0) {
      moveBlock(movement, rotation, aSide==1);
    }
    if (drop) {
      dropBlock(aSide==1);
    }
  }
  return true; // fully handled
}


void BlocksPage::startAccTimeout()
{
  startGame(playModeAccumulator);
}




bool BlocksPage::launchBlock(BlockType aBlockType, int aColumn, int aOrientation, bool aBottom)
{
  BlockRunner *b = &activeBlocks[aBottom ? 1 : 0];
  // remove block if any is already running
  if (b->block) b->block->remove();
  // create new block
  b->block = BlockPtr(new Block(*this, aBlockType, (aBottom ? 8 : 0)+blockDefs[aBlockType].color));
  b->movingUp = aBottom;
  b->stepInterval = stepInterval;
  b->dropping = false;
  b->droppedsteps = 0;
  // position it
  int minx,maxx,miny,maxy;
  b->block->getExtents(aBottom ? (aOrientation+2) % 4 : aOrientation, minx, maxx, miny, maxy);
  LOG(LOG_DEBUG, "launchblock: blocktype=%d, orientation=%d, extents: minx=%d, maxx=%d, miny=%d, maxy=%d", aBlockType, aOrientation, minx, maxx, miny, maxy);
  // make sure it is within horizontal bounds
  if (aColumn+minx<0) aColumn = -minx;
  else if (aColumn+maxx>=PAGE_NUMCOLS) aColumn = PAGE_NUMCOLS-1-maxx;
  // position vertically with first pixel just visible
  int row;
  if (!aBottom) {
    row = PAGE_NUMROWS-1-miny;
  }
  else {
    row = -maxy;
  }
  LOG(LOG_DEBUG, "- positioning at row %d", row);
  if (!b->block->position(aColumn, row, aOrientation, aBottom)) {
    // cannot launch a block any more
    b->block = NULL;
    return false;
  }
  else {
    b->lastStep = MainLoop::now();
    return true;
  }
}



bool BlocksPage::launchRandomBlock(bool aBottom)
{
  BlockType bt = (BlockType)(rand() % numBlockTypes);
  //  int col = rand() % PAGE_NUMCOLS;
  int col = 5; // always the same
  return launchBlock(bt, col, 0, aBottom);
}



void BlocksPage::removeRow(int aY, bool aBlockFromBottom, int aRemovedRows)
{
  // unshow running blocks
  for (int i=0; i<2; i++) {
    BlockRunner *b = &activeBlocks[i];
    if (b->block) b->block->remove();
  }
  // move towards falling direction of block that has caused the row to fill
  int dir = aBlockFromBottom ? -1 : 1;
  for (int y=aY; (aBlockFromBottom ? y>=0 : y<PAGE_NUMROWS); y += dir) {
    for (int x=0; x<PAGE_NUMCOLS; x++) {
      setColorCodeAt(colorCodeAt(x, y+dir), x, y);
    }
  }
  // re-show running blocks
  for (int i=0; i<2; i++) {
    BlockRunner *b = &activeBlocks[i];
    if (b->block) b->block->show();
  }
  // count
  aRemovedRows++;
  // continue checking
  rowKillTicket = MainLoop::currentMainLoop().executeOnce(boost::bind(&BlocksPage::checkRows, this, aBlockFromBottom, aRemovedRows), rowKillDelay);
  makeDirty();
}


void BlocksPage::checkRows(bool aBlockFromBottom, int aRemovedRows)
{
  // check from top to bottom relative to falling block
  int dir = aBlockFromBottom ? -1 : 1;
  for (int y = aBlockFromBottom ? PAGE_NUMROWS-1 : 0; (aBlockFromBottom ? y>=0 : y<PAGE_NUMROWS); y += dir) {
    // check each row
    bool hasGap = false;
    for (int x=0; x<PAGE_NUMCOLS; x++) {
      if (colorCodeAt(x, y)==0) {
        hasGap = true;
        break;
      }
    }
    if (!hasGap) {
      // full row found
      for (int x=0; x<PAGE_NUMCOLS; x++) {
        // light up
        setColorCodeAt(32, x, y); // row flash
      }
      makeDirty();
      rowKillTicket = MainLoop::currentMainLoop().executeOnce(boost::bind(&BlocksPage::removeRow, this, y, aBlockFromBottom, aRemovedRows), rowKillDelay);
      return;
    }
  }
  // no more full rows found -> count score now
  if (aRemovedRows>0) {
    // - row removal scoring:
    //   - one row:      40*(level+1)
    //   - two rows:    100*(level+1)
    //   - three rows:  300*(level+1)
    //   - four rows:  1200*(level+1)
    int s=0;
    switch (aRemovedRows) {
      case 1 : s = 40; break;
      case 2 : s = 100; break;
      case 3 : s = 300; break;
      case 4 : s = 1200; break;
    }
    s = s *(level+1);
    LOG(LOG_INFO,"Scoring: %d rows removed in level %d -> %d points", aRemovedRows, level, s);
    score[aBlockFromBottom ? 1 : 0] += s;
  }
}



bool BlocksPage::step()
{
  MLMicroSeconds now = MainLoop::now();
  if (gameState==game_running) {
    int ab = 0;
    for (int i=0; i<2; i++) {
      BlockRunner *b = &activeBlocks[i];
      if (b->block) {
        if (now>=b->lastStep+b->stepInterval) {
          if (b->block->move(0, b->movingUp ? 1 : -1, 0, b->movingUp)) {
            // block could move
            b->lastStep = now;
            if (b->dropping) b->droppedsteps++; // count dropped steps
            makeDirty();
          }
          else {
            // could not move, means that we've collided with floor or existing pixels
            makeDirty();
            b->block->dim();
            // count dropping score
            if (b->dropping) {
              LOG(LOG_INFO, "Scoring: dropped %d steps -> %d points", b->droppedsteps, b->droppedsteps);
              score[b->movingUp ? 1 : 0] += b->droppedsteps;
            }
            // check rows
            checkRows(b->movingUp, 0);
            // remove block (but pixels will remain)
            b->block = NULL;
            // start new one
            if (!launchRandomBlock(b->movingUp)) {
              // game over
              gameOver();
            }
          }
        }
        if (b->block) {
          ab++;
        }
      }
    }
    if (ab==0) gameOver();
  }
  scoretext->step();
  if (scoretext->isDirty()) makeDirty();
  return true; // no need to call again immediately
}


void BlocksPage::moveBlock(int aDx, int aRot, bool aLower)
{
  BlockRunner *b = &activeBlocks[aLower ? 1 : 0];
  if (b->block) {
    if (b->block->move(aDx, 0, aRot, aLower)) {
      makeDirty();
    }
  }
}


void BlocksPage::dropBlock(bool aLower)
{
  BlockRunner *b = &activeBlocks[aLower ? 1 : 0];
  if (b->block) {
    b->stepInterval = dropStepInterval;
    b->dropping = true;
    b->droppedsteps = 0;
  }
}

