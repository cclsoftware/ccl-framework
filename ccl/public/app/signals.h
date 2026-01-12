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
// Filename    : ccl/public/app/signals.h
// Description : CCL Signals
//
//************************************************************************************************

#ifndef _ccl_signals_h
#define _ccl_signals_h

#include "ccl/public/base/iunknown.h"

namespace CCL {
namespace Signals {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Signals::Application
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Signals related to Application */
DEFINE_STRINGID (kApplication, "CCL.Application")

	/** (IN) Feature is disabled. */
	DEFINE_STRINGID (kFeatureDisabled, "FeatureDisabled")

	/** (IN) Request application restart. args[0]: additional message, args[1]: already confirmed by user (tbool) */
	DEFINE_STRINGID (kRequestRestart, "RequestRestart")

	/** (IN) Request application shutdown. args[0]: IVariant, false if quit has been canceled */
	DEFINE_STRINGID (kRequestQuit, "RequestQuit")

//////////////////////////////////////////////////////////////////////////////////////////////////
// Signals::kPresetManager
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Signals related to Preset Manager */
DEFINE_STRINGID (kPresetManager, "CCL.PresetManager")

	/** (OUT) arg[0]: IPreset. A new preset was created. */
	DEFINE_STRINGID (kPresetCreated, "PresetCreated")

	/** (OUT) arg[0]: IPreset. A preset was removed. */
	DEFINE_STRINGID (kPresetRemoved, "PresetRemoved")

	/** (OUT) New presets might have been found, others might have disappeared (no individual kPresetCreated/kPresetRemoved messages are sent). */
	DEFINE_STRINGID (kPresetsRefreshed, "PresetsRefreshed")

	/** (OUT) arg[0]: preset class (string, e.g. plug-in class ID); arg[1]: sortPath (string). The subfolders for a preset class have changed, e.g. a folder was created or removed. */
	DEFINE_STRINGID (kPresetSubFolderAdded, "PresetSubFolderAdded")
	DEFINE_STRINGID (kPresetSubFolderRemoved, "PresetSubFolderRemoved")

	/** (OUT) arg[0]: preset class (string, e.g. plug-in class ID), arg[1]: favorite folder path of interest (string). Favorite presets for a preset class have changed, e.g. favorite state or favorite folder of a preset changed, or favorite folder was created, removed, renamed. */
	DEFINE_STRINGID (kPresetFavoritesChanged, "PresetFavoritesChanged")

	/** (OUT) arg[0]: IPreset. The user wants to "open" the preset. */
	DEFINE_STRINGID (kOpenPreset, "OpenPreset")

	/** (OUT) arg[0]: IUrl of a preset file; arg[1]: meta info as IAttributeList. The preset should be "revealed" (e.g. in a browser). */
	DEFINE_STRINGID (kRevealPreset, "RevealPreset")

	/** (OUT) arg[0]: IMenu; arg[1]: IObjectNode (preset component). A preset menu is about to be opened and may be extended. */
	DEFINE_STRINGID (kExtendPresetMenu, "ExtendPresetMenu")

	/** (OUT) args[0]: IUnknownList with presets. Asynchronous preset scan has completed. */
	DEFINE_STRINGID (kGetPresetsCompleted, "GetPresetsCompleted")

//////////////////////////////////////////////////////////////////////////////////////////////////
// Signals::kDocumentManager
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Signals related to Document Manager */
DEFINE_STRINGID (kDocumentManager, "CCL.DocumentManager")

	/** (IN) args[0]: IDocument (optional) */
	DEFINE_STRINGID (kDocumentDirty, "DocumentDirty")

//////////////////////////////////////////////////////////////////////////////////////////////////
// Signals::kEditorRegistry
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Signals related to Editors */
DEFINE_STRINGID (kEditorRegistry, "CCL.EditorRegistry")

	/** (OUT) args[0]: IObject of editor. */
	DEFINE_STRINGID (kEditorActivated, "EditorActivated")

//////////////////////////////////////////////////////////////////////////////////////////////////
// Signals::kEditing
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Signals related to Editing */
DEFINE_STRINGID (kEditing, "CCL.Editing")

	/** (OUT) args[0]: (IUnkown) data (e.g. an IUrl) to be inserted at current "insert position"; arg[1]: (bool) replace existing data. */
	DEFINE_STRINGID (kInsertData, "InsertData")

//////////////////////////////////////////////////////////////////////////////////////////////////
// Signals::kDebug
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Signals related to Debugging */
DEFINE_STRINGID (kDebug, "CCL.Debug")

	/** (OUT) args[1]: IMenu */
	DEFINE_STRINGID (kExtendDebugMenu, "ExtendDebugMenu")

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Signals
} // namespace CCL

#endif // _ccl_signals_h
