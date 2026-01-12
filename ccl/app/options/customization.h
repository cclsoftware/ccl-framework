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

#ifndef _ccl_customization_h
#define _ccl_customization_h

#include "ccl/app/component.h"

#include "ccl/base/collections/objectlist.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/public/storage/filetype.h"
#include "ccl/public/gui/commanddispatch.h"

namespace CCL {

interface IMenu;
class Document;
class CustomizationPreset;

//************************************************************************************************
// CustomizationComponent
//************************************************************************************************

class CustomizationComponent: public Component,
							  public CommandDispatcher<CustomizationComponent>
{
public:
	DECLARE_CLASS (CustomizationComponent, Component)
	DECLARE_COMMANDS (CustomizationComponent)

	CustomizationComponent ();
	CustomizationComponent (StringRef name);
	~CustomizationComponent ();

	static StringRef Customization ();
	static StringRef EditCustomization ();

	static const Container& getInstances ();
	static CustomizationComponent* findCustomizationComponent (const FileType& documentType);

	static String getSettingsFileName ();
	static UrlRef getSettingsPath ();

	PROPERTY_BOOL (enabled, Enabled)
	PROPERTY_MUTABLE_CSTRING (formName, FormName)
	PROPERTY_OBJECT (FileType, documentFileType, DocumentFileType)
	PROPERTY_BOOL (modalEditor, ModalEditor)

	bool matchesDocument (const Document& document);

	void setMenu (IMenu* menu);
	void updateMenu ();

	CustomizationPreset* getPreset (StringRef name) const;
	CustomizationPreset* getPresetByID (StringRef id) const;
	CustomizationPreset* getSelectedPreset () const;
	CustomizationPreset* getLastUserSelectedPreset () const;
	int countPresets () const;
	int countUserPresets () const;
	bool selectPreset (StringRef name, bool userAction = true);
	void selectPreset (CustomizationPreset& preset, bool userAction = true);
	void addPreset (CustomizationPreset* preset);
	void removePreset (CustomizationPreset* preset);
	void renamePreset (CustomizationPreset* preset, StringRef newName);
	IParameter* getUserSelectedPresetParameter () const;

	CustomizationPreset* getFactoryPreset (StringRef name) const;

	void setDefaultVisible (StringID key, bool visible);

	// Component
	tresult CCL_API initialize (IUnknown* context) override;
	tresult CCL_API terminate () override;
	IParameter* CCL_API findParameter (StringID name) const override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override;

protected:
	static ObjectArray instances;

	ObjectList presets;
	CustomizationPreset* lastUserSelectedPreset;
	IMenu* menu;
	CustomizationPreset& stateBeforeEdit; // state to restore on "revert" (captured when editor opened / preset selected)
	bool wasEditConfirmed;
	bool settingsRestored;

	class PresetRenamer;
	class Manager;
	Manager* manager;

	Attributes& getSettings () const;
	void storeSettings ();
	void restoreSettings ();
	void resetPresets ();

	void updatePresetList ();
	void makeUniquePresetName (String& name);
	void storePreset (CustomizationPreset& preset);
	void restorePreset (const CustomizationPreset& preset);
	void setLastUserSelectedPreset (CustomizationPreset& preset);
	void createParameters (const CustomizationPreset& preset);

	// Commands
	DECLARE_COMMAND_CATEGORY ("View", Component)
	bool onShowConfigurationEditor (CmdArgs args);
	bool onShowConfigurationEditor (CmdArgs args, VariantRef context);
	bool onSelectPreset (CmdArgs args);
	bool onSelectPreset (CmdArgs args, VariantRef name);
};

//************************************************************************************************
// CustomizationPreset
//************************************************************************************************

class CustomizationPreset: public Object
{
public:
	DECLARE_CLASS (CustomizationPreset, Object)

	CustomizationPreset ();

	PROPERTY_STRING (id, ID)
	PROPERTY_STRING (name, Name)
	PROPERTY_BOOL (readOnly, ReadOnly)

	const Attributes& getAttributes () const;
	Attributes& getAttributes ();

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
	bool toString (String& string, int flags = 0) const override;

private:
	PersistentAttributes attributes;
};

//************************************************************************************************
// CustomizationPresetMemento
//************************************************************************************************

class CustomizationPresetMemento: public Object
{
public:
	CustomizationPresetMemento (CustomizationComponent* customizationComponent);

	/** Asks the user if he wants to keep the current preset (captured in constructor) or revert to the previous one. */
	void confirmCustomization ();

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

private:
	SharedPtr<CustomizationComponent> customizationComponent;
	SharedPtr<CustomizationPreset> previousPreset;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline int CustomizationComponent::countPresets () const
{ return presets.count (); }

inline const Attributes& CustomizationPreset::getAttributes () const
{ return attributes; }

inline Attributes& CustomizationPreset::getAttributes ()
{ return attributes; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_customization_h
