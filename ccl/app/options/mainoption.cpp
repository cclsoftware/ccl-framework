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
// Filename    : ccl/app/options/mainoption.cpp
// Description : Main Option
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/options/mainoption.h"
#include "ccl/app/components/pathselector.h"
#include "ccl/app/options/useroptionelement.h"

#include "ccl/app/params.h"
#include "ccl/app/paramalias.h"
#include "ccl/app/utilities/imagefile.h"
#include "ccl/app/presets/simplepreset.h"
#include "ccl/app/presets/presetcomponent.h"
#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/app/signals.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/settings.h"

#include "ccl/public/text/language.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/storage/ifileresource.h"
#include "ccl/public/system/ilocaleinfo.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/icolorscheme.h"
#include "ccl/public/gui/framework/iwin32specifics.h"
#include "ccl/public/gui/framework/imacosspecifics.h"
#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// LocaleOption::LanguageItem
//************************************************************************************************

class LocaleOption::LanguageItem: public Object
{
public:
	DECLARE_CLASS (LanguageItem, Object)

	LanguageItem (StringID language = nullptr, StringRef title = nullptr)
	: language (language),
	  title (title)
	{}

	PROPERTY_MUTABLE_CSTRING (language, Language)
	PROPERTY_STRING (title, Title)
	PROPERTY_SHARED_AUTO (ILanguagePack, languagePack, LanguagePack)
	PROPERTY_SHARED_AUTO (IImage, cachedIcon, CachedIcon)

	const IUrl* getLanguagePackPath () const;
	IImage* loadLanguagePackIcon (); ///< load icon from language pack

	// Object
	bool toString (String& string, int flags) const override
	{
		string = title;
		return true;
	}

	bool equals (const Object& obj) const override
	{
		if(const LanguageItem* other = ccl_cast<LanguageItem> (&obj))
		{
			if(other->languagePack)
				return languagePack == other->languagePack;
			else
				return language == other->language;
		}
		return SuperClass::equals (obj);
	}
};

DEFINE_CLASS_HIDDEN (LocaleOption::LanguageItem, Object)

//************************************************************************************************
// ContentLocationSaver
//************************************************************************************************

class ContentLocationSaver: public SettingsSaver
{
public:
	// SettingsSaver
	void restore (Settings& settings) override
	{
		Url contentPath;
		System::GetSystem ().getLocation (contentPath, System::kUserContentFolder);
		Attributes& a = settings.getAttributes ("Content");
		a.get (contentPath, "location");
		
		// do not restore content location if folder doesn't exist
		bool valid = contentPath.isFolder () && System::GetFileSystem ().fileExists (contentPath);		
		if(valid == true)
			System::GetSystem ().setLocation (System::kUserContentFolder, contentPath);
	}

	void flush (Settings& settings) override
	{
		AutoPtr<Url> contentPath = NEW Url;
		System::GetSystem ().getLocation (*contentPath, System::kUserContentFolder);
		Attributes& a = settings.getAttributes ("Content");
		a.set ("location", contentPath, Attributes::kShare);
	}
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("UserOption")
	XSTRING (Language, "Language")
	XSTRING (Locations, "Locations")
	XSTRING (Content, "User Data")
	XSTRING (LanguageWarning, "Language will be changed next time you start $APPNAME.")
	XSTRING (HighDPIRestartWarning, "High DPI Mode will be changed next time you start $APPNAME.")
	XSTRING (HighDPIPluginWarning, "Please note that third-party plug-ins will appear smaller if they do not support scaling.")
	XSTRING (HighDPIBlurryWarning, "Please note that the user interface of $APPNAME will appear blurry on high resolution screens.")
	XSTRING (GraphicsRestartWarning, "Graphics settings will be applied next time you start $APPNAME.")
	XSTRING (GraphicsAccelerationOffWarning, "Turning off hardware acceleration will slow down the user interface of $APPNAME and is not recommended for regular use.")
	XSTRING (PluginCompatibilityNote, "Use this option only for downwards compatibility with older third-party plug-ins.")
	XSTRING (UserInterface, "User Interface")
	XSTRING (Appearance, "Appearance")
	XSTRING (DarkMode, "Dark Mode")
	XSTRING (LightMode, "Light Mode")
END_XSTRINGS

BEGIN_XSTRINGS ("SliderMode")
	XSTRING (SliderModeTouch, "Touch")
	XSTRING (SliderModeJump, "Jump")
END_XSTRINGS

//************************************************************************************************
// LocaleOption::LanguageItem
//************************************************************************************************

const IUrl* LocaleOption::LanguageItem::getLanguagePackPath () const
{
	if(UnknownPtr<IFileResource> file = getLanguagePack ())
		return &file->getPath ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* LocaleOption::LanguageItem::loadLanguagePackIcon ()
{
	// Note: Unused packages are not mounted and must be opened separately here.
	IImage* icon = nullptr;
	if(const IUrl* langPackPath = getLanguagePackPath ())
	{
		AutoPtr<IPackageFile> pf = System::GetPackageHandler ().openPackage (*langPackPath);
		if(pf && pf->getFileSystem ())
		{
			const String kTempID (String () << "~languagepack" << System::GetThreadSelfID ());
			tresult tr = System::GetPackageHandler ().mountPackageVolume (pf, kTempID, IPackageVolume::kHidden);
			if(tr == kResultOk)
			{			
				PackageUrl path (kTempID, "language.png"); // make sure hi-res icons can be loaded via path
				icon = ImageFile::loadImage (path);
				System::GetPackageHandler ().unmountPackageVolume (pf);
			}
		}
	}
	return icon;
}

//************************************************************************************************
// LocaleOption
//************************************************************************************************

LocaleOption* LocaleOption::findInstance ()
{
	return unknown_cast<LocaleOption> (UserOptionManager::instance ().findOptionByName (CCLSTR ("LocaleOption")));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (LocaleOption, UserOption)

//////////////////////////////////////////////////////////////////////////////////////////////////

LocaleOption::LocaleOption ()
: UserOption (CCLSTR ("LocaleOption"), General ()),
  localeSink (*NEW SignalSink (Signals::kLocales)),
  languageRestartEnabled (false)
{
	// check for locale changes
	localeSink.setObserver (this);
	localeSink.enable (true);

	setFormName ("CCL/LocaleOption");

	paramList.addMenu (CSTR ("language"), kLanguage);

	addLanguage (LanguageCode::English); // default built-in language is English
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LocaleOption::~LocaleOption ()
{
	localeSink.enable (false);
	delete &localeSink;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LocaleOption::isCurrent (const LanguageItem* item) const
{
	bool selected = false;
	ILocaleManager& localeManager (System::GetLocaleManager ());
	if(ILanguagePack* languagePack = item->getLanguagePack ())
	{
		if(languagePack == localeManager.getActiveLanguagePack ())
			selected = true;
	}
	else
	{
		if(localeManager.getLanguage () == item->getLanguage ())
			selected = true;
	}
	return selected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LocaleOption::hasLanguage (const LanguageItem& item) const
{
	ListParam* listParam = paramList.byTag<ListParam> (kLanguage);
	return listParam->contains (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LocaleOption::addLanguage (LanguageItem* item)
{
	ListParam* listParam = paramList.byTag<ListParam> (kLanguage);
	listParam->appendObject (item);

	if(isCurrent (item))
		listParam->setValue (listParam->getMax ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LocaleOption::addLanguage (StringID languageCode)
{
	LanguageItem* item = NEW LanguageItem (languageCode);

	ILocaleManager& localeManager (System::GetLocaleManager ());
	const ILocaleInfo* localeInfo = localeManager.getLocale (languageCode);
	ASSERT (localeInfo != nullptr)
	if(localeInfo)
		item->setTitle (localeInfo->getTitle ());
	else
		item->setTitle (String (languageCode).toUppercase ());

	addLanguage (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LocaleOption::addLanguagePacks (bool update)
{
	ILocaleManager& localeManager (System::GetLocaleManager ());
	IterForEachUnknown (localeManager.createLanguagePackIterator (), unk)
		UnknownPtr<ILanguagePack> languagePack (unk);
		ASSERT (languagePack.isValid ())

		if(update == true) // avoid duplicates on update
			if(hasLanguage (LanguageItem (languagePack->getLanguage ())))
				continue;

		LanguageItem* item = NEW LanguageItem (languagePack->getLanguage ());
		item->setTitle (languagePack->getTitle ());
		item->setLanguagePack (languagePack);

		addLanguage (item);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LocaleOption::makeLanguageMenu (IMenu& menu)
{
	IMenu* subMenu = menu.createMenu ();
	subMenu->setMenuAttribute (IMenu::kMenuTitle, XSTR (Language));
	menu.addMenu (subMenu);

	ListParam* listParam = paramList.byTag<ListParam> (kLanguage);
	for(int i = listParam->getMin ().asInt (); i <= listParam->getMax ().asInt (); i++)
	{
		String title;
		listParam->getString (title, i);

		MutableCString commandName;
		commandName.appendFormat ("%d", i);
		subMenu->addCommandItem (title, "Select Language", commandName, this);
	}

	// add language icons to submenu
	notify (listParam, Message (IParameter::kExtendMenu, subMenu));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LocaleOption::paramChanged (IParameter* param)
{
	if(param->getTag () == kLanguage)
	{
		ListParam* listParam = unknown_cast<ListParam> (param);
		ASSERT (listParam != nullptr)

		LanguageItem* item = listParam->getObject<LanguageItem> (listParam->getValue ());
		ASSERT (item != nullptr)

		bool restartNeeded = false;
		if(isCurrent (item) == false)
		{
			if(isLanguageRestartEnabled ())
				restartNeeded = true;
			else
				Alert::info (XSTR (LanguageWarning));
		}

		ILocaleManager& localeManager (System::GetLocaleManager ());
		if(ILanguagePack* languagePack = item->getLanguagePack ())
		{
			localeManager.setActiveLanguagePack (languagePack); // always call setActiveLanguagePack(), otherwise revert does not work!
		}
		else
		{
			StringID language = item->getLanguage ();
			localeManager.setLanguage (language); // always call setLanguage(), otherwise revert does not work!
		}

		// emit restart signal
		if(restartNeeded)
			SignalSource (Signals::kApplication).deferSignal (NEW Message (Signals::kRequestRestart, XSTR (LanguageWarning)));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LocaleOption::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kApplicationLanguageChanged)
	{
		LanguageItem item (MutableCString (msg[0].asString ()));
		if(msg.getArgCount () > 1)
			item.setLanguagePack (UnknownPtr<ILanguagePack> (msg[1].asUnknown ()));

		// sync language menu
		ListParam* listParam = paramList.byTag<ListParam> (kLanguage);
		int index = listParam->getObjectIndex (item);
		ASSERT (index != -1)
		listParam->setValue (index);
	}
	else if(msg == IParameter::kExtendMenu)
		if(ListParam* listParam = unknown_cast<ListParam> (subject))
			if(listParam->getTag () == kLanguage)
			{
				ITheme* frameworkTheme = System::GetThemeManager ().getTheme ("cclgui");
				ASSERT (frameworkTheme != nullptr)

				UnknownPtr<IMenu> menu (msg.getArg (0));
				ASSERT (menu != nullptr)
				if(menu) for(int i = 0; i < menu->countItems (); i++)
				{
					LanguageItem* item = listParam->getObject<LanguageItem> (i);
					ASSERT (item != nullptr)
					if(!item)
						continue;

					if(item->getCachedIcon () == nullptr)
					{
						AutoPtr<IImage> icon;

						// try to load from framework theme
						if(frameworkTheme)
						{
							MutableCString iconName ("LanguageCode:");
							iconName += item->getLanguage ();
							icon = return_shared (frameworkTheme->getImage (iconName));
						}
						
						// try to load from language pack file
						if(icon == nullptr)
							icon = item->loadLanguagePackIcon ();

						ASSERT (icon != nullptr)
						item->setCachedIcon (icon);
					}

					IMenuItem* menuItem = menu->getItem (i);
					menuItem->setItemAttribute (IMenuItem::kItemIcon, item->getCachedIcon ());

					#if 0 && DEBUG // show path in debug build
					if(const IUrl* path = item->getLanguagePackPath ())
					{
						String title = String () << item->getTitle () << " (" << UrlDisplayString (*path) << ")";
						menuItem->setItemAttribute (IMenuItem::kItemTitle, title);
				}
					#endif
				}
				return;
			}

	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LocaleOption::checkCommandCategory (CStringRef category) const
{
	if(category == "Select Language")
		return true;
	return SuperClass::checkCommandCategory (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LocaleOption::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == "Select Language")
	{
		int64 index = -1;
		msg.name.getIntValue (index);
		ListParam* listParam = paramList.byTag<ListParam> (kLanguage);

		if(msg.checkOnly ())
		{
			if(UnknownPtr<IMenuItem> menuItem = msg.invoker)
				menuItem->setItemAttribute (IMenuItem::kItemChecked, index == listParam->getValue ().asInt ());
		}
		else
			listParam->setValue (index, true);
		return true;
	}
	return SuperClass::interpretCommand (msg);
}

//************************************************************************************************
// ContentLocationOption
//************************************************************************************************

void ContentLocationOption::registerSaver ()
{
	Settings& settings = Settings::instance ();
	settings.addSaver (NEW ContentLocationSaver);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ContentLocationOption, UserOption)

//////////////////////////////////////////////////////////////////////////////////////////////////

ContentLocationOption::ContentLocationOption ()
: UserOption (CCLSTR ("ContentLocationOption")),
  pathSelector (NEW PathSelector (CCLSTR ("ContentPath")))
{	
	setTitle (String () << XSTR (Locations) << strSeparator << XSTR (Content));
	setFormName ("CCL/ContentLocationOption");

	Url contentPath;
	System::GetSystem ().getLocation (contentPath, System::kUserContentFolder);
	
	pathSelector->setPath (contentPath);
	pathSelector->addObserver (this);
	addComponent (pathSelector);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ContentLocationOption::~ContentLocationOption ()
{
	pathSelector->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ContentLocationOption::notify (ISubject* subject, MessageRef msg)
{
	if(subject == pathSelector)
	{
		// this causes a global signal to be invoked
		System::GetSystem ().setLocation (System::kUserContentFolder, pathSelector->getPath ());
		Settings::instance ().flush ();
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContentLocationOption::runSelector ()
{
	IView* view = getTheme ()->createView ("CCL/ContentLocationSelector", this->asUnknown ());
	ASSERT (view != nullptr)
	if(view)
		DialogBox ()->runDialog (view);
    
	Url contentPath;
	System::GetSystem ().getLocation (contentPath, System::kUserContentFolder);

	AutoPtr<IFolderSelector> fs = ccl_new<IFolderSelector> (ClassID::FolderSelector);
	fs->setPath (contentPath);    
	if(!fs->run ())
		return false;

	contentPath = fs->getPath ();
        
	String folderName;
	contentPath.getName (folderName);
	LegalFileName appName (RootComponent::instance ().getApplicationTitle ());
	if(folderName != appName)
	{
		AutoPtr<IFileIterator> iterator = System::GetFileSystem ().newIterator (contentPath);
		if(iterator && iterator->next () != nullptr)
			contentPath.descend (appName, IUrl::kFolder);
	}

	// this causes a global signal to be invoked
	System::GetSystem ().setLocation (System::kUserContentFolder, contentPath);
	Settings::instance ().flush ();
	return true;
}

//************************************************************************************************
// AutoSaveOption
//************************************************************************************************

void AutoSaveOption::registerSaver ()
{
	Settings& settings = Settings::instance ();
	settings.addSaver (NEW ConfigurationSaver ("Application.AutoSaver", "enabled"));
	settings.addSaver (NEW ConfigurationSaver ("Application.AutoSaver", "period"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (AutoSaveOption, UserOption)

//////////////////////////////////////////////////////////////////////////////////////////////////

AutoSaveOption::AutoSaveOption ()
: UserOption (CCLSTR ("AutoSaveOption"))
{	
	setTitle (String () << XSTR (Locations) << strSeparator << XSTR (Content));
	setFormName ("CCL/AutoSaveOption");

	addElement (NEW ConfigurationElement ("Application.AutoSaver", "enabled", NEW Parameter));
	Parameter* autoSavePeriod = NEW IntParam (30, 3600);
	autoSavePeriod->setFormatter (AutoPtr<IFormatter> (NEW Format::Duration (ILocaleInfo::kMinutes)));
	addElement (NEW ConfigurationElement ("Application.AutoSaver", "period", autoSavePeriod));
}

//************************************************************************************************
// UserInterfaceOption
//************************************************************************************************

void UserInterfaceOption::registerSaver ()
{
	Settings& settings = Settings::instance ();
	Configuration::IRegistry& registry = System::GetFrameworkConfiguration ();
	settings.addSaver (NEW ConfigurationSaver ("GUI.Controls.Slider", "mode", &registry));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserOptionElement* UserInterfaceOption::createSliderModes ()
{
	ListParam* sliderMode = NEW ListParam;
	sliderMode->appendString (XSTR (SliderModeTouch));	// Styles::kSliderModeTouch
	sliderMode->appendString (XSTR (SliderModeJump));	// Styles::kSliderModeJump

	return NEW FrameworkOptionElement ("GUI.Controls.Slider", "mode", sliderMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (UserInterfaceOption, UserOption)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserInterfaceOption::UserInterfaceOption ()
: UserOption (CCLSTR ("UserInterfaceOption"))
{
	setTitle (General ());
	setFormName ("CCL/UserInterfaceOption");

	addElement (createSliderModes ());
}

//************************************************************************************************
// DpiAwarenessOption
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DpiAwarenessOption, UserOption)

//////////////////////////////////////////////////////////////////////////////////////////////////

DpiAwarenessOption::DpiAwarenessOption ()
{
	setTitle (General ());
	setFormName ("CCL/DpiAwarenessOption");

	AutoPtr<Win32::IDpiInfo> dpiInfo = ccl_new<Win32::IDpiInfo> (Win32::ClassID::DpiInfo);
	ASSERT (dpiInfo.isValid ())
	paramList.addParam ("dpiAwarenessEnabled", kEnabled)->setValue (dpiInfo && dpiInfo->isDpiAwarenessEnabled ());
	paramList.addString ("scaling", kScaling);
	updateScaling (1.f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DpiAwarenessOption::updateScaling (float dpiFactor)
{
	int percent = ccl_to_int (dpiFactor * 100.f);
	paramList.byTag (kScaling)->fromString (String () << percent << "%");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DpiAwarenessOption::paramChanged (IParameter* param)
{
	if(param->getTag () == kEnabled)
	{	
		AutoPtr<Win32::IDpiInfo> dpiInfo = ccl_new<Win32::IDpiInfo> (Win32::ClassID::DpiInfo);
		ASSERT (dpiInfo.isValid ())
		if(dpiInfo)
		{
			bool active = dpiInfo->isDpiAware () != 0;
			bool enabled = param->getValue ().asBool ();
			dpiInfo->setDpiAwarenessEnabled (enabled);

			if(enabled != active)
			{
				// emit restart signal
				String message (XSTR (HighDPIRestartWarning));
				if(enabled)
					message << "\n\n" << XSTR (HighDPIPluginWarning);
				else
					message << "\n\n" << XSTR (HighDPIBlurryWarning) << " " << XSTR (PluginCompatibilityNote);
						
				SignalSource (Signals::kApplication).deferSignal (NEW Message (Signals::kRequestRestart, message));
			}
		}
		return true;
	}
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DpiAwarenessOption::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "isDpiAware")
	{
		AutoPtr<Win32::IDpiInfo> dpiInfo = ccl_new<Win32::IDpiInfo> (Win32::ClassID::DpiInfo);
		ASSERT (dpiInfo.isValid ())
		var = dpiInfo && dpiInfo->isDpiAware ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DpiAwarenessOption::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "dpiChanged") // triggered by skin
	{
		float dpiFactor = 1.f;
		if(UnknownPtr<IView> view = msg[0].asUnknown ())
			if(IWindow* window = view->getIWindow ())
				dpiFactor = window->getContentScaleFactor ();

		updateScaling (dpiFactor);
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// GraphicsEngineOption
//************************************************************************************************

DEFINE_CLASS_HIDDEN (GraphicsEngineOption, UserOption)

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsEngineOption::GraphicsEngineOption ()
{
	setTitle (General ());
	setFormName ("CCL/GraphicsEngineOption");

	AutoPtr<MacOS::IMetalGraphicsInfo> graphicsInfo = ccl_new<MacOS::IMetalGraphicsInfo> (MacOS::ClassID::MetalGraphicsInfo);
	ASSERT (graphicsInfo.isValid ())
	IParameter* p = paramList.addParam ("hwAccelerationEnabled", kHWAccelerationEnabled);
	p->setValue (!graphicsInfo || (graphicsInfo && (graphicsInfo->isMetalEnabled () && graphicsInfo->isMetalAvailable ())));
	#if RELEASE
	p->enable (graphicsInfo && graphicsInfo->isMetalAvailable ());
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API GraphicsEngineOption::paramChanged (IParameter* param)
{
	if(param->getTag () == kHWAccelerationEnabled)
	{	
		AutoPtr<MacOS::IMetalGraphicsInfo> graphicsInfo = ccl_new<MacOS::IMetalGraphicsInfo> (MacOS::ClassID::MetalGraphicsInfo);
		ASSERT (graphicsInfo.isValid ())
		bool active = !graphicsInfo || (graphicsInfo && graphicsInfo->isMetalEnabled ());
		bool enabled = param->getValue ().asBool ();
		if(graphicsInfo)
			graphicsInfo->setMetalEnabled (enabled);

		if(enabled != active)
		{
			// emit restart signal
			String message (XSTR (GraphicsRestartWarning));
			if(!enabled)
				message << "\n\n" << XSTR (GraphicsAccelerationOffWarning) << " " << XSTR (PluginCompatibilityNote);
						
			SignalSource (Signals::kApplication).deferSignal (NEW Message (Signals::kRequestRestart, message));
		}
		return true;
	}
	return SuperClass::paramChanged (param);
}

//************************************************************************************************
// ColorSchemeOption
//************************************************************************************************

void ColorSchemeOption::addConfigurationSavers (StringID schemeName)
{
	MutableCString persistentName (IColorScheme::kPersistentPrefix);
	persistentName += schemeName;
	Settings& settings = Settings::instance ();
	Configuration::IRegistry& registry = System::GetFrameworkConfiguration ();
	settings.addSaver (NEW ConfigurationSaver (persistentName, IColorScheme::kHueLevel, &registry));
	settings.addSaver (NEW ConfigurationSaver (persistentName, IColorScheme::kSaturationLevel, &registry));
	settings.addSaver (NEW ConfigurationSaver (persistentName, IColorScheme::kLuminanceLevel, &registry));
	settings.addSaver (NEW ConfigurationSaver (persistentName, IColorScheme::kContrastLevel, &registry));
	settings.addSaver (NEW ConfigurationSaver (persistentName, IColorScheme::kColorInversion, &registry));
	settings.addSaver (NEW ConfigurationSaver (persistentName, IColorScheme::kMainSchemeDependent, &registry));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (ColorSchemeOption, UserOption)
DEFINE_STRINGID_MEMBER_ (ColorSchemeOption, kLevelChanged, "levelChanged")

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum ColorSchemeOptionTags
	{
		kCombinedLuminance = 1000,
		kLuminance,
		kColorInversion,
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorSchemeOption::ColorSchemeOption (StringID schemeName)
: UserOption (String () << "ColorSchemeOption" << schemeName),
  colorScheme (nullptr),
  initialHue (0.f),
  initialSaturation (0.f),
  initialLuminance (0.f),
  initialContrast (0.f),
  initialColorInversion (0.f),
  initialMainSchemeDependentState (0.f),
  insideLevelEditing (false)
{
	setTitle (String () << General () << strSeparator << XSTR (Appearance));
	setFormName ("CCL/ColorSchemeOption");

	AutoPtr<IColorSchemes> colorSchemes = ccl_new<IColorSchemes> (ClassID::ColorSchemes);
	ASSERT (colorSchemes != nullptr)
	colorScheme = colorSchemes->getScheme (schemeName, true);
	colorScheme->retain ();
	ISubject::addObserver (colorScheme, this);
	
	IParameter* hueParam = paramList.addInteger (0, 360, IColorScheme::kHueLevel);
	paramList.addParam (IColorScheme::kColorInversion, Tag::kColorInversion);
	paramList.addParam (IColorScheme::kMainSchemeDependent);
	
	IParameter* p1 = paramList.addFloat (-1.f, 1.f, IColorScheme::kSaturationLevel);
	p1->setFormatter (AutoPtr<IFormatter> (NEW CCL::Format::Bipolar (NEW CCL::Format::Percent)));
	IParameter* p2 = paramList.addFloat (-1.f, 1.f, IColorScheme::kLuminanceLevel, Tag::kLuminance);
	p2->setFormatter (AutoPtr<IFormatter> (NEW CCL::Format::Bipolar (NEW CCL::Format::Percent)));
	IParameter* p3 = paramList.addFloat (-1.f, 1.f, IColorScheme::kContrastLevel);
	p3->setFormatter (AutoPtr<IFormatter> (NEW CCL::Format::Bipolar (NEW CCL::Format::Percent)));
	
	IParameter* combinedLuminanceParam = paramList.addFloat (0.f, 1.f, "combinedLuminance", Tag::kCombinedLuminance);
	combinedLuminanceParam->setFormatter (AutoPtr<IFormatter> (NEW CCL::Format::Bipolar (NEW CCL::Format::Percent)));
	combinedLuminanceParam->setDefaultValue (0.25f);
	
	// TODO: implement a better way to make luminance accessible...
	MutableCString persistentName (IColorScheme::kPersistentPrefix);
	persistentName += schemeName;	
	AliasParam* hostAppLuminance = NEW AliasParam ("hostAppLuminance");
	hostAppLuminance->setOriginal (combinedLuminanceParam);	
	ConfigurationPublisher::addParam (persistentName, "hostAppLuminance", hostAppLuminance);
	
	AliasParam* hostAppSaturation = NEW AliasParam ("hostAppSaturation");
	hostAppSaturation->setOriginal (p1);
	ConfigurationPublisher::addParam (persistentName, "hostAppSaturation", hostAppSaturation);
	
	AliasParam* hostAppHue = NEW AliasParam ("hostAppHue");
	hostAppHue->setOriginal (hueParam);
	ConfigurationPublisher::addParam (persistentName, "hostAppHue", hostAppHue);
	
	initLevels ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorSchemeOption::~ColorSchemeOption ()
{
	ISubject::removeObserver (colorScheme, this);
	colorScheme->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorSchemeOption::initLevels ()
{
	initialHue = colorScheme->getLevel (IColorScheme::kHueLevel);
	initialSaturation = colorScheme->getLevel (IColorScheme::kSaturationLevel);
	initialLuminance = colorScheme->getLevel (IColorScheme::kLuminanceLevel);
	initialContrast = colorScheme->getLevel (IColorScheme::kContrastLevel);
	initialColorInversion = colorScheme->getLevel (IColorScheme::kColorInversion);
	initialMainSchemeDependentState = colorScheme->getLevel (IColorScheme::kMainSchemeDependent);
	updateEditLevels ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorSchemeOption::updateEditLevels ()
{
	const CString levelNames[] = 
	{
		IColorScheme::kHueLevel, 
		IColorScheme::kSaturationLevel, 
		IColorScheme::kLuminanceLevel, 
		IColorScheme::kContrastLevel,
		IColorScheme::kColorInversion,
		IColorScheme::kMainSchemeDependent
	};

	for(int i = 0; i < ARRAY_COUNT (levelNames); i++)
	{
		float editLevel = getEditLevel (levelNames[i]);
		float schemeLevel = colorScheme->getLevel (levelNames[i]);
		
		if(editLevel != schemeLevel)
		{
			setEditLevel (levelNames[i], schemeLevel);
			signal (Message (kLevelChanged, String (levelNames[i]), schemeLevel));
		}
	}

	float luminanceLevel = colorScheme->getLevel (IColorScheme::kLuminanceLevel) / 2.f;
	if(colorScheme->getLevel (IColorScheme::kColorInversion) != 0.f)
		luminanceLevel += 0.5f;
	
	paramList.byTag (Tag::kCombinedLuminance)->setNormalized (luminanceLevel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float ColorSchemeOption::getEditLevel (StringID id) const
{
	return paramList.lookup (id)->getNormalized ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorSchemeOption::setEditLevel (StringID id, float value)
{
	return paramList.lookup (id)->setNormalized (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorSchemeOption::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kCombinedLuminance)
	{
		ScopedVar<bool> scope (insideLevelEditing, true);
		
		float combinedLuminance = param->getValue ();
		float inversionLevel = (combinedLuminance > 0.5f) ? 1.f : 0.f;
		
		if(getEditLevel (IColorScheme::kColorInversion) != inversionLevel)
		{
			setEditLevel (IColorScheme::kColorInversion, inversionLevel);
			colorScheme->setLevel (IColorScheme::kColorInversion, inversionLevel);
			signal (Message (kLevelChanged, String (IColorScheme::kColorInversion), inversionLevel));
		}
			
		if(combinedLuminance > 0.5f)
			combinedLuminance -= 0.5f;
		
		setEditLevel (IColorScheme::kLuminanceLevel, combinedLuminance * 2.f);
		colorScheme->setLevel (IColorScheme::kLuminanceLevel, combinedLuminance * 2.f);
		signal (Message (kLevelChanged, String (IColorScheme::kLuminanceLevel), combinedLuminance * 2.f));
		
		deferChanged (); // update apply button
		return true;
	}
	if(param->getTag () == Tag::kLuminance)
	{
		float luminanceLevel = param->getNormalized ();
		colorScheme->setLevel (IColorScheme::kLuminanceLevel, luminanceLevel);
		
		float combinedLuminance = luminanceLevel / 2.f;
		if(getEditLevel (IColorScheme::kColorInversion) > 0)
			combinedLuminance += 0.5f;
		paramList.byTag (Tag::kCombinedLuminance)->setValue (combinedLuminance);
		
		signal (Message (kLevelChanged, String (IColorScheme::kLuminanceLevel), luminanceLevel));
		
		deferChanged (); // update apply button
		return true;
	}
	if(param->getTag () == Tag::kColorInversion)
	{
		float inversionLevel = param->getValue ().asFloat ();
		colorScheme->setLevel (IColorScheme::kColorInversion, inversionLevel);
		
		float combinedLuminance = (inversionLevel > 0.f) ? 0.5f : 0.f;
		float luminanceLevel = getEditLevel (IColorScheme::kLuminanceLevel);
		combinedLuminance += luminanceLevel / 2.f;
		
		paramList.byTag (Tag::kCombinedLuminance)->setValue (combinedLuminance);
		
		signal (Message (kLevelChanged, String (IColorScheme::kColorInversion), inversionLevel));
		
		deferChanged (); // update apply button
		return true;
	}
	
	StringID id = param->getName ();
	if(id == IColorScheme::kHueLevel || id == IColorScheme::kSaturationLevel || id == IColorScheme::kContrastLevel || id == IColorScheme::kMainSchemeDependent)
	{
		ScopedVar<bool> scope (insideLevelEditing, true);
		float level = getEditLevel (id);
		colorScheme->setLevel (id, level);

		signal (Message (kLevelChanged, String (id), level));

		deferChanged (); // update apply button
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorSchemeOption::needsApply () const
{
	if(getEditLevel (IColorScheme::kHueLevel) != initialHue)
		return true;
	if(getEditLevel (IColorScheme::kSaturationLevel) != initialSaturation)
		return true;
	if(getEditLevel (IColorScheme::kLuminanceLevel) != initialLuminance)
		return true;
	if(getEditLevel (IColorScheme::kContrastLevel) != initialContrast)
		return true;
	if(getEditLevel (IColorScheme::kColorInversion) != initialColorInversion)
		return true;
	if(getEditLevel (IColorScheme::kMainSchemeDependent) != initialMainSchemeDependentState)
		return true;
	return SuperClass::needsApply ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorSchemeOption::apply ()
{
	initLevels ();
	return SuperClass::apply ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorSchemeOption::opened ()
{
	initLevels ();

	SuperClass::opened ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorSchemeOption::closed ()
{
	// restore previous levels if canceled
	if(needsApply ())
	{
		colorScheme->setLevel (IColorScheme::kHueLevel, initialHue, IColorScheme::kIgnore);
		colorScheme->setLevel (IColorScheme::kSaturationLevel, initialSaturation, IColorScheme::kIgnore);
		colorScheme->setLevel (IColorScheme::kLuminanceLevel, initialLuminance, IColorScheme::kIgnore);
		colorScheme->setLevel (IColorScheme::kContrastLevel, initialContrast, IColorScheme::kIgnore);
		colorScheme->setLevel (IColorScheme::kColorInversion, initialColorInversion, IColorScheme::kIgnore);
		colorScheme->setLevel (IColorScheme::kMainSchemeDependent, initialMainSchemeDependentState, IColorScheme::kForce);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorSchemeOption::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && isEqualUnknown (subject, colorScheme))
	{
		if(insideLevelEditing == false)
		{
			updateEditLevels ();

			deferChanged (); // update apply button
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorSchemeOption::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "schemeName")
	{
		String name (colorScheme->getName ());
		var = name;
		var.share ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// MainColorSchemeOption
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MainColorSchemeOption, ColorSchemeOption)

//////////////////////////////////////////////////////////////////////////////////////////////////

MainColorSchemeOption::MainColorSchemeOption ()
: ColorSchemeOption (ThemeNames::kMain),
  useDirectSave (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MainColorSchemeOption::addConfigurationSavers ()
{
	ColorSchemeOption::addConfigurationSavers (ThemeNames::kMain);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MainColorSchemeOption::enableDirectSave (bool state)
{
	useDirectSave = state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MainColorSchemeOption::makeAppearanceMenu (IMenu& menu, bool useSubMenu)
{
	IMenu* subMenu = &menu;
	if(useSubMenu)
	{
		subMenu = menu.createMenu ();
		subMenu->setMenuAttribute (IMenu::kMenuTitle, XSTR (Appearance));
		menu.addMenu (subMenu);
	}	
	subMenu->addCommandItem (XSTR (DarkMode), "Appearance", "Dark Mode", this);
	subMenu->addCommandItem (XSTR (LightMode), "Appearance", "Light Mode", this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* MainColorSchemeOption::getCombinedLuminance () const
{
	return paramList.byTag (Tag::kCombinedLuminance);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MainColorSchemeOption::checkCommandCategory (CStringRef category) const
{
	if(category == "Appearance")
		return true;
	return SuperClass::checkCommandCategory (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MainColorSchemeOption::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == "Appearance")
	{
		int index = msg.name.startsWith ("Light") ? 1 : 0;
		IParameter* inversionParam = paramList.byTag (Tag::kColorInversion);
		
		if(msg.checkOnly ())
		{
			if(UnknownPtr<IMenuItem> menuItem = msg.invoker)
				menuItem->setItemAttribute (IMenuItem::kItemChecked, index == inversionParam->getValue ().asInt ());
		}
		else
			inversionParam->setValue (index, true);
		return true;
	}
	return SuperClass::interpretCommand (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MainColorSchemeOption::paramChanged (IParameter* param)
{
	tbool result = ColorSchemeOption::paramChanged (param);
	
	if(useDirectSave == true)
		if(param->getTag () == Tag::kCombinedLuminance || param->getTag () == Tag::kColorInversion)
			Settings::instance ().flush ();
	
	return result;
}

//************************************************************************************************
// ColorSchemePresetOption
//************************************************************************************************

IStorable* ColorSchemePresetOption::createSchemeFile (const PresetDescription& description)
{
	AutoPtr<IColorSchemes> colorSchemes = ccl_new<IColorSchemes> (ClassID::ColorSchemes);
	ASSERT (colorSchemes != nullptr)

	Vector<IColorScheme*> schemeList;
	for(int i = 0; i < description.count; i++)
		if(IColorScheme* colorScheme = colorSchemes->getScheme (description.names[i], true))
			schemeList.add (colorScheme);

	return colorSchemes->createSchemeFile (schemeList, schemeList.count (), description.importer, description.revision);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorSchemePresetOption::getUserPresetPath (Url& userPath, IStorable* colorSchemeFile)
{
	FileType fileType;
	if(colorSchemeFile != nullptr)
		colorSchemeFile->getFormat (fileType);
	else
	{
		AutoPtr<IColorSchemes> colorSchemes = ccl_new<IColorSchemes> (ClassID::ColorSchemes);
		ASSERT (colorSchemes != nullptr)
		fileType = colorSchemes->getSchemeFileType ();
	}

	System::GetSystem ().getLocation (userPath, System::kAppSettingsFolder);
	userPath.descend ("user");
	userPath.setFileType (fileType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorSchemePresetOption::restoreUserPreset (const PresetDescription& description)
{
	AutoPtr<IStorable> colorSchemeFile = createSchemeFile (description);

	Url userPath;
	getUserPresetPath (userPath, colorSchemeFile);
	
	bool restored = false;
	if(System::GetFileSystem ().fileExists (userPath))
		if(AutoPtr<IStream> stream = System::GetFileSystem ().openStream (userPath))
			restored = colorSchemeFile->load (*stream) != 0;

	if(restored == false)
	{
		// migrate from user settings
		bool migrated = false;
		Attributes& configuration = ConfigurationSaver::getAttributes (Settings::instance ());
		if(UnknownPtr<IContainer> c = static_cast<IUnknown*>(colorSchemeFile))
			ForEachUnknown (*c, unk)
				if(UnknownPtr<IColorScheme> scheme = unk)
				{
					MutableCString persistentName (IColorScheme::kPersistentPrefix);
					persistentName += scheme->getName ();

					auto makeAttrName = [&] (StringID id)
					{
						return ConfigurationSaver (persistentName, id).getAttributeName ();
					};
				
					const CString idList[] =
					{
						IColorScheme::kHueLevel, 
						IColorScheme::kSaturationLevel,
						IColorScheme::kLuminanceLevel,
						IColorScheme::kContrastLevel,
						IColorScheme::kColorInversion,
						IColorScheme::kMainSchemeDependent
					};

					if(configuration.contains (makeAttrName (idList[0])))
					{
						migrated = true;

						// copy from configuration to local attributes
						Attributes a;
						for(int i = 0; i < ARRAY_COUNT (idList); i++)
							a.set (idList[i], configuration.getFloat (makeAttrName (idList[i])));

						// adjust attributes via importer
						if(UnknownPtr<IColorSchemeImporter> importer = static_cast<IUnknown*>(colorSchemeFile))
							importer->adjustScheme (scheme->getName (), a, 0);

						int count = ARRAY_COUNT (idList);
						for(int i = 0; i < count; i++)
						{
							StringID id = idList[i];
							float value = (float)a.getFloat (id);
							bool update = i == count-1;
							scheme->setLevel (id, value, update ? IColorScheme::kForce : IColorScheme::kIgnore);
						}
					}
				}
			EndFor

		if(migrated == true)
			storeUserPreset (colorSchemeFile);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorSchemePresetOption::storeUserPreset (IStorable* colorSchemeFile)
{
	Url userPath;
	getUserPresetPath (userPath, colorSchemeFile);

	if(AutoPtr<IStream> stream = System::GetFileSystem ().openStream (userPath, IStream::kCreateMode))
		colorSchemeFile->save (*stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorSchemePresetOption* ColorSchemePresetOption::theInstance = nullptr;
ColorSchemePresetOption* ColorSchemePresetOption::getInstance () { return theInstance; }

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (ColorSchemePresetOption, UserOption)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorSchemePresetOption::ColorSchemePresetOption (const PresetDescription& description)
: UserOption ("ColorSchemePresetOption"),
  colorSchemeFile (createSchemeFile (description)),
  presetComponent (nullptr)
{
	ASSERT (theInstance == nullptr)
	theInstance = this;

	setTitle (String () << General () << strSeparator << XSTR (Appearance));
	setFormName ("CCL/ColorSchemePresetOption");

	paramList.addParam ("resetLevels", kResetLevels);

	// Presets
	FileType fileType;
	colorSchemeFile->getFormat (fileType);
	this->presetClassName = fileType.getDescription ();
	this->presetCategory = CCLSTR ("ColorScheme");

	SimplePresetHandler* handler = NEW SimplePresetHandler (fileType);
	handler->setPresetFolderName (CCLSTR ("Color Schemes"));
	handler->setPresetCategory (presetCategory);
	handler->setPresetClassName (presetClassName);
	handler->registerSelf ();

	presetComponent = NEW PresetComponent (this);
	presetComponent->setPresetType (MutableCString (fileType.getMimeType ()));
	presetComponent->setOptions (0);
	presetComponent->setCurrentPresetName (nullptr);
	addComponent (presetComponent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorSchemePresetOption::~ColorSchemePresetOption ()
{
	if(theInstance == this)
		theInstance = nullptr;

	colorSchemeFile->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorSchemePresetOption::isSchemeFile (UrlRef path) const
{
	FileType fileType;
	colorSchemeFile->getFormat (fileType);
	return path.getFileType () == fileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorSchemePresetOption::openFile (UrlRef path)
{
	if(isSchemeFile (path))
		return presetComponent->restorePreset (path) != 0;
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* ColorSchemePresetOption::createDragHandler (const DragEvent& event, IView* view)
{
	return presetComponent->createDragHandler (event, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ColorSchemePresetOption::getPresetTarget ()
{
	return colorSchemeFile;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorSchemePresetOption::getPresetMetaInfo (IAttributeList& metaInfo)
{
	PresetMetaAttributes metaAttributes (metaInfo);
	metaAttributes.setCategory (presetCategory);
	metaAttributes.setClassName (presetClassName);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorSchemePresetOption::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case kResetLevels :
		resetLevels ();
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorSchemePresetOption::resetLevels ()
{
	if(UnknownPtr<IContainer> c = colorSchemeFile)
		ForEachUnknown (*c, unk)
			if(UnknownPtr<IColorScheme> colorScheme = unk)
				colorScheme->resetToDefaults ();
		EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorSchemePresetOption::closed ()
{
	SuperClass::closed ();

	storeUserPreset (colorSchemeFile);
}
