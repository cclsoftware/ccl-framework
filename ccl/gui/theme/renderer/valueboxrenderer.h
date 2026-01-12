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
// Filename    : ccl/gui/theme/renderer/valueboxrenderer.h
// Description : Control Renderer
//
//************************************************************************************************

#ifndef _ccl_valueboxrenderer_h
#define _ccl_valueboxrenderer_h

#include "ccl/gui/theme/renderer/editboxrenderer.h"

namespace CCL {

//************************************************************************************************
// ValueBoxRenderer
//************************************************************************************************

class ValueBoxRenderer: public EditBoxRenderer
{
public:
	ValueBoxRenderer (VisualStyle* visualStyle);

	// to be continued...
};

DECLARE_VISUALSTYLE_CLASS (ValueBox)

} // namespace CCL

#endif // _ccl_valueboxrenderer_h
