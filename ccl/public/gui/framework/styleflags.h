//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/public/gui/framework/styleflags.h
// Description : Style Flags
//
//************************************************************************************************

#ifndef _ccl_styleflags_h
#define _ccl_styleflags_h

#include "ccl/public/base/enumdef.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
/** Style Definitions
	\ingroup gui_view */
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Styles
{
	/** Common view styles. */
	enum CommonStyleFlags
	{
		kHorizontal = 1<<0,		///< control orientation
		kVertical = 1<<1,		///< control orientation
		kBorder = 1<<2,			///< usually default
		kTransparent = 1<<3,	///< don't draw background
		kDirectUpdate = 1<<4,	///< redraw control directly
		kComposited = 1<<5,		///< use parent background when updated directly
		kTranslucent = 1<<6,	///< alpha channel needs to be respected
		kTrigger = 1<<7,		///< enable property triggers
		kSmall = 1<<8,			///< smaller variant
		kLeft = 1<<9,			///< left variant
		kRight = 1<<10,			///< right variant
		kMiddle = 1<<11,		///< middle variant
		kLayerUpdate = 1<<12,	///< each invalidate also forces updating sub layers of all child views (deep)
		kNoHelpId = 1<<13		///< deliver empty help identifier (instead of delegating to parent): siblings (underneath) will get a chance to contribute a help id
	};

	/** Transition types. */
	DEFINE_ENUM (TransitionType)
	{
		kTransitionNone,                ///< no transition
		kTransitionFade = 'fade',		///< old content fades out while new content becomes visible
		kTransitionMoveIn = 'mvin',		///< new content slides in on top of old content (from left)
		kTransitionMoveOut = 'mvot',	///< old content slides out and new content is revealed (to left - inverse of movein)
		kTransitionConceal = 'conc',	///< new content slides in on top of old content (from right)
		kTransitionReveal = 'revl',		///< old content slides out and new content is revealed (to right - inverse of conceal)
		kTransitionPush = 'push',		///< new content pushes old content to the right as it slides in
		kTransitionPushLeft = 'pusl',	///< old content pushed to the left as new content slides in (inverse of push)
		kTransitionPushDown = 'pusd',	///< new content pushes old content to the bottom as it slides in
		kTransitionPushUp = 'pusu',		///< old content pushed to the top as new content slides in (inverse of pushdown)
		kTransitionFall = 'fall',		///< new content falls on top of old content
		kTransitionLift = 'lift',		///< old content lifts up and new content is revealed while (inverse of fall)
		kTransitionRise = 'rise',		///< new content rises on top of old content
		kTransitionSink = 'sink',		///< old content sinks down and new content is revealed (inverse of rise)
		kTransitionZoomIn = 'zmin',		///< new content is zoomed in on top of old content
		kTransitionZoomOut = 'zmot'		///< old content is zoomed out on top of new content (inverse of zoom)
	};
}

using Styles::TransitionType;

//************************************************************************************************
// StyleDef
//************************************************************************************************

typedef EnumInfo StyleDef;

#define DECLARE_STYLEDEF(name)	DECLARE_ENUMINFO(name)
#define BEGIN_STYLEDEF(name)	BEGIN_ENUMINFO(name)
#define END_STYLEDEF			END_ENUMINFO

//************************************************************************************************
/** StyleFlags
	\ingroup gui_view */
//************************************************************************************************

struct StyleFlags
{
	int common;
	int custom;

	StyleFlags (int common = 0, int custom = 0)
	: common (common), 
	  custom (custom)
	{}

	bool isEmpty () const;
	bool isCommonStyle (int which) const;
	bool isCustomStyle (int which) const;
	void setCommonStyle (int which, bool state = true);
	void setCustomStyle (int which, bool state = true);

	bool isHorizontal () const;
	bool isVertical () const;
	bool isTransparent () const;
	bool isOpaque () const;
	bool isBorder () const;
	bool isDirectUpdate () const;
	bool isComposited () const;
	bool isTranslucent () const;
	bool isTrigger () const;
	bool isSmall () const;

	int64 toLargeInt () const;
	StyleFlags& fromLargeInt (int64 value);

	bool operator == (const StyleFlags&) const;
	bool operator != (const StyleFlags&) const;
};

typedef const StyleFlags& StyleRef;

//////////////////////////////////////////////////////////////////////////////////////////////////
// StyleFlags inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool StyleFlags::isEmpty () const
{ return common == 0 && custom == 0; }

inline bool StyleFlags::isCommonStyle (int which) const 
{ return (common & which) != 0; }

inline bool StyleFlags::isCustomStyle (int which) const 
{ return (custom & which) != 0; }

inline void StyleFlags::setCommonStyle (int which, bool state)
{ if(state) common |= which; else common &= ~which; }

inline void StyleFlags::setCustomStyle (int which, bool state)
{ if(state) custom |= which; else custom &= ~which; }

inline int64 StyleFlags::toLargeInt () const
{ return common | (int64)custom << 32; }

inline StyleFlags& StyleFlags::fromLargeInt (int64 value)
{ common = (int)(value & 0xFFFFFFFF);
  custom = (int)((value >> 32) & 0xFFFFFFFF); 
  return *this; }

inline bool StyleFlags::operator == (const StyleFlags& flags) const
{ return common == flags.common && custom == flags.custom; }

inline bool StyleFlags::operator != (const StyleFlags& flags) const
{ return common != flags.common || custom != flags.custom; }

inline bool StyleFlags::isHorizontal () const { return isCommonStyle (Styles::kHorizontal); }
inline bool StyleFlags::isVertical () const { return isCommonStyle (Styles::kVertical); }
inline bool StyleFlags::isTransparent () const { return isCommonStyle (Styles::kTransparent); }
inline bool StyleFlags::isOpaque () const { return !isTransparent (); }
inline bool StyleFlags::isBorder () const { return isCommonStyle (Styles::kBorder); }
inline bool StyleFlags::isDirectUpdate () const { return isCommonStyle (Styles::kDirectUpdate); }
inline bool StyleFlags::isComposited () const { return isCommonStyle (Styles::kComposited); }
inline bool StyleFlags::isTranslucent () const { return isCommonStyle (Styles::kTranslucent); }
inline bool StyleFlags::isTrigger () const { return isCommonStyle (Styles::kTrigger); }
inline bool StyleFlags::isSmall () const { return isCommonStyle (Styles::kSmall); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_styleflags_h
