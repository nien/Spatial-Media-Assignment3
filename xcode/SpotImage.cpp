/*
 *  SpotImage.cpp
 *  SpotDetect
 *
 *  Created by Nien Lam on 2/16/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */


#include "SpotImage.h"
#include "cinder/gl/gl.h"

using namespace ci;


void SpotImage::setCenterPt( ci::Vec2i centerPt ) 
{
	mCenterPt = centerPt;
}

void SpotImage::draw()
{
	gl::color( mColor );
	gl::drawSolidCircle( mCenterPt, mRadius );
}

