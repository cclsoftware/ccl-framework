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
// Filename    : ccl/app/options/mainoption.h
// Description : Main Option
//
//************************************************************************************************

#ifndef _ccl_mainoption_h
#define _ccl_mainoption_h

#include "ccl/app/options/useroption.h"

#include "ccl/public/app/ipreset.h"

namespace CCL {

interface IMenu;
interface IColorScheme;
interface IColorSchemes;
interface IColorSchemeImporter;
interface IStorable;
interface IDragHandler;

struct DragEvent;
class SignalSink;
class PathSelector;
class PresetComponent;

//************************************************************************************************
// LocaleOption
//************************************************************************************************

class LocaleOption: public UserOption
{
public:
	DECLARE_CLASS (LocaleOption, UserOption)

	LocaleOption ();
	~LocaleOption ();

	static LocaleOption* findInstance ();

	PROPERTY_BOOL (languageRestartEnabled, LanguageRestartEnabled)

	void addLanguage (StringID languageCode);
	void addLanguagePacks (bool update = false);

	void makeLanguageMenu (IMenu& menu);

	// UserOption
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

protected:
	SignalSink& localeSink;

	class LanguageItem;
	enum Tags { kLanguage = 100 };

	void addLanguage (LanguageItem* item);
	bool hasLanguage (const LanguageItem& item) const;
	bool isCurrent (const LanguageItem* item) const;
};

//************************************************************************************************
// ContentLocationOption
//************************************************************************************************

class ContentLocationOption: public UserOption
{
public:
	DECLARE_CLASS (ContentLocationOption, UserOption)

	ContentLocationOption ();
	~ContentLocationOption ();

	static void registerSaver ();

	bool runSelector ();

	// UserOption
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	PathSelector* pathSelector;
};

//************************************************************************************************
// AutoSaveOption
//************************************************************************************************

class AutoSaveOption: public UserOption
{
public:
	DECLARE_CLASS (AutoSaveOption, UserOption)

	AutoSaveOption ();

	static void registerSaver ();
};

//************************************************************************************************
// UserInterfaceOption
//************************************************************************************************

class UserInterfaceOption: public UserOption
{
public:
	DECLARE_CLASS (UserInterfaceOption, UserOption)

	UserInterfaceOption ();

	static void registerSaver ();
	static UserOptionElement* createSliderModes ();
};

//************************************************************************************************
// DpiAwarenessOption - WINDOWS only!
//************************************************************************************************

class DpiAwarenessOption: public UserOption
{
public:
	DECLARE_CLASS (DpiAwarenessOption, UserOption)

	DpiAwarenessOption ();

	// UserOption
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	enum Tags { kEnabled = 100, kScaling };

	void updateScaling (float dpiFactor);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// GraphicsEngineOption - MAC only!
//************************************************************************************************

class GraphicsEngineOption: public UserOption
{
public:
	DECLARE_CLASS (GraphicsEngineOption, UserOption)

	GraphicsEngineOption ();

	// UserOption
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	enum Tags { kHWAccelerationEnabled = 100 };
};

//************************************************************************************************
// ColorSchemeOption
//************************************************************************************************

class ColorSchemeOption: public UserOption
{
public:
	DECLARE_CLASS_ABSTRACT (ColorSchemeOption, UserOption)

	ColorSchemeOption (StringID schemeName);
	~ColorSchemeOption ();

	DECLARE_STRINGID_MEMBER (kLevelChanged)	///< args[0]: name, args[1]: value

	static void addConfigurationSavers (StringID schemeName);
	
	float getEditLevel (StringID id) const;
	
	// UserOption
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	tbool CCL_API needsApply () const override;
	tbool CCL_API apply () override;
	void CCL_API opened () override;
	void CCL_API closed () override;
	
protected:
	IColorScheme* colorScheme;
	float initialHue;
	float initialSaturation;
	float initialLuminance;
	float initialContrast;
	float initialColorInversion;
	float initialMainSchemeDependentState;
	bool insideLevelEditing;
	
	void initLevels ();
	virtual void updateEditLevels ();
	void setEditLevel (StringID id, float value);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// MainColorSchemeOption
//************************************************************************************************

class MainColorSchemeOption: public ColorSchemeOption
{
public:
	DECLARE_CLASS_ABSTRACT (MainColorSchemeOption, ColorSchemeOption)
	
	MainColorSchemeOption ();
	
	static void addConfigurationSavers ();
	void enableDirectSave (bool state); ///< flush settings directly when color parameters changed
	
	void makeAppearanceMenu (IMenu& menu, bool useSubMenu = false);
	
	IParameter* getCombinedLuminance () const;
	
	// ColorSchemeOption
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;
	tbool CCL_API paramChanged (IParameter* param) override;

private:
	bool useDirectSave;
};
	
//************************************************************************************************
// ColorSchemePresetOption
//************************************************************************************************

class ColorSchemePresetOption: public UserOption,
							   public AbstractPresetMediator
{
public:
	DECLARE_CLASS_ABSTRACT (ColorSchemePresetOption, UserOption)

	struct PresetDescription
	{
		CStringPtr* names;
		int count;
		IColorSchemeImporter* importer;
		int revision;

		PresetDescription (CStringPtr names[], int count, 
						   IColorSchemeImporter* importer = nullptr, int revision = 0)
		: names (names),
		  count (count),
		  importer (importer),
		  revision (revision)
		{}
	};

	ColorSchemePresetOption (const PresetDescription& description);
	~ColorSchemePresetOption ();

	static ColorSchemePresetOption* getInstance ();
	static void getUserPresetPath (Url& userPath, IStorable* = nullptr);
	static void restoreUserPreset (const PresetDescription& description);

	bool isSchemeFile (UrlRef path) const;
	bool openFile (UrlRef path);
	IDragHandler* createDragHandler (const DragEvent& event, IView* view);

	// UserOption
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API closed () override;

	CLASS_INTERFACE (IPresetMediator, UserOption)

protected:
	static IStorable* createSchemeFile (const PresetDescription& description);
	static void storeUserPreset (IStorable*);
	static ColorSchemePresetOption* theInstance;

	enum Tags { kResetLevels = 100 };

	IStorable* colorSchemeFile;
	PresetComponent* presetComponent;
	String presetCategory;
	String presetClassName;

	void resetLevels ();

	// IPresetMediator
	IUnknown* CCL_API getPresetTarget () override;
	tbool CCL_API getPresetMetaInfo (IAttributeList& metaInfo) override;
};

} // namespace CCL

#endif // _ccl_mainoption_h
