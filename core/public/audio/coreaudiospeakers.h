//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : core/public/audio/coreaudiospeakers.h
// Description : Audio Speaker Definitions
//
//************************************************************************************************

#ifndef _coreaudiospeakers_h
#define _coreaudiospeakers_h

#include "core/public/coreprimitives.h"
#include "core/public/audio/coreaudiodatatypes.h"

namespace Core {

/** Speaker type. */
typedef uint32 SpeakerID;

/** Bit field of speakers. */
typedef uint32 SpeakerBitField;

namespace SpeakerDef  {

//********************************************************************************************
// Speaker Definitions
//********************************************************************************************

constexpr SpeakerID kNoSpeaker		    = 0;
constexpr SpeakerID kFrontLeft		    = 1<<0;		///< L: Front Left
constexpr SpeakerID kFrontRight		    = 1<<1;		///< R: Front Right
constexpr SpeakerID kFrontCenter	    = 1<<2;		///< C: Front Center
constexpr SpeakerID kLowFrequency	    = 1<<3;		///< Lfe: Low Frequency
constexpr SpeakerID kBackLeft		    = 1<<4;		///< Ls: Left Surround (rear)
constexpr SpeakerID kBackRight          = 1<<5;		///< Rs: Surround Right  (rear)
constexpr SpeakerID kFrontLeftOfCenter  = 1<<6;		///< Lc: Front Left Of Center
constexpr SpeakerID kFrontRightOfCenter = 1<<7;		///< Rc: Front Right Of Center
constexpr SpeakerID kBackCenter			= 1<<8;		///< S:  Back Center
constexpr SpeakerID kSideLeft	        = 1<<9;		///< Sl: Left Side 
constexpr SpeakerID kSideRight	        = 1<<10;	///< Sr: Right Side 
constexpr SpeakerID kTopCenter			= 1<<11;	///< Tc: Top Center
constexpr SpeakerID kTopFrontLeft		= 1<<12;	///< Tfl: Top Front Left
constexpr SpeakerID kTopFrontCenter		= 1<<13;	///< Tfc: Top Front Center
constexpr SpeakerID kTopFrontRight		= 1<<14;	///< Tfr: Top Front Right
constexpr SpeakerID kTopBackLeft		= 1<<15;	///< Tbl: Top Back Left
constexpr SpeakerID kTopBackCenter		= 1<<16;	///< Tbc: Top Back Center
constexpr SpeakerID kTopBackRight		= 1<<17;	///< Tbr: Top Back Right
constexpr SpeakerID kTopMiddleLeft		= 1<<18;	///< Tml: Top Middle Left
constexpr SpeakerID kTopMiddleRight		= 1<<19;	///< Tmr: Top Middle Right
constexpr SpeakerID kWideLeft			= 1<<20;	///< Lw: Left wide speakers (9.1.6)
constexpr SpeakerID kWideRight			= 1<<21; 	///< Rw: Right wide speakers (9.1.6)

//********************************************************************************************
// Speaker Configurations
//********************************************************************************************

constexpr SpeakerBitField kEmpty            = kNoSpeaker;
constexpr SpeakerBitField kMono             = kFrontLeft;

// 2.0 Configurations
constexpr SpeakerBitField kStereo           = kFrontLeft | kFrontRight;
constexpr SpeakerBitField kStereoSurround   = kBackLeft | kBackRight;
constexpr SpeakerBitField kStereoCenter     = kFrontLeftOfCenter | kFrontRightOfCenter;
constexpr SpeakerBitField kStereoSide       = kSideLeft | kSideRight;
constexpr SpeakerBitField kStereoCenterLfe  = kFrontCenter | kLowFrequency;

// 3.0 / 3.1 Configurations
constexpr SpeakerBitField kCine30           = kStereo | kFrontCenter;
constexpr SpeakerBitField kMusic30          = kStereo | kBackCenter;
constexpr SpeakerBitField kCine31           = kCine30 | kLowFrequency;
constexpr SpeakerBitField kMusic31          = kMusic30 | kLowFrequency;

// 4.0 / 4.1 Configurations
constexpr SpeakerBitField kCine40           = kStereo | kFrontCenter | kBackCenter;
constexpr SpeakerBitField kMusic40          = kStereo | kBackLeft | kBackRight;
constexpr SpeakerBitField kCine41           = kCine40 | kLowFrequency;
constexpr SpeakerBitField kMusic41          = kMusic40 | kLowFrequency;

// 5.0 / 5.1 / 5.1.2 / 5.1.4 Configurations
constexpr SpeakerBitField k50               = kStereo | kFrontCenter | kBackLeft | kBackRight;
constexpr SpeakerBitField k51               = k50 | kLowFrequency;
constexpr SpeakerBitField k51itu            = kStereo | kFrontCenter | kSideLeft | kSideRight | kLowFrequency;
constexpr SpeakerBitField k512              = k51 | kTopMiddleLeft | kTopMiddleRight;
constexpr SpeakerBitField k514              = k51 | kTopFrontLeft | kTopFrontRight | kTopBackLeft | kTopBackRight;

// 7.0 7.1 / 7.1.2 / 7.1.4 / 7.1.6 Configurations
constexpr SpeakerBitField k70               = k50 | kSideLeft | kSideRight;
constexpr SpeakerBitField k71               = k70 | kLowFrequency;
constexpr SpeakerBitField k702              = k70 | kTopMiddleLeft | kTopMiddleRight;
constexpr SpeakerBitField k712              = k702 | kLowFrequency;
constexpr SpeakerBitField k714              = k71 | kTopFrontLeft | kTopFrontRight | kTopBackLeft | kTopBackRight;
constexpr SpeakerBitField k716              = k714 | kTopMiddleLeft | kTopMiddleRight;

// 9.1.6 Configuration 
constexpr SpeakerBitField k916              = k714 | kWideLeft | kWideRight | kTopMiddleLeft | kTopMiddleRight; ///< (Dolby)
constexpr SpeakerBitField k916itu           = k714 | kFrontLeftOfCenter | kFrontRightOfCenter | kTopMiddleLeft | kTopMiddleRight;

constexpr int kMaxSetupChannels = 16; ///< Number of channels in largest known speaker config 
constexpr int kNumSpeakerBits = sizeof(SpeakerBitField) * 8; 

inline SpeakerBitField getDefault (int channelCount)
{
	switch(channelCount)
	{
	case 0 : return kEmpty;
	case 1 : return kMono;
	case 2 : return kStereo;
	default :
		{
			SpeakerBitField result = 0;
			for(int i = 0; i < channelCount; i++)
				set_bit<SpeakerBitField> (result, i);
			return result;
		}		
	}
}

} // namespace SpeakerDef

} // namespace Core

#endif // _coreaudiospeakers_h
