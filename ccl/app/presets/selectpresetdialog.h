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
// Filename    : ccl/app/presets/selectpresetdialog.h
// Description : Select Preset Dialog
//
//************************************************************************************************

#ifndef _ccl_selectpresetdialog_h
#define _ccl_selectpresetdialog_h

#include "ccl/app/browser/browser.h"

namespace CCL {

interface IPreset;
class FilePreviewComponent;
class DialogBox;

//************************************************************************************************
// SelectPresetDialog
// Shows a dialog with a browser for selecting a preset
//************************************************************************************************

class SelectPresetDialog: public Browser
{
public:	
	DECLARE_CLASS_ABSTRACT (SelectPresetDialog, Browser)

	SelectPresetDialog (IAttributeList* metaInfo, StringRef name, StringRef title = 0);

	/// add a preview component with a custom form for presets
	void setPreviewForm (StringID skinNameSpace, StringID infoFormName);

	/// add a custom preview component
	void addFilePreviewComponent (FilePreviewComponent* component);

	IPreset* runDialog (StringRef title);

protected:
	FilePreviewComponent* filePreviewComponent;
	SharedPtr<IPreset> selected;
	DialogBox* dialogBox;
	MutableCString formName;

	class PresetInfoComponent;

	IPreset* getSelectedPreset () const;
	virtual void updateFileInfo ();
	virtual void extendInfoComponent (Component& infoComponent){}

	// Browser
	void onNodeFocused (BrowserNode* node, bool inList) override; 
	bool openNode (BrowserNode* node) override;
	void restoreCurrentState () override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

} // namespace CCL

#endif // _ccl_selectpresetdialog_h
