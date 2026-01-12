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
// Filename    : ccl/app/presets/presetcomponent.h
// Description : Preset Component
//
//************************************************************************************************

#ifndef _ccl_presetcomponent_h
#define _ccl_presetcomponent_h

#include "ccl/app/component.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/public/app/ipreset.h"
#include "ccl/public/gui/idatatarget.h"
#include "ccl/public/gui/commanddispatch.h"

namespace CCL {

class PresetBrowser;
class PresetMetaAttributes;
class ObjectPreset;
interface IAsyncOperation;
interface IDragHandler;
interface IMenu;
struct DragEvent;

//************************************************************************************************
// PresetManagementComponent
/** Basic skeleton component for preset management (does not use IPreset or related interfaces). */
//************************************************************************************************

class PresetManagementComponent: public Component,
								 public CommandDispatcher<PresetManagementComponent>
{
public:
	DECLARE_CLASS_ABSTRACT (PresetManagementComponent, Component)

	PresetManagementComponent (StringRef name);
	~PresetManagementComponent ();

	static bool askPresetName (String& presetName); ///< ask for preset name only
	static IAsyncOperation* askPresetNameAsync (StringRef initialName);
	static bool askRemovePreset (bool singular, StringRef description);
	static String getStorePresetTitle (bool follow = false);
	static String getLoadPresetTitle ();
	static String getDeletePresetTitle ();
	static String getRenamePresetTitle ();
	static String getPresetExistsMessage ();
	static String getDefaultPresetName ();
	static String getUpdatePresetTitle ();

	static bool isInGUIActionScope ();
	struct GUIActionScope;

	enum Options
	{
		kCanStoreAsDefault	= 1<<1,
		kCanReplacePreset	= 1<<2,
		kCanRenamePreset	= 1<<3,	///< (via menu)
		kCanDeletePreset	= 1<<4, ///< (via menu)
		kHasPresetFavorites = 1<<5,	///< supports preset favorites (must be implemented in derived class)

		kLastPresetManagementFlag = 5
	};

	PROPERTY_VARIABLE (int, options, Options)
	PROPERTY_FLAG (options, kCanStoreAsDefault, canStoreAsDefault)
	PROPERTY_FLAG (options, kCanReplacePreset, canReplacePreset)
	PROPERTY_FLAG (options, kCanRenamePreset, canRenamePreset)
	PROPERTY_FLAG (options, kCanDeletePreset, canDeletePreset)
	PROPERTY_FLAG (options, kHasPresetFavorites, hasPresetFavorites)

	enum StorePresetMode { kStoreNewPreset, kStoreDefaultPreset, kReplacePreset };

	virtual void resetCurrentPreset (StringRef presetName);
	virtual void extendPresetMenu (IMenu* menu);
	virtual tbool storePreset (int mode = kStoreNewPreset, StringID toFormat = nullptr) = 0;
	virtual bool renamePreset (bool checkOnly) = 0;
	virtual bool deletePreset (bool checkOnly) = 0;
	virtual bool isFactoryPreset (UrlRef presetUrl) const;

	IParameter* getPresetNameParam () const;
	virtual String getCurrentPresetName () const;
	virtual void setCurrentPresetName (StringRef presetName);
	UrlRef getCurrentPresetUrl () const;
	void takeDataFrom (const PresetManagementComponent& other);
	void enable (bool state);
	bool isEnabled () const;

	/// dirty state (target object has changed since preset was stored/restored)
	bool isDirty () const;
	void setDirty (bool state);

	// Component
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	// Command Methods
	DECLARE_COMMANDS (PresetManagementComponent)
	DECLARE_COMMAND_CATEGORY ("Presets", Component)
	bool onStorePreset (CmdArgs);
	bool onStorePresetAs (CmdArgs, VariantRef data);
	bool onStoreAsDefaultPreset (CmdArgs args);
	bool onRenamePreset (CmdArgs args);
	bool onDeletePreset (CmdArgs args);
	virtual bool onSetFavorite (CmdArgs args);

protected:
	Url currentPresetUrl;
	CCL::int64 restoreDeadTimeTicks;

	static constexpr CCL::int64 kRestoreDeadTime = 1000;

private:
	bool dirty;
	bool enabled;
	static bool inGUIAction;
};

//************************************************************************************************
// PresetComponent
/** Preset component using IPreset, IPresetFileHandler, etc. */
//************************************************************************************************

class PresetComponent: public PresetManagementComponent,
					   public IDataTarget
{
public:
	DECLARE_CLASS (PresetComponent, PresetManagementComponent)

	PresetComponent (IPresetMediator* presetMediator = nullptr);
	~PresetComponent ();

	bool askPresetInfo (PresetMetaAttributes& metaAttributes); ///< ask for preset name and other info
	static bool askPresetInfo (PresetMetaAttributes& metaAttributes, const Attributes* metaInfo);

	DECLARE_STRINGID_MEMBER (kFilePreset)
	DECLARE_STRINGID_MEMBER (kMemoryPreset)
	DECLARE_STRINGID_MEMBER (kMultiPreset)
	DECLARE_STRINGID_MEMBER (kBrowserOpened)
	DECLARE_STRINGID_MEMBER (kBrowserClosed)

	PROPERTY_MUTABLE_CSTRING (presetType, PresetType)

	enum Options
	{
		kHasPresetInfo            	 = 1<<(kLastPresetManagementFlag + 1),
		kMediatorInformsRestore   	 = 1<<(kLastPresetManagementFlag + 2), ///< preset mediator will call 'onPresetRestored' when a preset is recalled
		kBrowserAcceptOnMouseDown 	 = 1<<(kLastPresetManagementFlag + 3), ///< preset browser closes after first item is selected
		kDisableBrowserCommands   	 = 1<<(kLastPresetManagementFlag + 4), ///< preset browser should not handle commands (next / previous)
		kEnableDirtyTimeout       	 = 1<<(kLastPresetManagementFlag + 5), ///< ignore setDirty after restore during timeout
		kDisableBrowserContextMenus  = 1<<(kLastPresetManagementFlag + 6), ///< preset browser should not show item context menus
		kDisableBrowserAutoSelection = 1<<(kLastPresetManagementFlag + 7), ///< preset browser should not auto select items
		kEnableBrowserSearch		 = 1<<(kLastPresetManagementFlag + 8), ///< add a search component to the preset browser
		kAskBeforePresetDeletion     = 1<<(kLastPresetManagementFlag + 9), ///< ask before deleting a preset
		kEnableBrowserSourceFilter   = 1<<(kLastPresetManagementFlag + 10) ///< add a source filter to the preset browser (user / factory)
	};

	PROPERTY_FLAG (options, kHasPresetInfo, hasPresetInfo)
	PROPERTY_FLAG (options, kMediatorInformsRestore, mediatorInformsRestore)
	PROPERTY_FLAG (options, kBrowserAcceptOnMouseDown, browserAcceptOnMouseDown)
	PROPERTY_FLAG (options, kDisableBrowserCommands, disableBrowserCommands)
	PROPERTY_FLAG (options, kEnableDirtyTimeout, enableDirtyTimeout)
	PROPERTY_FLAG (options, kDisableBrowserContextMenus, disableBrowserContextMenus)
	PROPERTY_FLAG (options, kDisableBrowserAutoSelection, disableBrowserAutoSelection)
	PROPERTY_FLAG (options, kEnableBrowserSearch, enableBrowserSearch)
	PROPERTY_FLAG (options, kAskBeforePresetDeletion, askBeforePresetDeletion)
	PROPERTY_FLAG (options, kEnableBrowserSourceFilter, enableBrowserSourceFilter)

	tbool writePreset (UrlRef url, IAttributeList& metaInfo, IPresetFileHandler& handler, int notificationHint);
	IPreset* openPreset (UrlRef url);
	IPreset* openDefaultPreset ();
	tbool restorePreset (UrlRef url);
	tbool restorePreset (IPreset* preset); ///< 0: default preset
	Attributes* createMetaInfo ();
	void getCheckedPresetsFromBrowser (Vector<IPreset*>& checkedPresets) const;
	
	void finishObjectPreset (ObjectPreset& preset);
	String makeUniquePresetName (const FileType* fileType = nullptr);

	IUnknown* getTarget ();
	IPresetMediator* getPresetMediator () { return presetMediator; }
	void setPresetMediator (IPresetMediator* pm) { presetMediator = pm; }
	static IPresetFileHandler& getPresetHandler (StringID presetType);
	IPresetFileHandler& getPresetHandler ();
	PresetBrowser* getPresetBrowser () const;
	
	void onPresetRestored (const IPreset& preset);

	IDragHandler* createDragHandler (const DragEvent& event, IView* view);

	bool storePreset (IAttributeList& metaAttributes, int mode, StringID format);
	bool storePreset (IAttributeList& metaAttributes, int mode);
	
	// Treat private, these should be called by the preset browser
	void onPresetBrowserOpened ();
	void onPresetBrowserClosed (bool success);
	
	// IDataTarget
	tbool CCL_API canInsertData (const IUnknownList& data, IDragSession* session = nullptr, IView* targetView = nullptr, int insertIndex = -1) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;

	// PresetManagementComponent
	tbool storePreset (int mode = kStoreNewPreset, StringID toFormat = nullptr) override;
	void setCurrentPresetName (StringRef presetName) override;
	void resetCurrentPreset (StringRef presetName) override;
	void extendPresetMenu (IMenu* menu) override;
	bool renamePreset (bool checkOnly) override;
	bool deletePreset (bool checkOnly) override;
	bool onSetFavorite (CmdArgs args) override;
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	bool isFactoryPreset (UrlRef presetUrl) const override;

	CLASS_INTERFACE (IDataTarget, PresetManagementComponent)
	
private:
	IPresetMediator* presetMediator;
	PresetBrowser* presetBrowser;
	AutoPtr<Attributes> currentPresetMetaInfo;

	class PresetDragControl;

	void initMetaInfoFromCurrent (PresetMetaAttributes& metaAttributes);
	bool prepareStorePresetMetaData (PresetMetaAttributes& metaAttributes, int mode);
	
	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

//************************************************************************************************
// PresetManagementComponent::GUIActionScope
//************************************************************************************************

struct PresetManagementComponent::GUIActionScope: ScopedVar<bool>
{
	GUIActionScope (bool state = true)
	: ScopedVar<bool> (inGUIAction, state)
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline UrlRef PresetManagementComponent::getCurrentPresetUrl () const
{ return currentPresetUrl; }

inline bool PresetManagementComponent::isEnabled () const
{ return enabled; }

} // namespace CCL


#endif // _ccl_presetcomponent_h
