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


static BlockDef blockDefs[numBlockTypes] = {
  { 1, 4, { {-2,0},{-1,0},{0,0},{1,0} }}, // line
  { 2, 4, { {0,0},{1,0},{1,1},{0,1} }}, // square
  { 3, 4, { {0,-1},{1,-1},{0,0},{0,1} }}, // L
  { 4, 4, { {0,-1},{1,-1},{1,0},{1,1} }}, // L reversed
  { 5, 4, { {-1,0},{0,0},{1,0},{0,1} }}, // T
  { 6, 4, { {0,-1},{0,0},{1,0},{1,1} }}, // squiggly
  { 7, 4, { {1,-1},{1,0},{0,0},{0,1} }}, // squiggly reversed
};


// MARK: ===== Block

Block::Block(PlayField &aPlayField, BlockType aBlockType, ColorCode aColorCode) :
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
  BlockDef *bd = &blockDefs[blockType];
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
    case 0: aX=aCenterX+dx; aY=aCenterY+dy; break;
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
      playField.setColorAt(0, px, py);
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
      playField.setColorAt(colorCode, px, py);
      idx++;
    }
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
    if (!playField.isWithin(px, py, true, aOpenAtBottom) || playField.colorAt(px, py)!=0) {
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



// MARK: ===== PlayField


PlayField::PlayField() :
  dirty(false),
  stepInterval(0.7*Second),
  dropStepInterval(0.05*Second),
  rowKillDelay(0.05*Second)
{
  clear();
}


void PlayField::clear(ColorCode aFillColor)
{
  for (int i=0; i<PLAYFIELD_NUMPIXELS; ++i) {
    colorCodes[i] = aFillColor;
  }
}


bool PlayField::isWithin(int aX, int aY, bool aOpen, bool aOpenAtBottom)
{
  if (aX<0 || aX>=PLAYFIELD_NUMCOLS) return false; // may not extend sidewards
  if ((aOpenAtBottom || !aOpen) && aY>=PLAYFIELD_NUMROWS) return false; // may not extend above playfield
  if ((!aOpenAtBottom || !aOpen) && aY<0) return false; // may not extend below playfield
  return true; // within
}


ColorCode PlayField::colorAt(int aX, int aY)
{
  if (!isWithin(aX, aY)) return 0;
  return colorCodes[aY*PLAYFIELD_NUMCOLS+aX];
}


void PlayField::setColorAt(ColorCode aColorCode, int aX, int aY)
{
  if (!isWithin(aX, aY)) return;
  colorCodes[aY*PLAYFIELD_NUMCOLS+aX] = aColorCode;
}


void PlayField::launchBlock(BlockType aBlockType, int aColumn, int aOrientation, bool aBottom)
{
  BlockRunner *b = &activeBlocks[aBottom ? 1 : 0];
  // remove block if any is already running
  if (b->block) b->block->remove();
  // create new block
  b->block = BlockPtr(new Block(*this, aBlockType));
  b->movingUp = aBottom;
  b->stepInterval = stepInterval;
  // position it
  int minx,maxx,miny,maxy;
  b->block->getExtents(aBottom ? (aOrientation+2) % 4 : aOrientation, minx, maxx, miny, maxy);
  // make sure it is within horizontal bounds
  if (aColumn+minx<0) aColumn = -minx;
  else if (aColumn+maxx>=PLAYFIELD_NUMCOLS) aColumn = PLAYFIELD_NUMCOLS-1-maxx;
  // position vertically with first pixel just visible
  int row;
  if (!aBottom) {
    row = PLAYFIELD_NUMROWS-1+miny;
  }
  else {
    row = -maxy;
  }
  b->block->position(aColumn, row, aOrientation, aBottom);
  b->lastStep = MainLoop::now();
}



void PlayField::launchRandomBlock(bool aBottom)
{
  BlockType bt = (BlockType)(rand() % numBlockTypes);
  //  int col = rand() % PLAYFIELD_NUMCOLS;
  int col = 5; // always the same
  launchBlock(bt, col, 0, aBottom);
}



void PlayField::removeRow(int aY, bool aBlockFromBottom)
{
  // unshow running blocks
  for (int i=0; i<2; i++) {
    BlockRunner *b = &activeBlocks[i];
    if (b->block) b->block->remove();
  }
  // move towards falling direction of block that has caused the row to fill
  int dir = aBlockFromBottom ? -1 : 1;
  for (int y=aY; (aBlockFromBottom ? y>=0 : y<PLAYFIELD_NUMROWS); y += dir) {
    for (int x=0; x<PLAYFIELD_NUMCOLS; x++) {
      setColorAt(colorAt(x, y+dir), x, y);
    }
  }
  // re-show running blocks
  for (int i=0; i<2; i++) {
    BlockRunner *b = &activeBlocks[i];
    if (b->block) b->block->show();
  }
  // ccontinue checking
  MainLoop::currentMainLoop().executeOnce(boost::bind(&PlayField::checkRows, this, aBlockFromBottom), rowKillDelay);
  dirty = true;
}


void PlayField::checkRows(bool aBlockFromBottom)
{
  for (int y=0; y<PLAYFIELD_NUMROWS; y++) {
    // check each row
    bool hasGap = false;
    for (int x=0; x<PLAYFIELD_NUMCOLS; x++) {
      if (colorAt(x, y)==0) {
        hasGap = true;
        break;
      }
    }
    if (!hasGap) {
      // full row found
      for (int x=0; x<PLAYFIELD_NUMCOLS; x++) {
        // light up
        setColorAt(7, x, y);
      }
      dirty = true;
      MainLoop::currentMainLoop().executeOnce(boost::bind(&PlayField::removeRow, this, y, aBlockFromBottom), rowKillDelay);
    }
  }
}



bool PlayField::step()
{
  dirty = false;
  MLMicroSeconds now = MainLoop::now();
  for (int i=0; i<2; i++) {
    BlockRunner *b = &activeBlocks[i];
    if (b->block) {
      if (now>=b->lastStep+b->stepInterval) {
        if (b->block->move(0, b->movingUp ? 1 : -1, 0, b->movingUp)) {
          b->lastStep = now;
          dirty = true;
        }
        else {
          // could not move, means that we've collided with floor or existing pixels
          checkRows(b->movingUp);
          // remove block (but pixels will remain)
          b->block = NULL;
          // start new one
          launchRandomBlock(b->movingUp);
        }
      }
    }
  }
  return dirty;
}


void PlayField::moveBlock(int aDx, int aRot, bool aLower)
{
  BlockRunner *b = &activeBlocks[aLower ? 1 : 0];
  if (b->block) {
    if (b->block->move(aDx, 0, aRot, aLower))
      dirty = true;
  }
}


void PlayField::dropBlock(bool aLower)
{
  BlockRunner *b = &activeBlocks[aLower ? 1 : 0];
  if (b->block) {
    b->stepInterval = dropStepInterval;
  }
}

