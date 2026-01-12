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
// Filename    : ccl/gui/skin/coreskinmodel.cpp
// Description : Core Skin Model
//
//************************************************************************************************

#include "ccl/gui/skin/coreskinmodel.h"

#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/filmstrip.h"

#include "ccl/base/typelib.h"
#include "ccl/base/singleton.h"
#include "ccl/base/storage/jsonarchive.h"
#include "ccl/base/storage/textfile.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/cclerror.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

#include "core/gui/coreskinformat.impl.h"
#include "core/gui/corebmphandler.h"

namespace CCL {

//************************************************************************************************
// CoreSkinNonViewClasses
//************************************************************************************************

namespace CoreSkinNonViewClasses
{
	const CStringPtr kBitmap = "Bitmap";
	const CStringPtr kFont = "Font";
	const CStringPtr kStyle = "Style";
}

//************************************************************************************************
// CoreSkinElementClass
//************************************************************************************************

class CoreSkinElementClass: public TypeInfo
{
public:
	DECLARE_CLASS_ABSTRACT (CoreSkinElementClass, TypeInfo)

	CoreSkinElementClass (CStringPtr name, const CoreSkinElementClass* parentClass = nullptr)
	: TypeInfo (name, parentClass)
	{}

	void addMember (StringID name, DataType dataType, StringID typeName = nullptr)
	{
		members.add (Model::MemberDescription (name, dataType, typeName));
	}

	// TypeInfo
	bool getDetails (ITypeInfoDetails& details) const override
	{
		for(auto member : members)
			details.addMember (member);
		return true;
	}

	IUnknown* CCL_API createInstance () const override
	{
		CoreSkinElement* element = NEW CoreSkinElement;
		element->setElementClass (this);
		return static_cast<ISkinElement*> (element);
	}

protected:
	Vector<Model::MemberDescription> members;
};

//************************************************************************************************
// CoreSkinImageElementClass
//************************************************************************************************

class CoreSkinImageElementClass: public CoreSkinElementClass
{
public:
	DECLARE_CLASS (CoreSkinImageElementClass, CoreSkinElementClass)

	CoreSkinImageElementClass ()
	: CoreSkinElementClass (CoreSkinNonViewClasses::kBitmap)
	{}

	// CoreSkinElementClass
	IUnknown* CCL_API createInstance () const override
	{
		CoreSkinElement* element = NEW CoreSkinImageElement;
		element->setElementClass (this);
		return static_cast<ISkinElement*> (element);
	}
};

//************************************************************************************************
// CoreControlClass
//************************************************************************************************

class CoreControlClass: public CoreSkinElementClass
{
public:
	DECLARE_CLASS_ABSTRACT (CoreControlClass, CoreSkinElementClass)

	CoreControlClass (CStringPtr name, const CoreControlClass* parentClass = nullptr)
	: CoreSkinElementClass (name, parentClass),
	  flags (0)
	{}

	PROPERTY_FLAG (flags, 1<<0, isBaseClass)
	PROPERTY_FLAG (flags, 1<<1, isFormClass)

	// CoreSkinElementClass
	IUnknown* CCL_API createInstance () const override
	{
		CoreSkinViewElement* viewElement = isFormClass () ? NEW CoreSkinFormElement : NEW CoreSkinViewElement;
		viewElement->setElementClass (this);
		return static_cast<ISkinElement*> (viewElement);
	}

protected:
	int flags;
};

//************************************************************************************************
// CoreSkinTypeLibrary
//************************************************************************************************

class CoreSkinTypeLibrary: public TypeLibrary,
						   public Singleton<CoreSkinTypeLibrary>
{
public:
	CoreSkinTypeLibrary ();

	PROPERTY_POINTER (CoreSkinElementClass, imageClass, ImageClass)
	PROPERTY_POINTER (CoreSkinElementClass, fontClass, FontClass)
	PROPERTY_POINTER (CoreSkinElementClass, styleClass, StyleClass)

	PROPERTY_POINTER (CoreControlClass, viewClass, ViewClass)
	PROPERTY_POINTER (CoreControlClass, containerViewClass, ContainerViewClass)
	PROPERTY_POINTER (CoreControlClass, formClass, FormClass)

	CoreSkinImageElement* createImageElement (StringID name = nullptr)
	{
		auto* element = NEW CoreSkinImageElement (name);
		element->setElementClass (imageClass);
		return element;
	}

	CoreSkinElement* createFontElement (StringID name = nullptr)
	{
		auto* element = NEW CoreSkinElement (name); // no dedicated class
		element->setElementClass (fontClass);
		return element;
	}

	CoreSkinElement* createStyleElement (StringID name = nullptr)
	{
		auto* element = NEW CoreSkinElement (name); // no dedicated class
		element->setElementClass (styleClass);
		return element;
	}

	CoreSkinFormElement* createFormElement (StringID name = nullptr)
	{
		auto* element = NEW CoreSkinFormElement (name);
		element->setElementClass (formClass);
		return element;
	}

protected:
	Vector<MutableCString> nameStrings;

	void addOptionEnum (StringID prefix, StringID name, const Core::EnumInfo enumInfo[]);
};

//************************************************************************************************
// CoreViewElementAccessor
//************************************************************************************************

class CoreViewElementAccessor
{
public:
	CoreViewElementAccessor (const CoreSkinModel& model, const CoreSkinViewElement& viewElement)
	: model (model),
	  viewElement (viewElement)
	{}

	IImage* getImage (StringID name = nullptr) const;
	int getOptions (const Core::EnumInfo enumInfo[], StringID name = nullptr) const;
	bool getStyleValue (Variant& value, StringID name) const;
	bool getStyleColor (Color& color, StringID name) const;
	bool getFont (Font& font) const;
	int getTextAlignment (int defAlign = Alignment::kLeftCenter) const;

protected:
	const CoreSkinModel& model;
	const CoreSkinViewElement& viewElement;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (BitmapFont, "Bitmap Font")
END_XSTRINGS

//************************************************************************************************
// CoreSkinElementArray
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CoreSkinElementArray, ObjectArray)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinElementArray::addChildElement (ISkinElement* _childElement, int index)
{
	CoreSkinElement* childElement = unknown_cast<CoreSkinElement> (_childElement);
	ASSERT (childElement != nullptr)
	if(childElement == nullptr)
		return false;
	if(index != -1 && insertAt (index, childElement))
		return true;

	add (childElement);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinElementArray::removeChildElement (ISkinElement* _childElement, int* oldIndex)
{
	CoreSkinElement* childElement = unknown_cast<CoreSkinElement> (_childElement);
	ASSERT (childElement != nullptr)
	if(childElement == nullptr)
		return false;

	if(oldIndex)
		*oldIndex = index (childElement);
	return remove (childElement);
}

//************************************************************************************************
// CoreSkinModel
//************************************************************************************************

ITypeLibrary& CoreSkinModel::getTypeLibrary ()
{
	return CoreSkinTypeLibrary::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (CoreSkinModel, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

CoreSkinModel::CoreSkinModel ()
: package (nullptr),
  binaryFormatDetected (false),
  editSupport (nullptr)
{
	fontElements.objectCleanup (true);
	styleElements.objectCleanup (true);
	imageElements.objectCleanup (true);
	formElements.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CoreSkinModel::~CoreSkinModel ()
{
	safe_release (package);
	safe_release (editSupport);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreSkinModel::queryInterface (UIDRef iid, void** ptr)
{
	// make additional interfaces accessible
	if(iid == ccl_iid<ISkinEditSupport> ())
	{
		if(editSupport == nullptr)
			editSupport = NEW CoreSkinEditSupport (*this);
		return editSupport->queryInterface (iid, ptr);
	}

	QUERY_INTERFACE (ISkinModel)
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CoreSkinModel::load (UrlRef url, IProgressNotify* progress)
{
	AutoPtr<IPackageFile> package = System::GetPackageHandler ().openPackage (url);
	if(!package)
		return false;

	ASSERT (package->getFileSystem () != nullptr)
	take_shared<IPackageFile> (this->package, package);

	// Order is important here - images, etc. must be loaded before views!
	loadFonts (progress);
	loadStyles (progress);
	loadBitmaps (progress);

	return loadViews (progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes* CoreSkinModel::detectSourceFile (Url& path, CStringPtr fileName1, CStringPtr fileName2)
{
	ASSERT (package != nullptr)

	path.descend (fileName1);
	bool binary = false;
	if(!package->getFileSystem ()->fileExists (path))
	{
		path.setName (fileName2);
		if(!package->getFileSystem ()->fileExists (path))
			return nullptr;
		binary = true;
	}

	binaryFormatDetected = binary; // keep it for save
	return parseSourceFile (path, binary);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes* CoreSkinModel::parseSourceFile (UrlRef path, bool binary)
{
	ASSERT (package != nullptr)
	AutoPtr<Attributes> data;
	if(AutoPtr<IStream> stream = package->getFileSystem ()->openStream (path))
	{
		data = NEW Attributes;
		bool loaded = binary ? UBJsonArchive (*stream).loadAttributes (nullptr, *data) :
					  JsonArchive (*stream).loadAttributes (nullptr, *data);
		if(loaded == false)
			data.release ();
	}
	return data.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CoreSkinModel::loadFonts (IProgressNotify* progress)
{
	Url fontIndexPath;
	AutoPtr<Attributes> fontIndexData = detectSourceFile (fontIndexPath, Core::Skin::FileNames::kFontFile1,
														   Core::Skin::FileNames::kFontFile2);
	if(!fontIndexData)
		return false;

	String sourceFile;
	fontIndexPath.getName (sourceFile);

	if(progress)
		progress->updateAnimated (sourceFile);

	auto& typeLib = CoreSkinTypeLibrary::instance ();
	IterForEach (fontIndexData->newQueueIterator (nullptr, ccl_typeid<Attributes> ()), Attributes, fontAttr)
		MutableCString name = fontAttr->getCString (Core::Skin::ResourceAttributes::kName);
		fontAttr->remove (Core::Skin::ResourceAttributes::kName);

		CoreSkinElement* fontElement = typeLib.createFontElement (name);
		fontElement->setSourceFile (sourceFile);
		fontElement->setAttributes (*fontAttr);
		fontElements.add (fontElement);
	EndFor

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

bool CoreSkinModel::loadStyles (IProgressNotify* progress)
{
	Url styleIndexPath;
	AutoPtr<Attributes> styleIndexData = detectSourceFile (styleIndexPath, Core::Skin::FileNames::kStyleFile1,
														   Core::Skin::FileNames::kStyleFile2);
	if(!styleIndexData)
		return false;

	String sourceFile;
	styleIndexPath.getName (sourceFile);

	if(progress)
		progress->updateAnimated (sourceFile);

	auto& typeLib = CoreSkinTypeLibrary::instance ();
	IterForEach (styleIndexData->newQueueIterator (nullptr, ccl_typeid<Attributes> ()), Attributes, styleAttr)
		MutableCString name = styleAttr->getCString (Core::Skin::ResourceAttributes::kName);
		styleAttr->remove (Core::Skin::ResourceAttributes::kName);

		CoreSkinElement* styleElement = typeLib.createStyleElement (name);
		styleElement->setSourceFile (sourceFile);
		styleElement->setAttributes (*styleAttr);
		styleElements.add (styleElement);
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* CoreSkinModel::loadBitmap (StringRef fileName)
{
	AutoPtr<Image> image;
	Url bitmapPath;
	bitmapPath.setPath (fileName);
	AutoPtr<IStream> stream = package->getFileSystem ()->openStream (bitmapPath);
	if(stream)
	{
		auto& format = bitmapPath.getFileType ();
		#if 1 // BMP format is not supported on all platforms, use handler from corelib
		if(format == FileTypes::bmp)
		{
			CoreStream streamAdapter (*stream);
			Core::BMPHandler bmpHandler (streamAdapter);
			if(bmpHandler.readInfo ())
			{
				auto& info = bmpHandler.getInfo ();
				BitmapLockData dstData;
				AutoPtr<Bitmap> dstBitmap = NEW Bitmap (info.width, abs (info.height), Bitmap::kRGBAlpha);
				if(dstBitmap->lockBits (dstData, Bitmap::kRGBAlpha, IBitmap::kLockWrite) == kResultOk)
				{
					if(bmpHandler.readBitmapData (dstData))
						image.share (dstBitmap);

					dstBitmap->unlockBits (dstData);
				}
			}
		}
		else
		#endif
			image = Image::loadImage (*stream, format);
	}
	return image.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CoreSkinModel::loadBitmaps (IProgressNotify* progress)
{
	Url bitmapIndexPath;
	AutoPtr<Attributes> bitmapIndexData = detectSourceFile (bitmapIndexPath, Core::Skin::FileNames::kBitmapFile1,
															Core::Skin::FileNames::kBitmapFile2);
	if(!bitmapIndexData)
		return false;

	String sourceFile;
	bitmapIndexPath.getName (sourceFile);

	auto& typeLib = CoreSkinTypeLibrary::instance ();
	IterForEach (bitmapIndexData->newQueueIterator (nullptr, ccl_typeid<Attributes> ()), Attributes, bitmapAttr)
		MutableCString name = bitmapAttr->getCString (Core::Skin::ResourceAttributes::kName);
		String fileName = bitmapAttr->getString (Core::Skin::ResourceAttributes::kFile);
		bitmapAttr->remove (Core::Skin::ResourceAttributes::kName);
		bitmapAttr->remove (Core::Skin::ResourceAttributes::kFile);

		CoreSkinImageElement* imageElement = typeLib.createImageElement (name);
		imageElement->setSourceFile (sourceFile);
		imageElement->setImagePath (fileName);
		imageElement->setAttributes (*bitmapAttr);
		imageElements.add (imageElement);

		if(progress)
			progress->updateAnimated (fileName);

		AutoPtr<Image> image = loadBitmap (fileName);
		imageElement->setImage (image);
		imageElement->updateFilmstrip ();
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CoreSkinModel::loadViews (IProgressNotify* progress)
{
	Url viewIndexPath;
	AutoPtr<Attributes> viewIndexData = detectSourceFile (viewIndexPath, Core::Skin::FileNames::kViewFile1,
														  Core::Skin::FileNames::kViewFile2);
	if(!viewIndexData)
		return false;

	//String sourceFile;
	//viewIndexPath.getName (sourceFile);

	auto& typeLib = CoreSkinTypeLibrary::instance ();
	IterForEach (viewIndexData->newQueueIterator (nullptr, ccl_typeid<Attributes> ()), Attributes, viewAttr)
		MutableCString name = viewAttr->getCString (Core::Skin::ResourceAttributes::kName);
		String fileName = viewAttr->getString (Core::Skin::ResourceAttributes::kFile);
		bool binary = fileName.endsWith (UBJsonArchive::getFileType ().getExtension ());

		Url sourcePath;
		sourcePath.descend (fileName);

		if(progress)
			progress->updateAnimated (fileName);

		if(AutoPtr<Attributes> viewData = parseSourceFile (sourcePath, binary))
		{
			CoreSkinFormElement* formElement = typeLib.createFormElement (name);
			formElements.add (formElement);

			String comment = viewAttr->getString (Core::Skin::ResourceAttributes::kComment);
			viewAttr->remove (Core::Skin::ResourceAttributes::kComment);
			formElement->setComment (comment);

			struct Helper
			{
				CoreSkinModel& model;
				const ITypeInfo* containerViewClass;

				Helper (CoreSkinModel& model)
				: model (model),
				  containerViewClass (CoreSkinTypeLibrary::instance ().getContainerViewClass ())
				{}

				static bool shouldSizeToBitmap (StringID typeName)
				{
					const CStringPtr typeNames[] =
					{
						Core::Skin::ViewClasses::kImageView,
						Core::Skin::ViewClasses::kButton,
						Core::Skin::ViewClasses::kToggle,
						Core::Skin::ViewClasses::kRadioButton,
						Core::Skin::ViewClasses::kSelectBox
					};
					for(auto n : typeNames)
						if(typeName == n)
							return true;
					return false;
				};

				bool shouldSizeToChildren (const ITypeInfo& elementClass)
				{
					ASSERT (containerViewClass != nullptr)
					return ccl_is_base_of (containerViewClass, &elementClass);
				}

				void processItem (StringRef sourceFile, CoreSkinViewElement* currentElement, Attributes& attr)
				{
					currentElement->setSourceFile (sourceFile);

					Variant children;
					if(attr.getAttribute (children, Core::Skin::ViewAttributes::kChildren))
					{
						attr.remove (Core::Skin::ViewAttributes::kChildren);
						if(AttributeQueue* childArray = unknown_cast<AttributeQueue> (children.asUnknown ()))
						{
							ArrayForEach (*childArray, Attribute, a)
								if(Attributes* childAttr = unknown_cast<Attributes> (a->getValue ().asUnknown ()))
								{
									CoreSkinViewElement* childElement = NEW CoreSkinViewElement;
									processItem (sourceFile, childElement, *childAttr);
									currentElement->getChildren ().add (childElement);
								}
							EndFor
						}
					}

					MutableCString typeName = attr.getCString (Core::Skin::ViewAttributes::kType);
					attr.remove (Core::Skin::ViewAttributes::kType);
					if(!typeName.isEmpty ()) // don't overwrite form name
						currentElement->setName (typeName);

					const ITypeInfo* elementClass = CoreSkinTypeLibrary::instance ().findType (typeName);
					//SOFT_ASSERT (elementClass != 0, "Control class not found!\n")
					if(elementClass == nullptr) // default to a base class, could reference another form
						elementClass = containerViewClass;

					currentElement->setElementClass (elementClass);

					#if (0 && DEBUG_LOG)
					attr.dump ();
					#endif
					currentElement->setAttributes (attr);

					// determine size
					Rect size;
					MutableCString sizeString = attr.getCString (Core::Skin::ViewAttributes::kSize);
					if(!sizeString.isEmpty ())
						Core::Skin::ResourceAttributes::parseSize (size, sizeString);
					else
					{
						Coord width = attr.getInt (Core::Skin::ViewAttributes::kWidth);
						Coord height = attr.getInt (Core::Skin::ViewAttributes::kHeight);
						size (0, 0, width, height);
					}

					if(size.isEmpty ())
					{
						if(shouldSizeToBitmap (typeName))
							if(IImage* image = CoreViewElementAccessor (model, *currentElement).getImage ())
							{
								// adjust width/height but keep left/top
								size.setWidth (image->getWidth ());
								size.setHeight (image->getHeight ());
							}
					}

					// resize to children
					if(size.isEmpty () && elementClass && shouldSizeToChildren (*elementClass))
					{
						Rect childSize;
						for(auto child : iterate_as<CoreSkinViewElement> (currentElement->getChildren ()))
							childSize.join (child->getSize ());
						size.setWidth (childSize.right);
						size.setHeight (childSize.bottom);
					}

					// TODO: handle children of AlignView!

					currentElement->setSize (size);
				}
			};

			Helper (*this).processItem (fileName, formElement, *viewData);
		}
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CoreSkinElement* CoreSkinModel::findElement (const CoreSkinElementArray& elements, StringID name) const
{
	if(!name.isEmpty ())
		for(auto item : iterate_as<CoreSkinElement> (static_cast<const ObjectArray&> (elements))) // FIX ME: CCL-409
			if(item->getName () == name)
				return item;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CoreSkinImageElement* CoreSkinModel::findImageElement (StringID name) const
{
	return static_cast<CoreSkinImageElement*> (findElement (imageElements, name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CoreSkinElement* CoreSkinModel::findStyleElement (StringID name) const
{
	return static_cast<CoreSkinElement*> (findElement (styleElements, name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CoreSkinElement* CoreSkinModel::findFontElement (StringID name) const
{
	return static_cast<CoreSkinElement*> (findElement (fontElements, name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CoreSkinViewElement* CoreSkinModel::findFormElement (StringID name) const
{
	return static_cast<CoreSkinViewElement*> (findElement (formElements, name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IContainer* CCL_API CoreSkinModel::getContainerForType (ElementType which)
{
	switch(which)
	{
	case kFontsElement : return &fontElements;
	case kStylesElement : return &styleElements;
	case kImagesElement : return &imageElements;
	case kFormsElement : return &formElements;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreSkinModel::getImportedPaths (IUnknownList& paths) const
{
	// nothing here
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISkinModel* CCL_API CoreSkinModel::getSubModel (StringID name)
{
	// nothing here
	return nullptr;
}

//************************************************************************************************
// CoreSkinLoader
//************************************************************************************************

DEFINE_CLASS (CoreSkinLoader, Object)
DEFINE_CLASS_UID (CoreSkinLoader,  0x8683f346, 0x6f53, 0x4f8e, 0x99, 0xd5, 0x9, 0x94, 0x53, 0x7e, 0x48, 0xc7)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinLoader::loadSkin (UrlRef path, IProgressNotify* progress)
{
	skinModel = NEW CoreSkinModel;
	return skinModel->load (path, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinLoader::createSkin (UrlRef path)
{
	ASSERT (path.isFolder ())
	if(!path.isFolder ())
		return false;

	// create empty view index file
	Url viewPath (path);
	viewPath.descend (Core::Skin::FileNames::kViewFile1);
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (viewPath, IStream::kCreateMode);
	if(!stream.isValid ())
		return false;

	const char data[] = {'[', ']'};
	stream->write (data, sizeof(data));
	stream.release ();

	return loadSkin (path);
}

//************************************************************************************************
// CoreSkinElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CoreSkinElement, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreSkinElement::clone (ISkinElement*& _element) const
{
	auto element = static_cast<CoreSkinElement*> (clone ());
	if(element) // don't copy source file
		element->setSourceFile (String::kEmpty);
	_element = element;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinElement::getSourceInfo (String& fileName, int32& lineNumber, IUrl* packageUrl) const
{
	// TODO: line number and package URL aren't known here!
	fileName = sourceFile;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreSkinElement::setAttributeValue (StringID name, VariantRef value, int index)
{
	attributes.setAttribute (name, value);
	if(index != -1)
		attributes.setAttributeIndex (name, index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinElement::removeAttribute (StringID name, int* oldIndex)
{
	if(oldIndex)
		*oldIndex = attributes.getAttributeIndex (name);
	return attributes.remove (name);
}

//************************************************************************************************
// CoreSkinImageElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CoreSkinImageElement, CoreSkinElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

void CoreSkinImageElement::updateFilmstrip ()
{
	Image* originalImage = image ? unknown_cast<Image> (image->getOriginal ()) : nullptr;
	if(originalImage == nullptr)
		return;

	Variant value;
	getAttributeValue (value, Core::Skin::ResourceAttributes::kFrames);
	int frameCount = value.asInt ();
	if(frameCount > 1)
	{
		AutoPtr<Filmstrip> filmstrip = NEW Filmstrip (originalImage);
		filmstrip->parseFrameNames (String (MutableCString ().appendInteger (frameCount)));
		image.share (filmstrip);
	}
	else
	{
		if(auto filmstrip = unknown_cast<Filmstrip> (image))
			image.share (originalImage);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreSkinImageElement::setAttributeValue (StringID name, VariantRef value, int index)
{
	SuperClass::setAttributeValue (name, value, index);

	if(name == Core::Skin::ResourceAttributes::kFrames)
		updateFilmstrip ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinImageElement::removeAttribute (StringID name, int* oldIndex)
{
	if(!SuperClass::removeAttribute (name, oldIndex))
		return false;

	if(name == Core::Skin::ResourceAttributes::kFrames)
		updateFilmstrip ();
	return true;
}

//************************************************************************************************
// CoreSkinViewElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CoreSkinViewElement, CoreSkinElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

CoreSkinViewElement::CoreSkinViewElement (StringID name)
: CoreSkinElement (name)
{
	children.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreSkinViewElement::queryInterface (UIDRef iid, void** ptr)
{
	// make additional interfaces accessible
	if(iid == ccl_iid<IContainer> () || iid == ccl_iid<ISkinElementChildren> ())
		return children.queryInterface (iid, ptr);

	QUERY_INTERFACE (ISkinViewElement)
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinViewElement::getDataDefinition (String& string, StringID id) const
{
	CCL_NOT_IMPL ("Do we need this?")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StyleFlags CCL_API CoreSkinViewElement::getStandardOptions () const
{
	CCL_NOT_IMPL ("Do we need this?")
	return StyleFlags ();
}

//************************************************************************************************
// CoreSkinFormElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CoreSkinFormElement, CoreSkinViewElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

CoreSkinFormElement::CoreSkinFormElement (StringID name)
: CoreSkinViewElement (name),
  modified (false)
{}

//************************************************************************************************
// CoreSkinEditSupport
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CoreSkinEditSupport, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

CoreSkinEditSupport::CoreSkinEditSupport (CoreSkinModel& model)
: model (model),
  modifications (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreSkinEditSupport::createInstance (UIDRef cid, UIDRef iid, void** obj)
{
	auto& typeLib = CoreSkinTypeLibrary::instance ();
	AutoPtr<CoreSkinElement> newElement;

	if(cid == ClassID::FormElement)
	{
		newElement = typeLib.createFormElement ();

		// NOTE: caller has to assign source file per form.
	}
	else if(cid == ClassID::ImageElement)
	{
		newElement = typeLib.createImageElement ();

		// init source file (doesn't change)
		newElement->setSourceFile (model.isBinaryFormat () ?
								   Core::Skin::FileNames::kBitmapFile2 :
								   Core::Skin::FileNames::kBitmapFile1);
	}
	else if(cid == ClassID::StyleElement)
	{
		newElement = typeLib.createStyleElement ();

		// init source file (doesn't change)
		newElement->setSourceFile (model.isBinaryFormat () ?
								   Core::Skin::FileNames::kStyleFile2 :
								   Core::Skin::FileNames::kStyleFile1);
	}
	else if(cid == ClassID::FontElement)
	{
		newElement = typeLib.createFontElement ();

		// init source file (doesn't change)
		newElement->setSourceFile (model.isBinaryFormat () ?
								   Core::Skin::FileNames::kFontFile2 :
								   Core::Skin::FileNames::kFontFile1);

		// add default font attributes
		newElement->setAttributeValue (Core::Skin::ResourceAttributes::kFile, String::kEmpty);
		newElement->setAttributeValue (Core::Skin::ResourceAttributes::kFontFace, String::kEmpty);
		newElement->setAttributeValue (Core::Skin::ResourceAttributes::kSize, 10);
	}

	if(newElement)
		return newElement->queryInterface (iid, obj);
	else
	{
		*obj = nullptr;
		return kResultNoInterface;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ITypeLibrary* CCL_API CoreSkinEditSupport::getTypeLibrary () const
{
	return &CoreSkinTypeLibrary::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ITypeInfo* CCL_API CoreSkinEditSupport::getViewBaseClass () const
{
	return CoreSkinTypeLibrary::instance ().getViewClass ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ITypeInfo* CCL_API CoreSkinEditSupport::getFormClass () const
{
	return CoreSkinTypeLibrary::instance ().getFormClass ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinEditSupport::suggestSourceFile (String& sourceFile, UIDRef cid, StringRef initialName) const
{
	if(cid == ClassID::FormElement)
	{
		String fileName = LegalFileName (initialName);
		fileName.toLowercase ();
		if(fileName.isEmpty ())
			fileName = CCLSTR ("form");

		sourceFile = Core::Skin::FileNames::kViewsFolder;
		sourceFile << Url::strPathChar << fileName;
		if(model.isBinaryFormat ())
			sourceFile << "." << UBJsonArchive::getFileType ().getExtension ();
		else
			sourceFile << "." << JsonArchive::getFileType ().getExtension ();
		return true;
	}

	CCL_NOT_IMPL ("Implement me!\n")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinEditSupport::suggestAssetFolder (IUrl& folder, UIDRef cid) const
{
	Url packagePath (model.getPackage ()->getPath ());
	ASSERT (packagePath.isFolder ())
	folder.assign (packagePath);

	CString subFolder;
	if(cid == ClassID::FormElement)
		subFolder = Core::Skin::FileNames::kViewsFolder;
	else if(cid == ClassID::ImageElement)
		subFolder = Core::Skin::FileNames::kBitmapsFolder;
	else if(cid == ClassID::FontElement)
		subFolder = Core::Skin::FileNames::kFontsFolder;

	if(!subFolder.isEmpty ())
		folder.descend (String (subFolder), Url::kFolder);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinEditSupport::getSupportedFileTypes (IFileTypeFilter& fileTypes, UIDRef cid) const
{
	if(cid == ClassID::ImageElement)
	{
		fileTypes.addFileType (FileTypes::bmp);
		fileTypes.addFileType (FileTypes::png);
		return true;
	}
	else if(cid == ClassID::FontElement)
	{
		static FileType bitmapFontType (nullptr, "fnt");
		FileTypes::init (bitmapFontType, XSTR (BitmapFont));
		fileTypes.addFileType (bitmapFontType);
		return true;
	}
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API CoreSkinEditSupport::loadImage (StringRef fileName) const
{
	return model.loadBitmap (fileName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMemoryStream* CCL_API CoreSkinEditSupport::loadBinaryFile (StringRef fileName) const
{
	Url path;
	path.setPath (fileName);
	AutoPtr<IStream> stream = model.getPackage ()->getFileSystem ()->openStream (path);
	return stream ? System::GetFileUtilities ().createStreamCopyInMemory (*stream) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinAttributeType CCL_API CoreSkinEditSupport::getAttributeType (ISkinElement* _element, StringID attributeName) const
{
	auto element = unknown_cast<CoreSkinElement> (_element);
	auto viewElement = ccl_cast<CoreSkinViewElement> (element);
	auto controlClass = viewElement ? unknown_cast<CoreControlClass> (viewElement->getElementClass ()) : nullptr;

	if(attributeName.endsWith ("color") || attributeName.endsWith ("color.disabled") || attributeName.endsWith ("color.on"))
		return SkinAttributeTypes::kColor;
	else if(attributeName == Core::Skin::ViewAttributes::kStyle || attributeName == Core::Skin::ViewAttributes::kInherit)
		return SkinAttributeTypes::kStyle;
	else if(attributeName.endsWith ("image") || attributeName == Core::Skin::ViewAttributes::kIcon || attributeName == Core::Skin::ViewAttributes::kBackground)
		return SkinAttributeTypes::kImage;
	else if(attributeName == Core::Skin::ViewAttributes::kOptions ||
			attributeName == Core::Skin::ViewAttributes::kTextAlign ||
			attributeName == Core::Skin::ViewAttributes::kTextTrimMode ||
			attributeName == Core::Skin::ViewAttributes::kKeyboardLayout)
		return SkinAttributeTypes::kEnum;
	else if(controlClass && controlClass->isBaseClass () && attributeName == Core::Skin::ViewAttributes::kName)
		return SkinAttributeTypes::kForm;
	else if(attributeName == Core::Skin::ViewAttributes::kViewName)
		return SkinAttributeTypes::kForm;
	else if(attributeName == Core::Skin::ResourceAttributes::kWidth ||
			attributeName == Core::Skin::ResourceAttributes::kHeight ||
			attributeName == Core::Skin::ViewAttributes::kRadioValue)
		return SkinAttributeTypes::kInteger;
	else if(viewElement == nullptr)
	{
		// asset attributes
		if(attributeName == Core::Skin::ViewAttributes::kFont) // font referenced in style
			return SkinAttributeTypes::kFont;
		else if(attributeName == Core::Skin::ResourceAttributes::kSize) // font size
			return SkinAttributeTypes::kFloat;
		else if(attributeName == Core::Skin::ResourceAttributes::kFrames || // image frame count
				attributeName == Core::Skin::ResourceAttributes::kFontNumber)
			return SkinAttributeTypes::kInteger;
		else if(attributeName == Core::Skin::ResourceAttributes::kMonochrome ||
				attributeName == Core::Skin::ResourceAttributes::kAlwaysCached ||
				attributeName == Core::Skin::ResourceAttributes::kDefault)
			return SkinAttributeTypes::kBool;
	}

	return SkinAttributeTypes::kUnspecified;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinEditSupport::isVariantOrTabView (ISkinViewElement* _viewElement) const
{
	auto viewElement = unknown_cast<CoreSkinViewElement> (_viewElement);
	auto elementClass = viewElement ? viewElement->getElementClass () : nullptr;
	return elementClass ? CString (elementClass->getClassName ()) == Core::Skin::ViewClasses::kVariantView : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinEditSupport::canHaveChildViews (ISkinViewElement* _viewElement) const
{
	auto viewElement = unknown_cast<CoreSkinViewElement> (_viewElement);
	auto containerViewClass = CoreSkinTypeLibrary::instance ().getContainerViewClass ();
	return viewElement && ccl_is_base_of (containerViewClass, viewElement->getElementClass ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISkinViewElement* CCL_API CoreSkinEditSupport::getReferencedForm (ISkinViewElement* _viewElement) const
{
	auto viewElement = unknown_cast<CoreSkinViewElement> (_viewElement);
	auto controlClass = viewElement ? unknown_cast<CoreControlClass> (viewElement->getElementClass ()) : nullptr;
	if(controlClass)
	{
		if(controlClass->isBaseClass ())
		{
			return model.findFormElement (viewElement->getName ());
		}
		else if(CString (controlClass->getClassName ()) == Core::Skin::ViewClasses::kDelegate)
		{
			Variant value;
			viewElement->getAttributeValue (value, Core::Skin::ViewAttributes::kViewName);
			MutableCString viewName (value.asString ());
			return model.findFormElement (viewName);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinEditSupport::getSizeChange (SkinValueChange& valueChange, ISkinViewElement* viewElement, RectRef newSize) const
{
	auto printSize = [] (RectRef size)
	{
		return String () << size.left << "," << size.top << "," << size.getWidth () << "," << size.getHeight ();
	};

	valueChange = SkinValueChange (Core::Skin::ViewAttributes::kSize, printSize (newSize));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinEditSupport::detectSizeChange (Rect& newSize, ISkinViewElement* viewElement, const SkinValueChange& valueChange) const
{
	if(valueChange.name == Core::Skin::ViewAttributes::kSize)
	{
		MutableCString sizeString (valueChange.value.asString ());
		Core::Skin::ResourceAttributes::parseSize (newSize, sizeString);
		// TODO: implement auto-size to bitmap here???
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinEditSupport::drawFormBackground (IGraphics& graphics, ISkinViewElement* _viewElement) const
{
	auto viewElement = unknown_cast<CoreSkinViewElement> (_viewElement);
	ASSERT (viewElement)
	if(!viewElement)
		return false;

	// background drawn by RootView in corelib

	RectRef viewRect = viewElement->getSize ();
	CoreViewElementAccessor accessor (model, *viewElement);

	Color backColor;
	if(accessor.getStyleColor (backColor, Core::Skin::ViewAttributes::kBackColor))
		graphics.fillRect (viewRect, SolidBrush (backColor));

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinEditSupport::drawViewElement (IGraphics& graphics, ISkinViewElement* _viewElement) const
{
	auto viewElement = unknown_cast<CoreSkinViewElement> (_viewElement);
	auto controlClass = viewElement ? unknown_cast<CoreControlClass> (viewElement->getElementClass ()) : nullptr;
	if(!controlClass || controlClass->isBaseClass ())
		return false;

	RectRef viewRect = viewElement->getSize ();
	StringID className = controlClass->getClassName ();
	CoreViewElementAccessor accessor (model, *viewElement);
	ClipSetter cs (graphics, viewRect);

	bool result = true;
	if(className == Core::Skin::ViewClasses::kImageView)
	{
		if(auto image = accessor.getImage ())
		{
			graphics.drawImage (image, viewRect.getLeftTop ());
		}
		else
		{
			int options = accessor.getOptions (Core::Skin::Enumerations::imageViewOptions);
			bool colorize = (options & Core::Skin::kImageViewAppearanceColorize) != 0;
			if(colorize)
			{
				Color backColor;
				accessor.getStyleColor (backColor, Core::Skin::ViewAttributes::kBackColor);
				graphics.fillRect (viewRect, SolidBrush (backColor));
			}
		}
	}
	else if(className == Core::Skin::ViewClasses::kLabel)
	{
		int options = accessor.getOptions (Core::Skin::Enumerations::labelOptions);
		bool colorize = (options & Core::Skin::kLabelAppearanceColorize) != 0;
		if(colorize)
		{
			Color backColor;
			accessor.getStyleColor (backColor, Core::Skin::ViewAttributes::kBackColor);
			graphics.fillRect (viewRect, SolidBrush (backColor));
		}

		Variant var;
		viewElement->getAttributeValue (var, Core::Skin::ViewAttributes::kTitle);
		String title (var.asString ());
		Font font;
		accessor.getFont (font);

		Color textColor;
		accessor.getStyleColor (textColor, Core::Skin::ViewAttributes::kTextColor);
		Alignment alignment = accessor.getTextAlignment ();

		graphics.drawString (viewRect, title, font, SolidBrush (textColor), alignment);
	}
	else if(className == Core::Skin::ViewClasses::kMultiLineLabel)
	{
		Variant var;
		viewElement->getAttributeValue (var, Core::Skin::ViewAttributes::kTitle);
		String title (var.asString ());
		Font font;
		accessor.getFont (font);

		Color textColor;
		accessor.getStyleColor (textColor, Core::Skin::ViewAttributes::kTextColor);
		Alignment alignment = accessor.getTextAlignment ();

		graphics.drawText (viewRect, title, font, SolidBrush (textColor), TextFormat (alignment, TextFormat::kWordBreak));
	}
	else if(className == Core::Skin::ViewClasses::kButton ||
			className == Core::Skin::ViewClasses::kToggle ||
			className == Core::Skin::ViewClasses::kRadioButton)
	{
		if(auto image = accessor.getImage ())
		{
			graphics.drawImage (image, viewRect.getLeftTop ());
		}
		else
		{
			int options = accessor.getOptions (Core::Skin::Enumerations::buttonOptions);
			bool transparent = (options & Core::Skin::kButtonAppearanceTransparent) != 0;
			if(transparent == false)
			{
				Color backColor;
				accessor.getStyleColor (backColor, Core::Skin::ViewAttributes::kBackColor);
				graphics.fillRect (viewRect, SolidBrush (backColor));
			}
		}

		if(auto icon = accessor.getImage (Core::Skin::ViewAttributes::kIcon))
		{
			Rect srcRect (0, 0, icon->getWidth (), icon->getHeight ());
			Rect dstRect (0, 0, srcRect.getWidth (), srcRect.getHeight ());
			dstRect.center (viewRect);
			graphics.drawImage (icon, srcRect, dstRect);
		}

		Variant title;
		if(viewElement->getAttributeValue (title, Core::Skin::ViewAttributes::kTitle))
		{
			Color textColor;
			accessor.getStyleColor (textColor, Core::Skin::ViewAttributes::kTextColor);
			Font font;
			accessor.getFont (font);
			Alignment alignment = accessor.getTextAlignment ();

			graphics.drawString (viewRect, title.asString (), font, SolidBrush (textColor), alignment);
		}
	}
	else if(className == Core::Skin::ViewClasses::kValueBar ||
			className == Core::Skin::ViewClasses::kSlider)
	{
		if(auto image = accessor.getImage ())
			graphics.drawImage (image, viewRect.getLeftTop ());
		else if(auto background = accessor.getImage (Core::Skin::ViewAttributes::kBackground))
			graphics.drawImage (image, viewRect.getLeftTop ());
		else
		{
			Color backColor;
			accessor.getStyleColor (backColor, Core::Skin::ViewAttributes::kBackColor);
			graphics.fillRect (viewRect, SolidBrush (backColor));
		}
	}
	else if(className == Core::Skin::ViewClasses::kTextBox)
	{
		// Note: TextBox doesn't draw a background but we still want to return true.
	}
	else if(className == Core::Skin::ViewClasses::kEditBox)
	{
		Color foreColor;
		accessor.getStyleColor (foreColor, Core::Skin::ViewAttributes::kForeColor);
		graphics.drawRect (viewRect, Pen (foreColor));
	}
	else if(className == Core::Skin::ViewClasses::kSelectBox)
	{
		if(auto image = accessor.getImage ())
		{
			graphics.drawImage (image, viewRect.getLeftTop ());
		}
		else
		{
			Color foreColor;
			accessor.getStyleColor (foreColor, Core::Skin::ViewAttributes::kForeColor);
			graphics.drawRect (viewRect, Pen (foreColor));
		}
	}
	else if(className == Core::Skin::ViewClasses::kListView)
	{
		Color backColor;
		accessor.getStyleColor (backColor, Core::Skin::ViewAttributes::kBackColor);
		graphics.fillRect (viewRect, SolidBrush (backColor));
	}
	else
		result = false;

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinEditSupport::getSourceCodeForElement (String& sourceCode, ISkinElement* element) const
{
	if(auto formElement = unknown_cast<CoreSkinFormElement> (element))
	{
		Attributes a;
		saveForm (a, *formElement);
		MemoryStream ms;
		saveData (ms, a, false);
		ms.rewind ();
		sourceCode = TextUtils::loadString (ms, String::getLineEnd (), Text::kUTF8);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreSkinEditSupport::setModelDirty (ISkinModel::ElementType type, ISkinElement* changedElement)
{
	switch(type)
	{
	case ISkinModel::kFontsElement : fontsModified (true); break;
	case ISkinModel::kStylesElement : stylesModified (true); break;
	case ISkinModel::kImagesElement : imagesModified (true); break;
	case ISkinModel::kFormsElement :
		if(auto formElement = unknown_cast<CoreSkinFormElement> (changedElement))
			formElement->setModified (true);
		else
			formsModified (true);
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreSkinEditSupport::saveModelChanges (IProgressNotify* progress)
{
	bool hasErrors = false;

	auto saveElementIndex = [&] (CStringPtr fileName1, CStringPtr fileName2, const Container& elements)
	{
		String indexFileName (model.isBinaryFormat () ? fileName2 : fileName1);

		if(progress)
			progress->updateAnimated (indexFileName);

		if(!saveIndex (indexFileName, elements))
			hasErrors = true;
	};

	// fonts
	if(fontsModified ())
	{
		saveElementIndex (Core::Skin::FileNames::kFontFile1,
						  Core::Skin::FileNames::kFontFile2,
						  model.getFontElements ());
	}

	// styles
	if(stylesModified ())
	{
		saveElementIndex (Core::Skin::FileNames::kStyleFile1,
						  Core::Skin::FileNames::kStyleFile2,
						  model.getStyleElements ());
	}

	// images
	if(imagesModified ())
	{
		saveElementIndex (Core::Skin::FileNames::kBitmapFile1,
						  Core::Skin::FileNames::kBitmapFile2,
						  model.getImageElements ());
	}

	// forms
	if(formsModified ())
	{
		saveElementIndex (Core::Skin::FileNames::kViewFile1,
						  Core::Skin::FileNames::kViewFile2,
						  model.getFormElements ());
	}

	// form data
	for(auto formElement : iterate_as<CoreSkinFormElement> (model.getFormElements ()))
		if(formElement->isModified ())
		{
			ASSERT (!formElement->getSourceFile ().isEmpty ())

			if(progress)
				progress->updateAnimated (formElement->getSourceFile ());

			Attributes formData;
			saveForm (formData, *formElement);

			Url path;
			path.setPath (formElement->getSourceFile ());
			AutoPtr<IStream> stream = model.getPackage ()->getFileSystem ()->openStream (path, IStream::kCreateMode);
			if(stream && saveData (*stream, formData, model.isBinaryFormat ()))
				formElement->setModified (false);
			else
			{
				ccl_raise (formElement->getSourceFile ());
				hasErrors = true;
			}
		}

	if(hasErrors == false)
		modifications = 0;
	return !hasErrors;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CoreSkinEditSupport::saveIndex (StringRef fileName, const Container& elements)
{
	AttributeQueue indexData;
	for(auto element : iterate_as<CoreSkinElement> (elements))
	{
		Attributes* elementAttr = NEW Attributes;
		elementAttr->set (Core::Skin::ResourceAttributes::kName, element->getName ());

		if(auto formElement = ccl_cast<CoreSkinFormElement> (element))
		{
			String comment;
			element->getComment (comment);
			if(!comment.isEmpty ())
				elementAttr->set (Core::Skin::ResourceAttributes::kComment, comment);

			elementAttr->set (Core::Skin::ResourceAttributes::kFile, formElement->getSourceFile ());
		}
		else
		{
			if(auto imageElement = ccl_cast<CoreSkinImageElement> (element))
				elementAttr->set (Core::Skin::ResourceAttributes::kFile, imageElement->getImagePath ());

			Attributes attributes;
			element->getAttributes (attributes);
			elementAttr->addFrom (attributes);
		}

		indexData.addAttributes (elementAttr, Attributes::kOwns);
	}

	Url path;
	path.setName (fileName);
	AutoPtr<IStream> stream = model.getPackage ()->getFileSystem ()->openStream (path, IStream::kCreateMode);
	if(stream && saveData (*stream, indexData, model.isBinaryFormat ()))
		return true;
	else
	{
		ccl_raise (fileName);
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CoreSkinEditSupport::saveForm (Attributes& a, const CoreSkinViewElement& formElement) const
{
	struct Helper
	{
		static void childrenToAttributes (Attributes& a, const CoreSkinViewElement& viewElement)
		{
			for(auto childElement : iterate_as<CoreSkinViewElement> (viewElement.getChildren ()))
			{
				Attributes* childAttr = NEW Attributes;
				toAttributes (*childAttr, *childElement);
				a.queue (Core::Skin::ViewAttributes::kChildren, childAttr, Attributes::kOwns);
			}
		}

		static void toAttributes (Attributes& a, const CoreSkinViewElement& viewElement)
		{
			if(!viewElement.getName ().isEmpty ())
				a.set (Core::Skin::ViewAttributes::kType, viewElement.getName ());

			Attributes attributes;
			viewElement.getAttributes (attributes);
			a.addFrom (attributes);

			if(!viewElement.getChildren ().isEmpty ())
				childrenToAttributes (a, viewElement);
		}
	};

	formElement.getAttributes (a);
	Helper::childrenToAttributes (a, formElement);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CoreSkinEditSupport::saveData (IStream& stream, const Attributes& a, bool binary) const
{
	if(binary)
		return UBJsonArchive (stream).saveAttributes (nullptr, a);
	else
		return JsonArchive (stream).saveAttributes (nullptr, a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CoreSkinEditSupport::saveData (IStream& stream, const AttributeQueue& queue, bool binary) const
{
	if(binary)
		return UBJsonArchive (stream).saveArray (queue);
	else
		return JsonArchive (stream).saveArray (queue);
}

//************************************************************************************************
// CoreViewElementAccessor
//************************************************************************************************

IImage* CoreViewElementAccessor::getImage (StringID _name) const
{
	MutableCString name (_name);
	if(name.isEmpty ())
		name = Core::Skin::ViewAttributes::kImage;

	Variant imageVar;
	viewElement.getAttributeValue (imageVar, name);
	MutableCString imageName (imageVar.asString ());

	auto imageItem = model.findImageElement (imageName);
	return imageItem ? imageItem->getImage () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CoreViewElementAccessor::getOptions (const Core::EnumInfo enumInfo[], StringID _name) const
{
	MutableCString name (_name);
	if(name.isEmpty ())
		name = Core::Skin::ViewAttributes::kOptions;

	Variant optionVar;
	viewElement.getAttributeValue (optionVar, name);
	MutableCString optionString (optionVar.asString ());
	return Core::EnumInfo::parseMultiple (optionString, enumInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CoreViewElementAccessor::getStyleValue (Variant& value, StringID name) const
{
	Variant styleVar;
	viewElement.getAttributeValue (styleVar, Core::Skin::ViewAttributes::kStyle);
	if(styleVar.isString ()) // name of shared style
	{
		MutableCString styleName (styleVar.asString ());
		int retryCount = 0;
		while(!styleName.isEmpty () && retryCount < 10)
		{
			auto styleElement = model.findStyleElement (styleName);
			if(!styleElement)
				break;

			if(styleElement->getAttributeValue (value, name))
				return true;

			// handle style inheritance
			Variant inheritVar;
			styleElement->getAttributeValue (inheritVar, Core::Skin::ViewAttributes::kInherit);
			styleName = inheritVar.asString ();
			retryCount++;
		}
	}
	else if(auto styleAttr = unknown_cast<Attributes> (styleVar.asUnknown ()))
		return styleAttr->getAttribute (value, name) != 0;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CoreViewElementAccessor::getStyleColor (Color& color, StringID name) const
{
	Variant value;
	if(getStyleValue (value, name))
		return Colors::fromString (color, value.asString ());
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CoreViewElementAccessor::getFont (Font& font) const
{
	Variant var;
	getStyleValue (var, Core::Skin::ViewAttributes::kFont);
	MutableCString fontName (var.asString ());
	auto fontElement = model.findFontElement (fontName);
	if(fontElement)
	{
		Variant faceVar, sizeVar;
		if(!fontElement->getAttributeValue (faceVar, Core::Skin::ResourceAttributes::kFontFace))
			faceVar = Font::getDefaultFont ().getFace ();
		if(!fontElement->getAttributeValue (sizeVar, Core::Skin::ResourceAttributes::kSize))
			sizeVar = Font::getDefaultFont ().getSize ();
		font.setFace (faceVar.asString ());
		font.setSize (sizeVar.asFloat ());
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CoreViewElementAccessor::getTextAlignment (int defAlign) const
{
	Variant var;
	if(getStyleValue (var, Core::Skin::ViewAttributes::kTextAlign))
	{
		MutableCString alignString (var);
		return Core::EnumInfo::parseMultiple (alignString, Core::Skin::Enumerations::alignment);
	}
	else
		return defAlign;
}

//************************************************************************************************
// CoreSkinTypeLibrary
//************************************************************************************************

DEFINE_SINGLETON (CoreSkinTypeLibrary)

//////////////////////////////////////////////////////////////////////////////////////////////////

CoreSkinTypeLibrary::CoreSkinTypeLibrary ()
: TypeLibrary (CORE_SKIN_TYPELIB_NAME),
  imageClass (nullptr),
  fontClass (nullptr),
  styleClass (nullptr),
  viewClass (nullptr),
  containerViewClass (nullptr),
  formClass (nullptr)
{
	objectCleanup (true);

	// Non-view classes
	imageClass = NEW CoreSkinImageElementClass;
	imageClass->addMember (Core::Skin::ResourceAttributes::kName, ITypeInfo::kString);
	imageClass->addMember (Core::Skin::ResourceAttributes::kFile, ITypeInfo::kString);
	imageClass->addMember (Core::Skin::ResourceAttributes::kMonochrome, ITypeInfo::kBool);
	imageClass->addMember (Core::Skin::ResourceAttributes::kAlwaysCached, ITypeInfo::kBool);
	imageClass->addMember (Core::Skin::ResourceAttributes::kFrames, ITypeInfo::kInt);
	addType (imageClass);

	fontClass = NEW CoreSkinElementClass (CoreSkinNonViewClasses::kFont);
	fontClass->addMember (Core::Skin::ResourceAttributes::kName, ITypeInfo::kString);
	fontClass->addMember (Core::Skin::ResourceAttributes::kFile, ITypeInfo::kString);
	fontClass->addMember (Core::Skin::ResourceAttributes::kMonochrome, ITypeInfo::kBool);
	fontClass->addMember (Core::Skin::ResourceAttributes::kFontNumber, ITypeInfo::kInt);
	fontClass->addMember (Core::Skin::ResourceAttributes::kFontFace, ITypeInfo::kString);
	fontClass->addMember (Core::Skin::ResourceAttributes::kDefault, ITypeInfo::kBool);
	addType (fontClass);

	styleClass = NEW CoreSkinElementClass (CoreSkinNonViewClasses::kStyle);
	styleClass->addMember (Core::Skin::ResourceAttributes::kName, ITypeInfo::kString);
	styleClass->addMember (Core::Skin::ViewAttributes::kInherit, ITypeInfo::kString);
	styleClass->addMember (Core::Skin::ViewAttributes::kBackColor, ITypeInfo::kString);
	styleClass->addMember (Core::Skin::ViewAttributes::kBackColorDisabled, ITypeInfo::kString);
	styleClass->addMember (Core::Skin::ViewAttributes::kForeColor, ITypeInfo::kString);
	styleClass->addMember (Core::Skin::ViewAttributes::kForeColorDisabled, ITypeInfo::kString);
	styleClass->addMember (Core::Skin::ViewAttributes::kTextColorOn, ITypeInfo::kString);
	styleClass->addMember (Core::Skin::ViewAttributes::kTextColorDisabled, ITypeInfo::kString);
	styleClass->addMember (Core::Skin::ViewAttributes::kHiliteColor, ITypeInfo::kString);
	styleClass->addMember (Core::Skin::ViewAttributes::kFont, ITypeInfo::kString);
	styleClass->addMember (Core::Skin::ViewAttributes::kTextAlign, ITypeInfo::kString);
	addType (styleClass);

	// Base classes
	this->viewClass = NEW CoreControlClass (Core::Skin::ViewClasses::kView);
	viewClass->isBaseClass (true);
	viewClass->addMember (Core::Skin::ViewAttributes::kName, ITypeInfo::kString);
	viewClass->addMember (Core::Skin::ViewAttributes::kWidth, ITypeInfo::kInt);
	viewClass->addMember (Core::Skin::ViewAttributes::kHeight, ITypeInfo::kInt);
	viewClass->addMember (Core::Skin::ViewAttributes::kSize, ITypeInfo::kString);
	viewClass->addMember (Core::Skin::ViewAttributes::kStyle, ITypeInfo::kString);
	viewClass->addMember (Core::Skin::ViewAttributes::kOptions, ITypeInfo::kString);
	addOptionEnum (Core::Skin::ViewClasses::kView, Core::Skin::ViewAttributes::kOptions, Core::Skin::Enumerations::viewOptions);
	addType (viewClass);

	this->containerViewClass = NEW CoreControlClass (Core::Skin::ViewClasses::kContainerView, viewClass);
	containerViewClass->isBaseClass (true);
	containerViewClass->addMember (Core::Skin::ViewAttributes::kController, ITypeInfo::kString);
	addType (containerViewClass);

	this->formClass = NEW CoreControlClass ("Form"/*not used/defined in corelib*/, containerViewClass);
	formClass->isFormClass (true);
	addType (formClass);

	// Labels
	auto labelClass = NEW CoreControlClass (Core::Skin::ViewClasses::kLabel, viewClass);
	labelClass->addMember (Core::Skin::ViewAttributes::kTitle, ITypeInfo::kString);
	addOptionEnum (Core::Skin::ViewClasses::kLabel, Core::Skin::ViewAttributes::kOptions, Core::Skin::Enumerations::labelOptions);
	addType (labelClass);

	auto multiLineLabelClass = NEW CoreControlClass (Core::Skin::ViewClasses::kMultiLineLabel, viewClass);
	multiLineLabelClass->addMember (Core::Skin::ViewAttributes::kTitle, ITypeInfo::kString);
	addType (multiLineLabelClass);

	// Container views
	auto imageViewClass = NEW CoreControlClass (Core::Skin::ViewClasses::kImageView, containerViewClass);
	imageViewClass->addMember (Core::Skin::ViewAttributes::kImage, ITypeInfo::kString);
	addOptionEnum (Core::Skin::ViewClasses::kImageView, Core::Skin::ViewAttributes::kOptions, Core::Skin::Enumerations::imageViewOptions);
	addType (imageViewClass);

	auto variantViewClass = NEW CoreControlClass (Core::Skin::ViewClasses::kVariantView, containerViewClass);
	addType (variantViewClass);

	auto delegateViewClass = NEW CoreControlClass (Core::Skin::ViewClasses::kDelegate, containerViewClass);
	delegateViewClass->addMember (Core::Skin::ViewAttributes::kViewName, ITypeInfo::kString);
	addType (delegateViewClass);

	auto alignViewClass = NEW CoreControlClass (Core::Skin::ViewClasses::kAlignView, containerViewClass);
	alignViewClass->addMember (Core::Skin::ViewAttributes::kTextAlign, ITypeInfo::kString);
	addOptionEnum (Core::Skin::ViewClasses::kAlignView, Core::Skin::ViewAttributes::kTextAlign, Core::Skin::Enumerations::alignment);
	addType (alignViewClass);

	// Controls
	auto controlBaseClass = viewClass; // abstract Control class doesn't have additional members

	auto buttonClass = NEW CoreControlClass (Core::Skin::ViewClasses::kButton, controlBaseClass);
	buttonClass->addMember (Core::Skin::ViewAttributes::kImage, ITypeInfo::kString);
	buttonClass->addMember (Core::Skin::ViewAttributes::kIcon, ITypeInfo::kString);
	buttonClass->addMember (Core::Skin::ViewAttributes::kTitle, ITypeInfo::kString);
	addOptionEnum (Core::Skin::ViewClasses::kButton, Core::Skin::ViewAttributes::kOptions, Core::Skin::Enumerations::buttonOptions);
	addType (buttonClass);

	auto toggleClass = NEW CoreControlClass (Core::Skin::ViewClasses::kToggle, buttonClass);
	addType (toggleClass);

	auto radioButtonClass = NEW CoreControlClass (Core::Skin::ViewClasses::kRadioButton, buttonClass);
	addType (radioButtonClass);

	auto valueBarClass = NEW CoreControlClass (Core::Skin::ViewClasses::kValueBar, controlBaseClass);
	addOptionEnum (Core::Skin::ViewClasses::kValueBar, Core::Skin::ViewAttributes::kOptions, Core::Skin::Enumerations::valueBarOptions);
	valueBarClass->addMember (Core::Skin::ViewAttributes::kImage, ITypeInfo::kString);
	valueBarClass->addMember (Core::Skin::ViewAttributes::kBackground, ITypeInfo::kString);
	addType (valueBarClass);

	auto sliderClass = NEW CoreControlClass (Core::Skin::ViewClasses::kSlider, valueBarClass);
	addType (sliderClass);

	auto textBoxClass = NEW CoreControlClass (Core::Skin::ViewClasses::kTextBox, controlBaseClass);
	addOptionEnum (Core::Skin::ViewClasses::kTextBox, Core::Skin::ViewAttributes::kOptions, Core::Skin::Enumerations::textBoxOptions);
	textBoxClass->addMember (Core::Skin::ViewAttributes::kTextTrimMode, ITypeInfo::kString);
	addOptionEnum (Core::Skin::ViewClasses::kTextBox, Core::Skin::ViewAttributes::kTextTrimMode, Core::Skin::Enumerations::textTrimModes);
	addType (textBoxClass);

	auto editBoxClass = NEW CoreControlClass (Core::Skin::ViewClasses::kEditBox, textBoxClass);
	editBoxClass->addMember (Core::Skin::ViewAttributes::kKeyboardLayout, ITypeInfo::kString);
	addOptionEnum (Core::Skin::ViewClasses::kEditBox, Core::Skin::ViewAttributes::kKeyboardLayout, Core::Skin::Enumerations::keyboardLayouts);
	addType (editBoxClass);

	auto selectBoxClass = NEW CoreControlClass (Core::Skin::ViewClasses::kSelectBox, textBoxClass);
	selectBoxClass->addMember (Core::Skin::ViewAttributes::kImage, ITypeInfo::kString);
	addType (selectBoxClass);

	auto listViewClass = NEW CoreControlClass (Core::Skin::ViewClasses::kListView, viewClass);
	addType (listViewClass);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CoreSkinTypeLibrary::addOptionEnum (StringID prefix, StringID name, const Core::EnumInfo enumInfo[])
{
	MutableCString fullName (prefix);
	fullName += ".";
	fullName += name;
	nameStrings.add (fullName);

	addEnum (NEW TEnumTypeInfo<Core::EnumInfo> (fullName.str (), enumInfo, Core::EnumInfo::getCount (enumInfo)));
}

//************************************************************************************************
// CoreSkinElementClass / CoreSkinImageElementClass / CoreControlClass
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (CoreSkinElementClass, TypeInfo)
DEFINE_CLASS_HIDDEN (CoreSkinImageElementClass, CoreSkinElementClass)
DEFINE_CLASS_ABSTRACT_HIDDEN (CoreControlClass, CoreSkinElementClass)
