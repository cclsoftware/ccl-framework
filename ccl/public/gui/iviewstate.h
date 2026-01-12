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
// Filename    : ccl/public/gui/iviewstate.h
// Description : View state handler Interface
//
//************************************************************************************************

#ifndef _ccl_iviewstate_h
#define _ccl_iviewstate_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/gui/graphics/point.h"

namespace CCL {

interface IAttributeList;

//************************************************************************************************
// IViewState
/**
	\ingroup gui_view */
//************************************************************************************************

interface IViewState: IUnknown
{
	/** Get size of view currently being saved. */
	virtual PointRef CCL_API getViewSize () const = 0;

	/** Manipulate the saved size of the view currently being restored. */
	virtual void CCL_API setViewSize (PointRef size) = 0;

	DECLARE_IID (IViewState)
};

DEFINE_IID (IViewState, 0xADAAE758, 0x8FB8, 0x41C9, 0x99, 0x7B, 0x06, 0x7A, 0xCA, 0x68, 0x59, 0x28)

//************************************************************************************************
// IViewStateHandler
/** Handler for saving/loading the state of a view as attributes, to be implemented by a controller.
	Only primitive attributes (int, float, string) are allowed!
	The IViewState argument is only required in rare cases. 
	\ingroup gui_view */
//************************************************************************************************

interface IViewStateHandler: IUnknown
{
	/** Save view state as attributes. */
	virtual tbool CCL_API saveViewState (StringID viewID, StringID viewName, IAttributeList& attributes, const IViewState* state) const = 0;

	/** Load view state from attributes. */
	virtual tbool CCL_API loadViewState (StringID viewID, StringID viewName, const IAttributeList& attributes, IViewState* state) = 0;

	DECLARE_IID (IViewStateHandler)
};

DEFINE_IID (IViewStateHandler, 0x795B0192, 0xEAF0, 0x4959, 0x92, 0x88, 0x48, 0x84, 0xCC, 0x2A, 0xDC, 0xA6)

//************************************************************************************************
// ILayoutStateProvider
/** Provides attribute lists for storing layout states of views.
	Only primitive attributes (int, float, string) are allowed! 
	\ingroup gui_view */
//************************************************************************************************

interface ILayoutStateProvider: IUnknown
{
	/** Get the attribute list that holds the layout state. */
	virtual IAttributeList* CCL_API getLayoutState (StringID id, tbool create = false) = 0;

	DECLARE_IID (ILayoutStateProvider)
};

DEFINE_IID (ILayoutStateProvider, 0xB5707D9A, 0xC08F, 0x4F8F, 0xB0, 0x4B, 0xBB, 0xDE, 0x03, 0xE1, 0x61, 0xAF)

} // namespace CCL

#endif // _ccl_iviewstate_h
