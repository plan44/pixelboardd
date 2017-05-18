//
//  Copyright (c) 2017 plan44.ch / Lukas Zeller, Zurich, Switzerland
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

#include "life.hpp"

using namespace p44;


// MARK: ===== LifePage


LifePage::LifePage(PixelPageInfoCB aInfoCallback) :
  inherited("life", aInfoCallback),
  defaultMode(0x01),
  generationInterval(777*MilliSecond),
  generationTicket(0),
  reviveTicket(0)
{
  stop();
}


void LifePage::hide()
{
  stop();
}


void LifePage::clear()
{
  MainLoop::currentMainLoop().cancelExecutionTicket(reviveTicket);
  for (int i=0; i<PAGE_NUMPIXELS; ++i) {
    cells[i] = 0;
  }
  makeDirty();
}


void LifePage::stop()
{
  MainLoop::currentMainLoop().cancelExecutionTicket(generationTicket);
}


#define MIN_SPAWN_START 3

void LifePage::show(PageMode aMode)
{
  defaultMode = aMode;
  // make ready
  int transitioning;
  do {
    createRandomCells(20,30);
    transitioning = calculateGeneration();
  } while (transitioning<17);
  // start
  nextGeneration();
}


bool LifePage::step()
{
  return true;
}


void LifePage::nextGeneration()
{
  int transitioning = calculateGeneration();
  if (transitioning==0) {
    if (reviveTicket==0) {
      // died out or became static
      MainLoop::currentMainLoop().executeTicketOnce(reviveTicket, boost::bind(&LifePage::revive, this), generationInterval*10);
    }
  }
  else if (reviveTicket) {
    MainLoop::currentMainLoop().cancelExecutionTicket(reviveTicket);
  }
  // next generation
  timeNext();
  makeDirty();
}


void LifePage::timeNext()
{
  MainLoop::currentMainLoop().executeTicketOnce(generationTicket,boost::bind(&LifePage::nextGeneration, this), generationInterval);
}


void LifePage::revive()
{
  MainLoop::currentMainLoop().cancelExecutionTicket(reviveTicket);
  // shoot in some new cells
  createRandomCells(12,42);
  // re-start
  timeNext();
  makeDirty();
}



int LifePage::cellindex(int aX, int aY, bool aWrap)
{
  if (aX<0) {
    if (!aWrap) return PAGE_NUMPIXELS; // out of range
    aX += PAGE_NUMCOLS;
  }
  else if (aX>=PAGE_NUMCOLS) {
    if (!aWrap) return PAGE_NUMPIXELS; // out of range
    aX -= PAGE_NUMCOLS;
  }
  if (aY<0) {
    if (!aWrap) return PAGE_NUMPIXELS; // out of range
    aY += PAGE_NUMROWS;
  }
  else if (aY>=PAGE_NUMROWS) {
    if (!aWrap) return PAGE_NUMPIXELS; // out of range
    aY -= PAGE_NUMROWS;
  }
  return aY*PAGE_NUMCOLS+aX;
}


int LifePage::calculateGeneration()
{
  int transitioning = 0;
  // cell age 0 : dead for longer
  // cell age 1 : killed in this cycle
  // cell age 2...n : living, with
  // - 2 = created out of void
  // - 3 = spawned
  // - 4..n aged
  // first age all living cells by 1, clean those that were killed in last cycle
  for (int i=0; i<PAGE_NUMPIXELS; ++i) {
    int age = cells[i];
    if (age==1) {
      cells[i] = 0;
    }
    else if (age==2) {
      cells[i] = 4; // skip 3
    }
    else if (age>2) {
      cells[i]++; // just age normally
    }
  }
  // apply rules
  for (int x=0; x<PAGE_NUMCOLS; ++x) {
    for (int y=0; y<PAGE_NUMROWS; y++) {
      int ci = cellindex(x, y, false);
      // calculate number of neighbours
      int nn = 0;
      for (int dx=-1; dx<2; dx+=1) {
        for (int dy=-1; dy<2; dy+=1) {
          if (dx!=0 || dy!=0) {
            // one of the 8 neighbours
            int nci = cellindex(x+dx, y+dy, true);
            if (nci<PAGE_NUMPIXELS && (cells[nci]>3 || cells[nci]==1)) {
              // is a living neighbour (exclude newborns of this cycle, include those that will die at end of this cycle
              nn++;
            }
          }
        }
      }
      // rules for living cells:
      if (cells[ci]>0) {
        // - Any live cell with fewer than two live neighbours dies,
        //   as if caused by underpopulation.
        // - Any live cell with more than three live neighbours dies, as if by overpopulation.
        if (nn<2 || nn>3) {
          cells[ci] = 1; // will die at end of this cycle (but still alive for calculation)
          transitioning++; // a dying cell is a transition
        }
        // - Any live cell with two or three live neighbours lives on to the next generation.
      }
      // rule for dead cells:
      else {
        // - Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
        if (nn==3) {
          cells[ci] = 3; // spawned
          transitioning++; // a new cell is a transition
        }
      }
    }
  }
  return transitioning;
}


void LifePage::createRandomCells(int aMinCells, int aMaxCells)
{
  int numcells = rand() % (aMaxCells-aMinCells+1);
  while (numcells-- > 0) {
    int ci = rand() % PAGE_NUMPIXELS;
    cells[ci] = 2; // created out of void
  }
  makeDirty();
}


typedef struct {
  int x;
  int y;
} PixPos;

typedef struct {
  int numpix;
  const PixPos *pixels;
} PixPattern;


static const PixPos blinker[] = { {0,-1}, {0,0}, {0,1} };
static const PixPos toad[] = { {1,0}, {2,0}, {3,0}, {0,1}, {1,1}, {2,1} };
static const PixPos beacon[] = { {0,0}, {1,0}, {0,1}, {3,2}, {2,3}, {3,3} };
static const PixPos pentadecathlon[] = {
  {-1,-5}, {0,-5}, {1,-5},
  {0,-4},
  {0,-3},
  {-1,-2}, {0,-2}, {1,-2},
  {-1,0}, {0,0}, {1,0},
  {-1,1}, {0,1}, {1,1},
  {-1,3}, {0,3}, {1,3},
  {0,4},
  {0,5},
  {-1,6}, {0,6}, {1,6}
};
static const PixPos rpentomino[] = { {0,-1}, {1,-1}, {-1,0}, {0,0}, {1,1} };
static const PixPos diehard[] = { {-3,0}, {-2,0}, {-2,1}, {2,1}, {3,-1}, {3,1}, {4,1} };
static const PixPos acorn[] = { {-3,1}, {-2,-1}, {-2,1}, {0,0}, {1,1}, {2,1}, {3,1} };
static const PixPos glider[] = { {0,-1}, {1,0}, {-1,1}, {0,1}, {1,1} };

#define PAT(x) { sizeof(x)/sizeof(PixPos), x }


static const PixPattern patterns[] = {
  PAT(blinker),
  PAT(toad),
  PAT(beacon),
  PAT(pentadecathlon),
  PAT(rpentomino),
  PAT(diehard),
  PAT(acorn),
  PAT(glider)
};
#define NUMPATTERNS (sizeof(patterns)/sizeof(PixPattern))


void LifePage::placePattern(uint16_t aPatternNo, bool aWrap, int aCenterX, int aCenterY, int aOrientation)
{
  if (aPatternNo>=NUMPATTERNS) return;
  if (aCenterX<0) aCenterX = rand() % PAGE_NUMCOLS;
  if (aCenterY<0) aCenterY = rand() % PAGE_NUMROWS;
  if (aOrientation<0) aOrientation = rand() % 4;
  for (int i=0; i<patterns[aPatternNo].numpix; i++) {
    int x,y;
    PixPos px = patterns[aPatternNo].pixels[i];
    switch (aOrientation) {
      default: x=aCenterX+px.x; y=aCenterY+px.y; break;
      case 1: x=aCenterX+px.y; y=aCenterY-px.x; break;
      case 2: x=aCenterX-px.x; y=aCenterY-px.y; break;
      case 3: x=aCenterX-px.y; y=aCenterY+px.x; break;
    }
    int ci = cellindex(x, y, aWrap);
    cells[ci] = 2; // created out of void
  }
}



PixelColor LifePage::colorAt(int aX, int aY)
{
  PixelColor pix;
  pix.a = 255;
  pix.r = 0;
  pix.g = 0;
  pix.b = 0;
  // simplest colorisation: from yellow (young) to red
  int ci = cellindex(aX, aY, false);
  if (ci>=PAGE_NUMPIXELS) return pix; // out of range
  int age = cells[ci];
  if (age<2) return pix; // dead
  else if (age==2) {
    // artificially created
    pix.b = 255;
    pix.r = 200;
    pix.g = 220;
  }
  else if (age==3) {
    // grown
    pix.b = 0;
    pix.r = 128;
    pix.g = 255;
  }
  else {
    age -= 3;
    pix.r = 255;
    if (age<8)
      pix.g = (7-age)*32;
    else
      pix.g = 0; // higher ages are just red
  }
  return pix;
}



KeyCodes LifePage::keyLedState(int aSide)
{
  if (aSide==0) {
    return keycode_outer+keycode_middleleft;
  }
  else {
    return keycode_all;
  }
}


bool LifePage::handleKey(int aSide, KeyCodes aNewPressedKeys, KeyCodes aCurrentPressed)
{
  if (aSide==0) {
    if (aNewPressedKeys & keycode_left)
      clear();
    else if (aNewPressedKeys & keycode_middleleft)
      revive();
    else if (aNewPressedKeys & keycode_middleright)
      postInfo("quit");
    else if (aNewPressedKeys & keycode_right)
      placePattern(5); // diehard
  }
  else {
    if (aNewPressedKeys & keycode_left)
      placePattern(2); // beacon
    else if (aNewPressedKeys & keycode_middleleft)
      placePattern(3, false, 5, 10, 0); // pentadecathlon centered
    else if (aNewPressedKeys & keycode_middleright)
      placePattern(7); // glider
    else if (aNewPressedKeys & keycode_right)
      placePattern(6); // acorn
  }
  timeNext();
  makeDirty();
  return true; // fully handled
}

