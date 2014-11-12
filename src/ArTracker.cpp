/*
 Copyright (C) 2013 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "cinder/gl/gl.h"

#include "ArTracker.h"

#include "ARToolKitPlus/TrackerSingleMarkerImpl.h"
#include "ARToolKitPlus/TrackerMultiMarkerImpl.h"

using namespace ci;

namespace mndl { namespace artkp {

ArTracker::ArTracker( int32_t width, int32_t height, Options options ) :
	mObj( new Obj( width, height, options ) )
{}

void ArTracker::update( Surface &surface )
{
	if ( !mObj->mOptions.mMultiMarker )
	{
		// TODO filter out -1 ids from mMarkerInfo
		mObj->mTrackerSingleRef->calc( surface.getData(), -1, false, &mObj->mMarkerInfo, &mObj->mNumMarkers );
	}
	else
	{
		mObj->mTrackerMultiRef->calc( surface.getData() );
		mObj->mNumMarkers = mObj->mTrackerMultiRef->getNumDetectedMarkers();
	}
}

int ArTracker::getNumMarkers() const
{
	return mObj->mNumMarkers;
}

int ArTracker::getMarkerId( int i ) const
{
	if ( ( i >= 0 ) && ( i < mObj->mNumMarkers ) )
	{
		if ( !mObj->mOptions.mMultiMarker )
		{
			return mObj->mMarkerInfo[ i ].id;
		}
		else
		{
			return mObj->mTrackerMultiRef->getDetectedMarker( i ).id;
		}
	}
	else
	{
		throw ArTrackerExcMarkerIndexOutOfRange();
	}
}

float ArTracker::getMarkerConfidence( int i ) const
{
	if ( ( i >= 0 ) && ( i < mObj->mNumMarkers ) )
	{
		if ( !mObj->mOptions.mMultiMarker )
		{
			return mObj->mMarkerInfo[ i ].cf;
		}
		else
		{
			return mObj->mTrackerMultiRef->getDetectedMarker( i ).cf;
		}
	}
	else
	{
		throw ArTrackerExcMarkerIndexOutOfRange();
	}
}

void ArTracker::setProjection() const
{
	const ARFloat *proj;

	if ( !mObj->mOptions.mMultiMarker )
		proj = mObj->mTrackerSingleRef->getProjectionMatrix();
	else
		proj = mObj->mTrackerMultiRef->getProjectionMatrix();

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
	if ( !mObj->mOptions.mMultiMarker )
	{
		// TODO: check the return value
		// FIXME: calc has done this already, hasn't it? check where the transformation is stored
		mObj->mTrackerSingleRef->executeSingleMarkerPoseEstimator( &mObj->mMarkerInfo[ i ], patternCenter, patternWidth, patternTrans );
	}
	else
	{
		const ARToolKitPlus::ARMultiMarkerInfoT *mmConfig = mObj->mTrackerMultiRef->getMultiMarkerConfig();

#if 0
		ARToolKitPlus::ARMarkerInfo markers[ mObj->mNumMarkers ];
		for ( int i = 0; i < mObj->mNumMarkers; i++ )
		{
			markers[ i ] = mObj->mTrackerMultiRef->getDetectedMarker( i );
		}

		// TODO check the return value
		// FIXME: calc has done this already, hasn't it?
		mObj->mTrackerMultiRef->executeMultiMarkerPoseEstimator( markers, mObj->mNumMarkers, &mmConfig );
#endif

		memcpy( patternTrans, mmConfig->trans, sizeof( ARFloat ) * 12 );
	}

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

void ArTracker::enableAutoThreshold( bool enable )
{
	if ( !mObj->mOptions.mMultiMarker )
		mObj->mTrackerSingleRef->activateAutoThreshold( enable );
	else
		mObj->mTrackerMultiRef->activateAutoThreshold( enable );
}

bool ArTracker::isAutoThresholdEnabled() const
{
	if ( !mObj->mOptions.mMultiMarker )
		return mObj->mTrackerSingleRef->isAutoThresholdActivated();
	else
		return mObj->mTrackerMultiRef->isAutoThresholdActivated();
}

void ArTracker::setThreshold( int thres )
{
	if ( !mObj->mOptions.mMultiMarker )
		mObj->mTrackerSingleRef->setThreshold( thres );
	else
		mObj->mTrackerMultiRef->setThreshold( thres );
}

int ArTracker::getThreshold() const
{
	if ( !mObj->mOptions.mMultiMarker )
		return mObj->mTrackerSingleRef->getThreshold();
	else
		return mObj->mTrackerMultiRef->getThreshold();
}

ArTracker::Obj::Obj( int32_t width, int32_t height, Options options )
{
	// FIXME: try to use one tracker only if multi works with single markers

	// NOTE camera parameter file is required, otherwise the image size is not set
	// TODO check how this relates to width & height
	if ( options.mCameraParamFile.empty() )
	{
		throw ArTrackerExcCameraFileNotFound();
	}

	/* - template based markers need 16x16 samples
	 * - id based ones need 6, 12 or 18, TODO: in options
	 * create a tracker that does:
	 *  - size x size sized marker images
	 *  - samples at a maximum of size x size
	 *  - works with rgb images
	 *  - can load a maximum of 32 patterns, TODO: in options
	 *  - can detect a maximum of 32 patterns in one image
	 */
	if ( options.mMultiMarker )
	{
		if ( options.mMode == MARKER_TEMPLATE )
			mTrackerMultiRef = std::shared_ptr< ARToolKitPlus::TrackerMultiMarker >(
					new ARToolKitPlus::TrackerMultiMarkerImpl< 16, 16, 16, 32, 32 >( width, height ) );
		else
			mTrackerMultiRef = std::shared_ptr< ARToolKitPlus::TrackerMultiMarker >(
					new ARToolKitPlus::TrackerMultiMarkerImpl< 12, 12, 48, 32, 32 >( width, height ) );

		if ( options.mLoggingEnabled )
			mTrackerMultiRef->setLogger( &mLogger );

		// TODO set this in options
		mTrackerMultiRef->setPixelFormat( ARToolKitPlus::PIXEL_FORMAT_RGB );

		if ( !mTrackerMultiRef->init( options.mCameraParamFile.string().c_str(),
					options.mMultiMarkerFile.string().c_str(),
					options.mNearPlane, options.mFarPlane ) )
		{
			throw ArTrackerExcInitFail();
		}
		mTrackerMultiRef->changeCameraSize( width, height );

		// pattern width specified by marker config only
		// mTrackerMultiRef->setPatternWidth( options.mPatternWidth );

		if ( options.mMode == MARKER_ID_BCH )
			mTrackerMultiRef->setBorderWidth( 0.125f );
		else
			mTrackerMultiRef->setBorderWidth( 0.250f );
		mTrackerMultiRef->setUndistortionMode( ARToolKitPlus::UNDIST_STD );

		mTrackerMultiRef->activateAutoThreshold( options.mThresholdAuto );
		mTrackerMultiRef->setImageProcessingMode( (ARToolKitPlus::IMAGE_PROC_MODE)( AR_IMAGE_PROC_IN_FULL ) );

		// NOTE: RPP "Robust Planar Pose" estimator does not work for 3d multi marker objects
		mTrackerMultiRef->setPoseEstimator( ARToolKitPlus::POSE_ESTIMATOR_ORIGINAL );
		//mTrackerMultiRef->setPoseEstimator( ARToolKitPlus::POSE_ESTIMATOR_ORIGINAL_CONT );

		mTrackerMultiRef->setMarkerMode( static_cast< ARToolKitPlus::MARKER_MODE >( options.mMode ) );
	}
	else
	{
		if ( options.mMode == MARKER_TEMPLATE )
			mTrackerSingleRef = std::shared_ptr< ARToolKitPlus::TrackerSingleMarker >(
					new ARToolKitPlus::TrackerSingleMarkerImpl< 16, 16, 16, 32, 32 >( width, height ) );
		else
			mTrackerSingleRef = std::shared_ptr< ARToolKitPlus::TrackerSingleMarker >(
					new ARToolKitPlus::TrackerSingleMarkerImpl< 12, 12, 12, 32, 32 >( width, height ) );

		if ( options.mLoggingEnabled )
			mTrackerSingleRef->setLogger( &mLogger );

		// TODO set this in options
		mTrackerSingleRef->setPixelFormat( ARToolKitPlus::PIXEL_FORMAT_RGB );

		if ( !mTrackerSingleRef->init( options.mCameraParamFile.string().c_str(),
					options.mNearPlane, options.mFarPlane ) )
		{
			throw ArTrackerExcInitFail();
		}
		mTrackerSingleRef->changeCameraSize( width, height );

		mTrackerSingleRef->setPatternWidth( options.mPatternWidth );
		if ( options.mMode == MARKER_ID_BCH )
			mTrackerSingleRef->setBorderWidth( 0.125f );
		else
			mTrackerSingleRef->setBorderWidth( 0.250f );
		mTrackerSingleRef->setUndistortionMode( ARToolKitPlus::UNDIST_STD );

		mTrackerSingleRef->activateAutoThreshold( options.mThresholdAuto );

		// RPP is more robust than ARToolKit's standard pose estimator
		mTrackerSingleRef->setPoseEstimator( ARToolKitPlus::POSE_ESTIMATOR_RPP );

		mTrackerSingleRef->setMarkerMode( static_cast< ARToolKitPlus::MARKER_MODE >( options.mMode ) );

	}

	mOptions = options;
}

ArTracker::Obj::~Obj()
{
	if ( mTrackerSingleRef )
		mTrackerSingleRef->cleanup();
	if ( mTrackerMultiRef )
		mTrackerMultiRef->cleanup();
}

} } // namespace mndl::artkp

