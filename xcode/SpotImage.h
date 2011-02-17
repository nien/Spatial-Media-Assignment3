/*
 *  SpotImage.h
 *  SpotDetect
 *
 *  Created by Nien Lam on 2/16/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */
#pragma once

#include "cinder/Vector.h"
#include "cinder/Color.h"


class SpotImage
{
  public:
	SpotImage( ci::Vec2i centerPt, int radius ) : mCenterPt( centerPt ), mRadius( radius ) {}
	void      setCenterPt( ci::Vec2i centerPt );
	ci::Vec2i getCenterPt() { return mCenterPt; }
	void      setColor( ci::Color color ) { mColor = color; }
	int       getRadius() { return mRadius; }	
	void      draw();
	
  private:	
	ci::Vec2i  mCenterPt;
	int        mRadius;
	ci::Color  mColor;
};

