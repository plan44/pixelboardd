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
  generationTicket(0)
{
  stop();
}


void LifePage::hide()
{
  stop();
}


void LifePage::clear()
{
  for (int i=0; i<PAGE_NUMPIXELS; ++i) {
    cells[i] = 0;
  }
  makeDirty();
}


void LifePage::stop()
{
  MainLoop::currentMainLoop().cancelExecutionTicket(generationTicket);
}



void LifePage::show(PageMode aMode)
{
  defaultMode = aMode;
  // make ready
  nextGeneration();
}


bool LifePage::step()
{
  return true;
}


#define MIN_CELLS 7
#define MAX_CELLS 40

void LifePage::nextGeneration()
{
  int numCells = calculateGeneration();
  if (numCells<MIN_CELLS) {
    createRandomCells(MIN_CELLS-numCells, MAX_CELLS-numCells);
  }
  makeDirty();
  MainLoop::currentMainLoop().executeTicketOnce(generationTicket,boost::bind(&LifePage::nextGeneration, this), generationInterval);
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
  int n = 0;
  // cell age 0 : dead for longer
  // cell age 1 : killed in this cycle
  // cell age 2...n : living, with
  // - 2 = artificially newborn
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
        if (nn<2 || nn>3) cells[ci] = 1; // will die at end of this cycle (but still alive for calculation)
        // - Any live cell with two or three live neighbours lives on to the next generation.
      }
      // rule for dead cells:
      else {
        // - Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
        if (nn==3) cells[ci] = 3; // spawned
      }
      // count now living
      if (cells[ci]>1) n++;
    }
  }
  return n;
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



uint8_t LifePage::keyLedState(int aSide)
{
  return 0x09;
}


bool LifePage::handleKey(int aSide, int aKeyNum)
{
  switch (aKeyNum) {
    case 0 : clear(); break; // left
    case 1 : createRandomCells(10, 20); break; // right
    case 2 : postInfo("quit"); break; // drop
    case 3 : createRandomCells(1, 5); break; // right
  }
  return true; // fully handled
}




void LifePage::createRandomCells(int aMinCells, int aMaxCells)
{
  int numcells = rand() % (aMaxCells-aMinCells+1);
  while (numcells-- > 0) {
    int ci = rand() % PAGE_NUMPIXELS;
    cells[ci] = 2; // newborn
  }
  MainLoop::currentMainLoop().rescheduleExecutionTicket(generationTicket, generationInterval);
  makeDirty();
}


