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
// Filename    : ccl/gui/skin/coreskinmodel.h
// Description : Core Skin Model
//
//************************************************************************************************

#ifndef _ccl_coreskinmodel_h
#define _ccl_coreskinmodel_h

#include "ccl/public/gui/framework/iskineditsupport.h"
#include "ccl/public/gui/graphics/iimage.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"

namespace CCL {

interface IPackageFile;
class Image;

class CoreSkinElement;
class CoreSkinImageElement;
class CoreSkinViewElement;
class CoreSkinEditSupport;

/*
	TODO:
	- new class derived Theme (CoreTheme)
	- make work with ThemeManager
*/

//************************************************************************************************
// CoreSkinElementArray
//************************************************************************************************

class CoreSkinElementArray: public ObjectArray,
							public ISkinElementChildren
{
public:
	DECLARE_CLASS (CoreSkinElementArray, ObjectArray)

	// ISkinElementChildren
	tbool CCL_API addChildElement (ISkinElement* childElement, int index = -1) override;
	tbool CCL_API removeChildElement (ISkinElement* childElement, int* oldIndex = nullptr) override;

	CLASS_INTERFACE (ISkinElementChildren, ObjectArray)
};

//************************************************************************************************
// CoreSkinModel
//************************************************************************************************

class CoreSkinModel: public Object,
					 public ISkinModel
{
public:
	DECLARE_CLASS (CoreSkinModel, Object)

	CoreSkinModel ();
	~CoreSkinModel ();

	static ITypeLibrary& getTypeLibrary ();

	bool load (UrlRef url, IProgressNotify* progress = nullptr);

	CoreSkinImageElement* findImageElement (StringID name) const;
	CoreSkinElement* findStyleElement (StringID name) const;
	CoreSkinElement* findFontElement (StringID name) const;
	CoreSkinViewElement* findFormElement (StringID name) const;

	// used by CoreSkinEditSupport:
	IPackageFile* getPackage () { return package; }
	bool isBinaryFormat () const { return binaryFormatDetected; }
	const CoreSkinElementArray& getFontElements () const { return fontElements; }
	const CoreSkinElementArray& getStyleElements () const { return styleElements; }
	const CoreSkinElementArray& getImageElements () const { return imageElements; }
	const CoreSkinElementArray& getFormElements () const { return formElements; }
	Image* loadBitmap (StringRef fileName);

	// ISkinModel
	IContainer* CCL_API getContainerForType (ElementType which) override;
	void CCL_API getImportedPaths (IUnknownList& paths) const override;
	ISkinModel* CCL_API getSubModel (StringID name) override;

	CLASS_INTERFACES (Object)

protected:
	IPackageFile* package;
	bool binaryFormatDetected;
	CoreSkinElementArray fontElements;
	CoreSkinElementArray styleElements;
	CoreSkinElementArray imageElements;
	CoreSkinElementArray formElements;
	CoreSkinEditSupport* editSupport;

	Attributes* detectSourceFile (Url& path, CStringPtr fileName1, CStringPtr fileName2);
	Attributes* parseSourceFile (UrlRef path, bool binary);

	bool loadFonts (IProgressNotify* progress);
	bool loadStyles (IProgressNotify* progress);
	bool loadBitmaps (IProgressNotify* progress);
	bool loadViews (IProgressNotify* progress);

	CoreSkinElement* findElement (const CoreSkinElementArray& elements, StringID name) const;
};

//************************************************************************************************
// CoreSkinLoader
//************************************************************************************************

class CoreSkinLoader: public Object,
					  public ISkinLoader
{
public:
	DECLARE_CLASS (CoreSkinLoader, Object)

	// ISkinLoader
	tbool CCL_API loadSkin (UrlRef path, IProgressNotify* progress = nullptr) override;
	tbool CCL_API createSkin (UrlRef path) override;
	ISkinModel* CCL_API getISkinModel () override { return skinModel; }

	CLASS_INTERFACE (ISkinLoader, Object)

protected:
	AutoPtr<CoreSkinModel> skinModel;
};

//************************************************************************************************
// CoreSkinElement
//************************************************************************************************

class CoreSkinElement: public Object,
					   public ISkinElement
{
public:
	DECLARE_CLASS (CoreSkinElement, Object)

	CoreSkinElement (StringID name = nullptr)
	: elementClass (nullptr),
	  name (name)	  
	{}

	void setElementClass (const ITypeInfo* _elementClass) { elementClass = _elementClass; }
	StringRef getSourceFile () const { return sourceFile; }

	// ISkinElement
	StringID CCL_API getName () const override { return name; }
	void CCL_API setName (StringID _name) override { name = _name; }
	void CCL_API getComment (String& _comment) const override { _comment = comment; }
	void CCL_API setComment (StringRef _comment) override { comment = _comment; }
	tbool CCL_API getSourceInfo (String& fileName, int32& lineNumber, IUrl* packageUrl = nullptr) const override;
	void CCL_API setSourceFile (StringRef fileName) override { sourceFile = fileName; }
	void CCL_API getAttributes (IAttributeList& _attributes) const override { _attributes.copyFrom (attributes); }
	void CCL_API setAttributes (const IAttributeList& _attributes) override { attributes.copyFrom (_attributes); }
	tbool CCL_API getAttributeValue (Variant& value, StringID name) const override { return attributes.getAttribute (value, name); }
	void CCL_API setAttributeValue (StringID name, VariantRef value, int index = -1) override;
	tbool CCL_API removeAttribute (StringID name, int* oldIndex = nullptr) override;
	const ITypeInfo* CCL_API getElementClass () const override { return elementClass; }
	void CCL_API clone (ISkinElement*& element) const override;

	CLASS_INTERFACE (ISkinElement, Object)

protected:
	const ITypeInfo* elementClass;
	MutableCString name;
	String comment;
	String sourceFile;
	Attributes attributes;
};

//************************************************************************************************
// CoreSkinImageElement
//************************************************************************************************

class CoreSkinImageElement: public CoreSkinElement,
							public ISkinImageElement
{
public:
	DECLARE_CLASS (CoreSkinImageElement, CoreSkinElement)

	CoreSkinImageElement (StringID name = nullptr)
	: CoreSkinElement (name)
	{}

	void updateFilmstrip ();

	// ISkinImageElement
	IImage* CCL_API getImage () const override { return image; }
	void CCL_API setImage (IImage* _image) override { image.share (_image); }
	StringRef CCL_API getImagePath () const override { return imagePath; }
	void CCL_API setImagePath (StringRef _imagePath) override { imagePath = _imagePath; }

	// CoreSkinElement
	void CCL_API setAttributeValue (StringID name, VariantRef value, int index) override;
	tbool CCL_API removeAttribute (StringID name, int* oldIndex = nullptr) override;

	CLASS_INTERFACE (ISkinImageElement, CoreSkinElement)

protected:
	AutoPtr<IImage> image;
	String imagePath;
};

//************************************************************************************************
// CoreSkinViewElement
//************************************************************************************************

class CoreSkinViewElement: public CoreSkinElement,
						   public ISkinViewElement
{
public:
	DECLARE_CLASS (CoreSkinViewElement, CoreSkinElement)

	CoreSkinViewElement (StringID name = nullptr);

	CoreSkinElementArray& getChildren () { return children; }
	const CoreSkinElementArray& getChildren () const { return children; }

	// ISkinViewElement
	RectRef CCL_API getSize () const override { return size; }
	void CCL_API setSize (RectRef _size) override { size = _size; }
	tbool CCL_API getDataDefinition (String& string, StringID id) const override;
	StyleFlags CCL_API getStandardOptions () const override;

	CLASS_INTERFACES (CoreSkinElement)

protected:
	Rect size;
	CoreSkinElementArray children;
};

//************************************************************************************************
// CoreSkinFormElement
//************************************************************************************************

class CoreSkinFormElement: public CoreSkinViewElement
{
public:
	DECLARE_CLASS (CoreSkinFormElement, CoreSkinViewElement)

	CoreSkinFormElement (StringID name = nullptr);

	PROPERTY_BOOL (modified, Modified)
};

//************************************************************************************************
// CoreSkinEditSupport
//************************************************************************************************

class CoreSkinEditSupport: public Object,
						   public ISkinEditSupport
{
public:
	DECLARE_CLASS_ABSTRACT (CoreSkinEditSupport, Object)

	CoreSkinEditSupport (CoreSkinModel& model);

	// IClassAllocator
	tresult CCL_API createInstance (UIDRef cid, UIDRef iid, void** obj) override;

	// ISkinEditSupport
	const ITypeLibrary* CCL_API getTypeLibrary () const override;
	const ITypeInfo* CCL_API getViewBaseClass () const override;
	const ITypeInfo* CCL_API getFormClass () const override;
	tbool CCL_API suggestSourceFile (String& sourceFile, UIDRef cid, StringRef initialName) const override;
	tbool CCL_API suggestAssetFolder (IUrl& folder, UIDRef cid = kNullUID) const override;
	tbool CCL_API getSupportedFileTypes (IFileTypeFilter& fileTypes, UIDRef cid) const override;
	IImage* CCL_API loadImage (StringRef fileName) const override;
	IMemoryStream* CCL_API loadBinaryFile (StringRef fileName) const override;
	SkinAttributeType CCL_API getAttributeType (ISkinElement* element, StringID attributeName) const override;
	tbool CCL_API isVariantOrTabView (ISkinViewElement* viewElement) const override;
	tbool CCL_API canHaveChildViews (ISkinViewElement* viewElement) const override;
	ISkinViewElement* CCL_API getReferencedForm (ISkinViewElement* viewElement) const override;
	tbool CCL_API getSizeChange (SkinValueChange& valueChange, ISkinViewElement* viewElement, RectRef newSize) const override;
	tbool CCL_API detectSizeChange (Rect& size, ISkinViewElement* viewElement, const SkinValueChange& valueChange) const override;
	tbool CCL_API drawFormBackground (IGraphics& graphics, ISkinViewElement* viewElement) const override;
	tbool CCL_API drawViewElement (IGraphics& graphics, ISkinViewElement* viewElement) const override;
	tbool CCL_API getSourceCodeForElement (String& sourceCode, ISkinElement* element) const override;
	void CCL_API setModelDirty (ISkinModel::ElementType type, ISkinElement* changedElement = nullptr) override;
	tbool CCL_API saveModelChanges (IProgressNotify* progress = nullptr) override;

	CLASS_INTERFACE2 (ISkinEditSupport, IClassAllocator, Object)

protected:
	CoreSkinModel& model;
	
	int modifications;	
	PROPERTY_FLAG (modifications, 1<<0, fontsModified)
	PROPERTY_FLAG (modifications, 1<<1, stylesModified)
	PROPERTY_FLAG (modifications, 1<<2, imagesModified)
	PROPERTY_FLAG (modifications, 1<<3, formsModified)

	bool saveIndex (StringRef fileName, const Container& elements);
	void saveForm (Attributes& a, const CoreSkinViewElement& formElement) const;
	bool saveData (IStream& stream, const Attributes& a, bool binary) const;
	bool saveData (IStream& stream, const AttributeQueue& queue, bool binary) const;
};

} // namespace CCL

#endif // _ccl_coreskinmodel_h
