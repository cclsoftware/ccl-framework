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
// Filename    : ccl/app/editing/tools/toolconfig.h
// Description : Tool Configuration
//
//************************************************************************************************

#ifndef _ccl_toolconfig_h
#define _ccl_toolconfig_h

#include "ccl/app/editing/tools/edittool.h"
#include "ccl/app/editing/tools/itoolconfig.h"

namespace CCL {

//************************************************************************************************
// ConfigTool
//************************************************************************************************

class ConfigTool: public EditTool
{
public:
	DECLARE_CLASS (ConfigTool, EditTool)

	ConfigTool (IToolConfiguration* config = nullptr);

	// EditTool
	void onAttached (EditView& editView, bool state) override;
	bool onContextMenu (IContextMenu& contextMenu) override;
	bool extendModeMenu (IMenu& menu) override;
	void mouseEnter (EditView& editView, const MouseEvent& mouseEvent) override;
	void mouseMove (EditView& editView, const MouseEvent& mouseEvent) override;
	void mouseLeave (EditView& editView, const MouseEvent& mouseEvent) override;
	EditHandler* mouseDown (EditView& editView, const MouseEvent& mouseEvent) override;
	ITouchHandler* createTouchHandler (EditView& editView, const TouchEvent& event) override;
	String getTooltip () override;
	IPresentable* createHelpInfo (EditView& editView, const MouseEvent& mouseEvent) override;
	void setActiveMode (EditToolMode* mode) override;

protected:
	SharedPtr<IToolConfiguration> config;
	UnknownPtr<IToolHelp> toolHelp;
	AutoPtr<IToolAction> action;

	class TouchMouseAction;
};

} // namespace CCL

#endif // _ccl_toolconfig_h
