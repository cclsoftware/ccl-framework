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
// Filename    : ccl/app/presets/presetcomponent.cpp
// Description : Preset Component
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/presets/presetcomponent.h"
#include "ccl/app/presets/presettrader.h"
#include "ccl/app/presets/presetbrowser.h"
#include "ccl/app/presets/presetsystem.h"
#include "ccl/app/presets/presetfileexporter.h"
#include "ccl/app/presets/presetfileprimitives.h"
#include "ccl/app/presets/presetcollection.h"
#include "ccl/app/presets/preset.h"
#include "ccl/app/presets/memorypreset.h"
#include "ccl/app/presets/objectpreset.h"
#include "ccl/app/presets/presetdrag.h"
#include "ccl/app/presets/presetparam.h"

#include "ccl/app/controls/dragcontrol.h"
#include "ccl/app/paramcontainer.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/app/signals.h"

#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/collections/variantvector.h"
#include "ccl/public/plugins/stubobject.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/cclversion.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Presets")
	XSTRING (StoreAsDefaultPreset, "Store as Default Preset")
	XSTRING (AskStoreAsDefault, "Do you want to store the current settings as default preset?")
	XSTRING (StorePreset, "Store Preset")
	XSTRING (StoreAsXPreset, "Store %(1)")
	XSTRING (UpdatePreset, "Update Preset")
	XSTRING (LoadPreset, "Load Preset")
	XSTRING (DefaultPresetName, "default")
	XSTRING (DragPreset, "Click+Drag Preset")
	XSTRING (Preset_, "Preset:")
	XSTRING (DeletePreset, "Delete Preset")
	XSTRING (RenamePreset, "Rename Preset")
	XSTRING (DoYouWantToDeleteThisPreset, "Do you want to delete this preset?")
	XSTRING (DoYouWantToDeleteThesePresets, "Do you want to delete these presets?")
	XSTRING (PresetAlreadyExists, "A preset with this name already exists.")
END_XSTRINGS

//************************************************************************************************
// PresetMediatorStub
/** Stub class for IPresetMediator. */
//************************************************************************************************

class PresetMediatorStub: public StubObject,
						  public IPresetMediator
{
public:
	DECLARE_STUB_METHODS (IPresetMediator, PresetMediatorStub)

	// IPresetMediator
	IUnknown* CCL_API getPresetTarget () override
	{
		return static_cast<IStubObject*> (this);
	}

	StringRef CCL_API getDefaultPresetType () override
	{
		return String::kEmpty; // todo (if necessary)
	}

	tbool CCL_API getPresetMetaInfo (IAttributeList& metaInfo) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("getPresetMetaInfo", &metaInfo));
		return returnValue.asBool ();
	}

	String CCL_API makePresetName (tbool forExport) override
	{
		return String::kEmpty; // todo (if necessary)
	}

	tbool CCL_API storePreset (IPreset& preset) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("storePreset", &preset));
		return returnValue.asBool ();
	}

	tbool CCL_API restorePreset (const IPreset& preset) override
	{
		Variant returnValue;
		invokeMethod (returnValue, Message ("restorePreset", const_cast<IPreset*> (&preset)));
		return returnValue.asBool ();
	}
};

//************************************************************************************************
// InsertPresetDragHandler
//************************************************************************************************

class InsertPresetDragHandler: public PresetDragHandler
{
public:
	InsertPresetDragHandler (IView* view, PresetComponent& component)
	: PresetDragHandler (view),
	  component (component)
	{}

	tbool CCL_API afterDrop (const DragEvent& event) override
	{
		if(event.session.getResult () == IDragSession::kDropNone)
			return false;

		PresetComponent::GUIActionScope guiAction (true);
		component.insertData (getData (), &event.session);
		return PresetDragHandler::afterDrop (event);
	}

private:
	PresetComponent& component;
};

//************************************************************************************************
// PresetDragFilter
//************************************************************************************************

class PresetDragFilter: public ObjectFilter
{
public:
	PresetDragFilter (PresetComponent& component)
	{
		AutoPtr<IAttributeList> metaInfo = component.createMetaInfo ();
		PresetMetaAttributes metaAttributes (*metaInfo);
		metaAttributes.getClassID (targetClassID);
		metaAttributes.getAlternativeClassID (alternativeClassID);

		targetCategory = metaAttributes.getCategory ();
		targetSubCategory = metaAttributes.getSubCategory ();
	}

	// ObjectFilter
	tbool CCL_API matches (IUnknown* object) const override
	{
		UnknownPtr<IPreset> preset (object);
		if(preset)
			if(IAttributeList* metaInfo = preset->getMetaInfo ())
			{
				PresetMetaAttributes metaAttributes (*metaInfo);

				if(targetClassID.isValid ())
				{
					// when the target class is set, preset has to match the target
					// presets with classID are identified by classID
					UID cid;
					if(metaAttributes.getClassID (cid))
						return cid == targetClassID || (alternativeClassID.isValid () && cid == alternativeClassID);
					else
					{
						// presets with no classID are identified by gategory and subcategory
						// (this is the case when one preset type can be handled by multiple plugins)
						return targetCategory == metaAttributes.getCategory () && targetSubCategory == metaAttributes.getSubCategory ();
					}
				}
				else
				{
					// when the target class is not set, presets are identified by category
					return targetCategory == metaAttributes.getCategory ();
				}
			}
		return false;
	}

protected:
	UID targetClassID;
	UID alternativeClassID;
	String targetCategory;
	String targetSubCategory;
};

//************************************************************************************************
// PresetComponent::PresetDragControl
//************************************************************************************************

class PresetComponent::PresetDragControl: public DragControl
{
public:
	PresetDragControl (RectRef size, PresetComponent* presetComponent)
	: DragControl (size),
	  presetComponent (presetComponent)
	{
		setDragTooltip (XSTR (DragPreset));
		setModifier (KeyState::kCommand);
	}

	void prepareDrag (IDragSession& session) override
	{
		session.setSource (ccl_as_unknown (presetComponent));
		session.getItems ().add (presetComponent->getPresetMediator (), true);
	}

protected:
	SharedPtr<PresetComponent> presetComponent;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (PresetComponent, kFirstRun)
{
	REGISTER_STUB_CLASS (IPresetMediator, PresetMediatorStub)
	PresetParam::registerClass ();
	FilePromise::registerExporter<PresetFileExporter> ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum PresetTags
	{
		kPresetName = 100,
		kPresetMenu,
		kStorePreset,
		kUpdatePreset,
		kTransferring,
		kTransferProgress
	};
};

//************************************************************************************************
// PresetManagementComponent
//************************************************************************************************

bool PresetManagementComponent::inGUIAction = false;

DEFINE_CLASS (PresetManagementComponent, Component)
IMPLEMENT_COMMANDS (PresetManagementComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (PresetManagementComponent)
	DEFINE_COMMAND ("Presets", "Store Preset",				PresetManagementComponent::onStorePreset)
	DEFINE_COMMAND ("Presets", "Update Preset",				PresetManagementComponent::onStorePreset)
	DEFINE_COMMAND_ ("Presets", "Replace Preset",			PresetManagementComponent::onStorePreset, CommandFlags::kHidden) // old name of "Update Preset" (might appear in old macros)
	DEFINE_COMMAND ("Presets", "Store as Default Preset",	PresetManagementComponent::onStoreAsDefaultPreset)
	DEFINE_COMMAND_("Presets", "Rename",					PresetManagementComponent::onRenamePreset, CommandFlags::kHidden) // (only in own menu)
	DEFINE_COMMAND_("Presets", "Delete",					PresetManagementComponent::onDeletePreset, CommandFlags::kHidden) // (only in own menu)
	DEFINE_COMMAND_("Presets", "Set Favorite",				PresetManagementComponent::onSetFavorite, CommandFlags::kHidden) // (only in own menu)
END_COMMANDS (PresetManagementComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetManagementComponent::getStorePresetTitle (bool follow)
{
	if(follow)
		return String () << XSTR (StorePreset) << IMenu::strFollowIndicator;
	else
		return XSTR (StorePreset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetManagementComponent::getLoadPresetTitle ()
{
	return XSTR (LoadPreset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetManagementComponent::getDeletePresetTitle ()
{
	return XSTR (DeletePreset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetManagementComponent::getRenamePresetTitle ()
{
	return XSTR (RenamePreset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetManagementComponent::getDefaultPresetName ()
{
	return XSTR (DefaultPresetName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetManagementComponent::getPresetExistsMessage ()
{
	return XSTR (PresetAlreadyExists);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetManagementComponent::getUpdatePresetTitle ()
{
	return XSTR (UpdatePreset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManagementComponent::askPresetName (String& presetName)
{
	ParamContainer params;
	IParameter* param = params.addString (CSTR ("Name"));// todo: translate?
	param->fromString (presetName);

	if(DialogBox ()->runWithParameters (CCLSTR ("StorePresetDialog"), params, getStorePresetTitle ()) != DialogResult::kOkay)
		return false;

	presetName = param->getValue ().asString ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* PresetManagementComponent::askPresetNameAsync (StringRef presetName)
{
	AutoPtr<ParamContainer> params (NEW ParamContainer);
	IParameter* param = params->addString (CSTR ("Name"));// todo: translate?
	param->fromString (presetName);

	Promise promise (DialogBox ()->runWithParametersAsync (CCLSTR ("StorePresetDialog"), *params, getStorePresetTitle ()));
	return return_shared<IAsyncOperation> (promise.then ([=] (IAsyncOperation& op)
	{
		if(op.getResult ().asInt () == DialogResult::kOkay)
			op.setResult (Variant (param->getValue ().asString (), true));
		else
			op.setResult (Variant ());
	}));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManagementComponent::askRemovePreset (bool singular, StringRef description)
{
	String text;
	text << (singular ? XSTR (DoYouWantToDeleteThisPreset) : XSTR (DoYouWantToDeleteThesePresets));

	if(!description.isEmpty ())
		text << "\n\n" << description;

	return Alert::ask (text) == Alert::kYes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManagementComponent::isInGUIActionScope ()
{
	return inGUIAction;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetManagementComponent::PresetManagementComponent (StringRef name)
: Component (name),
  dirty (false),
  enabled (true),
  restoreDeadTimeTicks (0),
  options (kCanStoreAsDefault|kCanReplacePreset)
{
	IParameter* nameParam = paramList.addString (CSTR ("presetName"), Tag::kPresetName);
	nameParam->setValue (XSTR (DefaultPresetName));
	paramList.addMenu (CSTR ("presetMenu"), Tag::kPresetMenu)->setOutOfRange ();

	paramList.addCommand (CSTR ("Presets"), CSTR ("Store Preset"), CSTR ("storePreset"), Tag::kStorePreset);
	paramList.addCommand (CSTR ("Presets"), CSTR ("Update Preset"), CSTR ("updatePreset"), Tag::kUpdatePreset);

	paramList.addParam (CSTR ("transferring"), Tag::kTransferring);
	paramList.addFloat (0., 1., CSTR ("transferProgress"), Tag::kTransferProgress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetManagementComponent::~PresetManagementComponent ()
{
	signal (Message (kDestroyed));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* PresetManagementComponent::getPresetNameParam () const
{
	return paramList.byTag (Tag::kPresetName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetManagementComponent::getCurrentPresetName () const
{
	String presetName;
	paramList.byTag (Tag::kPresetName)->toString (presetName);
	if(isDirty () && presetName.lastChar () == '*')
		presetName.truncate (presetName.length () - 1);
	return presetName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManagementComponent::setCurrentPresetName (StringRef _presetName)
{
	String presetName (_presetName);
	if(isDirty () && !presetName.endsWith ("*"))
		presetName.append ("*");

	paramList.byTag (Tag::kPresetName)->fromString (presetName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManagementComponent::takeDataFrom (const PresetManagementComponent& other)
{
	setCurrentPresetName (other.getCurrentPresetName ());
	currentPresetUrl = other.currentPresetUrl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManagementComponent::enable (bool state)
{
	if(enabled != state)
	{
		enabled = state;

		paramList.byTag (Tag::kPresetMenu)->enable (state);
		paramList.byTag (Tag::kPresetName)->enable (state);
		paramList.byTag (Tag::kStorePreset)->enable (state);
		paramList.byTag (Tag::kUpdatePreset)->enable (state);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManagementComponent::isDirty () const
{
	return dirty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManagementComponent::setDirty (bool state)
{
	if(enabled == false && state == true)
		return;

	if(dirty != state)
	{
		paramList.byTag (Tag::kPresetName)->enable (!state);
		int64 now = System::GetSystemTicks ();
		if(now > restoreDeadTimeTicks)
		{
			dirty = state;
			if(dirty)
			{
				String name;
				paramList.byTag (Tag::kPresetName)->toString (name);
				if(name.lastChar () != '*')
				{
					name.append ("*");
					paramList.byTag (Tag::kPresetName)->setValue (name);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManagementComponent::resetCurrentPreset (StringRef presetName)
{
	setDirty (false);
	setCurrentPresetName (presetName);
	currentPresetUrl = Url::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetManagementComponent::extendPresetMenu (IMenu* menu)
{
	menu->addCommandItem (CommandWithTitle (CSTR ("Presets"), CSTR ("Store Preset"), XSTR (StorePreset)), this, true);

	// derived class that calls this should insert additional items here
	int insertPosition = menu->countItems ();

	if(canReplacePreset ())
		menu->addCommandItem (XSTR (UpdatePreset), CSTR ("Presets"), CSTR ("Update Preset"), this);
	if(canStoreAsDefault ())
		menu->addCommandItem (XSTR (StoreAsDefaultPreset), CSTR ("Presets"), CSTR ("Store as Default Preset"), this);

	if(canRenamePreset () || canDeletePreset ())
	{
		menu->addSeparatorItem ();
		if(canRenamePreset ())
			menu->addCommandItem (CommandWithTitle (CSTR ("Presets"), CSTR ("Rename"), XSTR (RenamePreset)), this, true);
		if(canDeletePreset ())
			menu->addCommandItem (XSTR (DeletePreset), CSTR ("Presets"), CSTR ("Delete"), this);
	}

	menu->setInsertPosition (insertPosition);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManagementComponent::isFactoryPreset (UrlRef presetUrl) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetManagementComponent::paramChanged (IParameter* param)
{ 
	if(param->getTag () == Tag::kTransferProgress)
	{
		if(!param->isEnabled ())
			paramList.byTag (Tag::kTransferring)->setValue (false);
	}

	return SuperClass::paramChanged (param); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetManagementComponent::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IParameter::kExtendMenu)
	{
		UnknownPtr<IParameter> param (subject);
		UnknownPtr<IMenu> menu (msg[0]);
		if(menu && param && param->getTag () == Tag::kPresetMenu)
		{
			extendPresetMenu (menu);

			if(countChildren () > 0)
				ArrayForEach (getChildren (), Component, c)
					c->notify (subject, msg);
				EndFor

			if(hasPresetFavorites ())
			{
				menu->addSeparatorItem ();
				menu->addCommandItem (BrowserStrings::strFavorite (), CSTR ("Presets"), CSTR ("Set Favorite"), this);
			}

			// allow external components to extend the menu further
			SignalSource (Signals::kPresetManager).signal (Message (Signals::kExtendPresetMenu, msg[0], this->asUnknown ()));
		}
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManagementComponent::onStorePreset (CmdArgs args)
{
	bool isReplace = args.name.contains ("Update") || args.name.contains ("Replace");
	if(args.checkOnly ())
		return !isReplace || !isFactoryPreset (currentPresetUrl);
	else
		storePreset (isReplace ? kReplacePreset : kStoreNewPreset);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManagementComponent::onStorePresetAs (CmdArgs args, VariantRef data)
{
	if(!args.checkOnly ())
	{
		MutableCString toFormat (data.asString ());
		ASSERT (!toFormat.isEmpty ())
		storePreset (kStoreNewPreset, toFormat);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManagementComponent::onStoreAsDefaultPreset (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		if(Alert::ask (XSTR (AskStoreAsDefault), Alert::kYesNo) == Alert::kYes)
			storePreset (kStoreDefaultPreset);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManagementComponent::onRenamePreset (CmdArgs args)
{
	if(!args.checkOnly ())
		if(System::GetDesktop ().closePopupAndDeferCommand (this, args))
			return true;

	return renamePreset (args.checkOnly ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManagementComponent::onDeletePreset (CmdArgs args)
{
	return deletePreset (args.checkOnly ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManagementComponent::onSetFavorite (CmdArgs args)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManagementComponent::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	a.get (currentPresetUrl, "url");
	setDirty (a.getBool ("dirty"));
	setCurrentPresetName (a.getString ("pname"));	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetManagementComponent::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	if(!currentPresetUrl.isEmpty ())
		a.set ("url", currentPresetUrl);
	a.set ("pname", getCurrentPresetName ());
	a.set ("dirty", isDirty ());
	return true;
}

//************************************************************************************************
// PresetComponent
//************************************************************************************************

bool PresetComponent::askPresetInfo (PresetMetaAttributes& metaAttributes)
{
	AutoPtr<Attributes> metaInfo (createMetaInfo ());
	return askPresetInfo (metaAttributes, metaInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetComponent::askPresetInfo (PresetMetaAttributes& metaAttributes, const Attributes* metaInfo)
{
	ParamContainer params;
	IParameter* name = params.addString (CSTR ("Name"));// todo: translate?
	IParameter* description = params.addString (CSTR ("Description"));
	IParameter* subFolder = params.addString (CSTR ("Subfolder"));
	IParameter* subFolderListParam = params.addList (CSTR ("subfolderList"));
	UnknownPtr<IListParameter> subFolderList (subFolderListParam);

	name->fromString (metaAttributes.getTitle ());
	description->fromString (metaAttributes.getDescription ());
	subFolder->fromString (metaAttributes.getSubFolder ());
	
	// collect subfolder values occurring for this class meta info
	VariantVector subFolders;
	System::GetPresetManager ().collectSubFolders (subFolders, metaInfo);
	for(auto& v : subFolders)
	{
		String subFolderString (v.asString ());
		if(!subFolderString.isEmpty ())
			subFolderList->appendString (subFolderString);
	}
	
	if(subFolderList->selectValue (metaAttributes.getSubFolder ()) == false)
		subFolderListParam->setOutOfRange (true); 
	subFolderListParam->setSignalAlways (true); // needed if there is only one folder

	ITheme* theme = RootComponent::instance ().getTheme ();
	ASSERT (theme != nullptr)
	IView* dialogView = theme ? theme->createView ("CCL/StorePresetDialog", params.asUnknown ()) : nullptr;
	if(!dialogView && System::IsInMainModule () == false)
	{
		// use host form
		if(ITheme* appTheme = System::GetThemeManager ().getApplicationTheme ())
			dialogView = appTheme->createView ("CCL/StorePresetDialog", params.asUnknown ());
	}

	int answer = dialogView
		? DialogBox ()->runDialog (dialogView)
		: DialogBox ()->runWithParameters (CCLSTR ("StorePresetDialog"), params, getStorePresetTitle ());

	if(answer != DialogResult::kOkay)
		return false;

	metaAttributes.setTitle (name->getValue ().asString ());
	metaAttributes.setDescription (description->getValue ().asString ());
	metaAttributes.setSubFolder (subFolder->getValue ().asString ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (PresetComponent, PresetManagementComponent)
DEFINE_CLASS_UID (PresetComponent, 0x54467fa5, 0xa98d, 0x4edd, 0x9d, 0xe0, 0xc6, 0x35, 0x75, 0xd2, 0x8a, 0x7a)
DEFINE_CLASS_NAMESPACE (PresetComponent, "Host")

DEFINE_STRINGID_MEMBER_ (PresetComponent, kFilePreset,   CCL_MIME_TYPE "-preset")
DEFINE_STRINGID_MEMBER_ (PresetComponent, kMemoryPreset, CCL_MIME_TYPE "-memorypreset")
DEFINE_STRINGID_MEMBER_ (PresetComponent, kMultiPreset,  CCL_MIME_TYPE "-multipreset")
DEFINE_STRINGID_MEMBER_ (PresetComponent, kBrowserOpened, "presetBrowserOpened")
DEFINE_STRINGID_MEMBER_ (PresetComponent, kBrowserClosed, "presetBrowserClosed")

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetComponent::PresetComponent (IPresetMediator* presetMediator)
: PresetManagementComponent (CCLSTR ("Presets")),
  presetMediator (presetMediator),
  presetType (kFilePreset),
  presetBrowser (nullptr)
{
	hasPresetInfo (true);
	askBeforePresetDeletion (true);
	
	addComponent (NEW PresetTrader (*this));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetComponent::~PresetComponent ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* PresetComponent::getTarget ()
{
	ASSERT (presetMediator)
	return presetMediator ? presetMediator->getPresetTarget () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API PresetComponent::getObject (StringID name, UIDRef classID)
{
	if(name == "dataTarget")
		return this->asUnknown ();

	if(name == "PresetBrowser")
	{
		if(!presetBrowser)
		{
			presetBrowser = NEW PresetBrowser (*this);
			if(browserAcceptOnMouseDown ())
				presetBrowser->acceptOnMouseDown (true);
			if(disableBrowserCommands ())
				presetBrowser->setCommandsDisabled (true);
			if(enableBrowserSearch ())
				presetBrowser->addSearch ();
			if(enableBrowserSourceFilter ())
				presetBrowser->addSourceFilter ();
			
			StyleFlags style (presetBrowser->getTreeStyle ());
			if(disableBrowserContextMenus ())
				style.setCustomStyle (Styles::kItemViewBehaviorNoContextMenu, true);
			if(disableBrowserAutoSelection ())
				style.setCustomStyle (Styles::kItemViewBehaviorAutoSelect, false);
			presetBrowser->setTreeStyle (style);
			
			addComponent (presetBrowser);
		}
		return presetBrowser->asUnknown ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes* PresetComponent::createMetaInfo ()
{
	PackageInfo* metaInfo = NEW PackageInfo;
	if(presetMediator)
		presetMediator->getPresetMetaInfo (*metaInfo);
	return metaInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetComponent::getCheckedPresetsFromBrowser (Vector<IPreset*>& checkedPresets) const
{
	if(presetBrowser != nullptr)
		presetBrowser->getCheckedPresets (checkedPresets);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetComponent::finishObjectPreset (ObjectPreset& preset)
{
	// add information that only we have, to allow a more complete transfer
	String presetName = getCurrentPresetName ();
	if(presetName.isEmpty () == false)
		preset.setName (presetName);
	preset.setPresetUrl (getCurrentPresetUrl ());
	preset.modified (isDirty ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/IPresetFileHandler& PresetComponent::getPresetHandler (StringID presetType)
{
	if(presetType == kFilePreset && PresetPackageHandler::peekInstance ())
		return PresetPackageHandler::instance ();

	if(presetType == kMemoryPreset && MemoryPresetHandler::peekInstance ())
		return MemoryPresetHandler::instance ();

	if(presetType == kMultiPreset && PresetCollectionHandler::peekInstance ())
		return PresetCollectionHandler::instance ();
	
	IPresetFileHandler* handler = System::GetPresetFileRegistry ().getHandlerForMimeType (presetType);
	ASSERT (handler)
	if(handler)
		return *handler;
	
	return PresetPackageHandler::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetBrowser* PresetComponent::getPresetBrowser () const
{
	return presetBrowser;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPresetFileHandler& PresetComponent::getPresetHandler ()
{
	return getPresetHandler (presetType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PresetComponent::makeUniquePresetName (const FileType* fileType)
{
	AutoPtr<PackageInfo> metaInfo (NEW PackageInfo);
	presetMediator->getPresetMetaInfo (*metaInfo);

	PresetMetaAttributes metaAttributes (*metaInfo);
	initMetaInfoFromCurrent (metaAttributes); // e.g. subFolder must be considered to determine if preset name exists

	String name = getCurrentPresetName ();
	if(name.isEmpty ())
		name = presetMediator->makePresetName (false); // maybe the presetMediator can suggest something other than "Preset"

	// use current preset name, remove the dirty marker
	return PresetFilePrimitives::makeUniquePresetName (name, metaInfo, fileType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetComponent::onPresetBrowserOpened ()
{
	if(presetBrowser != nullptr)
		presetBrowser->restoreCurrentState ();
	
	signalDeep (Message (kBrowserOpened));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetComponent::onPresetBrowserClosed (bool success)
{
	signalDeep (Message (kBrowserClosed, success));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetComponent::initMetaInfoFromCurrent (PresetMetaAttributes& metaAttributes)
{
	// 1. try attribute list we kept from last store / restore
	SharedPtr<IAttributeList> source (currentPresetMetaInfo);

	// 2. try to open current preset file
	if(!source)
	{
		AutoPtr<IPreset> currentPreset (openPreset (currentPresetUrl));
		if(currentPreset)
			source = currentPreset->getMetaInfo ();
	}

	if(source)
	{
		PresetMetaAttributes sourceAttribs (*source);
		if(metaAttributes.getDescription ().isEmpty ())
			metaAttributes.setDescription (sourceAttribs.getDescription ());

		String subFolder (sourceAttribs.getSubFolder ());
		if(subFolder.isEmpty ())
			subFolder = PresetFilePrimitives::determineRelativeSubFolder (getPresetHandler (), *source, currentPresetUrl);

		metaAttributes.setSubFolder (subFolder);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetComponent::prepareStorePresetMetaData (PresetMetaAttributes& metaAttributes, int mode)
{
	String presetName = getCurrentPresetName ();
	if(mode == kReplacePreset && presetName.isEmpty ())
		mode = kStoreNewPreset;
	
	if(mode == kReplacePreset)
	{
		if(!presetName.isEmpty ())
		{
			metaAttributes.setTitle (presetName);
			return true;
		}
		else
			mode = kStoreNewPreset;
	}
	
	if(mode == kStoreNewPreset)
	{
		if(hasPresetInfo ())
			return askPresetInfo (metaAttributes);
		else
		{
			presetName = makeUniquePresetName (&PresetComponent::getPresetHandler (presetType).getFileType ());
			bool needsNewName = isDirty () || getCurrentPresetName () != presetName;
			if(needsNewName && !askPresetName (presetName))
				return false;
			
			metaAttributes.setTitle (presetName);
			return true;
		}
	}
	
	if(mode == kStoreDefaultPreset)
	{
		metaAttributes.setTitle (PresetFilePrimitives::kDefaultPresetFileName);
		metaAttributes.setSubFolder (String::kEmpty);
		return true;
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetComponent::storePreset (IAttributeList& metaAttributes, int mode, StringID format)
{
	if(isEnabled () == false)
		return false;
	
	PresetMetaAttributes presetMetaAttributes (metaAttributes);
	String name = presetMetaAttributes.getTitle ();
	ASSERT (!name.isEmpty ())
	if(name.isEmpty ())
		return false;
	
	// when replacing the current preset, keep its handler if it's a "VIP"
	IPresetFileHandler* currentHandler = nullptr;
	if(mode == kReplacePreset)
	{
		const FileType& fileType = getCurrentPresetUrl ().getFileType ();
		if(fileType.isValid ())
		{
			IPresetFileHandler* handler = System::GetPresetFileRegistry ().getHandlerForFileType (fileType);
			if(handler && handler->getFlags () & IPresetFileHandler::kIsVIPFormat)
				currentHandler = handler;
		}
	}
	IPresetFileHandler& handler = currentHandler ? *currentHandler : getPresetHandler (format);
	
	Url newUrl;
	if(!handler.getWriteLocation (newUrl, &presetMetaAttributes.getList ()))
		return false;
	
	PresetFilePrimitives::descendSubFolder (newUrl, presetMetaAttributes.getList ());
	PresetFilePrimitives::descendPresetName (newUrl, presetMetaAttributes.getTitle (), handler, false);

	if(mode == kReplacePreset)
	{
		// announce removal of the old preset (can be another file, when a factory preset gets replaced (hidden) by a user preset)
		AutoPtr<IPreset> currentPreset (openPreset (currentPresetUrl));
		if(currentPreset)
			System::GetPresetManager ().onPresetRemoved (currentPresetUrl, *currentPreset);
	}
	
	int notificationHint = mode == kStoreDefaultPreset? IPresetNotificationSink::kStoreDefaultPreset : IPresetNotificationSink::kStorePreset;
	if(!writePreset (newUrl, presetMetaAttributes.getList (), handler, notificationHint))
		return false;
	
	currentPresetUrl = newUrl;
	
	currentPresetMetaInfo = NEW Attributes;
	currentPresetMetaInfo->copyFrom (presetMetaAttributes.getList ());
	
	setDirty (false);
	setCurrentPresetName (presetMetaAttributes.getTitle ()); // from metaAttributes

	if(presetBrowser && presetBrowser->getTreeView ())
		presetBrowser->selectCurrentPreset ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetComponent::storePreset (IAttributeList& metaAttributes, int mode)
{
	return storePreset (metaAttributes, mode, presetType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool PresetComponent::storePreset (int mode, StringID toFormat)
{
	ASSERT (mode != kReplacePreset || toFormat.isEmpty ())
	if(mode == kReplacePreset && !toFormat.isEmpty ())
		return false;
	
	AutoPtr<Attributes> metaInfo (createMetaInfo ());
	PresetMetaAttributes metaAttributes (*metaInfo);
	initMetaInfoFromCurrent (metaAttributes);
	if(!prepareStorePresetMetaData (metaAttributes, mode))
		return false;

	return storePreset (metaAttributes.getList (), mode, !toFormat.isEmpty () ? toFormat : presetType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool PresetComponent::writePreset (UrlRef url, IAttributeList& metaInfo, IPresetFileHandler& handler, int notificationHint)
{
	return PresetFilePrimitives::writePreset (url, metaInfo, handler, *presetMediator, notificationHint);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* PresetComponent::openPreset (UrlRef url)
{
	return System::GetPresetManager ().openPreset (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool PresetComponent::restorePreset (UrlRef url)
{
	AutoPtr<IPreset> preset (openPreset (url));
	return preset ? restorePreset (preset) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* PresetComponent::openDefaultPreset ()
{
	AutoPtr<IAttributeList> metaInfo (createMetaInfo ());
	IPresetFileHandler& handler = getPresetHandler ();
	return System::GetPresetManager ().openDefaultPreset (handler, metaInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool PresetComponent::restorePreset (IPreset* preset)
{
	paramList.byTag (Tag::kTransferring)->setValue (false);
	paramList.byTag (Tag::kTransferProgress)->setValue (0);

	AutoPtr<IPreset> defaultPreset;
	if(preset == nullptr)
	{
		// try to open default preset
		if(preset = openDefaultPreset ())
			defaultPreset = preset;
	}

	if(!preset)
		return false;

	Url presetUrl;
	preset->getUrl (presetUrl);
	if(!System::GetFileSystem ().isLocalFile (presetUrl))
	{
		paramList.byTag (Tag::kTransferring)->setValue (true);
		paramList.byTag (Tag::kTransferProgress)->enable (true);
		preset->restore (this->asUnknown ()); // starts transfer of a remote preset
		return false; // (not restored yet)
	}

	ASSERT (presetMediator)
	if(!presetMediator)
		return false;

	// notify target (before)
	UnknownPtr<IPresetNotificationSink> targetNotify (presetMediator->getPresetTarget ());
	if(targetNotify)
		targetNotify->onPresetChanging (*preset, true);

	// apply preset
	tbool restored = presetMediator->restorePreset (*preset);
	if(restored && mediatorInformsRestore () == false)
		onPresetRestored (*preset);
	
	// notify target (after)
	if(targetNotify)
		targetNotify->onPresetChanging (*preset, false);

	// update preset browser
	if(isInGUIActionScope () == false && presetBrowser && presetBrowser->getTreeView ())
		presetBrowser->selectCurrentPreset ();

	return restored;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetComponent::onPresetRestored (const IPreset& preset)
{
	Url presetUrl;
	preset.getUrl (presetUrl);

	// keep current url and meta info
	currentPresetUrl = presetUrl;

	currentPresetMetaInfo = NEW Attributes ();
	if(IAttributeList* metaInfo = preset.getMetaInfo ())
	{
		currentPresetMetaInfo->copyFrom (*metaInfo);

		// ignore any subFolder saved in preset file metainfo (loaded directly from file in PresetFile::restore)
		// we (PresetStore) are only interested in the subFolder relative to the current location, not where it once was saved
		PresetMetaAttributes (*currentPresetMetaInfo).setSubFolder (String::kEmpty);
	}

	// set name value
	String name (preset.getPresetName ());
	if(name.isEmpty ())
		name = XSTR (DefaultPresetName);
	setDirty (preset.isModified () != 0);
	setCurrentPresetName (name);
	paramList.byTag (Tag::kPresetName)->enable (!isFactoryPreset (presetUrl));

	if(enableDirtyTimeout ())
		restoreDeadTimeTicks = System::GetSystemTicks () + kRestoreDeadTime;

	// notify target (after)
	UnknownPtr<IPresetNotificationSink> targetNotify (presetMediator->getPresetTarget ());	
	if(targetNotify)
		targetNotify->onPresetRestored (preset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetComponent::isFactoryPreset (UrlRef presetUrl) const
{
	Url factoryRoot;
	PresetPackageHandler::instance ().getFactoryRootFolder (factoryRoot);
	return factoryRoot.contains (presetUrl);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetComponent::setCurrentPresetName (StringRef presetName)
{
	SuperClass::setCurrentPresetName (presetName);

	if(presetMediator)
	{
		UnknownPtr<IPresetNotificationSink> targetNotify (presetMediator->getPresetTarget ());	
		if(targetNotify)
			targetNotify->onCurrentPresetNameChanged (getCurrentPresetName ());
	}			
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetComponent::resetCurrentPreset (StringRef presetName)
{
	SuperClass::resetCurrentPreset (presetName);
	currentPresetMetaInfo = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetComponent::onSetFavorite (CmdArgs args)
{
	if(hasPresetFavorites ())
	{
		AutoPtr<IPreset> preset (openPreset (currentPresetUrl));
		if(!preset)
			return false;

		tbool isFavorite = System::GetPresetManager ().isFavorite (*preset);

		if(args.checkOnly ())
		{
			UnknownPtr<IMenuItem> menuItem (args.invoker);
			if(menuItem)
				menuItem->setItemAttribute (IMenuItem::kItemChecked, isFavorite);
		}
		else
			System::GetPresetManager ().setFavorite (*preset, !isFavorite);

		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API PresetComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "DragControl")
		return *NEW PresetDragControl (bounds, this);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetComponent::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "mediator")
	{
		UnknownPtr<IPresetMediator> mediator (var);
		ASSERT (mediator.isValid ())
		this->presetMediator = mediator; // reference count???
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetComponent::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kPresetName)
	{
		AutoPtr<IPreset> currentPreset (System::GetPresetManager ().openPreset (currentPresetUrl));
		if(currentPreset.isValid ())
		{
			StringRef newName = param->getValue ().asString ();
			System::GetPresetManager ().renamePreset (*currentPreset, newName, &currentPresetUrl);
		}
		else if(!getCurrentPresetName ().isEmpty ())
		{
			storePreset (kStoreNewPreset);
			return true;
		}
	}

	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* PresetComponent::createDragHandler (const DragEvent& event, IView* view)
{
	int dropResult = IDragSession::kDropCopyReal;
	String title (XSTR (LoadPreset));

	// reject presets dragged by ourselves, but use the handler to show what is beeing dragged
	if(event.session.getSource () == this->asUnknown ())
	{
		dropResult = IDragSession::kDropNone;
		title = (XSTR (Preset_));
	}

	PresetDragFilter filter (*this);
	AutoPtr<PresetDragHandler> dragHandler (NEW InsertPresetDragHandler (view, *this));
	if(dragHandler->prepare (event.session.getItems (), &filter, title))
	{
		event.session.setResult (dropResult);
		dragHandler->retain ();
		return dragHandler;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetComponent::canInsertData (const IUnknownList& data, IDragSession* session, IView* targetView, int insertIndex)
{
	if(isEnabled () == false)
		return false;
			
	int dropResult = IDragSession::kDropCopyReal;
	String title (XSTR (LoadPreset));

	// reject presets dragged by ourselves, but use the handler to show what is beeing dragged
	if(session && session->getSource () == this->asUnknown ())
	{
		dropResult = IDragSession::kDropNone;
		title = (XSTR (Preset_));
	}

	PresetDragFilter filter (*this);
	AutoPtr<PresetDragHandler> dragHandler (NEW InsertPresetDragHandler (targetView, *this));
	if(dragHandler->prepare (data, &filter, title))
	{
		if(session)
		{
			session->setDragHandler (dragHandler);
			session->setResult (dropResult);
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetComponent::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	if(isEnabled () == false)
		return false;
	
	UnknownPtr<IPreset> preset (data.getFirst ());
	if(preset)
	{
		restorePreset (preset);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetComponent::extendPresetMenu (IMenu* menu)
{
	SuperClass::extendPresetMenu (menu);
			
	// additional menu items for important formats
	FileTypeFilter additionalFormats;
	System::GetPresetFileRegistry ().collectFileTypes (additionalFormats, getTarget (), IPresetFileHandler::kIsVIPFormat);
	if(!additionalFormats.getContent ().isEmpty ())
	{
		const FileType& defaultFormat = getPresetHandler ().getFileType ();
		VectorForEach (additionalFormats.getContent (), FileType, fileType)
			if(fileType == defaultFormat)
				continue;

			String title;
			title.appendFormat (XSTR (StoreAsXPreset), fileType.getDescription ());
			Variant data (fileType.getMimeType ());
			data.share ();
			menu->addCommandItem (CommandWithTitle (CSTR ("Presets"), CSTR ("Store Preset As"), title),
				CommandDelegate<PresetComponent>::make (this, &PresetComponent::onStorePresetAs, data), true);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetComponent::renamePreset (bool checkOnly)
{
	if(isFactoryPreset (currentPresetUrl))
		return false;

	if(!checkOnly)
	{
		// the actual rename is done if the presetName parameter is edited (see paramChanged)
		UnknownPtr<ISubject> subject = paramList.byTag (Tag::kPresetName);
		if(subject)
			subject->signal (Message (IParameter::kRequestFocus));
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetComponent::deletePreset (bool checkOnly)
{
	if(System::GetFileSystem ().fileExists (currentPresetUrl))
	{
		if(isFactoryPreset (currentPresetUrl))
			return false;

		if(checkOnly)
			return true;

		if(!askBeforePresetDeletion () || askRemovePreset (true, String::kEmpty))
		{
			AutoPtr<IPreset> currentPreset (System::GetPresetManager ().openPreset (currentPresetUrl));
			if(currentPreset.isValid ())
			{
				System::GetPresetManager ().removePreset (*currentPreset);
				resetCurrentPreset (String::kEmpty);
				return true;
			}
		}
	}

	return false;
}
