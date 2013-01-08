/*
 Copyright (C) 2013 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published
 by the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "cinder/gl/gl.h"

#include "ArTracker.h"

#include "ARToolKitPlus/TrackerSingleMarkerImpl.h"

using namespace ci;

namespace mndl { namespace artkp {

ArTracker::ArTracker( int32_t width, int32_t height, Options options ) :
	mObj( new Obj( width, height, options ) )
{}

void ArTracker::update( Surface &surface )
{
	mObj->mTrackerRef->calc( surface.getData(), -1, false, &mObj->mMarkerInfo, &mObj->mNumMarkers );
	// TODO filter out -1 ids from mMarkerInfo
}

int ArTracker::getNumMarkers() const
{
	return mObj->mNumMarkers;
}

int ArTracker::getMarkerId( int i ) const
{
	if ( ( i >= 0 ) && ( i < mObj->mNumMarkers ) )
		return mObj->mMarkerInfo[ i ].id;
	else
		throw ArTrackerExcMarkerIndexOutOfRange();
}

float ArTracker::getMarkerConfidence( int i ) const
{
	if ( ( i >= 0 ) && ( i < mObj->mNumMarkers ) )
		return mObj->mMarkerInfo[ i ].cf;
	else
		throw ArTrackerExcMarkerIndexOutOfRange();
}

void ArTracker::setProjection() const
{
	const ARFloat *proj = mObj->mTrackerRef->getProjectionMatrix();

	glMatrixMode( GL_PROJECTION );
#ifdef _USE_DOUBLE_ // ArtToolKit defines ARFloat precision
	glLoadMatrixd( proj );
#else
	glLoadMatrixf( proj );
#endif
}

ci::Matrix44f ArTracker::getModelView( int i ) const
{
	if ( ( i < 0 ) || ( i >= mObj->mNumMarkers ) )
		throw ArTrackerExcMarkerIndexOutOfRange();

	Matrix44f modelView;
	float glModelView[ 16 ];
	ARFloat patternWidth = mObj->mOptions.mPatternWidth;
	ARFloat patternCenter[ 2 ] = { 0.f, 0.f };
	ARFloat patternTrans[ 3 ][ 4 ];
	mObj->mTrackerRef->executeSingleMarkerPoseEstimator( &mObj->mMarkerInfo[ i ], patternCenter, patternWidth, patternTrans );

	for ( int j = 0; j < 3; j++ )
	{
		for( int i = 0; i < 4; i++ )
		{
			glModelView[ i * 4 + j ] = patternTrans[ j ][ i ];
		}
	}

	return Matrix44f( patternTrans[ 0 ][ 0 ], patternTrans[ 1 ][ 0 ], patternTrans[ 2 ][ 0 ], 0.f,
					patternTrans[ 0 ][ 1 ], patternTrans[ 1 ][ 1 ], patternTrans[ 2 ][ 1 ], 0.f,
					patternTrans[ 0 ][ 2 ], patternTrans[ 1 ][ 2 ], patternTrans[ 2 ][ 2 ], 0.f,
					patternTrans[ 0 ][ 3 ], patternTrans[ 1 ][ 3 ], patternTrans[ 2 ][ 3], 1.f );
}

void ArTracker::setModelView( int i ) const
{
	if ( ( i < 0 ) || ( i >= mObj->mNumMarkers ) )
		throw ArTrackerExcMarkerIndexOutOfRange();

	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixf( getModelView( i ).m );
}

ArTracker::Obj::Obj( int32_t width, int32_t height, Options options )
{
	/* - template based markers need 16x16 samples
	 * - id based ones need 6, 12 or 18, TODO: in options
	 * create a tracker that does:
	 *  - size x size sized marker images
	 *  - samples at a maximum of size x size
	 *  - works with rgb images
	 *  - can load a maximum of 32 patterns, TODO: in options
	 *  - can detect a maximum of 32 patterns in one image
	 */
	if ( options.mMode == MARKER_TEMPLATE )
		mTrackerRef = std::shared_ptr< ARToolKitPlus::TrackerSingleMarker >(
				new ARToolKitPlus::TrackerSingleMarkerImpl< 16, 16, 16, 32, 32 >( width, height ) );
	else
		mTrackerRef = std::shared_ptr< ARToolKitPlus::TrackerSingleMarker >(
				new ARToolKitPlus::TrackerSingleMarkerImpl< 12, 12, 12, 32, 32 >( width, height ) );

	if ( options.mLoggingEnabled )
		mTrackerRef->setLogger( &mLogger );

	// TODO set this in options
	mTrackerRef->setPixelFormat( ARToolKitPlus::PIXEL_FORMAT_RGB );

	// NOTE camera parameter file is required, otherwise the image size is not set
	// TODO check how this relates to width & height
	if ( options.mCameraParamFile.empty() )
	{
		throw ArTrackerExcCameraFileNotFound();
	}

	if ( !mTrackerRef->init( options.mCameraParamFile.string().c_str(),
				options.mNearPlane, options.mFarPlane ) )
	{
		throw ArTrackerExcInitFail();
	}

	mTrackerRef->setPatternWidth( options.mPatternWidth );
	if ( options.mMode == MARKER_ID_BCH )
		mTrackerRef->setBorderWidth( 0.125f );
	else
		mTrackerRef->setBorderWidth( 0.250f );
	mTrackerRef->setUndistortionMode( ARToolKitPlus::UNDIST_STD );

	mTrackerRef->activateAutoThreshold( options.mThresholdAuto );

	// RPP is more robust than ARToolKit's standard pose estimator
	mTrackerRef->setPoseEstimator( ARToolKitPlus::POSE_ESTIMATOR_RPP );

	mTrackerRef->setMarkerMode( static_cast< ARToolKitPlus::MARKER_MODE >( options.mMode ) );

	mOptions = options;
}

ArTracker::Obj::~Obj()
{
	if ( mTrackerRef )
		mTrackerRef->cleanup();
}

} } // namespace mndl::artkp

