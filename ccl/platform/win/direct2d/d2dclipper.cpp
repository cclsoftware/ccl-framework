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
// Filename    : ccl/platform/win/direct2d/d2dclipper.cpp
// Description : Direct2D Clipper
//
//************************************************************************************************

#include "ccl/platform/win/direct2d/d2dclipper.h"
#include "ccl/platform/win/direct2d/d2dpath.h"

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// D2DClipper::State
//************************************************************************************************

template<class TCoord>
TRect<TCoord> D2DClipper::State::makeAbsolute (const TRect<TCoord>& rect) const
{
	TRect<TCoord> absRect (rect);
	transform.transform (absRect);
	return absRect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TCoord>
TRect<TCoord> D2DClipper::State::makeRelative (const TRect<TCoord>& absRect) const
{
	TPoint<TCoord> p;
	transform.transform (p);
	TRect<TCoord> rect (absRect);
	rect.offset (-p.x, -p.y);
	return rect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DClipper::State::activateClip (D2DRenderTarget& target, bool state)
{
	if(clipActive != state)
	{
		if(state)
		{
			auto clipRectRel = makeRelative (clipRectAbs);
			target->PushAxisAlignedClip (D2DInterop::fromCCL (clipRectRel), D2D1_ANTIALIAS_MODE_ALIASED);

			// add path on top of clip rect
			if(clipPath) 
			{		
				// Note: Starting with Windows 8, you don't need to call CreateLayer. 
				// In most situations performance is better if you don't call this method and Direct2D manages the layer resources.
				auto params = D2D1::LayerParameters (D2D1::InfiniteRect (), clipPath);
				target->PushLayer (params, nullptr);			
			}		
		}
		else
		{
			if(clipPath)
				target->PopLayer ();			
			target->PopAxisAlignedClip ();
		}
		clipActive = state;
	}	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DClipper::State::restoreClip (D2DRenderTarget& target, const State& oldState)
{
	ASSERT (clipSuspended == false && oldState.clipSuspended == false)

	if(clipActive == oldState.clipActive && 
		clipRectAbs == oldState.clipRectAbs &&
		clipPath == oldState.clipPath)
		return true;

	resetClip (target);
	
	clipRectAbs = oldState.clipRectAbs;
	clipPath = oldState.clipPath;

	if(oldState.clipActive)
		activateClip (target, true);
	
	return true;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DClipper::State::suspendClip (D2DRenderTarget& target, bool state)
{
	if(state)
	{
		if(clipActive && clipSuspended == false)
		{
			activateClip (target, false);
			clipSuspended = true;
		}
	}
	else if(clipSuspended)
	{
		activateClip (target, true);
		clipSuspended = false;		
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DClipper::State::resetClip (D2DRenderTarget& target)
{
	activateClip (target, false);
	clipSuspended = false;
	clipPath.release ();
}

//************************************************************************************************
// D2DClipper
//************************************************************************************************

D2DClipper::D2DClipper ()
{
	activeState.clipRectAbs (0, 0, kMaxCoord, kMaxCoord);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DClipper::~D2DClipper ()
{
	ASSERT (activeState.clipActive == false) // calls must be balanced!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DClipper::updateState (D2DRenderTarget& target)
{
	D2D1::Matrix3x2F matrix;
	target->GetTransform (&matrix);
	activeState.transform = D2DInterop::fromMatrix (matrix);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DClipper::getState (Transform& transform, Rect& clipRect, bool absolute) const
{
	transform = activeState.transform;
	
	if(absolute)
		clipRect = rectFToInt (activeState.clipRectAbs);
	else
		clipRect = rectFToInt (activeState.makeRelative (activeState.clipRectAbs));	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DClipper::setOrigin (D2DRenderTarget& target, PointRef origin)
{
	target->SetTransform (D2D1::Matrix3x2F::Translation ((FLOAT)origin.x, (FLOAT)origin.y));
	
	updateState (target);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DClipper::addTransform (D2DRenderTarget& target, TransformRef t)
{
	D2D1::Matrix3x2F current;
	target->GetTransform (&current);

	D2D1::Matrix3x2F added = D2DInterop::toMatrix (t);
	target->SetTransform (added * current); // left-multiply!
	
	updateState (target);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DClipper::addClip (D2DRenderTarget& target, RectFRef _rect)
{	
	// bound to active rect
	RectF absRect = activeState.makeAbsolute (_rect);
	absRect.bound (activeState.clipRectAbs);

	// prevent negative size, seems to have unexpected results
	if(absRect.getWidth () < 0)
		absRect.setWidth (0);
	if(absRect.getHeight () < 0)
		absRect.setHeight (0);

	if(activeState.clipActive && activeState.clipRectAbs == absRect && activeState.clipPath.isValid () == false)
		return;

	activeState.resetClip (target);
	activeState.clipRectAbs = absRect;	
	activeState.activateClip (target, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DClipper::addClip (D2DRenderTarget& target, IGraphicsPath* path)
{
	D2DPathGeometry* d2dPath = unknown_cast<D2DPathGeometry> (path);
	ASSERT (d2dPath)
	if(d2dPath)
	{				
		ComPtr<ID2D1PathGeometry> iPath;
		iPath.share (d2dPath->getID2D1Path ());
		ASSERT (iPath.isValid ())
		if(iPath.isValid ())
		{			
			activeState.resetClip (target);
				
			RectF bounds;
			d2dPath->getBounds (bounds);
			ASSERT (bounds.isEmpty () == false)
			RectF absRect = activeState.makeAbsolute (bounds);
			absRect.bound (activeState.clipRectAbs);
			if(absRect.getWidth () < 0)
				absRect.setWidth (0);
			if(absRect.getHeight () < 0)
				absRect.setHeight (0);

			activeState.clipRectAbs = absRect;	
			activeState.clipPath = iPath;
			activeState.activateClip (target, true);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DClipper::saveState (D2DRenderTarget& target)
{
	State state = activeState;
	stateStack.push (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DClipper::restoreState (D2DRenderTarget& target)
{
	ASSERT (!stateStack.isEmpty ())
	if(stateStack.isEmpty ())
		return false;

	State prevState = stateStack.pop ();

	// restore transformation
	target->SetTransform (D2DInterop::toMatrix (prevState.transform));
	updateState (target);

	// restore clip
	activeState.restoreClip (target, prevState);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DClipper::suspendClip (D2DRenderTarget& target, bool state)
{	
	activeState.suspendClip (target, state);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DClipper::removeClip (D2DRenderTarget& target)
{	
	activeState.resetClip (target);
}
