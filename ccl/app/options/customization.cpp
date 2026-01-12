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
// Filename    : ccl/app/options/customization.h
// Description : Customization Component
//
//************************************************************************************************

#include "ccl/app/options/customization.h"

#include "ccl/app/params.h"
#include "ccl/app/presets/presetcomponent.h"
#include "ccl/app/components/filerenamer.h"

#include "ccl/app/documents/documentmanager.h"
#include "ccl/app/documents/document.h"

#include "ccl/base/singleton.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/iform.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/iwindowmanager.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/paramlist.h"
#include "ccl/public/guiservices.h"

namespace CCL {

//************************************************************************************************
// CustomizationSettings
//************************************************************************************************

class CustomizationSettings: public XmlSettings,
							 public Singleton<CustomizationSettings>
{
public:
	using Singleton<CustomizationSettings>::instance;

	static constexpr int kVersion = 2; // was 1 during early development (factory presets were also stored)

	CustomizationSettings ()
	: XmlSettings (CCLSTR ("customization"), kVersion)
	{
		restore ();
	}

	Container* getFactoryPresets (StringRef name) const
	{
		return getFactorySettings ().getAttributes (name).getObject<AttributeQueue> ("presets");
	}

	int countFactoryPresets (StringRef name) const
	{
		Container* factoryPresets = getFactoryPresets (name);
		return factoryPresets ? factoryPresets->count () : 0;
	}

	Iterator* newFactoryPresetsIterator (StringRef name) const
	{
		if(Container* factoryPresets = getFactoryPresets (name))
			return makeResolvingIterator (factoryPresets->newIterator (), [] (Object* obj)
			{
				auto attribute = ccl_cast<Attribute> (obj);
				auto preset = attribute ? unknown_cast<CustomizationPreset> (attribute->getValue ()) : nullptr;
				return preset;
			});

		return NEW NullIterator;
	}

	String getDefaultFactoryPresetName (StringRef name)
	{
		return getFactorySettings ().getAttributes (name).getString ("selected");
	}

	void selectDefaultFactoryPreset (StringRef name)
	{
		// take selected preset specified in factory settings
		getAttributes (name).set ("selected", getDefaultFactoryPresetName (name));
	}

	XmlSettings& getFactorySettings () const
	{
		if(!factorySettings)
		{
			ResourceUrl url (getName ());
			url.setFileType (CustomizationSettings::getFileType (), true);

			factorySettings = NEW XmlSettings;
			factorySettings->checkName (false);
			factorySettings->setPath (url);

			factorySettings->restore ();

			// init factory presets: set read only, translate name
			AutoPtr<Iterator> sectionsIter (factorySettings->getSections ());
			for(auto section : iterate_as<Settings::Section> (*sectionsIter))
			{
				AutoPtr<Iterator> presetIter (newFactoryPresetsIterator (section->getPath ()));
				for(auto preset : iterate_as<CustomizationPreset> (*presetIter))
				{
					preset->setReadOnly (true);

					String localizedName (TRANSLATE2 (CSTR ("Customization Preset"), preset->getName ()));
					if(!localizedName.isEmpty ())
						preset->setName (localizedName);
				}
			}
		}
		return *factorySettings;
	}

	private:
		mutable AutoPtr<XmlSettings> factorySettings;

		using SuperClass = XmlSettings;
};

} // namespace CCL

using namespace CCL;

DEFINE_SINGLETON (CustomizationSettings)

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Customization")
	XSTRING (Customize, "Customize")
	XSTRING (Customization, "Customization")
	XSTRING (EditCustomization, "Edit Customization")
	XSTRING (Store, "Store")
	XSTRING (Rename, "Rename")
	XSTRING (Delete, "Delete")
	XSTRING (ResetAllPresets, "Delete User Customization")
	XSTRING (DoYouWantToDeleteAllPresets, "Do you want delete all user customization presets?")
	XSTRING (UserPresetName, "User-defined")
	XSTRING (DoYouWantToKeepCustomization, "The customization was changed to %(1). Do you want to keep the new customization?")
	XSTRING (Keep, "Keep")
	XSTRING (Revert, "Revert")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum CustomizationManagerTags
	{
		// Management tags
		kPresetList = 1000,
		kUserSelectedPresetList,
		kEditorContext,
		kOKay,
		kRevert,

		// Customization options
		kVisible
	};
};

//************************************************************************************************
// CustomizationPreset
//************************************************************************************************

DEFINE_CLASS (CustomizationPreset, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomizationPreset::CustomizationPreset ()
: readOnly (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomizationPreset::load (const Storage& storage)
{
	storage.getAttributes ().get (id, "id");
	storage.getAttributes ().get (name, "name");
	storage.getAttributes ().get (getAttributes (), "data");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomizationPreset::save (const Storage& storage) const
{
	if(!getID ().isEmpty ())
		storage.getAttributes ().set ("id", getID ());
	storage.getAttributes ().set ("name", getName ());
	storage.getAttributes ().set ("data", getAttributes ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomizationPreset::toString (String& string, int flags) const
{
	string = getName ();
	return true;
}

//************************************************************************************************
// CustomizationPresetMemento
//************************************************************************************************

CustomizationPresetMemento::CustomizationPresetMemento (CustomizationComponent* customizationComponent)
: customizationComponent (customizationComponent),
  previousPreset (customizationComponent->getSelectedPreset ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationPresetMemento::confirmCustomization ()
{
	// defer initially (other modal dialogs might already be sheduled via messages)
	retain ();
	(NEW Message (CSTR ("confirm")))->post (this, 500);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CustomizationPresetMemento::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "confirm")
	{
		// defer further until other modal dialogs etc. are closed
		if(System::GetDesktop ().isInMode (IDesktop::kModalMode|IDesktop::kPopupMode|IDesktop::kMenuLoopMode))
			(NEW Message (CSTR ("confirm")))->post (this, 300);
		else
		{
			CustomizationPreset* currentPreset = customizationComponent->getSelectedPreset ();
			if(currentPreset && previousPreset && currentPreset != previousPreset)
			{
				String currentName;
				currentName << "\"" << currentPreset->getName () << "\"";

				int answer = Alert::ask (String ().appendFormat (XSTR (DoYouWantToKeepCustomization), currentName), XSTR (Keep), XSTR (Revert));
				if(answer == Alert::kSecondButton)
				{
					// revert to previous preset
					customizationComponent->selectPreset (*previousPreset, false);
				}
			}

			release ();
		}
	}
}

//************************************************************************************************
// CustomizationComponent::Manager
//************************************************************************************************

class CustomizationComponent::Manager: public PresetManagementComponent,
									   public IWindowEventHandler,
									   public AbstractDocumentEventHandler
{
public:
	Manager (CustomizationComponent& customization);

	void showEditor (StringRef context, bool modal);
	void enableRevert (bool enable);

	// PresetManagementComponent
	tbool storePreset (int mode, StringID toFormat) override;
	bool deletePreset (bool checkOnly) override;
	bool renamePreset (bool checkOnly) override;
	void extendPresetMenu (IMenu* menu) override;
	String getCurrentPresetName () const override;
	tresult CCL_API initialize (IUnknown* context) override;
	tresult CCL_API terminate () override;

	// IWindowEventHandler
	tbool CCL_API onWindowEvent (WindowEvent& windowEvent) override;

	// IDocumentEventHandler
	void CCL_API onDocumentEvent (IDocument& document, int eventCode) override;

	CLASS_INTERFACE2 (IWindowEventHandler, IDocumentEventHandler, PresetManagementComponent)

private:
	CustomizationComponent& customization;
	IWindowClass* windowClass;

	using SuperClass = PresetManagementComponent;

	void registerWindowClass (bool state);
};

//************************************************************************************************
// CustomizationComponent::PresetRenamer
//************************************************************************************************

class CustomizationComponent::PresetRenamer: public Renamer
{
public:
	PresetRenamer (CustomizationComponent& customization, CustomizationPreset* preset)
	: Renamer (preset->getName ()),
	  customization (customization),
	  preset (preset)
	{}

	// Renamer
	bool doesAlreadyExist (StringRef newName) override	{ return customization.getPreset (newName) != nullptr; }
	bool performRename (StringRef newName) override		{ customization.renamePreset (preset, newName); return true; }

protected:
	CustomizationComponent& customization;
	CustomizationPreset* preset;
};

//************************************************************************************************
// CustomizationComponent::Manager
//************************************************************************************************

CustomizationComponent::Manager::Manager (CustomizationComponent& customization)
: PresetManagementComponent (CCLSTR ("Manager")),
  customization (customization),
  windowClass (nullptr)
{
	// configure PresetManagementComponent
	setOptions (0);
	canRenamePreset (true);
	canDeletePreset (true);

	paramList.setController (&customization);
	paramList.addMenu (CSTR ("presets"), Tag::kPresetList);
	paramList.addMenu (CSTR ("userSelectedPreset"), Tag::kUserSelectedPresetList);
	paramList.addString (CSTR ("context"), Tag::kEditorContext);
	paramList.addParam (CSTR ("close"), Tag::kOKay);
	paramList.addParam (CSTR ("revert"), Tag::kRevert);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CustomizationComponent::Manager::getCurrentPresetName () const
{
	CustomizationPreset* preset = customization.getSelectedPreset ();
	return preset ? preset->getName () : String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::Manager::extendPresetMenu (IMenu* menu)
{
	menu->addCommandItem (CommandWithTitle (CSTR ("Presets"), CSTR ("Store Preset"), XSTR (Store)), this, true);
	menu->addCommandItem (CommandWithTitle (CSTR ("Presets"), CSTR ("Rename"), XSTR (Rename)), this, true);
	menu->addCommandItem (CommandWithTitle (CSTR ("Presets"), CSTR ("Delete"), XSTR (Delete)), this, true);
	menu->addSeparatorItem ();

	#if 0 // not needed since factory presets cannot be edited anymore
	// reset current preset to factory preset of same name (if available)
	CustomizationPreset* preset = customization.getSelectedPreset ();
	SharedPtr<CustomizationPreset> factoryPreset (preset ? customization.getFactoryPreset (preset->getName ()) : nullptr);
	if(factoryPreset)
	{
		AutoPtr<ICommandHandler> resetPreset (makeCommandDelegate ([this, factoryPreset] (CmdArgs args, VariantRef data)
		{
			if(!args.checkOnly () && Alert::ask (XSTR (DoYouWantToResetThisFactoryPreset)) == Alert::kYes)
				customization.restorePreset (*factoryPreset);

			return true;
		}, 0));
		menu->addCommandItem (XSTR (ResetPreset), CSTR ("Presets"), CSTR ("Reset"), resetPreset);
	}
	#endif

	// reset all back to factory presets (deletes user presets)
	AutoPtr<ICommandHandler> resetAll (makeCommandDelegate ([this] (CmdArgs args, VariantRef data)
	{
		if(args.checkOnly ())
			return customization.countUserPresets () > 0;
		else
		{
			if(Alert::ask (XSTR (DoYouWantToDeleteAllPresets)) == Alert::kYes)
				customization.resetPresets ();
		}
		return true;
	}, nullptr));
	menu->addCommandItem (CommandWithTitle (CSTR ("Presets"), CSTR ("Reset All"), XSTR (ResetAllPresets)), resetAll, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CustomizationComponent::Manager::storePreset (int mode, StringID toFormat)
{
	if(mode == kStoreNewPreset)
	{
		String presetName (getCurrentPresetName ());
		if(presetName.isEmpty ())
			presetName = getDefaultPresetName ();

		customization.makeUniquePresetName (presetName);

		if(askPresetName (presetName) && !presetName.isEmpty ())
		{
			if(customization.getPreset (presetName))
				customization.makeUniquePresetName (presetName);

			auto preset = NEW CustomizationPreset;
			preset->setName (presetName);
			customization.storePreset (*preset);
			customization.addPreset (preset);
			customization.selectPreset (presetName);
			return true;
		}
	}
	else
		ASSERT (0)

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomizationComponent::Manager::deletePreset (bool checkOnly)
{
	if(customization.countPresets () < 2)
		return false;

	if(CustomizationPreset* preset = customization.getSelectedPreset ())
	{
		if(preset->isReadOnly ())
			return false;

		if(!checkOnly)
			if(askRemovePreset (true, preset->getName ()))
				customization.removePreset (preset);

		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomizationComponent::Manager::renamePreset (bool checkOnly)
{
	if(CustomizationPreset* preset = customization.getSelectedPreset ())
	{
		if(preset->isReadOnly ())
			return false;

		if(!checkOnly)
		{
			PresetRenamer renamer (customization, preset);
			renamer.runDialog (getRenamePresetTitle ());
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::Manager::registerWindowClass (bool state)
{
	if(state)
	{
		ASSERT (!customization.getFormName ().isEmpty ())
		ASSERT (!windowClass)

		String controllerPath;
		RootComponent::instance ().getRelativePath (controllerPath, this);

		Url controllerUrl;
		RootComponent::instance ().makeUrl (controllerUrl, controllerPath);
		controllerUrl.getUrl (controllerPath);

		String groupID (CCLSTR ("Popups"));
		StringID appID (RootComponent::instance ().getApplicationID ());
		StringID workspaceID (appID);
		StringID themeID (appID);
		MutableCString windowClassID (getName ());

		windowClass = System::GetWindowManager ().registerClass (windowClassID, String (customization.getFormName ()), controllerPath, groupID, workspaceID, themeID);
	}
	else
	{
		if(windowClass)
		{
			if(System::GetWindowManager ().isWindowOpen (windowClass))
				System::GetWindowManager ().closeWindow (windowClass, true);
			System::GetWindowManager ().unregisterClass (windowClass);
			windowClass = nullptr;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CustomizationComponent::Manager::initialize (IUnknown* context)
{
	registerWindowClass (true);
	DocumentManager::instance ().addHandler (this);

	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CustomizationComponent::Manager::terminate ()
{
	registerWindowClass (false);
	DocumentManager::instance ().removeHandler (this);

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::Manager::enableRevert (bool enable)
{
	getParameterByTag (Tag::kRevert)->enable (enable);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::Manager::showEditor (StringRef context, bool modal)
{
	auto prepareOpen = [&] ()
	{
		customization.storePreset (customization.stateBeforeEdit);
		enableRevert (false);

		// context string might be used in editor skin (e.g. to select a tab)
		getParameterByTag (Tag::kEditorContext)->setValue (context);
	};

	if(modal)
	{
		prepareOpen ();
		customization.wasEditConfirmed = false;

		if(IView* view = getTheme ()->createView (customization.getFormName (), this->asUnknown ()))
			Promise (DialogBox ()->runDialogAsync (view, Styles::kWindowCombinedStyleDialog));
	}
	else
	{
		if(windowClass)
		{
			customization.wasEditConfirmed = true;

			if(System::GetWindowManager ().isWindowOpen (windowClass))
			{
				// toggle (close) if no context provided, otherwise reopen with new context
				System::GetWindowManager ().closeWindow (windowClass);
				if(context.isEmpty ())
					return;
			}

			prepareOpen ();

			System::GetWindowManager ().openWindow (windowClass, false);

			if(IWindow* window = System::GetDesktop ().getWindowByOwner (this->asUnknown ()))
				window->addHandler (this);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CustomizationComponent::Manager::onWindowEvent (WindowEvent& windowEvent)
{
	if(windowEvent.eventType == WindowEvent::kClose)
	{
		// restore last state before current preset was selected / editor opened, unless confirmed via OK button or toggle commmand
		if(!customization.wasEditConfirmed)
			customization.restorePreset (customization.stateBeforeEdit);

		windowEvent.window.removeHandler (this);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CustomizationComponent::Manager::onDocumentEvent (IDocument& document, int eventCode)
{
	// reset outdated context string, e.g. when window is restored by workspace (avoid interfering with TabView persistence)
	switch(eventCode)
	{
	case IDocument::kActivate:
	case IDocument::kDeactivate:
	case IDocument::kViewActivated:
		getParameterByTag (Tag::kEditorContext)->setValue (String::kEmpty);
		break;
	}
}

//************************************************************************************************
// CustomizationComponent
//************************************************************************************************

StringRef CustomizationComponent::Customization () { return XSTR (Customization); }
StringRef CustomizationComponent::EditCustomization () { return XSTR (EditCustomization); }

ObjectArray CustomizationComponent::instances;
const Container& CustomizationComponent::getInstances () { return instances; }

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomizationComponent* CustomizationComponent::findCustomizationComponent (const FileType& documentType)
{
	return getInstances ().findIf<CustomizationComponent> ([&] (CustomizationComponent& c)
	{
		return c.isEnabled () && c.getDocumentFileType () == documentType;
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CustomizationComponent::getSettingsFileName ()
{ 
	String fileName;
	CustomizationSettings::instance ().getFactorySettings ().getPath ().getName (fileName);
	return fileName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CustomizationComponent::getSettingsPath ()
{ 
	return CustomizationSettings::instance ().getPath ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (CustomizationComponent, Component)
IMPLEMENT_COMMANDS (CustomizationComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (CustomizationComponent)
	DEFINE_COMMAND_ARGS ("View",	"Customization",		CustomizationComponent::onShowConfigurationEditor, 0, "Context")
	DEFINE_COMMAND_ARGS ("View",	"Select Customization",	CustomizationComponent::onSelectPreset, CommandFlags::kHidden, "Name")
END_COMMANDS (CustomizationComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomizationComponent::CustomizationComponent ()
: CustomizationComponent (CCLSTR ("Customization"))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomizationComponent::CustomizationComponent (StringRef name)
: Component (name),
  enabled (true),  
  menu (nullptr),
  lastUserSelectedPreset (nullptr),
  modalEditor (false),
  wasEditConfirmed (false),
  settingsRestored (false),
  stateBeforeEdit (*NEW CustomizationPreset)
{
	manager = NEW Manager (*this);
	addChild (manager); // management params in separate component to isolate from our customization params

	presets.objectCleanup (true);

	instances.add (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomizationComponent::~CustomizationComponent ()
{
	instances.remove (this);

	safe_release (lastUserSelectedPreset);
	safe_release (menu);
	stateBeforeEdit.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomizationComponent::matchesDocument (const Document& document)
{
	if(!documentFileType.isValid ())
		return true;

	if(DocumentClass* docClass = document.getDocumentClass ())
		return docClass->getFileType () == documentFileType;
	else
		return document.getPath ().getFileType () == documentFileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::setMenu (IMenu* m)
{
	take_shared (menu, m);
	updateMenu ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CustomizationComponent::initialize (IUnknown* context)
{
	restoreSettings ();
	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CustomizationComponent::terminate ()
{
	storeSettings ();
	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::setDefaultVisible (StringID key, bool visible)
{
	IParameter* param = findParameter (key);
	param->setDefaultValue (visible);
	param->setValue (visible);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& CustomizationComponent::getSettings () const
{
	return CustomizationSettings::instance ().getAttributes (getName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomizationPreset* CustomizationComponent::getFactoryPreset (StringRef name) const
{
	AutoPtr<Iterator> presetIter (CustomizationSettings::instance ().newFactoryPresetsIterator (getName ()));
	for(auto preset : iterate_as<CustomizationPreset> (*presetIter))
		if(preset && preset->getName () == name)
			return preset;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CustomizationComponent::countUserPresets () const
{
	return countPresets () - CustomizationSettings::instance ().countFactoryPresets (getName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::storeSettings ()
{
	if(!settingsRestored)
		return;

	// store current state (params) in selected preset
	if(CustomizationPreset* preset = getSelectedPreset ())
		storePreset (*preset);

	Attributes& a = getSettings ();
	a.removeAll ();

	// store only writeable (non-factory) presets
	for(auto preset : iterate_as<CustomizationPreset> (presets))
		if(!preset->isReadOnly ())
			a.queue ("presets", preset, Attributes::kShare);

	// store selected preset
	String presetName;
	manager->getParameterByTag (Tag::kPresetList)->toString (presetName);
	a.set ("selected", presetName);

	manager->getParameterByTag (Tag::kUserSelectedPresetList)->toString (presetName);
	a.set ("userSelected", presetName);

	CustomizationSettings::instance ().flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::restoreSettings ()
{
	presets.removeAll ();

	// add factory presets
	AutoPtr<Iterator> presetIter (CustomizationSettings::instance ().newFactoryPresetsIterator (getName ()));
	for(auto preset : iterate_as<CustomizationPreset> (*presetIter))
	{
		addPreset (return_shared (preset));
		createParameters (*preset);
	}

	// add user presets
	getSettings ().unqueue (presets, "presets", ccl_typeid<CustomizationPreset> ());

	if(presets.isEmpty ()) // only if no factory presets are provided
	{
		auto preset = NEW CustomizationPreset;
		preset->setName (CCLSTR ("default"));
		addPreset (preset);
	}

	updatePresetList ();

	String selected = getSettings ().getString ("selected");
	if(selected.isEmpty ())
	{
		CustomizationSettings::instance ().selectDefaultFactoryPreset (getName ());
		selected = getSettings ().getString ("selected");
	}
	selectPreset (selected);

	CustomizationPreset* userSelectedPreset = getPreset (getSettings ().getString ("userSelected"));
	if(!userSelectedPreset)
		userSelectedPreset = getSelectedPreset ();
	if(userSelectedPreset)
		setLastUserSelectedPreset (*userSelectedPreset);

	settingsRestored = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::resetPresets ()
{
	// remove user settings, restore presets from factory settings
	getSettings ().removeAll ();

	CustomizationSettings::instance ().selectDefaultFactoryPreset (getName ());

	restoreSettings ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API CustomizationComponent::findParameter (StringID name) const
{
	// automatically add requested params (e.g. used in skin or loaded from preset)
	IParameter* param = SuperClass::findParameter (name);
	if(!param)
	{
		param = ccl_const_cast (paramList).addParam (name, Tag::kVisible);
		param->setDefaultValue (true);
		param->setValue (true); // visible by default
	}
	return param;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::addPreset (CustomizationPreset* preset)
{
	presets.add (preset);
	updatePresetList ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::removePreset (CustomizationPreset* preset)
{
	if(presets.remove (preset))
	{
		preset->release ();

		updatePresetList ();
		selectPreset (CustomizationSettings::instance ().getDefaultFactoryPresetName (getName ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::renamePreset (CustomizationPreset* preset, StringRef newName)
{
	preset->setName (newName);

	CustomizationPreset* selectedPreset = getSelectedPreset ();
	ASSERT (preset == selectedPreset)

	updatePresetList ();

	if(selectedPreset)
	{
		// select previouly selected preset again in new list
		UnknownPtr<IListParameter> listParam (manager->getParameterByTag (Tag::kPresetList));
		listParam->selectValue (Variant (selectedPreset->asUnknown ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::makeUniquePresetName (String& name)
{
	if(!getPreset (name)) // try to keep original name
		return;

	StringUtils::IndexedNameBuilder nameBuilder (name, String::kEmpty);
	do
	{
		nameBuilder.nextName (name);
	} while(getPreset (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CustomizationComponent::getUserSelectedPresetParameter () const
{
	return manager->getParameterByTag (Tag::kUserSelectedPresetList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::updatePresetList ()
{
	for(auto tag : {Tag::kPresetList, Tag::kUserSelectedPresetList})
	{
		UnknownPtr<IListParameter> listParam (manager->getParameterByTag (tag));
		listParam->removeAll ();

		for(auto preset : iterate_as<CustomizationPreset> (presets))
			listParam->appendValue (Variant (preset->asUnknown ()));

		if(tag == Tag::kUserSelectedPresetList)
		{
			CustomizationPreset* toSelect = getLastUserSelectedPreset ();
			if(!toSelect)
				toSelect = getSelectedPreset ();

			listParam->selectValue (Variant (ccl_as_unknown (toSelect)));
		}
	}

	updateMenu ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::updateMenu ()
{
	if(menu)
	{
		menu->removeAll ();
		menu->addCommandItem (CommandWithTitle ("View", "Customization", XSTR (EditCustomization)), this, true);
		if(enabled)
		{
			menu->addSeparatorItem ();

			// separator between factory and user presets
			int separatorPosition = CustomizationSettings::instance ().countFactoryPresets (getName ());

			int i = 0;
			for(auto preset : iterate_as<CustomizationPreset> (presets))
			{
				menu->addCommandItem (preset->getName (), "View", "Select Customization", 
						makeCommandDelegate (this, &CustomizationComponent::onSelectPreset, preset->getName ()));

				if(++i == separatorPosition)
					menu->addSeparatorItem ();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomizationPreset* CustomizationComponent::getSelectedPreset () const
{
	UnknownPtr<IListParameter> listParam (manager->getParameterByTag (Tag::kPresetList));
	return unknown_cast<CustomizationPreset> (listParam->getSelectedValue ().asUnknown ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomizationPreset* CustomizationComponent::getLastUserSelectedPreset () const
{
	return (lastUserSelectedPreset && presets.contains (lastUserSelectedPreset)) ? lastUserSelectedPreset : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomizationPreset* CustomizationComponent::getPreset (StringRef name) const
{
	return presets.findIf<CustomizationPreset> ([&] (const CustomizationPreset& p) { return p.getName () == name; });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CustomizationPreset* CustomizationComponent::getPresetByID (StringRef id) const
{
	return presets.findIf<CustomizationPreset> ([&] (const CustomizationPreset& p) { return p.getID () == id; });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomizationComponent::selectPreset (StringRef name, bool userAction)
{
	if(auto preset = getPreset (name))
	{
		selectPreset (*preset, userAction);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::selectPreset (CustomizationPreset& preset, bool userAction)
{
	UnknownPtr<IListParameter> listParam (manager->getParameterByTag (Tag::kPresetList));
	listParam->selectValue (Variant (preset.asUnknown ()));
	restorePreset (preset);
	manager->setCurrentPresetName (preset.getName ());

	if(userAction)
		setLastUserSelectedPreset (preset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::setLastUserSelectedPreset (CustomizationPreset& preset)
{
	ASSERT (presets.contains (&preset))
	take_shared (lastUserSelectedPreset, &preset);
	UnknownPtr<IListParameter> listParam (manager->getParameterByTag (Tag::kUserSelectedPresetList));
	listParam->selectValue (Variant (preset.asUnknown ()));
	CCL_PRINTF ("setLastUserSelectedPreset %s\n", MutableCString (preset.getName ()).str ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::storePreset (CustomizationPreset& preset)
{
	preset.getAttributes ().removeAll ();
	paramList.storeValues (preset.getAttributes ());
	manager->enableRevert (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::restorePreset (const CustomizationPreset& preset)
{
	// first make sure all params exists
	createParameters (preset);

	// restore param values
	const Attributes& attributes = preset.getAttributes ();
	for(int i = 0; i < paramList.count (); i++)
		if(IParameter* p = paramList.at (i))
		{
			bool restored = paramList.restoreValue (attributes, p);
			if(!restored) // new parameter, not known when preset was stored
				p->setValue (p->getDefaultValue ()); // init with default state ("visible" if not explicitly specified)
		}

	storePreset (stateBeforeEdit);
	manager->enableRevert (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizationComponent::createParameters (const CustomizationPreset& preset)
{
	const Attributes& attributes = preset.getAttributes ();

	int numAttribs = attributes.countAttributes ();
	for(int i = 0; i < numAttribs; i++)
	{
		MutableCString name;
		if(attributes.getAttributeName (name, i))
			findParameter (name);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CustomizationComponent::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kPresetList:
		if(CustomizationPreset* preset = getSelectedPreset ())
		{
			restorePreset (*preset);
			setLastUserSelectedPreset (*preset);
		}
		break;

	case Tag::kVisible:
		if(CustomizationPreset* preset = getSelectedPreset ())
		{
			if(preset->isReadOnly ())
			{
				// store edited state as another preset
				String presetName (XSTR (UserPresetName));
				makeUniquePresetName (presetName);

				auto newPreset = NEW CustomizationPreset;
				newPreset->setName (presetName);
				storePreset (*newPreset);
				addPreset (newPreset);
				selectPreset (presetName);

				// factory state as "restore point" (not the just edited state)
				stateBeforeEdit.getAttributes ().copyFrom (preset->getAttributes ());
				manager->enableRevert (true);
			}
			else
			{
				storePreset (*preset); // auto store on edit
				manager->enableRevert (true);
			}
		}
		break;

	case Tag::kOKay:
		wasEditConfirmed = true;
		if(IWindow* window = System::GetDesktop ().getWindowByOwner (manager->asUnknown ()))
			window->close ();
		break;

	case Tag::kRevert:
		if(isModalEditor ())
		{
			// close dialog, revert changes
			wasEditConfirmed = false;
			if(IWindow* window = System::GetDesktop ().getWindowByOwner (manager->asUnknown ()))
				window->close ();
		}
		else
		{
			// revert changes, keep non-modal window open
			restorePreset (stateBeforeEdit);
		}
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CustomizationComponent::notify (ISubject* subject, MessageRef msg)
{
	if(subject && msg == IParameter::kExtendMenu)
	{
		UnknownPtr<IMenu> menu (msg[0].asUnknown ());
		if(menu)
		{
			// separator between factory and user presets
			int position = CustomizationSettings::instance ().countFactoryPresets (getName ());
			if(position < menu->countItems ())
			{
				MenuInserter inserter (menu, position);
				menu->addSeparatorItem ();
			}
		}
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CustomizationComponent::appendContextMenu (IContextMenu& contextMenu)
{
	SuperClass::appendContextMenu (contextMenu);

	String context (contextMenu.getContextID ());
	contextMenu.addSeparatorItem ();
	contextMenu.addCommandItem (String (XSTR (Customize)) << IMenu::strFollowIndicator, "View", "Customization", makeCommandDelegate (this, &CustomizationComponent::onShowConfigurationEditor, context));
	return kResultTrue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomizationComponent::onShowConfigurationEditor (CmdArgs args, VariantRef data)
{
	if(enabled && !getFormName ().isEmpty ())
	{
		auto document = DocumentManager::instance ().getActiveDocument ();
		if(document && matchesDocument (*document))
		{
			if(!args.checkOnly ())
			{
				String context (data.asString ());
				manager->showEditor (context, isModalEditor ());
			}
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomizationComponent::onShowConfigurationEditor (CmdArgs args)
{
	// when no context is given as argument, the empty string resets the context (-> skin might use own persistence)
	String context (CommandAutomator::Arguments (args).getString ("Context"));
	return onShowConfigurationEditor (args, context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomizationComponent::onSelectPreset (CmdArgs args, VariantRef data)
{
	String name (data.asString ());
	if(args.checkOnly ())
	{
		CustomizationPreset* preset = getPreset (name);
		if(preset)
			if(UnknownPtr<IMenuItem> menuItem = args.invoker)
				menuItem->setItemAttribute (IMenuItem::kItemChecked, preset == getSelectedPreset ());

		return preset != nullptr;
	}
	else
		return selectPreset (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CustomizationComponent::onSelectPreset (CmdArgs args)
{
	String name (CommandAutomator::Arguments (args).getString ("Name"));
	return onSelectPreset (args, name);
}
