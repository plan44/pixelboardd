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

#include "viewanimator.hpp"

using namespace p44;


// MARK: ===== ViewAnimator

ViewAnimator::ViewAnimator() :
  repeating(false),
  currentStep(-1),
  animationState(as_begin)
{
}


ViewAnimator::~ViewAnimator()
{
}


void ViewAnimator::clear()
{
  stopAnimation();
  sequence.clear();
  inherited::clear();
}


void ViewAnimator::pushStep(ViewPtr aView, MLMicroSeconds aShowTime, MLMicroSeconds aFadeInTime, MLMicroSeconds aFadeOutTime)
{
  AnimationStep s;
  s.view = aView;
  s.showTime = aShowTime;
  s.fadeInTime = aFadeInTime;
  s.fadeOutTime = aFadeOutTime;
  sequence.push_back(s);
  makeDirty();
}


bool ViewAnimator::step()
{
  bool complete = inherited::step();
  if (currentStep<sequence.size()) {
    sequence[currentStep].view->step();
  }
  stepAnimation();
  return complete;
}


void ViewAnimator::stopAnimation()
{
  if (currentView) currentView->stopFading();
  animationState = as_begin;
  currentStep = -1;
}


void ViewAnimator::stepAnimation()
{
  if (currentStep<sequence.size()) {
    MLMicroSeconds now = MainLoop::now();
    MLMicroSeconds sinceLast = now-lastStateChange;
    AnimationStep as = sequence[currentStep];
    switch (animationState) {
      case as_begin:
        // initiate animation
        // - set current view
        currentView = as.view;
        if (as.fadeInTime>0) {
          currentView->setAlpha(0);
          currentView->fadeTo(255, as.fadeInTime);
        }
        animationState = as_show;
        lastStateChange = now;
        break;
      case as_show:
        if (sinceLast>as.fadeInTime+as.showTime) {
          // check fadeout
          if (as.fadeOutTime>0) {
            as.view->fadeTo(255, as.fadeOutTime);
            animationState = as_fadeout;
          }
          else {
            goto ended;
          }
          lastStateChange = now;
        }
        break;
      case as_fadeout:
        if (sinceLast<as.fadeOutTime) {
          break; // not yet
        }
      ended:
        // end of this step
        animationState = as_begin; // begins from start
        currentStep++;
        if (currentStep>=sequence.size()) {
          // all steps done
          // - call back
          if (completedCB) {
            SimpleCB cb=completedCB;
            if (!repeating) completedCB = NULL;
            cb();
          }
          // - possibly restart
          if (repeating) {
            currentStep = 0;
          }
          else {
            stopAnimation();
          }
        }
        break;
    }
  }
}


void ViewAnimator::startAnimation(bool aRepeat, SimpleCB aCompletedCB)
{
  repeating = aRepeat;
  completedCB = aCompletedCB;
  currentStep = 0;
  animationState = as_begin; // begins from start
  stepAnimation();
}



bool ViewAnimator::isDirty()
{
  if (inherited::isDirty()) return true; // dirty anyway
  return currentView ? currentView->isDirty() : false; // dirty if currently active view is dirty
}


void ViewAnimator::updated()
{
  inherited::updated();
  if (currentView) currentView->updated();
}


PixelColor ViewAnimator::contentColorAt(int aX, int aY)
{
  // default is the viewstack's background color
  if (alpha==0 || !currentView) {
    return transparent; // entire viewstack is invisible
  }
  else {
    // consult current step's view
    return currentView->colorAt(aX, aY);
  }
}
