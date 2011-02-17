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
	void mouseUp( MouseEvent event );	
	void prepareSettings( Settings *settings );
	void keyDown( KeyEvent event ) { if ( event.getChar() == 'p' ) toggleParams = !toggleParams; }
	void update();
	void draw();

 private:	
	Capture			mCapture;
	Channel		    mVideoChannel;
	Channel		    mProcessChannel;
	gl::Texture		mOutputTexture;

	// Threshold value for edge detection.
	int mEdgeThreshold;
	
	int mDetectThreshold;
	
	void edgeDetectProcess( Channel const &input, Channel &output, int threshold );

	// Parameters for calibration.
	params::InterfaceGl	mParams;
	bool toggleParams;
	
	int mMenuToggle;
	void cycleMenu() { mMenuToggle = ( mMenuToggle < 2 ) ? mMenuToggle + 1 : 0; }

	int  mNumCameras, mCurrentCamera;
	void cycleCamera();
	
	SpotImage *spotImage1;
	SpotImage *spotImage2;
	SpotImage *spotImage3;
	int mSpotSelected;
	
	bool mSpot1On;
	bool mSpot2On;
	bool mSpot3On;
	
	Vec2i mInitialMouseDown, mCurrentMouseDown, mIntialSpotImage;
};

void SpotDetectApp::cycleCamera()
{
	if ( mNumCameras < 2 )
		return;
	
	mCapture.stop();
	const vector<Capture::DeviceRef> &devices = Capture::getDevices();
	
	mCurrentCamera = ( mCurrentCamera < mNumCameras -1 ) ? mCurrentCamera + 1 : 0;
	
	mCapture = Capture( 640, 480, devices[mCurrentCamera] );
	mCapture.start();
}

void SpotDetectApp::setup()
{
	try {
		// Get list of devices.
		const vector<Capture::DeviceRef> &devices = Capture::getDevices();
		mNumCameras = devices.size();
		mCurrentCamera = 0;
		
		mCapture = Capture( 640, 480, devices[mCurrentCamera] );
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

	mProcessChannel = Channel( 640, 480 );

	// Initialize threshold value.
	mEdgeThreshold   = 30;
	mDetectThreshold = 3;
	mMenuToggle = 0;

	gl::enableAlphaBlending( false );

	
	mParams = params::InterfaceGl( "Spot Detection", Vec2i( 200, 160 ) );
	mParams.addParam( "Edge Threshold", &mEdgeThreshold, "min=0.0 max=255.0 step=1.0 keyIncr=w keyDecr=s" );
	mParams.addParam( "Detect Threshold", &mDetectThreshold, "min=0.0 max=100.0 step=1.0 keyIncr=q keyDecr=a" );
	mParams.addButton( "View Toggle", std::bind( &SpotDetectApp::cycleMenu, this ) , "keyIncr=m");
	mParams.addButton( "Camera Toggle", std::bind( &SpotDetectApp::cycleCamera, this ) , "keyIncr=c");
	mParams.hide();

	
	spotImage1 = new SpotImage( Vec2i( 100, 100 ), 40 );
	spotImage2 = new SpotImage( Vec2i( 200, 200 ), 40 );
	spotImage3 = new SpotImage( Vec2i( 300, 300 ), 40 );

	mInitialMouseDown = Vec2i::zero();
	mCurrentMouseDown = Vec2i::zero();

	toggleParams = false;
	mSpotSelected = 0;
	
	mSpot1On = false;
	mSpot2On = false;
	mSpot3On = false;
}

void SpotDetectApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
	settings->setFrameRate( 60.0f );
}

void SpotDetectApp::mouseDown( MouseEvent event )
{
	mCurrentMouseDown = mInitialMouseDown = event.getPos();
	
	if ( Vec2i(spotImage1->getCenterPt()).distance(mInitialMouseDown) < spotImage1->getRadius() )
	{
		mIntialSpotImage  = spotImage1->getCenterPt();
		mSpotSelected     = 1;
	}
	else if ( Vec2i(spotImage2->getCenterPt()).distance(mInitialMouseDown) < spotImage2->getRadius() )
	{
		mIntialSpotImage  = spotImage2->getCenterPt();
		mSpotSelected     = 2;
	}
	else if ( Vec2i(spotImage3->getCenterPt()).distance(mInitialMouseDown) < spotImage3->getRadius() )
	{
		mIntialSpotImage  = spotImage3->getCenterPt();
		mSpotSelected     = 3;
	}
}

void SpotDetectApp::mouseUp( MouseEvent event )
{
	mSpotSelected = 0;
}

void SpotDetectApp::mouseDrag( MouseEvent event )
{
	mCurrentMouseDown = event.getPos();

	Vec2i offset = mCurrentMouseDown - mInitialMouseDown;

	if ( mSpotSelected == 1 )
		spotImage1->setCenterPt( mIntialSpotImage + offset );
	else if ( mSpotSelected == 2 )
		spotImage2->setCenterPt( mIntialSpotImage + offset );
	else if ( mSpotSelected == 3 )
		spotImage3->setCenterPt( mIntialSpotImage + offset );
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
				
				if ( iterIn.getPos().distance( spotImage1->getCenterPt() ) < spotImage1->getRadius() )
					mSpot1Count++;
				if ( iterIn.getPos().distance( spotImage2->getCenterPt() ) < spotImage2->getRadius() )
					mSpot2Count++;
				if ( iterIn.getPos().distance( spotImage3->getCenterPt() ) < spotImage3->getRadius() )
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
	
	console() << mSpot1Count << endl;
	console() << pct * M_PI * spotImage1->getRadius() * spotImage1->getRadius() << endl;
	
	area = M_PI * spotImage1->getRadius() * spotImage1->getRadius(); 
	mSpot1On = ( mSpot1Count > pct * area ) ? true : false;

	area = M_PI * spotImage2->getRadius() * spotImage2->getRadius(); 
	mSpot2On = ( mSpot2Count > pct * area ) ? true : false;
	
	area = M_PI * spotImage3->getRadius() * spotImage3->getRadius(); 
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

	
	switch ( mMenuToggle ) 
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
			
		case 3:
			//gl::draw( mProcessChannel , Area( 10, 10, 170, 130 ) );
			
			gl::draw( mProcessChannel, Vec2i( 0,0 ) );
			
//			if ( mSpot1Toler > 100 ) 
//			{
//				gl::color( ColorA( 1.0f, 0.0f, 0.0f, 1.0 ) );
//				gl::drawSolidCircle( mSpot1, 50.0 );
//			}
//			
//			if ( mSpot2Toler > 100 )
//			{
//				gl::color( ColorA( 0.0f, 1.0f, 0.0f, 1.0 ) );
//				gl::drawSolidCircle( mSpot2, 50.0 );
//			}
//
//			if ( mSpot3Toler > 100 )
//			{
//				gl::color( ColorA( 0.0f, 0.0f, 1.0f, 1.0 ) );
//				gl::drawSolidCircle( mSpot3, 50.0 );
//			}
			
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
		spotImage1->setColor( Color( 1.0f, 0.0f, 0.0f ) );
	else
		spotImage1->setColor( Color( 0.0f, 1.0f, 0.0f ) );

	spotImage1->draw();

	if ( mSpot2On )
		spotImage2->setColor( Color( 1.0f, 0.0f, 0.0f ) );
	else
		spotImage2->setColor( Color( 0.0f, 1.0f, 0.0f ) );

	spotImage2->draw();

	if ( mSpot3On )
		spotImage3->setColor( Color( 1.0f, 0.0f, 0.0f ) );
	else
		spotImage3->setColor( Color( 0.0f, 1.0f, 0.0f ) );

	spotImage3->draw();
}


CINDER_APP_BASIC( SpotDetectApp, RendererGl )
