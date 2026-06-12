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
// Filename    : ccl/gui/theme/renderer/compositedrenderer.h
// Description : Composited Renderer
//
//************************************************************************************************

#ifndef _ccl_compositedrenderer_h
#define _ccl_compositedrenderer_h

#include "ccl/gui/theme/themerenderer.h"

namespace CCL {

interface IBackgroundView;

//************************************************************************************************
// CompositedRenderer
//************************************************************************************************

class CompositedRenderer: public ThemeRenderer
{
public:
	DECLARE_CLASS_ABSTRACT (CompositedRenderer, ThemeRenderer)

	CompositedRenderer (VisualStyle* visualStyle = nullptr);

	// ThemeRenderer
	void update (View* view, const UpdateInfo& info) override;

protected:
	IBackgroundView* backgroundView;
	Point position;

	void drawCompositedBackground (IGraphics& graphics, View* view, RectRef rect);
};

} // namespace CCL

#endif // _ccl_compositedrenderer_h
