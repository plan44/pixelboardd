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

#include "application.hpp"

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

Block::Block(BlocksPage &aGameController, BlockType aBlockType, ColorCode aColorCode) :
  gameController(aGameController),
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
      gameController.playfield->setColorCodeAt(0, px, py);
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
      gameController.playfield->setColorCodeAt(colorCode, px, py);
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
    gameController.playfield->setColorCodeAt(colorCode | 0x10, px, py);
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
    if (!gameController.isWithinPlayfield(px, py, true, aOpenAtBottom) || gameController.playfield->colorCodeAt(px, py)!=0) {
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
  int o = (orientation + aRot) & 0x3;
  int nx = x+aDx;
  int ny = y+aDy;
  return position(nx, ny, o, aOpenAtBottom);
}



// MARK: ===== BlocksView

BlocksView::BlocksView()
{
  setContentSize(10, 20); // Tetris has fixed size of 10x20
}


void BlocksView::clear()
{
  for (int i=0; i<PAGE_NUMPIXELS; ++i) {
    colorCodes[i] = 0;
  }
  makeDirty();
}


ColorCode BlocksView::colorCodeAt(int aX, int aY)
{
  if (!isInContentSize(aX, aY)) return 0;
  return colorCodes[aY*PAGE_NUMCOLS+aX];
}


void BlocksView::setColorCodeAt(ColorCode aColorCode, int aX, int aY)
{
  if (!isInContentSize(aX, aY)) return;
  colorCodes[aY*PAGE_NUMCOLS+aX] = aColorCode;
}


PixelColor BlocksView::contentColorAt(int aX, int aY)
{
  uint8_t cc = colorCodeAt(aX, aY);
  // map to real color
  PixelColor pix;
  const ColorDef *cdef = &colorDefs[cc];
  pix.r = cdef->r;
  pix.g = cdef->g;
  pix.b = cdef->b;
  pix.a = 255;
  if (cc>=16 && cc<32) {
    pix = dimPixel(pix, 188);
  }
  return pix;
}




// MARK: ===== BlocksPage

#define BLOCKS_HELP_ANIMATION_STEP_TIME (333*MilliSecond)

BlocksPage::BlocksPage(PixelPageInfoCB aInfoCallback) :
  inherited("blocks", aInfoCallback),
  defaultMode(0x01),
  gameState(game_ready),
  gameMode(0),
  playModeAccumulator(0),
  rowKillTicket(0),
  stateChangeTicket(0),
  stepInterval(0.7*Second),
  dropStepInterval(0.05*Second),
  rowKillDelay(0.15*Second)
{
  stop();
  // game view
  playfield = BlocksViewPtr(new BlocksView);
  sizeViewToPage(playfield);
  // help view
  infoView = ViewStackPtr(new ViewStack());
  sizeViewToPage(infoView);
  infoView->setFullFrameContent();
  ImageViewPtr iv = ImageViewPtr(new ImageView);
  sizeViewToPage(iv);
  iv->loadPNG(Application::sharedApplication()->resourcePath("images/blocks.png"));
  infoView->pushView(iv);
  // additional pixels for two-sided play
  twoSidedView = ImageViewPtr(new ImageView);
  sizeViewToPage(twoSidedView);
  twoSidedView->loadPNG(Application::sharedApplication()->resourcePath("images/blocks2s.png"));
  twoSidedView->hide(); // hidden to start with
  infoView->pushView(twoSidedView);
  // - the animation
  ViewAnimatorPtr va = ViewAnimatorPtr(new ViewAnimator);
  sizeViewToPage(va);
  va->setFullFrameContent();
  // - the steps
  iv = ImageViewPtr(new ImageView);
  sizeViewToPage(iv);
  iv->loadPNG(Application::sharedApplication()->resourcePath("images/blocks1.png"));
  va->pushStep(iv, BLOCKS_HELP_ANIMATION_STEP_TIME);
  iv = ImageViewPtr(new ImageView);
  sizeViewToPage(iv);
  iv->loadPNG(Application::sharedApplication()->resourcePath("images/blocks2.png"));
  va->pushStep(iv, BLOCKS_HELP_ANIMATION_STEP_TIME);
  iv = ImageViewPtr(new ImageView);
  sizeViewToPage(iv);
  iv->loadPNG(Application::sharedApplication()->resourcePath("images/blocks3.png"));
  va->pushStep(iv, BLOCKS_HELP_ANIMATION_STEP_TIME);
  // - push animation on top
  infoView->pushView(va);
  va->startAnimation(true);
  // score text view
  scoretext = TextViewPtr(new TextView(2, 0, 20, View::down));
  scoretext->setTextColor({255, 128, 0, 255});
  // main stack
  ViewStackPtr mv = ViewStackPtr(new ViewStack());
  sizeViewToPage(mv);
  mv->setFullFrameContent();
  // - game as basis
  mv->pushView(playfield);
  // - initially visible help, transparent score
  infoView->show();
  mv->pushView(infoView);
  scoretext->hide();
  mv->pushView(scoretext);
  // install it in this page
  setView(mv);
}


void BlocksPage::hide()
{
  stop();
}


void BlocksPage::clear()
{
  stop();
  playfield->clear();
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
    ledState[bi] = keycode_none; // LEDs off
  }
  if (music) music->stop();
  if (sound) sound->stop();
}


void BlocksPage::show(PageMode aMode)
{
  defaultMode = aMode;
  // make ready
  makeReady(true);
}


void BlocksPage::makeReady(bool aNewShow)
{
  gameState = game_ready;
  infoView->show();
  playModeAccumulator = 0;
  // show start keys
  ledState[0] = keycode_middleleft; // Turn = start
  ledState[1] = keycode_middleleft; // Turn = start
  if (aNewShow) {
    // do not show last score
    scoretext->hide();
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


bool BlocksPage::enabledSide(int aSide)
{
  return (gameMode & (aSide==1 ? pagemode_controls2 : pagemode_controls1)) != 0;
}


void BlocksPage::startGame(PageMode aMode)
{
  gameMode = aMode;
  MainLoop::currentMainLoop().cancelExecutionTicket(stateChangeTicket);
  stop();
  clear();
  if (music) music->play(Application::sharedApplication()->resourcePath("sounds/tetris.mod"));
  level = 0;
  score[0] = 0;
  score[1] = 0;
  ledState[0] = keycode_none;
  ledState[1] = keycode_none;
  gameState = game_running;
  playfield->show();
  scoretext->hide();
  infoView->hide();
  twoSidedView->setVisible((gameMode&pagemode_controls_mask)==pagemode_controls_mask); // show for next game if this game was two-sided
  if ((gameMode & pagemode_controls1) || gameMode==0) {
    // normal side
    launchRandomBlock(false);
    ledState[0] = keycode_all;
  }
  if (gameMode & pagemode_controls2) {
    // other side
    launchRandomBlock(true);
    ledState[1] = keycode_all;
  }
}


void BlocksPage::pause()
{
  gameState = game_paused;
  playfield->setAlpha(200); // dimmed
  if (enabledSide(0)) ledState[0] = keycode_middleleft;
  if (enabledSide(1)) ledState[1] = keycode_middleleft;
}


void BlocksPage::resume()
{
  gameState = game_running;
  playfield->show(); // normal
  if (enabledSide(0)) ledState[0] = keycode_all;
  if (enabledSide(1)) ledState[1] = keycode_all;
}



void BlocksPage::gameOver()
{
  if (sound) sound->play(Application::sharedApplication()->resourcePath("sounds/gameover.wav"));
  gameState = game_over;
  playfield->setAlpha(128); // dim board a lot
  MainLoop::currentMainLoop().executeTicketOnce(stateChangeTicket,boost::bind(&BlocksPage::makeReady, this, false), 5*Second);
  ledState[0] = keycode_none; // LEDs off
  ledState[1] = keycode_none; // LEDs off
  scoretext->setText(string_format("%d", score[0]+score[1]), true);
  scoretext->show(); // show
  makeDirty();
}


bool BlocksPage::isWithinPlayfield(int aX, int aY, bool aOpen, bool aOpenAtBottom)
{
  if (aX<0 || aX>=PAGE_NUMCOLS) return false; // may not extend sidewards
  if ((aOpenAtBottom || !aOpen) && aY>=PAGE_NUMROWS) return false; // may not extend above playfield
  if ((!aOpenAtBottom || !aOpen) && aY<0) return false; // may not extend below playfield
  return true; // within
}


KeyCodes BlocksPage::keyLedState(int aSide)
{
  return ledState[aSide];
}


bool BlocksPage::handleKey(int aSide, KeyCodes aNewPressedKeys, KeyCodes aCurrentPressed)
{
  if (gameState==game_paused && enabledSide(aSide)) {
    if (aNewPressedKeys & keycode_middleleft) {
      resume();
    }
    if (aNewPressedKeys & keycode_middleright) {
      postInfo("quit");
    }
  }
  else if (gameState==game_ready) {
    // turn starts (after 2 seconds timeout), others exit game
    if (aNewPressedKeys & keycode_middleleft) {
      PageMode newMode = aSide==1 ? pagemode_controls2 : pagemode_controls1;
      if (playModeAccumulator!=0) {
        // at least one player has already joined
        if ((playModeAccumulator & newMode)==0) {
          // second side joined, show their help
          twoSidedView->show();
        }
      }
      else {
        // first player, turn help for her if it is on the far side
        infoView->setOrientation(aSide==1 ? View::left : View::right);
      }
      // add to acculumator
      playModeAccumulator |= newMode;
      ledState[aSide==1 ? 1 : 0] = keycode_all; // immediate feedback: all 4 keys on
      // but start with a little delay so other player can also join
      MainLoop::currentMainLoop().executeTicketOnce(stateChangeTicket,boost::bind(&BlocksPage::startAccTimeout, this), 3*Second);
    }
    else if (aNewPressedKeys) {
      postInfo("quit");
    }
  }
  else if (gameState==game_running && enabledSide(aSide)) {
    int movement = 0;
    int rotation = 0;
    bool drop = false;
    // check special multi-key
    if (
      (aNewPressedKeys & keycode_outer) &&
      (aCurrentPressed & keycode_outer)==keycode_outer
    ) {
      pause();
      return true;
    }
    // prioritize moves over turn over drop
    if (aNewPressedKeys & keycode_left) {
      movement = -1;
    }
    else if (aNewPressedKeys & keycode_right) {
      movement = 1;
    }
    else if (aNewPressedKeys & keycode_middleleft) {
      rotation = -1;
    }
    else if (aNewPressedKeys & keycode_middleright) {
      drop = true;
      if (sound) sound->play(Application::sharedApplication()->resourcePath("sounds/drop.wav"));
    }
    // movement X is right edge, so need to swap for bottom end keys
    if (aSide==1) movement = -movement;
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
      playfield->setColorCodeAt(playfield->colorCodeAt(x, y+dir), x, y);
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
      if (playfield->colorCodeAt(x, y)==0) {
        hasGap = true;
        break;
      }
    }
    if (!hasGap) {
      // full row found
      for (int x=0; x<PAGE_NUMCOLS; x++) {
        // light up
        playfield->setColorCodeAt(32, x, y); // row flash
      }
      if (sound) sound->play(Application::sharedApplication()->resourcePath()+string_format("/sounds/%dline.wav", aRemovedRows+1));
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
  return inherited::step(); // let baseclass step (view etc.)
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

