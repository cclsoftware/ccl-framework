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
// Filename    : ccl/gui/skin/skinwizard.h
// Description : Skin Wizard
//
//************************************************************************************************

#ifndef _ccl_skinwizard_h
#define _ccl_skinwizard_h

#include "ccl/gui/skin/skinelement.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/variant.h"

namespace CCL {

class View;
class Url;
class Container;
class SkinModel;
class SkinProtocol;
class SkinOverlay;
class SkinVariable;
class FileType;
class VisualStyle;

namespace SkinElements {
class ViewElement; }

interface IPackageFile;
interface IAttributeList;

//************************************************************************************************
// SkinWizard
//************************************************************************************************

class SkinWizard: public Object,
				  public ISkinContext
{
public:
	DECLARE_CLASS (SkinWizard, Object)

	SkinWizard (StringID skinID = nullptr, Theme* theme = nullptr, ITranslationTable* table = nullptr);
	~SkinWizard ();

	static const FileType& getSkinFileType ();

	PROPERTY_VARIABLE (ModuleRef, moduleReference, ModuleReference)

	// Skin Loading
	bool loadSkin (UrlRef url, bool keepImages = false, bool loadAllImages = false);
	bool reloadSkin (bool keepImages = false);
	bool isSkinLoaded () const;
	static bool isReloadingSkin ();

	// Data Model
	bool setCurrentScope (CStringRef scope);
	CStringRef getCurrentScope () const;

	SkinModel& getRoot ();	///< returns root model
	SkinModel& getModel (); ///< returns model selected by setCurrentScope
	SkinModel* getScopeModel (CStringRef scope);

	// View Creation
	View* createView (CStringRef name, IUnknown* controller = nullptr);
	View* createView (SkinElements::Element* element, IUnknown* controller, View* parent = nullptr, SkinElements::ViewElement* parentElement = nullptr);

	// Helpers
	struct ResolvedName;
	CStringRef resolveName (MutableCString& resolvedName, CStringRef name) const;
	String resolveTitle (StringRef title) const;
	bool resolveNumber (Variant& resolvedValue, StringRef valueString, IUnknown* controller, SkinElements::Element* element) const;
	bool resolveDefine (Variant& resolvedValue, StringRef variableString, IUnknown* controller) const;
	IUnknown* lookupController (IUnknown* currentController, CStringRef path) const;
	VisualStyle* lookupStyle (StringID styleName, SkinElements::Element* caller) const;
	void setZoomFactor (float factor);
	float getZoomFactor () const;

	// Variables
	void addVariable (CStringRef name, VariantRef value);
	bool removeVariable (CStringRef name);
	void addVariables (IAttributeList& list);
	void removeVariables (IAttributeList& list);
	void getVariables (IAttributeList& list) const;
	const SkinVariable* getVariable (CStringRef name) const;

	// Overlays
	void addOverlay (SkinOverlay* overlay);
	void removeOverlay (SkinOverlay* overlay);

	// ISkinContext
	StringID getSkinID () const override;
	IFileSystem* getFileSystem () const override;
	ITranslationTable* getStringTable () const override;
	Theme* getTheme () const override;
	IPackageFile* getPackage () const override;

protected:
	static bool insideReloadSkin;
	MutableCString skinID;
	Theme* theme;
	ITranslationTable* stringTable;
	IPackageFile* package;
	SkinModel* model;
	MutableCString scopeName;
	SkinModel* scopeModel;
	ObjectArray variables;
	ObjectArray overlays;
	SkinProtocol* skinProtocol;
	float currentZoomFactor;

	bool loadIncludes (SkinModel* model, Container& resolved);
	bool loadImports (SkinModel* model, const Url& currentDir, Container& resolved);
	bool resolveImport (Url& result, StringRef name, const Url& skinsFolder, const Url& currentDir);
	void setModel (SkinModel* model, IPackageFile* package);
	SkinOverlay* findOverlay (StringID scope, StringID name) const;
	void checkResources ();

	View* createView (CStringRef scope, CStringRef name, IUnknown* controller = nullptr);
	View* createViewElements (SkinElements::Element* element, IUnknown* controller, View* parent, SkinElements::ViewElement* parentElement);

	friend class Form;
	void createChildElements (SkinElements::Element* containerElement, IUnknown* controller, View* parent, SkinElements::ViewElement* parentElement);
};

//************************************************************************************************
// SkinWizard::ResolvedName
//************************************************************************************************

struct SkinWizard::ResolvedName
{
	MutableCString temp;
	const CString* result;
	ResolvedName (const SkinWizard& wizard, CStringRef name, bool mustResolve = true)
	: result (nullptr)
	{
		result = mustResolve ? &wizard.resolveName (temp, name) : &name;
	}

	CStringRef string () const { return *result; }
};

//************************************************************************************************
// SkinScopeSelector
/** Helper class for selecting skin scope */
//************************************************************************************************

struct SkinScopeSelector
{
	SkinWizard& wizard;
	MutableCString oldScope;

	SkinScopeSelector (CStringRef scope, SkinWizard& wizard)
	: wizard (wizard),
	  oldScope (wizard.getCurrentScope ())
	{ wizard.setCurrentScope (scope); }

	~SkinScopeSelector ()
	{ wizard.setCurrentScope (oldScope); }
};

//************************************************************************************************
// SkinArgumentScope
/** Helper class to add/remove skin variables. */
//************************************************************************************************

struct SkinArgumentScope
{
	SkinWizard& wizard;
	IAttributeList* arguments;

	SkinArgumentScope (SkinWizard& wizard, IAttributeList* arguments)
	: wizard (wizard),
	  arguments (arguments)
	{ if(arguments) wizard.addVariables (*arguments); }

	~SkinArgumentScope ()
	{ if(arguments) wizard.removeVariables (*arguments); }
};

//************************************************************************************************
// SkinVariable
//************************************************************************************************

class SkinVariable: public Object
{
public:
	SkinVariable (CStringRef name = nullptr, VariantRef value = Variant ())
	: name (name),
	  value (value)
	{}

	static const CString kPrefix;
	static const String prefix;
	static const String themePrefix;

	PROPERTY_MUTABLE_CSTRING (name, Name)
	PROPERTY_OBJECT (Variant, value, Value)
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// SkinWizard inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void SkinWizard::createChildElements (SkinElements::Element* containerElement, IUnknown* controller, View* parent, SkinElements::ViewElement* parentElement)
{
	ArrayForEachFast (*containerElement, SkinElements::Element, e)
		createView (e, controller, parent, parentElement);
	EndFor
}

} // namespace CCL

#endif // _ccl_skinwizard_h
