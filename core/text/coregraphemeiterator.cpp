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
// Filename    : core/text/coregraphemeiterator.cpp
// Description : Grapheme Cluster Iterator
//
//************************************************************************************************

#include "core/text/coregraphemeiterator.h"

using namespace Core;
using namespace Text;
using namespace UTFCodec;

//************************************************************************************************
// GraphemeIterator
//************************************************************************************************

GraphemeIterator::GraphemeIterator (UStringPtr text, int length, int position)
: codepoints (text, length, position)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int GraphemeIterator::getPosition () const
{
	return codepoints.getPosition ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphemeIterator::setPosition (int position)
{
	codepoints.setPosition (position);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphemeIterator::isStart () const
{
	return codepoints.isStart ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphemeIterator::isEnd () const
{
	return codepoints.isEnd ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int GraphemeIterator::next ()
{
	if(codepoints.isEnd ())
		return codepoints.getPosition ();

	uchar32 firstCodepoint = codepoints.next ();
	if(codepoints.isEnd ())
		return codepoints.getPosition ();

	int prevClass = getGraphemeBreakClass (firstCodepoint);
	GraphemeState state = kStateNormal;

	// Initialize state based on first character
	if(prevClass == kGraphemeBreak_RegionalIndicator)
		state = kStateRI;
	else if(prevClass == kGraphemeBreak_ExtendedPictographic)
		state = kStateExtPicExtend;

	// Keep scanning until we find a grapheme break
	while(!codepoints.isEnd ())
	{
		int currClass = getGraphemeBreakClass (codepoints.peekNext ());

		if(shouldBreakBetweenClasses (prevClass, currClass, state))
			break;

		updateState (state, currClass);

		codepoints.next ();
		prevClass = currClass;
	}

	return codepoints.getPosition ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int GraphemeIterator::previous ()
{
	int position = codepoints.getPosition ();
	int length = codepoints.getLength ();

	ASSERT (position >= 0 && position <= length)

	if(position > length)
		position = length;
	if(position <= 0)
		return position;

	// Back up to a position that is a grapheme boundary regardless of preceding context
	codepoints.setPosition (position);
	int currClass = getGraphemeBreakClass (codepoints.previous ());
	while(!codepoints.isStart ())
	{
		int prevClass = getGraphemeBreakClass (codepoints.peekPrevious ());
		if(isContextFreeBreak (prevClass, currClass))
			break;

		codepoints.previous ();
		currClass = prevClass;
	}

	// Scan forward from there, finding all cluster boundaries
	int lastBoundary = codepoints.getPosition ();
	while(codepoints.getPosition () < position)
	{
		int nextBoundary = next ();

		// If next boundary reaches position, lastBoundary is where previous cluster starts
		if(nextBoundary >= position)
			break;

		lastBoundary = nextBoundary;
	}

	codepoints.setPosition (lastBoundary);
	return lastBoundary;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void GraphemeIterator::updateState (GraphemeState& state, int currClass)
{
	// Track Extended_Pictographic Extend* ZWJ sequences
	if(currClass == kGraphemeBreak_ExtendedPictographic)
	{
		state = kStateExtPicExtend;
	}
	else if(currClass == kGraphemeBreak_Extend)
	{
		if(state != kStateExtPicExtend)
			state = kStateNormal;
	}
	else if(currClass == kGraphemeBreak_ZWJ)
	{
		if(state == kStateExtPicExtend)
			state = kStateExtPicExtendZWJ;
		else
			state = kStateNormal;
	}
	else
	{
		if(state == kStateExtPicExtend || state == kStateExtPicExtendZWJ)
			state = kStateNormal;
	}

	// Regional Indicator state: toggle between first (kStateRI) and second (kStateNormal)
	if(currClass == kGraphemeBreak_RegionalIndicator)
	{
		if(state == kStateRI)
			state = kStateNormal;
		else
			state = kStateRI;
	}
	else if(state == kStateRI && currClass != kGraphemeBreak_RegionalIndicator)
	{
		state = kStateNormal;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphemeIterator::shouldBreakBetweenClasses (int prev, int curr, GraphemeState state)
{
	// GB3: CR x LF
	if(prev == kGraphemeBreak_CR && curr == kGraphemeBreak_LF)
		return false;

	// GB4: (Control | CR | LF) ÷
	if(prev == kGraphemeBreak_Control || prev == kGraphemeBreak_CR || prev == kGraphemeBreak_LF)
		return true;

	// GB5: ÷ (Control | CR | LF)
	if(curr == kGraphemeBreak_Control || curr == kGraphemeBreak_CR || curr == kGraphemeBreak_LF)
		return true;

	// GB6: L x (L | V | LV | LVT)
	if(prev == kGraphemeBreak_L && (curr == kGraphemeBreak_L || curr == kGraphemeBreak_V || curr == kGraphemeBreak_LV || curr == kGraphemeBreak_LVT))
		return false;

	// GB7: (LV | V) x (V | T)
	if((prev == kGraphemeBreak_LV || prev == kGraphemeBreak_V) && (curr == kGraphemeBreak_V || curr == kGraphemeBreak_T))
		return false;

	// GB8: (LVT | T) x T
	if((prev == kGraphemeBreak_LVT || prev == kGraphemeBreak_T) && curr == kGraphemeBreak_T)
		return false;

	// GB9: x (Extend | ZWJ)
	if(curr == kGraphemeBreak_Extend || curr == kGraphemeBreak_ZWJ)
		return false;

	// GB9a: x SpacingMark
	if(curr == kGraphemeBreak_SpacingMark)
		return false;

	// GB9b: Prepend x
	if(prev == kGraphemeBreak_Prepend)
		return false;

	// GB11: Extended_Pictographic Extend* ZWJ x Extended_Pictographic
	if(state == kStateExtPicExtendZWJ && curr == kGraphemeBreak_ExtendedPictographic)
		return false;

	// GB12, GB13: Regional_Indicator x Regional_Indicator (odd pairs don't break)
	if(state == kStateRI && curr == kGraphemeBreak_RegionalIndicator)
		return false;

	// GB999: Any ÷ Any
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GraphemeIterator::isContextFreeBreak (int prevClass, int currClass)
{
	// GB11, GB12, GB13 depend on context before prevClass
	if(prevClass == kGraphemeBreak_ZWJ && currClass == kGraphemeBreak_ExtendedPictographic)
		return false;
	if(prevClass == kGraphemeBreak_RegionalIndicator && currClass == kGraphemeBreak_RegionalIndicator)
		return false;

	// All other rules depend only on the adjacent classes
	return shouldBreakBetweenClasses (prevClass, currClass, kStateNormal);
}
