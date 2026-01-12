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
// Filename    : ccl/app/presets/presettrader.h
// Description : Preset Trader Component
//
//************************************************************************************************

#ifndef _ccl_presettrader_h
#define _ccl_presettrader_h

#include "ccl/app/component.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/gui/commanddispatch.h"

namespace CCL {

class PresetComponent;
class UnknownList;
interface IAttributeList;
interface IUnknownList;

//************************************************************************************************
// PresetTrader
/** Manages trading (import/export) of presets in foreign formats. */
//************************************************************************************************

class PresetTrader: public Component,
					public CommandDispatcher<PresetTrader>
{
public:
	DECLARE_CLASS_ABSTRACT (PresetTrader, Component)
	PresetTrader (PresetComponent& presetComponent);

	/** Import presets; optionally load the first one with a preset component. */
	static bool importPresets (const IUnknownList& urls, PresetComponent* targetComponent = nullptr);

	// Command Methods
	DECLARE_COMMANDS (PresetTrader)
	DECLARE_COMMAND_CATEGORY ("Presets", Component)
	bool onImportPreset (CmdArgs);
	bool onLoadPreset (CmdArgs);
	bool onExportPreset (CmdArgs);
	bool onExportPresetAs (CmdArgs);

	// Component
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

private:
	void collectFileTypes (bool forExport);
	void runFileSelector (UnknownList& urls, int fileSelectorType, StringRef title, const FileType* fileType, IAttributeList* metaInfo = nullptr);
	IUrl* selectFile (int fileSelectorType, StringRef title, const FileType* fileType = nullptr, IAttributeList* metaInfo = nullptr);
	bool exportPreset (StringRef title, const FileType* fileType);
	static String getExportTitle (const FileType& fileType);

	PresetComponent& presetComponent;
	FileTypeFilter fileTypes;
};

} // namespace CCL

#endif // _ccl_presettrader_h
