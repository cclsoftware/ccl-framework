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
// Filename    : ccl/gui/theme/thememanager.cpp
// Description : Theme Manager
//
//************************************************************************************************

#include "ccl/gui/theme/thememanager.h"
#include "ccl/gui/theme/usertheme.h"

#include "ccl/gui/skin/form.h"
#include "ccl/gui/skin/skinregistry.h"
#include "ccl/gui/skin/skinwizard.h"
#include "ccl/gui/controls/variantview.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/windows/desktop.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/gui/framework/controlsignals.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IThemeManager& CCL_API System::CCL_ISOLATED (GetThemeManager) ()
{
	return ThemeManager::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (ThemeManager, kFrameworkLevelSecond)
{
	// force load of framework translations and theme
	FrameworkTheme::instance ();
	return true;
}

//************************************************************************************************
// FrameworkTheme
//************************************************************************************************

Theme& FrameworkTheme::instance ()
{
	return ThemeManager::instance ().getFrameworkTheme ();
}

//************************************************************************************************
// ThemeManager
//************************************************************************************************

DEFINE_SINGLETON (ThemeManager)
DEFINE_CLASS_ABSTRACT_HIDDEN (ThemeManager, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeManager::ThemeManager ()
: defaultTheme (nullptr),
  frameworkTheme (nullptr),
  frameworkStrings (nullptr)
{
	themes.objectCleanup (true);

	#if SKIN_DEVELOPMENT_LOCATIONS_ENABLED
	SkinRegistry::instance ().loadDevelopmentLocations ();
	#endif

	NativeGraphicsEngine::instance ().addCleanup (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeManager::~ThemeManager ()
{
	if(frameworkStrings)
	{
		if(LocalString::hasTable () && LocalString::getTable () == frameworkStrings)
			LocalString::tableDestroyed ();
	
		System::GetLocaleManager ().unloadStrings (frameworkStrings);
		frameworkStrings = nullptr;
	}

	cleanupGraphics ();
	
	ASSERT (themes.isEmpty () == true)

	if(defaultTheme && !themes.contains (defaultTheme))
		defaultTheme->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemeManager::cleanupGraphics ()
{
	if(frameworkTheme)
	{
		unloadTheme (frameworkTheme);
		frameworkTheme = nullptr;
	}

	ThemePainter::resetStandardStyles ();
	Theme::resetSharedStyles ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Theme& ThemeManager::getDefaultTheme ()
{
	ASSERT (defaultTheme != nullptr)
	if(!defaultTheme)
		defaultTheme = NEW Theme;
	return *defaultTheme;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemeManager::updateSystemMetrics ()
{
	NativeThemePainter& systemPainter = NativeThemePainter::instance ();
	int sysMetric = 0;
	
	if(systemPainter.getSystemMetric (sysMetric, ThemeElements::kSystemStatusBarHeight))
		frameworkTheme->setMetric (ThemeElements::kSystemStatusBarHeight, sysMetric);
	if(systemPainter.getSystemMetric (sysMetric, ThemeElements::kSystemNavigationBarHeight))
		frameworkTheme->setMetric (ThemeElements::kSystemNavigationBarHeight, sysMetric);
	if(systemPainter.getSystemMetric (sysMetric, ThemeElements::kSystemMarginLeft))
		frameworkTheme->setMetric (ThemeElements::kSystemMarginLeft, sysMetric);
	if(systemPainter.getSystemMetric (sysMetric, ThemeElements::kSystemMarginRight))
		frameworkTheme->setMetric (ThemeElements::kSystemMarginRight, sysMetric);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemeManager::updateSystemColors ()
{
	NativeThemePainter& systemPainter = NativeThemePainter::instance ();
	Color sysColor;
	
	if(systemPainter.getSystemColor (sysColor, ThemeElements::kTooltipBackColor))
		frameworkTheme->setColor (ThemeElements::kTooltipBackColor, sysColor);
	if(systemPainter.getSystemColor (sysColor, ThemeElements::kTooltipTextColor))
		frameworkTheme->setColor (ThemeElements::kTooltipTextColor, sysColor);
	if(systemPainter.getSystemColor (sysColor, ThemeElements::kListViewBackColor))
		frameworkTheme->setColor (ThemeElements::kListViewBackColor, sysColor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemeManager::updateSystemFonts ()
{
	NativeThemePainter& systemPainter = NativeThemePainter::instance ();
	Font sysFont;
	
	if(systemPainter.getSystemFont (sysFont, ThemeElements::kMenuFont))
		frameworkTheme->setFont (ThemeElements::kMenuFont, sysFont);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Theme& ThemeManager::getFrameworkTheme ()
{
	if(!frameworkTheme)
	{
		// load from resources
		ASSERT (frameworkStrings == nullptr)
		ModuleRef module = System::GetCurrentModuleRef ();
		System::GetLocaleManager ().loadModuleStrings (frameworkStrings, module, kFrameworkSkinID);
		LocalString::setTable (frameworkStrings);

        #if CCL_STATIC_LINKAGE
		ResourceUrl skinUrl (CCLSTR ("cclgui"), Url::kFolder);
		module = 0; // otherwise getApplicationTheme() would return the framework theme!
        #else
		ResourceUrl skinUrl (CCLSTR ("skin"), Url::kFolder);
        #endif
		
		ITheme* theme = nullptr;
		loadTheme (theme, skinUrl, kFrameworkSkinID, frameworkStrings, module);
		frameworkTheme = unknown_cast<Theme> (theme);
		ASSERT (frameworkTheme != nullptr)

		updateSystemColors ();
		updateSystemFonts ();
		updateSystemMetrics ();
	}
	return *frameworkTheme;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API ThemeManager::getThemeFileType () const
{
	return UserTheme::getFileType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ThemeManager::loadTheme (ITheme*& theme, UrlRef _path, StringID themeID, ITranslationTable* table, ModuleRef module)
{
	ASSERT (module == nullptr || getModuleTheme (module) == nullptr) // avoid duplicates!

	#if SKIN_DEVELOPMENT_LOCATIONS_ENABLED
	Url path (_path), devPath;
	if(SkinRegistry::instance ().getDevelopmentLocation (devPath, String (themeID)))
		path = devPath;
	#else
	UrlRef path = _path;
	#endif

	UserTheme* userTheme = NEW UserTheme (themeID, table, module);

	bool loaded = userTheme->load (path);
	
	theme = userTheme;
	themes.add (userTheme);

	if(!defaultTheme)
		defaultTheme = userTheme;

	return loaded ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITheme* CCL_API ThemeManager::getTheme (StringID themeID) const
{
	if(SkinWizard* skin = SkinRegistry::instance ().getSkin (themeID))
		return skin->getTheme ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITheme* CCL_API ThemeManager::getModuleTheme (ModuleRef module) const
{
	if(SkinWizard* skin = SkinRegistry::instance ().getModuleSkin (module))
		return skin->getTheme ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITheme* CCL_API ThemeManager::getApplicationTheme () const
{
	if(SkinWizard* skin = SkinRegistry::instance ().getApplicationSkin ())
		return skin->getTheme ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ThemeManager::reloadTheme (ITheme* _theme, tbool keepImages)
{
	UserTheme* theme = unknown_cast<UserTheme> (_theme);
	ASSERT (theme != nullptr)
	return theme && theme->reload () ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ThemeManager::reloadAll (tbool keepImages)
{
	Theme::resetSharedStyles ();

	ForEach (themes, UserTheme, theme)
		theme->reload (keepImages != 0);
	EndFor

	// reset invalid SkinElement references in forms remaining from old skin
	struct FormResetter
	{
		static void resetForms (View* parent)
		{
			if(Form* form = ccl_cast<Form> (parent))
				form->setSkinElement (nullptr);
			else if(VariantView* variant = ccl_cast<VariantView> (parent))
			{
				IterForEach (variant->getVariants (), View, var)
					resetForms (var); // recursion
				EndFor
				return;
			}

			ForEachViewFast (*parent, view)
				resetForms (view); // recursion
			EndFor
		}
	};

	for(int i = 0, numWindows = Desktop.countWindows (); i < numWindows; i++)
		if(Window* window = unknown_cast<Window> (Desktop.getWindow (i)))
			FormResetter::resetForms (window);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ThemeManager::addSearchLocation (UrlRef folder)
{
	SkinRegistry::instance ().addSearchLocation (folder);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemeManager::onSystemMetricsChanged ()
{
	updateSystemMetrics ();
	SignalSource (Signals::kGUI).signal (Message (Signals::kSystemMetricsChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemeManager::onSystemColorsChanged ()
{
	updateSystemColors ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemeManager::onSystemFontsChanged ()
{
	updateSystemFonts ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ThemeManager::unloadTheme (ITheme* _theme)
{
	Theme* theme = unknown_cast<Theme> (_theme);
	ASSERT (theme != nullptr)
	if(theme)
	{
		if(theme == defaultTheme)
			defaultTheme = nullptr;

		themes.remove (theme);
		theme->release ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ThemeManager)
	DEFINE_METHOD_NAME ("getTheme")
	DEFINE_METHOD_NAME ("loadTheme")
	DEFINE_METHOD_NAME ("unloadTheme")
END_METHOD_NAMES (ThemeManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ThemeManager::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "getTheme")
	{
		MutableCString themeID (msg[0].asString ());
		ITheme* theme = getTheme (themeID);
		returnValue = theme;
		return true;
	}
	else if(msg == "loadTheme")
	{
		ITheme* theme = nullptr;
		UnknownPtr<IUrl> path (msg[0]);
		MutableCString themeID (msg[1].asString ());
		if(path)
			loadTheme (theme, *path, themeID); // TODO: translations???
		returnValue = theme;
		return true;
	}
	else if(msg == "unloadTheme")
	{
		Theme* theme = unknown_cast<Theme> (msg[0]);
		tresult result = theme ? unloadTheme (theme) : kResultInvalidArgument;
		returnValue = result;
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
