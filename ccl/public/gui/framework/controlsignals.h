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
// Filename    : ccl/public/gui/framework/controlsignals.h
// Description : Control Signals
//
//************************************************************************************************

#ifndef _ccl_controlsignals_h
#define _ccl_controlsignals_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
namespace Signals {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Signals::kGUI
//////////////////////////////////////////////////////////////////////////////////////////////////

/** General GUI related signals */
DEFINE_STRINGID (kGUI, "GUI")

	/** arg[0]: new orientation (OrientationType) */
	DEFINE_STRINGID (kOrientationChanged, "OrientationChanged")

	/** arg[0]: IColorScheme */
	DEFINE_STRINGID (kColorSchemeChanged, "ColorSchemeChanged")
	
	/** Dimensions of status bar etc. changed */
	DEFINE_STRINGID (kSystemMetricsChanged, "SystemMetricsChanged")

	/** Graphics engine reset happened */
	DEFINE_STRINGID (kGraphicsEngineReset, "GraphicsEngineReset")

//////////////////////////////////////////////////////////////////////////////////////////////////
// Signals::kControls
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Signals related to Controls */
DEFINE_STRINGID (kControls, "GUI.Controls")

	/** arg[0]: menu (IContextMenu), arg[1]: identity (IUnknown), arg[2]: result (IVariant), args[3]: control class (optional, UID string) */
	DEFINE_STRINGID (kControlContextMenu, "ControlContextMenu")

	/** Hide all user tooltips. */
	DEFINE_STRINGID (kHideTooltip, "HideTooltip")

	/** Hide (non-modal) context menu. */
	DEFINE_STRINGID (kHideContextMenu, "HideContextMenu")

	/** Restore a previously hidden (non-modal) context menu; arg[0]: x-position (int), arg[1]: y-position (int) - both optional, in screen coords */
	DEFINE_STRINGID (kRestoreContextMenu, "RestoreContextMenu")

	/** Signal that a context menu was opened (arg0 true) or closed (arg0 false) */
	DEFINE_STRINGID (kContextMenuOpened, "ContextMenuOpened")

//////////////////////////////////////////////////////////////////////////////////////////////////
// TabView Messages
/** Messages sent by a TabView to the controller of its parameter. */
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Sent when tab menu icon clicked; arg[0]: parameter name (string), arg[1]: tab index (int), arg[2]: (IMenu) */
DEFINE_STRINGID (kTabViewTabMenu, "GUI.TabView.TabMenu")

/** Sent before dragging a tab; arg[0]: parameter name (string), arg[1]: dragged tab index (int), arg[2]: drag session (IDragSession) */
DEFINE_STRINGID (kTabViewBeforeDrag, "GUI.TabView.BeforeDrag")

/** Sent e.g. when receiving a drag session; arg[0]: parameter name (string), arg[1]: result (IDataTarget, out), args[2]: data (IUnknownList), arg[3]: drag session (IDragSession) */
DEFINE_STRINGID (kTabViewGetDataTarget, "GUI.TabView.GetDataTarget")

/** Sent to perform tab reordering; arg[0]: parameter name (string), arg[1]: tab index to move (int), arg[2]: target index (int) */
DEFINE_STRINGID (kTabViewReorder, "GUI.TabView.Reorder")

/** Sent to query if tab reordering is allowed; arg[0]: parameter name (string), arg[1]: tab index to move (int), arg[2]: target index (int); arg[3]: result value (IVariant as bool) */
DEFINE_STRINGID (kTabViewCanReorder, "GUI.TabView.CanReorder")

//////////////////////////////////////////////////////////////////////////////////////////////////
// Divider Messages
/** Messages sent by a Divider to the controller of its parameter. */
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Sent on a double click on a divder, controller sets arg[1] to true if handled; arg[0]: parameter (IParameter), arg[1]: result (bool, out) */
DEFINE_STRINGID (kDividerDoubleClick, "GUI.Divider.DoubleClick")

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Signals
} // namespace CCL

#endif // _ccl_controlsignals_h
