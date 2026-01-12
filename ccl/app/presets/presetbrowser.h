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
// Filename    : ccl/app/presets/presetbrowser.h
// Description : Preset Browser
//
//************************************************************************************************

#ifndef _ccl_presetbrowser_h
#define _ccl_presetbrowser_h

#include "ccl/app/browser/browser.h"

#include "ccl/public/gui/framework/popupselectorclient.h"

namespace CCL {

interface IPreset;
class PresetComponent;
class FileTypeFilter;

namespace Browsable {
class PresetNode; }

//************************************************************************************************
// PresetBrowser
//************************************************************************************************

class PresetBrowser: public Browser,
					 public CommandDispatcher<PresetBrowser>,
					 public PopupSelectorClient
{
public:
	DECLARE_CLASS_ABSTRACT (PresetBrowser, Browser)

	enum FilterType
	{
		kFilterFactory = 0,
		kFilterUser,
		
		kFilterTypeCount
	};
	
	PresetBrowser (PresetComponent& presetComponent);
	~PresetBrowser ();

	PROPERTY_BOOL (commandsDisabled, CommandsDisabled)

	void getCheckedPresets (Vector<IPreset*>& checkedPresets) const;
	void filterPresets (FilterType type);
	void selectCurrentPreset ();

	// Browser
	tbool CCL_API paramChanged (IParameter* param) override;
	void onViewAttached (IItemView* itemView) override;
	void onNodeFocused (BrowserNode* node, bool inList) override;
	bool onEditNode (BrowserNode& node, StringID columnID, const IItemModel::EditInfo& info) override;
	void restoreCurrentState () override;
	void addSearch ();
	void addSourceFilter ();
	void onNodeRemoved (BrowserNode* node) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// PopupSelectorClient
	IView* CCL_API createPopupView (SizeLimit& limits) override;
	void CCL_API attached (IWindow& popupWindow) override;
	bool hasPopupResult () override;
	Result CCL_API onMouseDown (const MouseEvent& event, IWindow& popupWindow) override;
	Result CCL_API onKeyDown (const KeyEvent& event) override;
	void CCL_API onPopupClosed (Result result) override;
	tbool CCL_API mouseWheelOnSource (const MouseWheelEvent& event, IView* source) override;

	// Command Methods
	DECLARE_COMMANDS (PresetBrowser)
	DECLARE_COMMAND_CATEGORY ("Presets", Browser)
	bool onNextPreset (CmdArgs);
	bool onPrevPreset (CmdArgs);

	CLASS_INTERFACE (IPopupSelectorClient, Browser)

private:
	class PresetFilter;
	
	PresetComponent& presetComponent;
	IPreset* selectedPreset;
	IPreset* loadedPreset;
	FileTypeFilter* fileTypes;

	PresetBrowser ();
	BrowserNode* getPresetRootNode ();
	IAttributeList* createMetaInfo ();
	void updateMetaInfo (IPreset* preset);
	void loadSelectedPreset (bool force);
	bool selectNextPreset (int increment, bool checkOnly);
	Browsable::PresetNode* findPresetNodeWithUrl (UrlRef url) const;
	
	typedef void (*PresetNodeEditFunc) (Browsable::PresetNode*);
	void forEachPresetNode (PresetNodeEditFunc visitFunction);
};

} // namespace CCL

#endif // _ccl_presetbrowser_h
