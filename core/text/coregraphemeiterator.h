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
// Filename    : core/text/coregraphemeiterator.h
// Description : Grapheme Cluster Iterator
//
//************************************************************************************************

#ifndef _coregraphemeiterator_h
#define _coregraphemeiterator_h

#include "core/text/coreutfcodec.h"

namespace Core {
namespace Text {

//************************************************************************************************
// GraphemeIterator
/** Iterates grapheme cluster boundaries (UAX #29) of a UTF-16 string.
	Treats emoji with modifiers, ZWJ sequences, flags, and combining marks as atomic units.
	The position is given in UTF-16 code units. */
//************************************************************************************************

class GraphemeIterator
{
public:
	GraphemeIterator (UStringPtr text, int length, int position = 0);

	int getPosition () const;
	void setPosition (int position);
	bool isStart () const;
	bool isEnd () const;

	/** Move to next grapheme cluster boundary, returns new position. */
	int next ();

	/** Move to previous grapheme cluster boundary, returns new position. */
	int previous ();

protected:
	enum GraphemeState
	{
		kStateNormal,
		kStateExtPicExtend,		// After Extended_Pictographic, tracking Extend* for GB11
		kStateExtPicExtendZWJ,	// After Extended_Pictographic Extend* ZWJ, ready for another ExtPic
		kStateRI				// After first Regional_Indicator, ready to pair with second
	};

	UTF16CodepointIterator codepoints;

	static bool shouldBreakBetweenClasses (int prev, int curr, GraphemeState state);
	static void updateState (GraphemeState& state, int currClass);
	static bool isContextFreeBreak (int prevClass, int currClass);
};

} // namespace Text
} // namespace Core

#endif // _coregraphemeiterator_h
