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
// Filename    : ccl/platform/win/direct2d/d2dclipper.h
// Description : Direct2D Clipper
//
//************************************************************************************************

#ifndef _ccl_direct2d_clipper_h
#define _ccl_direct2d_clipper_h

#include "ccl/platform/win/direct2d/d2dbase.h"

#include "ccl/public/collections/stack.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// D2DClipper
//************************************************************************************************

class D2DClipper
{
public:
	D2DClipper ();
	~D2DClipper ();

	void getState (Transform& transform, Rect& clipRect, bool absolute) const;
	void setOrigin (D2DRenderTarget& target, PointRef origin);
	void addTransform (D2DRenderTarget& target, TransformRef t);
	void addClip (D2DRenderTarget& target, RectFRef rect);
	void addClip (D2DRenderTarget& target, IGraphicsPath* path);
	void saveState (D2DRenderTarget& target);
	bool restoreState (D2DRenderTarget& target);
	void suspendClip (D2DRenderTarget& target, bool state);
	void removeClip (D2DRenderTarget& target);

protected:	
	struct State
	{
		Transform transform; ///< transformation of render target
		
		// clip
		RectF clipRectAbs; ///< clipping rectangle in absolute device coordinates
		ComPtr<ID2D1PathGeometry> clipPath;	///< clipping path
		bool clipActive = false;
		bool clipSuspended = false;

		bool activateClip (D2DRenderTarget& target, bool state);	
		bool restoreClip (D2DRenderTarget& target, const State& other);	
		bool suspendClip (D2DRenderTarget& target, bool state);	
		void resetClip (D2DRenderTarget& target);		     

		template<class TCoord> TRect<TCoord> makeAbsolute (const TRect<TCoord>& rect) const;
		template<class TCoord> TRect<TCoord> makeRelative (const TRect<TCoord>& absRect) const;
	};

	State activeState;
	Stack<State> stateStack;

	void updateState (D2DRenderTarget& target);
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_direct2d_clipper_h
