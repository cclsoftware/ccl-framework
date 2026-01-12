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
// Filename    : ccl/gui/theme/renderer/valueboxrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/valueboxrenderer.h"

using namespace CCL;

//************************************************************************************************
// ValueBoxRenderer
/** Drawn like an EditBox. */
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (ValueBox, EditBox, "ValueBoxStyle")
END_VISUALSTYLE_CLASS (ValueBox)

//////////////////////////////////////////////////////////////////////////////////////////////////

ValueBoxRenderer::ValueBoxRenderer (VisualStyle* visualStyle)
: EditBoxRenderer (visualStyle)
{}
