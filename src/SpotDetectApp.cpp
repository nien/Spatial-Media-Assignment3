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
#include "cinder/params/Params.h"
#include "SpotImage.h"

using namespace ci;
using namespace ci::app;
using namespace std;


class SpotDetectApp : public AppBasic 
{
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void mouseDrag( MouseEvent event );
	void prepareSettings( Settings *settings );
	void keyDown( KeyEvent event ) { if ( event.getChar() == 'p' ) toggleParams = !toggleParams; }
	void update();
	void draw();

 private:	
	Capture			mCapture;
	Channel		    mVideoChannel;
	Channel		    mProcessChannel;
	gl::Texture		mOutputTexture;

	// Threshold values.
	int mEdgeThreshold;
	int mDetectThreshold;
	
	// Edge detection processing.
	void edgeDetectProcess( Channel const &input, Channel &output, int threshold );

	// Parameters for calibration.
	params::InterfaceGl	mParams;
	bool toggleParams;
	
	// Support different views.
	int  mViewToggle;
	void cycleViews() { mViewToggle = ( mViewToggle < 2 ) ? mViewToggle + 1 : 0; }

	// Support multiple cameras.
	int  mCurrentCamera;
	void cycleCameras();
	
	// Spot images and flags required for updating position and radius.
	SpotImage *mSpotImage1, *mSpotImage2, *mSpotImage3;
	int       mSpotSelected;
	int       mInitialSpotRadius;
	Vec2i     mInitialMouseDown, mCurrentMouseDown, mIntialSpotPt;
	bool      mSpot1On, mSpot2On, mSpot3On;
};

void SpotDetectApp::cycleCameras()
{
	const vector<Capture::DeviceRef> &devices = Capture::getDevices();
	int   mNumCameras = devices.size();
	
	if ( mNumCameras < 2 )
		return;
	
	mCapture.stop();
	mCurrentCamera = ( mCurrentCamera < mNumCameras -1 ) ? mCurrentCamera + 1 : 0;
	mCapture       = Capture( 640, 480, devices[mCurrentCamera] );
	mCapture.start();
}

void SpotDetectApp::setup()
{
	try {
		const vector<Capture::DeviceRef> &devices = Capture::getDevices();
		mCurrentCamera = 0;
		mCapture       = Capture( 640, 480, devices[mCurrentCamera] );
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

	// Create process channel.
	mProcessChannel = Channel( 640, 480 );

	// Initialize threshold values.
	mEdgeThreshold   = 30;
	mDetectThreshold = 2;
	
	// Setup params widget and flags.
	mParams = params::InterfaceGl( "Spot Detection", Vec2i( 200, 160 ) );
	mParams.addParam(  "Edge Threshold",   &mEdgeThreshold, "min=0.0 max=255.0 step=1.0 keyIncr=w keyDecr=s" );
	mParams.addParam(  "Detect Threshold", &mDetectThreshold, "min=0.0 max=100.0 step=1.0 keyIncr=q keyDecr=a" );
	mParams.addButton( "View Toggle",      std::bind( &SpotDetectApp::cycleViews, this ) , "keyIncr=v");
	mParams.addButton( "Camera Toggle",    std::bind( &SpotDetectApp::cycleCameras, this ) , "keyIncr=c");
	mParams.hide();
	toggleParams  = false;
	mViewToggle   = 0;

	// Create spots for detection.
	// TODO: Put SpotImages into an array;
	mSpotImage1   = new SpotImage( Vec2i( 100, 100 ), 40 );
	mSpotImage2   = new SpotImage( Vec2i( 200, 200 ), 40 );
	mSpotImage3   = new SpotImage( Vec2i( 300, 300 ), 40 );
	mSpotSelected = 0;
	mSpot1On      = false;
	mSpot2On      = false;
	mSpot3On      = false;

	// Support alpha channel setting.
	gl::enableAlphaBlending( false );
}

void SpotDetectApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
	settings->setFrameRate( 60.0f );
}

void SpotDetectApp::mouseDown( MouseEvent event )
{
	mCurrentMouseDown = mInitialMouseDown = event.getPos();
	mSpotSelected     = 0;
	
	// TODO: Clean up code. Put SpotImages in array.	
	if ( Vec2i(mSpotImage1->getCenterPt()).distance(mInitialMouseDown) < mSpotImage1->getRadius() )
	{
		mInitialSpotRadius = mSpotImage1->getRadius(); 
		mIntialSpotPt     = mSpotImage1->getCenterPt();
		mSpotSelected     = 1;
	}
	else if ( Vec2i(mSpotImage2->getCenterPt()).distance(mInitialMouseDown) < mSpotImage2->getRadius() )
	{
		mInitialSpotRadius = mSpotImage2->getRadius(); 
		mIntialSpotPt     = mSpotImage2->getCenterPt();
		mSpotSelected     = 2;
	}
	else if ( Vec2i(mSpotImage3->getCenterPt()).distance(mInitialMouseDown) < mSpotImage3->getRadius() )
	{
		mInitialSpotRadius = mSpotImage3->getRadius(); 
		mIntialSpotPt     = mSpotImage3->getCenterPt();
		mSpotSelected     = 3;
	}
}

void SpotDetectApp::mouseDrag( MouseEvent event )
{
	mCurrentMouseDown = event.getPos();

	// TODO: Clean up logic.
	Vec2i posOffset = mCurrentMouseDown - mInitialMouseDown;
	if ( mSpotSelected == 1 ) 
	{
		if ( event.isShiftDown() ) 
		{
			int radiusOffset = mSpotImage1->getCenterPt().distance(mCurrentMouseDown) - 
				               mSpotImage1->getCenterPt().distance(mInitialMouseDown);
			mSpotImage1->setRadius( mInitialSpotRadius + radiusOffset );
		}
		else 
		{
			mSpotImage1->setCenterPt( mIntialSpotPt + posOffset );
		}
	}
	else if ( mSpotSelected == 2 )
	{
		if ( event.isShiftDown() ) 
		{
			int radiusOffset = mSpotImage2->getCenterPt().distance(mCurrentMouseDown) - 
			mSpotImage2->getCenterPt().distance(mInitialMouseDown);
			mSpotImage2->setRadius( mInitialSpotRadius + radiusOffset );
		}
		else 
		{
			mSpotImage2->setCenterPt( mIntialSpotPt + posOffset );
		}
	}
	else if ( mSpotSelected == 3 )
	{
		if ( event.isShiftDown() ) 
		{
			int radiusOffset = mSpotImage3->getCenterPt().distance(mCurrentMouseDown) - 
			mSpotImage3->getCenterPt().distance(mInitialMouseDown);
			mSpotImage3->setRadius( mInitialSpotRadius + radiusOffset );
		}
		else 
		{
			mSpotImage3->setCenterPt( mIntialSpotPt + posOffset );
		}
	}
}

void SpotDetectApp::edgeDetectProcess( Channel const &input, Channel &output, int threshold )
{
	Channel::ConstIter iterIn  = input.getIter();
	Channel::Iter      iterOut = output.getIter();
	
	int mSpot1Count = 0;
	int mSpot2Count = 0;
	int mSpot3Count = 0;
	
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
			
			if ( edgeVal > threshold ) 
			{
				iterOut.v() = 255; 
				
				if ( iterIn.getPos().distance( mSpotImage1->getCenterPt() ) < mSpotImage1->getRadius() )
					mSpot1Count++;
				if ( iterIn.getPos().distance( mSpotImage2->getCenterPt() ) < mSpotImage2->getRadius() )
					mSpot2Count++;
				if ( iterIn.getPos().distance( mSpotImage3->getCenterPt() ) < mSpotImage3->getRadius() )
					mSpot3Count++;
			}
			else
			{
				iterOut.v() = 0; 
			}
		}
	}

	float pct =  mDetectThreshold / 100.0f;
	float area;
	
	area = M_PI * mSpotImage1->getRadius() * mSpotImage1->getRadius(); 
	mSpot1On = ( mSpot1Count > pct * area ) ? true : false;

	area = M_PI * mSpotImage2->getRadius() * mSpotImage2->getRadius(); 
	mSpot2On = ( mSpot2Count > pct * area ) ? true : false;
	
	area = M_PI * mSpotImage3->getRadius() * mSpotImage3->getRadius(); 
	mSpot3On = ( mSpot3Count > pct * area ) ? true : false;
}

void SpotDetectApp::update()
{
	if ( ! mCapture || ! mCapture.checkNewFrame() ) 
		return;
	
	mVideoChannel = Channel( mCapture.getSurface() );
	edgeDetectProcess( mVideoChannel, mProcessChannel, mEdgeThreshold );	
}

void SpotDetectApp::draw()
{
	if ( ! mVideoChannel )
		return;

	gl::clear( Color( 0, 0, 0 ) );

	gl::color( Color( 1.0f, 1.0f, 1.0f ) );

	
	switch ( mViewToggle ) 
	{
		case 0:
			gl::draw( mVideoChannel, Vec2i( 0,0 ) );
			break;
			
		case 1:
			gl::draw( mProcessChannel, Vec2i( 0,0 ) );
			break;
			
		case 2:
			gl::draw( mProcessChannel, Vec2i( 0,0 ) );
			gl::draw( mVideoChannel, Area( 10, 10, 170, 130 ) );
			break;
			
		default:
			console() << "Unsupported Draw State" << endl;
			exit( 1 );
	}

	if ( toggleParams ) 
	{
		params::InterfaceGl::draw();
		mParams.show();
	}
	else
	{
		mParams.hide();
	}

	if ( mSpot1On )
		mSpotImage1->setColor( Color( 1.0f, 0.0f, 0.0f ) );
	else
		mSpotImage1->setColor( Color( 0.0f, 1.0f, 0.0f ) );

	mSpotImage1->draw();

	if ( mSpot2On )
		mSpotImage2->setColor( Color( 1.0f, 0.0f, 0.0f ) );
	else
		mSpotImage2->setColor( Color( 0.0f, 1.0f, 0.0f ) );

	mSpotImage2->draw();

	if ( mSpot3On )
		mSpotImage3->setColor( Color( 1.0f, 0.0f, 0.0f ) );
	else
		mSpotImage3->setColor( Color( 0.0f, 1.0f, 0.0f ) );

	mSpotImage3->draw();
}


CINDER_APP_BASIC( SpotDetectApp, RendererGl )
