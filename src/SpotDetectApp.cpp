/*
 *  SpotDetectApp.cpp
 *  SpotDetect
 *
 *  Created by Nien Lam on 2/14/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/Capture.h"
#include "cinder/gl/Texture.h"
#include "cinder/Text.h"

using namespace ci;
using namespace ci::app;
using namespace std;


class SpotDetectApp : public AppBasic 
{
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void prepareSettings( Settings *settings );
	void keyDown( KeyEvent event );
	void update();
	void draw();

 private:	
	Capture			mCapture;
	Channel		    mVideoChannel;
	Channel		    mProcessChannel;
	gl::Texture		mOutputTexture;

	// Threshold value for edge detection.
	int mThreshold;
	
	void edgeDetectProcess( Channel const &input, Channel &output, int threshold );
};

void SpotDetectApp::setup()
{
	try {
		// Get list of devices.
		const vector<Capture::DeviceRef> &devices = Capture::getDevices();
		mCapture = Capture( 320, 240, devices[0] );
		mCapture.start();
	}
	catch( ... ) { // failed to initialize the webcam, create a warning texture
		// if we threw in the start, we'll set the Capture to null
		mCapture.reset();
		
		TextLayout layout;
		layout.clear( Color( 0.3f, 0.3f, 0.3f ) );
		layout.setColor( Color( 1, 1, 1 ) );
		layout.setFont( Font( "Arial", 96 ) );
		layout.addCenteredLine( "No Webcam" );
		layout.addCenteredLine( "Detected" );
		mOutputTexture = gl::Texture( layout.render() );
	}

	mProcessChannel = Channel( 320, 240 );

	// Initialize threshold value.
	mThreshold = 30;
}

void SpotDetectApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1020, 240 );
	settings->setFrameRate( 60.0f );
}

void SpotDetectApp::keyDown( KeyEvent event ) 
{
    if ( event.getCode() == KeyEvent::KEY_UP  )
	{
		mThreshold = min( 255, mThreshold + 1 ); 
	}
    else if ( event.getCode() == KeyEvent::KEY_DOWN  )
	{	
		mThreshold = max( 0, mThreshold - 1 );
	}
}	

void SpotDetectApp::mouseDown( MouseEvent event )
{
}

void SpotDetectApp::edgeDetectProcess( Channel const &input, Channel &output, int threshold )
{
	Channel::ConstIter iterIn  = input.getIter();
	Channel::Iter      iterOut = output.getIter();
	
	while ( iterIn.line() && iterOut.line() ) 
	{
		while ( iterIn.pixel() && iterOut.pixel() ) 
		{
			if ( iterIn.x() == 0 || iterIn.x() == iterIn.getWidth() - 1 || 
				 iterIn.y() == 0 || iterIn.y() == iterIn.getHeight() - 1 )
			{
				iterOut.v() = 0;
				continue;
			}
			
			int edgeVal = 0;
			
			edgeVal += abs( iterIn.v() - iterIn.v(  0, -1 ) );
			edgeVal += abs( iterIn.v() - iterIn.v(  0,  1 ) );
			edgeVal += abs( iterIn.v() - iterIn.v( -1,  0 ) );
			edgeVal += abs( iterIn.v() - iterIn.v(  1,  0 ) );
			
			iterOut.v() = ( edgeVal > threshold ) ? 255 : 0;
		}
	}
	
}

void SpotDetectApp::update()
{
	if ( ! mCapture || ! mCapture.checkNewFrame() ) 
		return;
	
	mVideoChannel = Channel( mCapture.getSurface() );
	edgeDetectProcess( mVideoChannel, mProcessChannel, mThreshold );	
}

void SpotDetectApp::draw()
{
	if ( ! mVideoChannel )
		return;

	gl::clear( Color( 0, 0, 0 ) ); 

	gl::draw( mVideoChannel, Vec2i( 0,0 ) );

	gl::draw( mProcessChannel, Vec2i( 320,0 ) );
}


CINDER_APP_BASIC( SpotDetectApp, RendererGl )
