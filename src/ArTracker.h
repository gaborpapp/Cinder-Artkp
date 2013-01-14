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

#pragma once

#include <iostream>

#include "ARToolKitPlus/TrackerSingleMarker.h"
#include "ARToolKitPlus/TrackerMultiMarker.h"

#include "cinder/app/App.h"
#include "cinder/Cinder.h"
#include "cinder/Exception.h"
#include "cinder/Filesystem.h"
#include "cinder/Surface.h"

namespace mndl { namespace artkp {

class Logger : public ARToolKitPlus::Logger
{
	void artLog( const char *str )
	{
		ci::app::console() << str << std::endl;
	}
};

class ArTracker
{
	public:
		struct Options;

		ArTracker() {}
		~ArTracker() {}

		typedef enum
		{
			MARKER_TEMPLATE = ARToolKitPlus::MARKER_TEMPLATE,
			MARKER_ID_SIMPLE = ARToolKitPlus::MARKER_ID_SIMPLE,
			MARKER_ID_BCH = ARToolKitPlus::MARKER_ID_BCH
		} MarkerMode;

		//! Creates an ArTracker for a \a width pixels wide and \a height pixels high camera image, using ArTracker::Options \a options.
		ArTracker( int32_t width, int32_t height, Options options = Options() );

		//! Detects the markers in the \a surface.
		void update( ci::Surface &surface );

		//! Returns the number of markers. id of the marker with the given index \a i.
		int getNumMarkers() const;

		//! Returns the marker id of the marker with the given index \a i.
		int getMarkerId( int i ) const;

		//! Returns the confidence value of the marker with the given index \a i.
		float getMarkerConfidence( int i ) const;

		//! Sets the \c PROJECTION matrix to reflect the values of the camera parameter file loaded for the ArTracker. Leaves the \c MatrixMode as \c PROJECTION.
		void setProjection() const;

		//! Returns the value of the \a i'th marker's \c MODELVIEW matrix as a Matrix44f.
		ci::Matrix44f getModelView( int i ) const;

		//! Sets the \c MODELVIEW matrix to reflect the values of the \a i'th marker. Leaves the \c MatrixMode as \c MODELVIEW.
		void setModelView( int i ) const;

		//! Enables automatic threshold calculation.
		void enableAutoThreshold( bool enable = true );

		//! Returns true if automatic threshold detection is enabled.
		bool isAutoThresholdEnabled() const;

		//! Sets the threshold value that is used for black/white conversion.
		void setThreshold( int thres );

		//! Returns the current threshold value.
		int getThreshold() const;

		//! Returns the ArTracker::Options of this ArTracker.
		const Options& getOptions() { return mObj->mOptions; }

		struct Options
		{
			public:
				//! Default constructor, sets the mode to \c MARKER_ID_SIMPLE, 1.0 near and 1000.0 far clipping distance, disable logging, 80 mm pattern width, no camera parameter file.
				Options() {};

				//! Sets the tracker mode. Defaults to \c MARKER_ID_SIMPLE, other options are \c MARKER_TEMPLATE and MARKER_ID_BCH.
				void setMode( MarkerMode mode ) { mMode = mode; }

				//! Sets the pattern width in millimeters. Defaults to 80.0.
				void setPatternWidth( float width ) { mPatternWidth = width; }

				//! Gets the pattern width in millimeters.
				float getPatternWidth() const { return mPatternWidth; }

				//! Sets the camera parameter file path to \a cameraFile.
				void setCameraFile( const ci::fs::path &cameraFile ) { mCameraParamFile = cameraFile; }

				//! Sets the multi marker file path to \a cameraFile. Enables multi marker setup.
				void setMultiMarker( const ci::fs::path &configFile ) { mMultiMarkerFile = configFile; mMultiMarker = true; }

			protected:
				MarkerMode mMode = MARKER_ID_SIMPLE;
				float mNearPlane = 1.f;
				float mFarPlane = 1000.f;
				bool mLoggingEnabled = false;
				float mPatternWidth = 80.f;
				bool mThresholdAuto = true;
				bool mMultiMarker = false;

				ci::fs::path mCameraParamFile;
				ci::fs::path mMultiMarkerFile;

				friend class ArTracker;
		};

	protected:
		struct Obj
		{
			Obj( int32_t width, int32_t height, Options options );
			~Obj();

			std::shared_ptr< ARToolKitPlus::TrackerSingleMarker > mTrackerSingleRef;
			std::shared_ptr< ARToolKitPlus::TrackerMultiMarker > mTrackerMultiRef;

			Logger mLogger;

			Options mOptions;

			int mNumMarkers = 0;
			ARToolKitPlus::ARMarkerInfo *mMarkerInfo;
		};

		std::shared_ptr< Obj > mObj;

};

class ArTrackerExc : public ci::Exception {};

class ArTrackerExcCameraFileNotFound : public ArTrackerExc {};
class ArTrackerExcInitFail : public ArTrackerExc {};
class ArTrackerExcMarkerIndexOutOfRange : public ArTrackerExc {};

} } // mndl::artkp

