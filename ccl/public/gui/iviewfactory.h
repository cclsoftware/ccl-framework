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
// Filename    : ccl/public/gui/iviewfactory.h
// Description : View Factory Interface
//
//************************************************************************************************

#ifndef _ccl_iviewfactory_h
#define _ccl_iviewfactory_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/gui/graphics/rect.h"

namespace CCL {

interface IView;

//************************************************************************************************
// ViewNames
/** Common view names. */
//************************************************************************************************

namespace ViewNames
{
	DEFINE_STRINGID (kEditor, "editor")
	DEFINE_STRINGID (kMicro, "micro")
	DEFINE_STRINGID (kInspector, "inspector")
	DEFINE_STRINGID (kCompact, "compact")
	DEFINE_STRINGID (kEmbedded, "embedded")
}

//************************************************************************************************
// IViewFactory
/**
	\ingroup gui_skin */
//************************************************************************************************

interface IViewFactory: IUnknown
{
	virtual IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) = 0;

	DECLARE_IID (IViewFactory)
};

DEFINE_IID (IViewFactory, 0x94fd1217, 0x5a3b, 0x4aa1, 0x9a, 0x4a, 0x6, 0x7e, 0x57, 0x5, 0x2b, 0x95)

} // namespace CCL

#endif // _ccl_iviewfactory_h
