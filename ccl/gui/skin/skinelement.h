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
// Filename    : ccl/gui/skin/skinelement.h
// Description : Skin Element class
//
//************************************************************************************************

#ifndef _ccl_skinelement_h
#define _ccl_skinelement_h

#include "ccl/gui/guitypelib.h"

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/gui/framework/iskinmodel.h"
#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

class Url;
class Theme;
class SkinAttributes;

interface IFileSystem;
interface IPackageFile;
interface ITranslationTable;

namespace SkinElements {
class Element; }

//************************************************************************************************
// Skin Warnings
//************************************************************************************************

extern void SkinWarning (const SkinElements::Element* element, CStringRef warning);

#define SKIN_WARNING(e, s, ...) \
{ CCL::MutableCString __warning; __warning.appendFormat (s, __VA_ARGS__); CCL::SkinWarning (e, __warning); }

//************************************************************************************************
// ISkinContext
//************************************************************************************************

class ISkinContext
{
public:
	virtual StringID getSkinID () const = 0;
	virtual IFileSystem* getFileSystem () const = 0;
	virtual ITranslationTable* getStringTable () const = 0;
	virtual Theme* getTheme () const = 0;
	virtual IPackageFile* getPackage () const = 0;

	DECLARE_STRINGID_MEMBER (kImportID)
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// SkinElements namespace starts here
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace SkinElements {

class Element;

//************************************************************************************************
// SkinElementLibrary
/** The Skin Elements type library: the CCL::TypeLibrary singleton that every Skin XML tag class
	and enumeration registers into. */
//************************************************************************************************

class SkinElementLibrary: public TypeLibrary,
						   public Singleton<SkinElementLibrary>
{
public:
	SkinElementLibrary ();

	constexpr static bool kTagsCaseSensitive = false; ///< case sensitivity of skin tags

	/** Resolve every registered TypeLibClass's parent-type link by walking the real RTTI
		parent chain until a registered ancestor is found. Call once, after all TypeLibClass
		for this library have self-registered (see CCL_KERNEL_INIT_LEVEL in skinelement.cpp). */
	void init ();

	Element* createElement (CStringRef name) const;
	const StyleDef* getStyleDef (CStringRef name, bool partialMatching = false) const;

protected:
	const TypeLibClass* findByMetaClass (MetaClassRef metaClass) const;
};

inline SkinElementLibrary& getTypeLibrary () { return SkinElementLibrary::instance (); }

#define SKIN_TYPELIB CCL::SkinElements::getTypeLibrary ()

//************************************************************************************************
// Element
/** The base class of all skin elements.
This class is not used directly, but all other skin element classes inherit from it. */
//************************************************************************************************

class Element: public ObjectArray,
			   public ITypeLibReference,
			   public ISkinElement,
			   public ISkinElementChildren
{
public:
	DECLARE_CLASS (Element, ObjectArray)
	DECLARE_TYPELIB_CLASS (Element)

	Element (CStringRef name = nullptr);

	static bool isSkinWarningsEnabled ();

	PROPERTY_MUTABLE_CSTRING (fileName, FileName)
	PROPERTY_VARIABLE (int, lineNumber, LineNumber)

	StringID CCL_API getName () const override { return name; } // ISkinElement
	void CCL_API setName (StringID _name) override { name = _name; } // ISkinElement
	void setName (StringRef name);

	virtual void addChild (Element* e, int index = -1);
	virtual void removeChild (Element* e);

	/** Called when a sibling with the same name is about to be added;
		Returns true if a custom action was performed, otherwise the existing element is removed (default). */
	virtual bool mergeElements (Element& other); 

	void takeElements (Element& source); ///< move all children from source to this

	Element* getParent () const;
	void setParent (Element* parent);
	Element* getParent (MetaClassRef typeId) const;

	Element* findElement (CStringRef name) const;
	Element* findElement (CStringRef name, MetaClassRef typeId) const;
	Element* findElement (MetaClassRef typeId) const;
	Element* getElement (int index) const;

	template <class T> T* findElement () const					{ return (T*)findElement (ccl_typeid<T> ()); }
	template <class T> T* findElement (CStringRef name) const	{ return (T*)findElement (name, ccl_typeid<T> ()); }

	virtual bool setAttributes (const SkinAttributes& a);
	virtual bool getAttributes (SkinAttributes& a) const;
	virtual void loadFinished (); ///< when element tag closes (after all children are loaded)
	virtual bool isOverrideEnabled () const; ///< silence warnings for intentional overrides

	virtual void appendCharacterData (UStringPtr data, int length);
	
	// Context information
	virtual ISkinContext* getSkinContext () const;
	IFileSystem* getFileSystem () const;
	Url& makeSkinUrl (Url& url, StringRef path, bool isFolder = false) const;
	virtual const Element* getTranslationScope () const;
	String translate (StringRef text) const;
	String translateWithScope (StringID scopeName, StringRef text) const;
	Theme* getTheme () const;
	tbool CCL_API getSourceInfo (String& fileName, int32& lineNumber, IUrl* packageUrl = nullptr) const override; // ISkinElement
	void CCL_API setSourceFile (StringRef fileName) override; // ISkinElement

	CLASS_INTERFACE3 (ITypeLibReference, ISkinElement, ISkinElementChildren, ObjectArray)

	class SortingSuspender;

protected:
	MutableCString name;
	String comment;
	Element* parent;
	bool sorted;

	// ObjectArray
	int compare (const Object& obj) const override;

	// ISkinElement
	void CCL_API getComment (String& _comment) const override { _comment = comment; }
	void CCL_API setComment (StringRef _comment) override { comment = _comment; }
	void CCL_API getAttributes (IAttributeList& attributes) const override;
	void CCL_API setAttributes (const IAttributeList& attributes) override;
	tbool CCL_API getAttributeValue (Variant& value, StringID name) const override;
	void CCL_API setAttributeValue (StringID name, VariantRef value, int index = -1) override;
	tbool CCL_API removeAttribute (StringID name, int* oldIndex = nullptr) override;
	void CCL_API clone (ISkinElement*& element) const override;

	// ISkinElementChildren
	tbool CCL_API addChildElement (ISkinElement* childElement, int index = -1) override;
	tbool CCL_API removeChildElement (ISkinElement* childElement, int* oldIndex = nullptr) override;
};

//************************************************************************************************
// SortedElement
/** Base class for skin elements whose children are looked up by name rather than kept in document
	order (e.g. flat resource/style/form containers) - keeps them stored sorted by name for fast
	lookup (see Element::addChild/findElement). Not itself a skin XML tag, so it registers as a
	plain (non-typelib) abstract class rather than via DECLARE_CLASS_ABSTRACT + DECLARE_TYPELIB_CLASS (Class). */
//************************************************************************************************

class SortedElement: public Element
{
public:
	DECLARE_CLASS_ABSTRACT (SortedElement, Element)

	SortedElement () { sorted = true; }
};

//************************************************************************************************
// Element::SortingSuspender
//************************************************************************************************

class Element::SortingSuspender
{
public:
	SortingSuspender (Element& element)
	: element (element),
	  wasSorted (element.sorted),
	  oldCount (element.count ())
	{
		element.sorted = false;
	}
	~SortingSuspender ()
	{
		if(wasSorted && element.count () != oldCount)
			element.sort ();
		element.sorted = wasSorted;
	}
private:
	Element& element;
	bool wasSorted;
	int oldCount;
};

} // namespace SkinElements
} // namespace CCL

#endif // _ccl_skinelement_h
