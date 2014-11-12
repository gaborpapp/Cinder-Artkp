/*
 Copyright (C) 2013 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/params/Params.h"
#include "cinder/Area.h"
#include "cinder/Capture.h"
#include "cinder/Cinder.h"

#include "ArTracker.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ArtkpApp : public AppBasic
{
	public:
		void setup();
		void setupCapture();

		void update();
		void draw();

		void shutdown();

	private:
		// capture
		ci::Capture mCapture;
		gl::Texture mCaptTexture;

		vector< ci::Capture > mCaptures;

		static const int CAPTURE_WIDTH = 640;
		static const int CAPTURE_HEIGHT = 480;

		int32_t mCurrentCapture;

		// ar
		mndl::artkp::ArTracker mArTracker;

		// params
		params::InterfaceGl mParams;

		float mFps;
};

void ArtkpApp::setup()
{
	gl::disableVerticalSync();

	mParams = params::InterfaceGl( "Parameters", Vec2i( 300, 300 ) );
	setupCapture();

	mParams.addParam( "Fps", &mFps, "", true );

	mndl::artkp::ArTracker::Options options;
	options.setCameraFile( getAssetPath( "camera_para.dat" ) );
	mArTracker = mndl::artkp::ArTracker( CAPTURE_WIDTH, CAPTURE_HEIGHT, options );
}

void ArtkpApp::setupCapture()
{
	// list out the capture devices
	vector< Capture::DeviceRef > devices( Capture::getDevices() );
	vector< string > deviceNames;

	for ( vector< Capture::DeviceRef >::const_iterator deviceIt = devices.begin();
			deviceIt != devices.end(); ++deviceIt )
	{
		Capture::DeviceRef device = *deviceIt;
		string deviceName = device->getName();

		try
		{
			if ( device->checkAvailable() )
			{
				mCaptures.push_back( Capture( CAPTURE_WIDTH, CAPTURE_HEIGHT,
							device ) );
				deviceNames.push_back( deviceName );
			}
			else
			{
				mCaptures.push_back( Capture() );
				deviceNames.push_back( deviceName + " not available" );
			}
		}
		catch ( CaptureExc & )
		{
			console() << "Unable to initialize device: " << device->getName() <<
				endl;
		}
	}

	if ( deviceNames.empty() )
	{
		deviceNames.push_back( "Camera not available" );
		mCaptures.push_back( Capture() );
	}

	mCurrentCapture = 0;
	mParams.addParam( "Capture", deviceNames, &mCurrentCapture );
}

void ArtkpApp::update()
{
	static int lastCapture = -1;

	mFps = getAverageFps();

	// switch between capture devices
	if ( lastCapture != mCurrentCapture )
	{
		if ( ( lastCapture >= 0 ) && ( mCaptures[ lastCapture ] ) )
			mCaptures[ lastCapture ].stop();

		if ( mCaptures[ mCurrentCapture ] )
			mCaptures[ mCurrentCapture ].start();

		mCapture = mCaptures[ mCurrentCapture ];
		lastCapture = mCurrentCapture;
	}

	// detect the markers
	if ( mCapture && mCapture.checkNewFrame() )
	{
		Surface8u captSurf( mCapture.getSurface() );
		mCaptTexture = gl::Texture( captSurf );
		mArTracker.update( captSurf );
	}
}

void ArtkpApp::draw()
{
	gl::clear( Color::black() );

	gl::disableDepthRead();
	gl::disableDepthWrite();

	// draws camera image
	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	gl::color( Color::white() );
	Area outputArea = Area::proportionalFit( Area( 0, 0, CAPTURE_WIDTH, CAPTURE_HEIGHT ), getWindowBounds(), true, true );
	if ( mCaptTexture )
		gl::draw( mCaptTexture, outputArea );

	// places a cube on each detected marker
	gl::enableDepthRead();
	gl::enableDepthWrite();

	gl::setViewport( outputArea );
	// sets the projection matrix
	mArTracker.setProjection();

	// scales the pattern according th the pattern width
	Vec3d patternScale = Vec3d::one() * mArTracker.getOptions().getPatternWidth();
	for ( int i = 0; i < mArTracker.getNumMarkers(); i++ )
	{
		// id -1 means unknown marker, false positive
		if ( mArTracker.getMarkerId( i ) == -1 )
			continue;

		// sets the modelview matrix of the i'th marker
		mArTracker.setModelView( i );
		gl::scale( patternScale );
		gl::translate( Vec3f( 0.f, .0f, .5f ) ); // places the cube on the marker instead of the center
		gl::drawColorCube( Vec3f::zero(), Vec3f::one() );
	}

	mParams.draw();
}

void ArtkpApp::shutdown()
{
	if ( mCapture )
	{
		mCapture.stop();
	}
}


CINDER_APP_BASIC( ArtkpApp, RendererGl )

