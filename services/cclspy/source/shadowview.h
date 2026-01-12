//************************************************************************************************
//
// CCL Spy
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
// Filename    : shadowview.h
// Description : Shadow view representing a foreign View
//
//************************************************************************************************

#ifndef _shadowview_h
#define _shadowview_h

#include "ccl/app/controls/usercontrol.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/public/gui/framework/iembeddedviewhost.h"

namespace Spy {

//************************************************************************************************
// ShadowView
/** Placeholder representing a foreign View. */
//************************************************************************************************

class ShadowView: public CCL::UserControl
{
public:
	DECLARE_CLASS_ABSTRACT (ShadowView, UserControl)

	// build a CCL View tree of placeholders representing foreign views
	static CCL::IView* buildViewTree (CCL::IEmbeddedViewHost& viewHost, CCL::IEmbeddedViewHost::ViewRef view = nullptr, ShadowView* parentShadowView = nullptr);

	ShadowView (CCL::IEmbeddedViewHost& viewHost, CCL::IEmbeddedViewHost::ViewRef view, ShadowView* parentShadowView);

	PROPERTY_OBJECT (CCL::Rect, nativeSize, NativeSize) ///< actual size of the foreign ViewRef; the inherited View::size can be scaled for screen display

	// Object
	CCL::tbool CCL_API getProperty (CCL::Variant& var, MemberID propertyId) const override;

protected:
	CCL::Attributes properties;
};

} // namespace Spy

#endif // _shadowview_h
