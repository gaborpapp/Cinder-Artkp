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

#pragma once

#include <iostream>

#include "ARToolKitPlus/TrackerSingleMarker.h"

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

class ArTracker {
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

		ArTracker( int32_t width, int32_t height, Options options = Options() );

		void update( ci::Surface &surface );

		int getNumMarkers() const;
		int getMarkerId( int i ) const;
		float getMarkerConfidence( int i ) const;

		void setProjection() const;
		ci::Matrix44f getModelView( int i ) const;
		void setModelView( int i ) const;

		//! Gets the pattern width in millimeters.
		float getPatternWidth() { return mObj->mOptions.mPatternWidth; }

		struct Options
		{
			public:
				//! Default constructor, sets the mode to \c MARKER_ID_SIMPLE, 1.0 near and 1000.0 far clipping distance, disable logging, 80 mm pattern width, no camera parameter file.
				Options()
					{};

				//! Sets the tracker mode. Defaults to \c MARKER_ID_SIMPLE, other options are \c MARKER_TEMPLATE and MARKER_ID_BCH.
				void setMode( MarkerMode mode ) { mMode = mode; }

				//! Sets the pattern width in millimeters. Defaults to 80.0.
				void setPatternWidth( float width ) { mPatternWidth = width; }

				void setCameraFile( const ci::fs::path &cameraFile ) { mCameraParamFile = cameraFile; }

			protected:
				MarkerMode mMode = MARKER_ID_SIMPLE;
				float mNearPlane = 1.f;
				float mFarPlane = 1000.f;
				bool mLoggingEnabled = false;
				float mPatternWidth = 80.f;
				bool mThresholdAuto = true;

				ci::fs::path mCameraParamFile;

				friend class ArTracker;
		};

	protected:
		struct Obj
		{
			Obj( int32_t width, int32_t height, Options options );
			~Obj();

			std::shared_ptr< ARToolKitPlus::TrackerSingleMarker > mTrackerRef;
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

