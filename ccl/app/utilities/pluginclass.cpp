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
// Filename    : ccl/app/utilities/pluginclass.cpp
// Description : Plug-In Class
//
//************************************************************************************************

#define DEBUG_LOG 0
#define DEBUG_MSG 0 //temporary logs for debugging

#include "ccl/app/utilities/pluginclass.h"
#include "ccl/app/utilities/imagefile.h"
#include "ccl/app/component.h"

#include "ccl/base/storage/file.h"
#include "ccl/base/storage/textfile.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/jsonarchive.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/message.h"

#include "ccl/public/text/language.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/plugins/icoderesource.h"
#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/gui/ipluginview.h"
#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"

namespace CCL {

//************************************************************************************************
// PlugInSnapshots::SnapshotItem
//************************************************************************************************

class PlugInSnapshots::SnapshotItem: public Object
{
public:
	SnapshotItem (UIDRef cid = kNullUID)
	: cid (cid),
	  owner (nullptr),
	  highlight (false)
	{}

	PROPERTY_OBJECT (Boxed::UID, cid, ClassID)
	PROPERTY_POINTER (SnapshotPackage, owner, Owner)

	StringRef getDefaultFileName () const
	{
		return defaultVariant.getFileName ();
	}

	void setDefaultFileName (StringRef fileName)
	{
		defaultVariant.setFileName (fileName);
	}
	
	StringRef getDefaultFileName2x () const
	{
		return defaultVariant.getFileName2x ();
	}
	
	void setDefaultFileName2x (StringRef fileName)
	{
		defaultVariant.setFileName2x (fileName);
	}

	void reset ()
	{
		defaultVariant.reset ();
	}

	IImage* getImage (StringID which)
	{
		ASSERT (which == kDefault)
		return getImageVariant (defaultVariant);
	}

	PROPERTY_STRING (defaultDescription, DefaultDescription)

	String getDescription (StringID which) const
	{
		ASSERT (which == kDefault)
		return defaultDescription;
	}

	PROPERTY_BOOL (highlight, Highlight)

	// Object
	bool equals (const Object& obj) const override
	{
		return cid == ((const SnapshotItem&)obj).cid;
	}

	int getHashCode (int size) const override
	{
		return cid.getHashCode (size);
	}

protected:
	class ImageVariant
	{
	public:
		PROPERTY_STRING (fileName, FileName)
		PROPERTY_STRING (fileName2x, FileName2x)
		PROPERTY_SHARED_AUTO (IImage, image, Image)
		PROPERTY_BOOL (failed, Failed)

		ImageVariant ()
		: failed (false)
		{}

		void reset ()
		{
			fileName.empty ();
			fileName2x.empty ();
			image.release ();
			failed = false;
		}
	};

	ImageVariant defaultVariant;

	IImage* getImageVariant (ImageVariant& iv);
};

//************************************************************************************************
// PlugInSnapshots::SnapshotPackage
//************************************************************************************************

class PlugInSnapshots::SnapshotPackage: public Object
{
public:
	SnapshotPackage ()
	{
		items.objectCleanup (true);
	}

	PROPERTY_OBJECT (Url, folder, Folder)
	PROPERTY_STRING (packageId, PackageID)
	PROPERTY_SHARED_AUTO (IPackageFile, packageFile, PackageFile)

	const ObjectArray& getItems () const { return items; }

	SnapshotItem* findItem (UIDRef cid) const
	{
		return (SnapshotItem*)items.findEqual (SnapshotItem (cid));
	}

	SnapshotItem* addItem (UIDRef cid, StringRef defaultFileName, StringRef defaultFileName2x, StringRef defaultDescription, bool highlight)
	{
		SnapshotItem* item = NEW SnapshotItem (cid);
		item->setDefaultFileName (defaultFileName);
		item->setDefaultDescription (defaultDescription);
		item->setDefaultFileName2x (defaultFileName2x);
		item->setHighlight (highlight);
		item->setOwner (this);
		items.add (item);
		return item;
	}

	void removeItem (SnapshotItem* item)
	{
		if(items.remove (item))
			item->release ();
	}

	void loadItems (const Attributes& a)
	{
		IterForEach (a.newQueueIterator ("snapshots", ccl_typeid<Attributes> ()), Attributes, attr)
			UID cid;
			cid.fromString (attr->getString ("cid"));
			String defaultFileName (attr->getString (kDefault));
			MutableCString default2x (kDefault);
			default2x.append ("2x");
			String defaultFileName2x (attr->getString (default2x));
			String defaultDescription = attr->getString ("description"); // description is optional
			bool highlight = attr->getBool ("highlight");
			if(cid.isValid () && !defaultFileName.isEmpty ())
				addItem (cid, defaultFileName, defaultFileName2x, defaultDescription, highlight);
		EndFor
	}
	
	void saveAttributeList (IAttributeList& attributeList, SnapshotItem* item) const
	{
		attributeList.setAttribute ("cid", UIDString (item->getClassID ()));
		attributeList.setAttribute (kDefault, item->getDefaultFileName ());
		if(!item->getDefaultFileName2x ().isEmpty ())
		{
			MutableCString default2x (kDefault);
			default2x.append ("2x");
			attributeList.setAttribute (default2x, item->getDefaultFileName2x ());
		}
		if(!item->getDefaultDescription ().isEmpty ())
			attributeList.setAttribute ("description", item->getDefaultDescription ());
	}

	void saveItems (Attributes& a) const
	{
		IAttributeQueue* itemQueue = a.newAttributeQueue ();
		ArrayForEach (items, SnapshotItem, item)
			IAttributeList* itemAttr = a.newAttributes ();
			saveAttributeList (*itemAttr, item);
			itemQueue->addValue (itemAttr, Attributes::kOwns);
		EndFor
		a.set ("snapshots", itemQueue, IAttributeList::kOwns);
	}

	IImage* loadImage (StringRef fileName, StringRef fileName2x)
	{
		if(!packageId.isEmpty ())
			return ImageFile::loadImage (PackageUrl (packageId, fileName));
		else
		{
			ASSERT (!folder.isEmpty ())

			auto expandPath = [this] (Url& path, StringRef file)
			{
				path.fromDisplayString (file);
				if(!path.getPath ().contains (Url::strPathChar))
					path.makeAbsolute (folder);
			};
			
			Url filePath;
			expandPath (filePath, fileName);
			
			// special case: try to load from skin
			if(folder.getProtocol () == IThemeManager::kThemeProtocol)
			{
				ITheme* theme = System::GetThemeManager ().getApplicationTheme ();
				return theme ? return_shared (theme->getImage (MutableCString (fileName))) : nullptr;
			}
			else if(!fileName2x.isEmpty ())
			{
				Vector<IImage*> images;
				Vector<float> scaleFactors;
				
				AutoPtr<IImage> smallSnapshot = GraphicsFactory::loadImageFile (filePath);
				images.add (smallSnapshot);
				
				Url filePath2x;
				expandPath (filePath2x, fileName2x);
				AutoPtr<IImage> largeSnapshot = GraphicsFactory::loadImageFile (filePath2x);
				images.add (largeSnapshot);
				
				scaleFactors.add (1.f);
				scaleFactors.add (2.f);
				
				return GraphicsFactory::createMultiResolutionBitmap (images, scaleFactors, images.count ());
			}
			else
				return ImageFile::loadImage (filePath);
		}
	}

protected:
	ObjectArray items;
};

//************************************************************************************************
// PlugInViewManagement
//************************************************************************************************

class PlugInViewManagement: public Object,
							public IPlugInViewManagement,
							public Singleton<PlugInViewManagement>
{
public:
	// IPlugInViewManagement
	tbool CCL_API isSystemScalingEnabled (UIDRef cid) const override
	{
		return System::GetPluginPresentation ().isSystemScalingEnabled (cid);
	}

	CLASS_INTERFACE (IPlugInViewManagement, Object)
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("CCL")
	XSTRING (PlugInSnapshots, "Plug-in Thumbnails")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SINGLETON (PlugInViewManagement)
static AutoPtr<IPlugInViewStatics> plugViewStatics;

// pointers to presentation / snapshots instances used in this module (from host or own singleton)
static struct
{
	IPluginPresentation* presentation = nullptr;
	IPlugInSnapshots* snapshots = nullptr;

	void terminate ()
	{
		presentation = nullptr;
		snapshots = nullptr;
	}
} moduleInstances;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (PluginPresentation, kFirstRun)
{
	if(System::IsInMainAppModule ())
	{
		System::GetObjectTable ().registerObject (&System::GetPluginPresentation (), ccl_iid<IPluginPresentation> (), "PluginPresentation");
		System::GetObjectTable ().registerObject (&System::GetPluginSnapshots (), ccl_iid<IPlugInSnapshots> (), "PlugInSnapshots");

		plugViewStatics = ccl_new<IPlugInViewStatics> (ClassID::PlugInViewStatics);
		if(plugViewStatics)
			plugViewStatics->setManagementInterface (&PlugInViewManagement::instance ());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_TERM (PluginPresentation)
{
	if(System::IsInMainAppModule ())
	{
		System::GetObjectTable ().unregisterObject (&System::GetPluginPresentation ());
		System::GetObjectTable ().unregisterObject (&System::GetPluginSnapshots ());

		if(plugViewStatics)
		{
			plugViewStatics->setManagementInterface (nullptr);
			plugViewStatics.release ();
		}
	}

	moduleInstances.terminate ();
}

//************************************************************************************************
// Access process-wide IPluginPresentation / IPlugInSnapshots singletons
//************************************************************************************************

IPluginPresentation& System::GetPluginPresentation ()
{
	if(moduleInstances.presentation == nullptr)
	{
		if(!System::IsInMainAppModule ())
		{
			UnknownPtr<IPluginPresentation> hostInstance (System::GetObjectTable ().getObjectByID (ccl_iid<IPluginPresentation> ()));
			ASSERT (hostInstance)
			moduleInstances.presentation = hostInstance;
		}

		if(moduleInstances.presentation == nullptr)
			moduleInstances.presentation = &PluginPresentation::instance ();
	}
	return *moduleInstances.presentation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPlugInSnapshots& System::GetPluginSnapshots ()
{
	if(moduleInstances.snapshots == nullptr)
	{
		if(!System::IsInMainAppModule ())
		{
			UnknownPtr<IPlugInSnapshots> hostInstance (System::GetObjectTable ().getObjectByID (ccl_iid<IPlugInSnapshots> ()));
			ASSERT (hostInstance)
			moduleInstances.snapshots  = hostInstance;
		}

		if(moduleInstances.snapshots == nullptr)
			moduleInstances.snapshots = &PlugInSnapshots::instance ();
	}
	return *moduleInstances.snapshots;
}

//************************************************************************************************
// PlugIn
//************************************************************************************************

bool PlugIn::getModulePath (IUrl& modulePath, const IClassDescription& description, int searchOptions)
{
	UnknownPtr<ICodeResource> codeResource = const_cast<IClassDescription*> (&description);
	if(codeResource == nullptr)
		return false;
	
	Url path;
	codeResource->getPath (path);
	if(path.isEmpty ())
		return false;
			
	if((searchOptions & kCheckKnownLocation) && !findModulePath (path))
		return false;

	modulePath = path;
	return !modulePath.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugIn::getModulePath (IUrl& modulePath, UIDRef classID, int searchOptions)
{
	const IClassDescription* description = System::GetPlugInManager ().getClassDescription (classID);
	if(description == nullptr)
		return false;
	return getModulePath (modulePath, *description, searchOptions);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugIn::findModulePath (IUrl& modulePath)
{
	bool succeeded = false;
	ForEachPlugInClass (PLUG_CATEGORY_CODERESOURCELOADER, description)
		if(ICodeResourceLoader* loader = ccl_new<ICodeResourceLoader> (description.getClassID ()))
		{
			Url path (modulePath);
			while(!path.isRootPath ())
			{
				if(loader->isCodeResource (path) && loader->isKnownLocation (path))
				{
					modulePath.assign (path);
					succeeded = true;
					break;
				}
				path.ascend ();
			}
			ccl_release (loader);
		}
		if(succeeded)
			break;
	EndFor
	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugIn::findModulePaths (IUnknownList& pluginPaths, const IUnknownList& modulePaths)
{
	ForEachPlugInClass (PLUG_CATEGORY_CODERESOURCELOADER, description)
		if(ICodeResourceLoader* loader = ccl_new<ICodeResourceLoader> (description.getClassID ()))
		{
			ForEachUnknown (modulePaths, unk)
				UnknownPtr<IUrl> modulePath (unk);
				if(modulePath)
				{			
					AutoPtr<Url> path = NEW Url (*modulePath);
					while(!path->isRootPath ())
					{
						if(loader->isCodeResource (*path) && loader->isKnownLocation (*path))
						{
							bool found = false;
							ForEachUnknown (pluginPaths, unk)
								UnknownPtr<IUrl> pluginPath (unk);
								if(path && pluginPath)
								{
									if(path->isEqualUrl (*pluginPath))
									{
										found = true;
										break;
									}
								}
							EndFor
							if(!found)
								pluginPaths.add (path->asUnknown (), true);
							break;
						}
						path->ascend ();
					}
				}
			EndFor
			ccl_release (loader);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugIn::getAlternativeCIDs (Vector<UID>& altIDs, UIDRef classID)
{
	String compatibilityString;
	if(const IClassDescription* description = System::GetPlugInManager ().getClassDescription (classID))
	{
		Variant classAttr;
		if(description->getClassAttribute (classAttr, CCL::Meta::kAlternativeClassID))
			compatibilityString = classAttr.asString ();
	}
	
	if(!compatibilityString.isEmpty ())
	{
		ForEachStringToken (compatibilityString, ",", compatibilitySubString)
			UID compatibilityId;
			if(compatibilityId.fromString (compatibilitySubString))
				altIDs.add (compatibilityId);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugIn::isAlternativeCID (UIDRef classID, UIDRef altID)
{
	Vector<UID> altIDs;
	getAlternativeCIDs (altIDs, classID);
	return altIDs.contains (altID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugIn::findDuplicates (Vector<UID>& classIds, UIDRef cid)
{
	const IClassDescription* originalDescription = System::GetPlugInManager ().getClassDescription (cid);
	if(originalDescription == nullptr)
		return;
	
	Vector<UID> altIDs;
	getAlternativeCIDs (altIDs, cid);
	for(UIDRef altID : altIDs)
	{
		if(const IClassDescription* description = System::GetPlugInManager ().getClassDescription (altID))
			classIds.add (altID);
	}

	const IVersionDescription& originalVersion = originalDescription->getModuleVersion ();
	ForEachPlugInClass (originalDescription->getCategory (), description)
		if(originalDescription->getClassID () == description.getClassID ())
			continue;
		if(originalDescription->getName () != description.getName ())
			continue;
		const IVersionDescription& version = description.getModuleVersion ();
		if(version.getName () != originalVersion.getName ())
			continue;
		if(version.getVendor () != originalVersion.getVendor ())
			continue;

		classIds.add (description.getClassID ());
	EndFor
}

//************************************************************************************************
// PlugInClass
//************************************************************************************************

DEFINE_CLASS (PlugInClass, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInClass::PlugInClass (UIDRef classID, StringRef name, StringRef category)
: classID (classID),
  name (name),
  category (category),
  menuPriority (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInClass::PlugInClass (const IClassDescription& description)
: menuPriority (0)
{
	assign (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInClass::assign (const IClassDescription& description)
{
	setClassID (description.getClassID ());
	setName (description.getName ());
	setCategory (description.getCategory ());
	setSubCategory (description.getSubCategory ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInClass::parseClassName (StringRef className)
{
	if(classID.fromString (className) == false)
	{
		int index = className.lastIndex (CCLSTR (":"));
		ASSERT (index != -1)
		name = className.subString (index+1);
		category = className.subString (0, index);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInClass::setCategories (const PlugInCategory& categories)
{
	setCategory (categories.getCategory ());
	setSubCategory (categories.getSubCategory ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInClass::getPresetMetaInfo (IAttributeList& info) const
{
	PresetMetaAttributes metaAttributes (info);

	metaAttributes.setClassID (getClassID ());
	metaAttributes.setClassName (getName ());
	metaAttributes.setCategory (getCategory ());
	metaAttributes.setSubCategory (getSubCategory ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInClass::toString (String& string, int flags) const
{
	if(!title.isEmpty ())
		string = title;
	else
		string = name;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInClass::equals (const Object& obj) const
{
	const PlugInClass* other = ccl_cast<PlugInClass> (&obj);
	if(other)
		return classID == other->classID;
	else
		return Object::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PlugInClass::compare (const Object& obj) const
{
	const PlugInClass* other = ccl_cast<PlugInClass> (&obj);
	if(other)
	{
		int cmp = ccl_compare (getMenuPriority (), other->getMenuPriority ());			
		if(cmp != 0)
			return cmp;
		
		cmp = category.compare (other->getCategory (), false);		
		if(cmp != 0)
			return cmp;

		// truncate subcategory path for sorting
		String cat1 (subCategory);
		if(int index = cat1.index (CCLSTR ("/")))
			cat1.truncate (index);
		String cat2 (other->getSubCategory ());
		if(int index = cat2.index (CCLSTR ("/")))
			cat2.truncate (index);

		cmp = cat1.compare (cat2, false);
		if(cmp != 0)
			return cmp;

		String s1, s2;
		toString (s1);
		other->toString (s2);
		return s1.compare (s2, false);
	}
	else
		return Object::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInClass::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	classID.fromString (a.getString ("classID"));
	name = a.getString ("name");
	category = a.getString ("category");
	subCategory = a.getString ("subCategory");

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInClass::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();

	if(classID.isValid ())
	{
		String idString;
		classID.toString (idString);
		a.set ("classID", idString);
	}

	if(!name.isEmpty ())
		a.set ("name", name);
	if(!category.isEmpty ())
		a.set ("category", category);
	if(!subCategory.isEmpty ())
		a.set ("subCategory", subCategory);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* PlugInClass::getExactIcon (bool withSubCategory) const
{
	ITheme* theme = System::GetThemeManager ().getApplicationTheme ();
	ASSERT (theme != nullptr)
	if(theme == nullptr)
		return nullptr;
	ITheme* theme2 = RootComponent::instance ().getTheme () != theme ? RootComponent::instance ().getTheme () : nullptr;

	MutableCString iconName ("ClassIcon:");
	iconName.append (category);
	if(!subCategory.isEmpty () && withSubCategory)
	{
		iconName.append (":");
		iconName.append (subCategory);
	}
	iconName.append (":");
	iconName.append (name);
	iconName.replace ('/', '-'); // normalize the name, slashes in resources are used to lookup other scopes

	IImage* icon = theme->getImage (iconName);
	if(!icon && theme2)
		icon = theme2->getImage (iconName);
	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* PlugInClass::getCategoryIcon () const
{
	return PlugInCategory (category, subCategory).getIcon ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* PlugInClass::getIcon (bool exact) const
{
	if(exact)
		if(IImage* icon = getExactIcon ())
			return icon;

	return getCategoryIcon ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PlugInClass::getClassVendor () const
{
	return getClassAttributeString (Meta::kClassVendor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PlugInClass::getClassFolder () const
{
	return getClassAttributeString (Meta::kClassFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PlugInClass::makeTitleWithVendor () const
{
	String vendorTitle;
	toString (vendorTitle);

	String vendor = getClassVendor ();
	if(!vendor.isEmpty ())
	{
		vendorTitle.prepend (" ");
		vendorTitle.prepend (vendor);
	}
	return vendorTitle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInClass::setClassAttribute (StringID id, VariantRef value)
{
	if(!additionalAttributes)
		additionalAttributes = NEW Attributes;
	additionalAttributes->setAttribute (id, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInClass::getClassAttribute (Variant& value, StringID id) const
{
	if(additionalAttributes)
		if(additionalAttributes->getAttribute (value, id))
			return true;

	if(classID.isValid ())
		if(const IClassDescription* description = System::GetPlugInManager ().getClassDescription (classID))
			if(description->getClassAttribute (value, id))
				return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PlugInClass::getClassAttributeString (StringID id) const
{
	Variant var;
	getClassAttribute (var, id);
	return var.asString ();
}

//************************************************************************************************
// PlugInCategory
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PlugInCategory, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInCategory::PlugInCategory (StringRef category, StringRef subCategory, StringRef title)
: category (category),
  subCategory (subCategory),
  title (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* PlugInCategory::getIcon () const
{
	IImage* icon = nullptr;
	if(!category.isEmpty ())
	{
		ITheme* theme = System::GetThemeManager ().getApplicationTheme ();
		ASSERT (theme != nullptr)
		if(theme == nullptr)
			return nullptr;
		ITheme* theme2 = RootComponent::instance ().getTheme () != theme ? RootComponent::instance ().getTheme () : nullptr;

		MutableCString iconName ("ClassIcon:");
		iconName.append (category);

		if(!subCategory.isEmpty ())
		{
			MutableCString iconName2 (iconName);
			iconName2.append (":");

			static const String separator = CCLSTR ("/");
			int index = subCategory.index (separator);
			if(index != -1)
				iconName2.append (subCategory.subString (0, index));
			else
				iconName2.append (subCategory);

			icon = theme->getImage (iconName2);
			if(!icon && theme2)
				icon = theme2->getImage (iconName2);
		}

		if(!icon)
			icon = theme->getImage (iconName);
		if(!icon && theme2)
			icon = theme2->getImage (iconName);
	}
	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCategory::toString (String& string, int flags) const
{
	if(!title.isEmpty ())
		string = title;
	else
	{
		if(!subCategory.isEmpty ())
			string = subCategory;
		else
			string = category;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCategory::equals (const Object& obj) const
{
	const PlugInCategory* other = ccl_cast<PlugInCategory> (&obj);
	if(other)
		return category == other->category && subCategory == other->subCategory;
	return Object::equals (obj);
}

//************************************************************************************************
// PlugInMetaInfo
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PlugInMetaInfo, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInMetaInfo::PlugInMetaInfo ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInMetaInfo::PlugInMetaInfo (UIDRef cid)
{
	assign (cid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInMetaInfo::assign (UIDRef cid)
{
	const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid);
	if(description)
	{
		AutoPtr<IImage> image;
		String text;

		IPluginMetaClass* metaClass = System::GetPlugInManager ().createMetaClass (cid);
		if(metaClass)
		{
			StringID language = System::GetLocaleManager ().getLanguage ();
			TextEncoding guessedEncoding = Text::kUnknownEncoding;
			if(language != LanguageCode::English)
				guessedEncoding = Text::kUTF8;  // enforce UTF-8 if encoding not specified in file

			Url imagePath;
			if(metaClass->getResourceLocation (imagePath, Meta::kClassImageResource, language) == kResultOk)
			{
				// special case: try to load from skin
				if(imagePath.getProtocol () == IThemeManager::kThemeProtocol)
				{
					MutableCString themeID (imagePath.getHostName ());
					ITheme* theme = themeID.isEmpty () ? System::GetThemeManager ().getApplicationTheme () : System::GetThemeManager ().getTheme (themeID);
					image = theme ? return_shared (theme->getImage (MutableCString (imagePath.getPath ()))) : nullptr;
				}
				else
					image = ImageFile::loadImage (imagePath);
			}

			Url textPath;
			if(metaClass->getResourceLocation (textPath, Meta::kClassTextResource, language) == kResultOk)
				text = TextUtils::loadString (textPath, String::getLineEnd (), guessedEncoding);

			ccl_release (metaClass);
		}

		setImage (image);
		setText (text);
	}
	else
	{
		setImage (nullptr);
		setText (String ());
	}
	return description != nullptr;
}

//************************************************************************************************
// PlugInSnapshots::SnapshotItem
//************************************************************************************************

IImage* PlugInSnapshots::SnapshotItem::getImageVariant (ImageVariant& iv)
{
	if(iv.getImage () == nullptr && !iv.isFailed ())
	{
		AutoPtr<IImage> image = owner->loadImage (iv.getFileName (), iv.getFileName2x ());
		iv.setImage (image);
		iv.setFailed (image == nullptr);
	}
	return iv.getImage ();
}

//************************************************************************************************
// PlugInSnapshots
//************************************************************************************************

const String PlugInSnapshots::kFolderName ("Snapshots");
const String PlugInSnapshots::kFileName ("snapshots.json");
String PlugInSnapshots::getTranslatedTitle () { return XSTR (PlugInSnapshots); }
DEFINE_SINGLETON (PlugInSnapshots)

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrlFilter* PlugInSnapshots::createBackupFilter ()
{
	// do not backup original snapshots to reduce size of backup
	class OriginalFilter: public UrlFilter
	{
	public:
		tbool CCL_API matches (UrlRef url) const override
		{
			String fileName;
			url.getName (fileName);
			static const String kOriginal (".original.png");
			if(fileName.endsWith (kOriginal, false))
				return false;
			else
				return true;
		}
	};

	return NEW OriginalFilter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInSnapshots::getAppLocation (IUrl& path)
{
	System::GetSystem ().getLocation (path, System::kAppDeploymentFolder);
	path.descend (kFolderName, Url::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInSnapshots::getUserLocation (IUrl& path)
{
	System::GetSystem ().getLocation (path, System::kUserContentFolder);
	path.descend (kFolderName, Url::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInSnapshots::PlugInSnapshots ()
: skinPackage (nullptr)
{
	packageList.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInSnapshots::addToTable (SnapshotItem* item, bool replace)
{
	SnapshotItem* existing = lookup (item->getClassID ());
	if(replace == false)
	{
		ASSERT (existing == nullptr)
		if(existing != nullptr)
		{
			CCL_WARN ("Plug-in snapshot for %s already exists!\n", MutableCString (UIDString (item->getClassID ())).str ());
			return false;
		}
	}

	if(existing)
		itemTable.remove (existing);

	#if (0 && DEBUG)
	item->getImage (kDefault);
	#endif

	itemTable.add (item);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInSnapshots::addPackage (SnapshotPackage* package)
{
	packageList.add (package);

	// add to hash table
	ArrayForEach (package->getItems (), SnapshotItem, item)
		addToTable (item, true);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInSnapshots::SnapshotPackage* PlugInSnapshots::findPackageForFolder (UrlRef folder) const
{
	ArrayForEach (packageList, SnapshotPackage, package)
		if(package->getFolder ().isEqualUrl (folder))
			return package;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInSnapshots::findPackagesForFolder (Vector<SnapshotPackage*>& result, UrlRef folder) const
{
	for(SnapshotPackage* package : iterate_as<SnapshotPackage> (packageList))
	{
		if(package->getFolder () == folder || Url (folder).contains (package->getFolder ()))
			result.add (package);
	}
	return !result.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PlugInSnapshots::addDefaultLocations ()
{
	Url path;
	getAppLocation (path);
	return addLocation (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PlugInSnapshots::addUserLocations ()
{
	int numFound = 0;
	Url userFolder;
	getUserLocation (userFolder);
	ForEachFile (File (userFolder).newIterator (IFileIterator::kFolders), subFolder)
		numFound += addLocation (*subFolder, false);
	EndFor
	return numFound;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInSnapshots::addSnapshotFile (UrlRef path)
{
	Url snapshotFile (path);
	Url snapshotFolder (path);
	snapshotFolder.ascend ();
	
	if(File (path).exists ())
	{
		Attributes attributes;
		if(AutoPtr<IStream> stream = File (snapshotFile).open ())
			JsonArchive (*stream).loadAttributes (nullptr, attributes);

		SnapshotPackage* package = NEW SnapshotPackage;
		package->setFolder (snapshotFolder);
		package->loadItems (attributes);

		addPackage (package);
		
		return true;
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PlugInSnapshots::addLocation (UrlRef path, tbool deep)
{
	int numFound = 0;

	Url snapshotFile (path);
	snapshotFile.descend (kFileName);
	
	if(File (snapshotFile).exists ())
	{
		Attributes attributes;
		if(AutoPtr<IStream> stream = File (snapshotFile).open ())
			JsonArchive (*stream).loadAttributes (nullptr, attributes);

		SnapshotPackage* package = NEW SnapshotPackage;
		package->setFolder (path);
		package->loadItems (attributes);

		addPackage (package);
		numFound++;
	}
	else if(deep) // deep means we expect multiple package files in this folder
	{
		snapshotFile = Url ();
		snapshotFile.setName (kFileName);

		ForEachFile (File (path).newIterator (), p)
			if(!System::GetPackageHandler ().isPackage (*p))
				continue;

			if(AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().openPackage (*p))
			{
				Attributes attributes;
				if(AutoPtr<IStream> stream = packageFile->getFileSystem ()->openStream (snapshotFile))
				{
					if(JsonArchive (*stream).loadAttributes (nullptr, attributes) == false)
					{
						CCL_PRINTF ("Syntax error in plug-in snapshot file: %s\n", MutableCString (UrlFullString (*p)).str ())
						ASSERT (0)
						//continue; no, continue with partially loaded file
					}
				}

				String id (UIDString::generate ());
				if(System::GetPackageHandler ().mountPackageVolume (packageFile, id, IPackageVolume::kHidden) == kResultOk)
				{
					SnapshotPackage* package = NEW SnapshotPackage;
					package->setPackageID (id);
					package->setPackageFile (packageFile);
					package->loadItems (attributes);
					package->setFolder (path);

					addPackage (package);
					numFound++;
				}
			}
		EndFor
	}

	return numFound;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInSnapshots::removeLocation (UrlRef path)
{
	Vector<SnapshotPackage*> toBeRemoved;
	if(findPackagesForFolder (toBeRemoved, path))
		for(SnapshotPackage* package : toBeRemoved)
		{
			for(SnapshotItem* item : iterate_as<SnapshotItem> (package->getItems ()))
				itemTable.remove (item);

			packageList.remove (package);
			package->release ();
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInSnapshots::hasLocation (UrlRef path) const
{
	Vector<SnapshotPackage*> unused;
	return findPackagesForFolder (unused, path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInSnapshots::addSkinSnapshot (UIDRef cid, StringID imageName)
{
	if(!skinPackage)
	{
		Url path;
		path.setProtocol (IThemeManager::kThemeProtocol);
		skinPackage = NEW SnapshotPackage;
		skinPackage->setFolder (path);
		packageList.add (skinPackage);
	}
	
	auto item = skinPackage->addItem (cid, String (imageName), String::kEmpty, String::kEmpty, false);
	addToTable (item, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInSnapshots::SnapshotItem* PlugInSnapshots::lookup (UIDRef cid) const
{
	return (SnapshotItem*)itemTable.lookup (SnapshotItem (cid));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API PlugInSnapshots::getSnapshot (UIDRef cid, StringID which) const
{
	SnapshotItem* item = lookup (cid);
	return item ? item->getImage (which) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInSnapshots::hasUserSnapshot (UIDRef cid) const
{
	SnapshotItem* item = lookup (cid);
	if(item && item->getOwner ())
	{
		// check if package is in user location
		Url userLocation;
		getUserLocation (userLocation);
		return userLocation.contains (item->getOwner ()->getFolder ());
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInSnapshots::setUserSnapshot (UIDRef cid, IImage* image)
{
	Url packageFolder;
	SnapshotPackage* package = nullptr;
	
	if(image)
	{
		// *** Create image files ***
		String defaultFileName;
		if(!makeSnapshotFiles (packageFolder, defaultFileName, cid, image))
			return false;

		// *** Add to package ***
		package = findPackageForFolder (packageFolder);
		if(package == nullptr)
		{
			package = NEW SnapshotPackage;
			package->setFolder (packageFolder);
			packageList.add (package);
		}

		SnapshotItem* item = package->findItem (cid);
		if(item == nullptr)
		{
			item = package->addItem (cid, defaultFileName, String::kEmpty, String::kEmpty, false);
			addToTable (item, true);
		}
		else
		{
			item->reset ();
			item->setDefaultFileName (defaultFileName);
		}
	}
	else
	{
		// *** Remove image files ***
		removeSnapshotFiles (packageFolder, cid);

		// *** Remove item from package ***
		package = findPackageForFolder (packageFolder);
		if(!package)
			return false;

		if(SnapshotItem* item = package->findItem (cid))
		{
			itemTable.remove (item);
			package->removeItem (item);
		}
		
		restoreDefaultSnapshot (cid);
	}

	// *** Rewrite snapshot file ***
	Attributes attributes;
	package->saveItems (attributes);
	Url snapshotFile (packageFolder);
	snapshotFile.descend (kFileName);
	if(AutoPtr<IStream> stream = File (snapshotFile).open (IStream::kCreateMode))
		JsonArchive (*stream).saveAttributes (nullptr, attributes);

	// *** Signal ***
	// TODO: explicit signal for snapshots???
	if(const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid))
		SignalSource (Signals::kPlugIns).signal (Message (Signals::kPluginPresentationChanged, Variant (IPluginPresentation::kSnapshotChanged), description->getCategory ()));

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInSnapshots::setDefaultSnapshot (UrlRef snapshotFile, UIDRef cid, UrlRef imageFile1x, UrlRef imageFile2x)
{
	Url snapshotFolder = snapshotFile;
	snapshotFolder.ascend ();
	
	SnapshotPackage* package = nullptr;
	
	package = findPackageForFolder (snapshotFolder);
	if(package == nullptr)
	{
		package = NEW SnapshotPackage;
		package->setFolder (snapshotFolder);
		packageList.add (package);
	}

	String imageFilePath1x;
	imageFile1x.toDisplayString (imageFilePath1x);
	
	String imageFilePath2x;
	if(!imageFile2x.isEmpty ())
		imageFile2x.toDisplayString (imageFilePath2x);
	
	SnapshotItem* item = package->findItem (cid);
	if(item == nullptr)
	{
		item = package->addItem (cid, imageFilePath1x, imageFilePath2x, String::kEmpty, false);
		addToTable (item, true);
	}
	else
	{
		item->reset ();
		item->setDefaultFileName (imageFilePath1x);
		item->setDefaultFileName2x (imageFilePath2x);
	}
	
	// *** Save snapshot file ***
	Attributes newAttributes;
	AutoPtr<IAttributeQueue> queue = newAttributes.newAttributeQueue ();
	
	AutoPtr<IAttributeList> itemAttributeList = newAttributes.newAttributes ();
	package->saveAttributeList (*itemAttributeList, item);

	if(File (snapshotFile).exists ())
	{
		Attributes savedAttributes;
		if(AutoPtr<IStream> stream = File (snapshotFile).open (IStream::kReadMode | IStream::kWriteMode))
			JsonArchive (*stream).loadAttributes(nullptr, savedAttributes);
		
		bool duplicate = false;
		IterForEach (savedAttributes.newQueueIterator ("snapshots", ccl_typeid<Object> ()), Object, list)
			UnknownPtr<IAttributeList> currentSavedList = list->asUnknown ();
			Variant currentSavedCidVar;
			currentSavedList->getAttribute (currentSavedCidVar, "cid");
			String currentSavedCid = currentSavedCidVar.asString ();
			queue->addValue (currentSavedList, Attributes::kTemp);
		
			if(currentSavedCid == UIDString (item->getClassID ()))
				duplicate = true;
		EndFor
		
		if(!duplicate)
			queue->addValue (itemAttributeList, Attributes::kTemp);
	}
	else
		queue->addValue (itemAttributeList, Attributes::kTemp);

	newAttributes.set ("snapshots", queue, IAttributeList::kTemp);

	if(AutoPtr<IStream> stream = File (snapshotFile).open (IStream::kCreateMode))
		JsonArchive (*stream).saveAttributes (nullptr, newAttributes);

	// *** Signal ***
	if(const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid))
		SignalSource (Signals::kPlugIns).signal (Message (Signals::kPluginPresentationChanged, Variant (IPluginPresentation::kSnapshotChanged), description->getCategory ()));

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInSnapshots::restoreDefaultSnapshot (UIDRef cid)
{
	ArrayForEach (packageList, SnapshotPackage, package)
		SnapshotItem* item = package->findItem (cid);
		if(item)
		{
			addToTable (item, true);
			break;
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInSnapshots::getSnapshotDescription (String& description, UIDRef cid, StringID which) const
{
	if(SnapshotItem* item = lookup (cid))
	{
		description = item->getDescription (which);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInSnapshots::isHighlight (UIDRef cid) const
{
	if(SnapshotItem* item = lookup (cid))
		return item->isHighlight ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInSnapshots::makeSnapshotFiles (IUrl& packageFolder, String& defaultFileName, UIDRef cid, IImage* image)
{
	// *** Create scaled images ***
	static const int kSnapshotRatio = 4; // 1/4 for high resolution

	Point scaledSize (image->getWidth (), image->getHeight ());
	scaledSize.x /= kSnapshotRatio;
	scaledSize.y /= kSnapshotRatio;
	if(scaledSize.x % 2)
		scaledSize.x++;
	if(scaledSize.y % 2)
		scaledSize.y++;

	auto createScaledImage = [] (IImage* srcImage, PointRef dstSize)
	{
		IImage* dstImage = GraphicsFactory::createBitmap (dstSize.x, dstSize.y);
		Rect srcRect (0, 0, srcImage->getWidth (), srcImage->getHeight ());
		Rect dstRect (0, 0, dstSize.x, dstSize.y);
		AutoPtr<IGraphics> g = GraphicsFactory::createBitmapGraphics (dstImage);
		ImageMode mode (1.f, ImageMode::kInterpolationHighQuality);
		g->drawImage (srcImage, srcRect, dstRect, &mode);
		return dstImage;
	};

	AutoPtr<IImage> image2x = createScaledImage (image, scaledSize);
	AutoPtr<IImage> image1x = createScaledImage (image, Point (scaledSize.x/2, scaledSize.y/2));

	// *** Save images to disk ***
	String fileName, folderName;
	getNamesForClass (fileName, folderName, cid);

	getUserLocation (packageFolder);
	packageFolder.descend (folderName, Url::kFolder);

	const struct { IImage* image; String suffix; } images[] =
	{
		{image,	".original"},
		{image1x, ""},
		{image2x, "@2x"}
	};

	for(int i = 0; i < ARRAY_COUNT (images); i++)
	{
		Url imagePath (packageFolder);
		imagePath.descend (String () << fileName << images[i].suffix << ".png");

		if(images[i].suffix.isEmpty ())
			imagePath.getName (defaultFileName); // report as default

		bool saved = ImageFile (ImageFile::kPNG, images[i].image).saveToFile (imagePath);
		ASSERT (saved == true)
		if(!saved)
			return false;
	}

	ASSERT (!defaultFileName.isEmpty ())
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInSnapshots::removeSnapshotFiles (IUrl& packageFolder, UIDRef cid)
{
	String fileName, folderName;
	getNamesForClass (fileName, folderName, cid);

	getUserLocation (packageFolder);
	packageFolder.descend (folderName, Url::kFolder);

	String suffixes[] =
	{
		".original",
		"",
		"@2x"
	};

	for(int i = 0; i < ARRAY_COUNT (suffixes); i++)
	{
		Url imagePath (packageFolder);
		imagePath.descend (String () << fileName << suffixes[i] << ".png");

		File imageFile (imagePath);
		if(imageFile.exists ())
			imageFile.remove ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInSnapshots::getNamesForClass (String& fileName, String& folderName, UIDRef cid) const
{
	static const String kUnknownFolder (CCLSTR ("(Unknown Vendor)")); // see also presetfile.cpp

	const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid);
	ASSERT (description != nullptr)
	if(description)
	{
		PlugInClass plugClass (*description);
		String vendor = plugClass.getClassVendor ();
		if(vendor.isEmpty ())
			vendor = description->getModuleVersion ().getVendor ();

		if(!vendor.isEmpty ())
			folderName = LegalFolderName (vendor);

		fileName = LegalFileName (plugClass.getName ());
	}

	if(folderName.isEmpty ())
		folderName = kUnknownFolder;

	if(fileName.isEmpty ())
		fileName = UIDString (cid);
}

//************************************************************************************************
// PluginPresentation::PlugInfo
//************************************************************************************************

class PluginPresentation::PlugInfo: public Object
{
public:
	DECLARE_CLASS (PlugInfo, Object)

	PlugInfo (UIDRef cid = kNullUID)
	: cid (cid),
	  lastUsage (0),
	  flags (0)
	{}

	PROPERTY_OBJECT (Boxed::UID, cid, ClassID)
	PROPERTY_STRING (sortPath, SortPath)
	PROPERTY_STRING (favoritePath, FavoritePath)
	PROPERTY_VARIABLE (int64, lastUsage, LastUsage)
	PROPERTY_FLAG (flags, 1<<0, isHidden)
	PROPERTY_FLAG (flags, 1<<1, isFavorite)
	PROPERTY_FLAG (flags, 1<<2, isSystemScalingEnabled)

	Attributes& getAttributes () { return attributes; }
	const Attributes& getAttributes () const { return attributes; }

	// Object
	bool equals (const Object& obj) const override
	{
		return cid == ((const PlugInfo&)obj).cid;
	}

	int getHashCode (int size) const override
	{
		return cid.getHashCode (size);
	}

	bool load (const Storage& storage) override
	{
		Attributes& a = storage.getAttributes ();

		cid.fromString (a.getString ("classID"));
		sortPath = makeLegalFolderPath (a.getString ("sortPath"));
		favoritePath = makeLegalFolderPath (a.getString ("favoritePath"));
		lastUsage = a.getInt64 ("lastUsage");
		flags = a.getInt ("flags");
		a.get (attributes, "attributes");
		return true;
	}

	bool save (const Storage& storage) const override
	{
		Attributes& a = storage.getAttributes ();

		String idString;
		cid.toString (idString);

		a.set ("classID", idString);
		a.set ("sortPath", sortPath);
		if(!favoritePath.isEmpty ())
			a.set ("favoritePath", favoritePath);
		a.set ("lastUsage", lastUsage);
		a.set ("flags", flags);
		if(!attributes.isEmpty ())
			a.set ("attributes", attributes);
		return true;
	}

private:
	int flags;
	Attributes attributes;
};

DEFINE_CLASS_PERSISTENT  (PluginPresentation::PlugInfo, Object, "PlugInInfo")

//************************************************************************************************
// PluginPresentation::PlugSortFolderList
//************************************************************************************************

class PluginPresentation::PlugSortFolderList: public CCL::SortFolderList
{
public:
	DECLARE_CLASS (PlugSortFolderList, SortFolderList)

	bool load (const Storage& storage) override
	{
		return loadFolders (storage, "category");
	}

	bool save (const Storage& storage) const override
	{
		return saveFolders (storage, "category");
	}
};

DEFINE_CLASS_PERSISTENT (PluginPresentation::PlugSortFolderList, SortFolderList, "PlugInFolderList")

//************************************************************************************************
// PluginPresentation::FolderTraits
/** Common interface for handling plug-in sort folders and favorite folders. */
//************************************************************************************************

class PluginPresentation::FolderTraits
{
public:
	FolderTraits (const PluginPresentation& presentation)
	: presentation (ccl_const_cast (presentation))
	{}

	virtual String getFolder (UIDRef cid) const = 0;
	virtual void setFolder (UIDRef cid, StringRef folder) const = 0;
	virtual SortFolderList* getFolderList (StringRef category) const = 0;

protected:
	PluginPresentation& presentation;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

class PluginPresentation::SortFolderTraits: public PluginPresentation::FolderTraits
{
public:
	using FolderTraits::FolderTraits;

	String getFolder (UIDRef cid) const override						{ return presentation.getSortPath (cid); }
	void setFolder (UIDRef cid, StringRef folder) const override		{ return presentation.setSortPath (cid, folder); }
	SortFolderList* getFolderList (StringRef category) const override	{ return presentation.getSortFolderList (category); }
};

//////////////////////////////////////////////////////////////////////////////////////////////////

class PluginPresentation::FavoriteFolderTraits: public PluginPresentation::FolderTraits
{
public:
	using FolderTraits::FolderTraits;

	String getFolder (UIDRef cid) const override						{ return presentation.getFavoriteFolder (cid); }
	void setFolder (UIDRef cid, StringRef folder) const override		{ return presentation.setFavorite (cid, true, folder); }
	SortFolderList* getFolderList (StringRef category) const override	{ return presentation.getFavoriteFolderList (category); }
};

//************************************************************************************************
// PluginPresentation
//************************************************************************************************

DEFINE_SINGLETON (PluginPresentation)
const String PluginPresentation::kSettingsName ("PluginPresentation");

//////////////////////////////////////////////////////////////////////////////////////////////////

PluginPresentation::PluginPresentation ()
: version (1)
{
	plugInfos.objectCleanup (true);
	sortFolders.setListClass (ccl_typeid<PlugSortFolderList> ());
	favoriteFolders.setListClass (ccl_typeid<PlugSortFolderList> ());

	loadSettings ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PluginPresentation::~PluginPresentation ()
{
	saveSettings ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PluginPresentation::getSettingsPath (IUrl& path)
{
	path.assign (XmlSettings (kSettingsName).getPath ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PluginPresentation::loadSettings ()
{
	XmlSettings settings (kSettingsName);
	settings.checkVersion (false);
	if(settings.restore ())
	{
		int settingsVersion = settings.getAttributes ("format").getInt ("version");
		settings.getAttributes ("plugins").unqueue (plugInfos, nullptr, ccl_typeid<PlugInfo> ());
		sortFolders.restore (settings.getAttributes ("folders"));
		favoriteFolders.restore (settings.getAttributes ("favoriteFolders"));

		ArrayForEach (plugInfos, PlugInfo, info)
			plugInfoTable.add (info);
		EndFor

		// try to load factory settings file with corrections for spontaneous recategorizations
		XmlSettings factorySettings;
		factorySettings.checkVersion (false);
		factorySettings.checkName (false);
		String fileName;
		settings.getPath ().getName (fileName);
		factorySettings.setPath (ResourceUrl (fileName));
		if(factorySettings.restore () && !factorySettings.isEmpty ())
		{
			version = factorySettings.getVersion ();
			if(version > settingsVersion) // only adjust once, user can still edit
			{
				ObjectList factoryPlugInfos;
				factorySettings.getAttributes ("plugins").unqueue (factoryPlugInfos, nullptr, ccl_typeid<PlugInfo> ());

				// copy sort paths from factory info
				ArrayForEach (factoryPlugInfos, PlugInfo, factoryInfo)
					if(PlugInfo* info = getPlugInfo (factoryInfo->getClassID (), true))
						info->setSortPath (factoryInfo->getSortPath ());
				EndFor
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::saveSettings ()
{
	XmlSettings settings (kSettingsName);
	settings.getAttributes ("format").set ("version", version); // cannot use Settings::version, an older build (with checkVersion still enabled) would ignore settings written with a version > 1
	settings.getAttributes ("plugins").queue (nullptr, plugInfos, Attributes::kShare);
	sortFolders.store (settings.getAttributes ("folders"));
	favoriteFolders.store (settings.getAttributes ("favoriteFolders"));
	settings.flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::reset ()
{
	ArrayForEach (plugInfos, PlugInfo, info)
		plugInfoTable.remove (info);
	EndFor
	ASSERT (plugInfoTable.isEmpty ())

	plugInfos.removeAll ();
	sortFolders.removeAll ();
	favoriteFolders.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::revert ()
{
	reset ();
	loadSettings ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PluginPresentation::PlugInfo* PluginPresentation::lookup (UIDRef cid) const
{
	return (PlugInfo*)plugInfoTable.lookup (PlugInfo (cid));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PluginPresentation::PlugInfo* PluginPresentation::getPlugInfo (UIDRef cid, bool create)
{
	PlugInfo* info = (PlugInfo*)plugInfoTable.lookup (PlugInfo (cid));
	if(!info && create)
	{
		info = NEW PlugInfo (cid);
		info->setSortPath (getInitialSortPath (cid));

		plugInfos.add (info);
		plugInfoTable.add (info);
	}
	return info;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PluginPresentation::isHidden (UIDRef cid) const
{
	const PlugInfo* info = lookup (cid);
	return info && info->isHidden ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::setHidden (UIDRef cid, tbool state)
{
	getPlugInfo (cid, true)->isHidden (state != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PluginPresentation::isFavorite (UIDRef cid) const
{
	const PlugInfo* info = lookup (cid);
	return info && info->isFavorite ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CCL_API PluginPresentation::getFavoriteFolder (UIDRef cid) const
{
	const PlugInfo* info = lookup (cid);
	return info ? info->getFavoritePath () : String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::setFavorite (UIDRef cid, tbool state, StringRef folder)
{
	#if DEBUG_MSG
	if(PlugInfo* info = getPlugInfo (cid, true))
	{
		MutableCString id;
		info->getClassID ().toCString (id);
		const IClassDescription* description = System::GetPlugInManager ().getClassDescription (info->getClassID ());
		CCL_WARN ("setFavorite (%d, %s) %s: %s\n", state, MutableCString (folder).str (), description ? MutableCString (description->getName ()).str () : "unknown plugin", id.str ());
	}
	#endif

	PlugInfo* info = getPlugInfo (cid, true);
	info->isFavorite (state != 0);
	info->setFavoritePath (state ? folder : String::kEmpty);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API PluginPresentation::getLastUsage (UIDRef cid) const
{
	const PlugInfo* info = lookup (cid);
	return info ? info->getLastUsage () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::logUsage (UIDRef cid)
{
	getPlugInfo (cid, true)->setLastUsage (UnixTime::getTime ()); // can't use system ticks here, value is persistent!

	if(const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid))
		SignalSource (Signals::kPlugIns).signal (Message (Signals::kPluginPresentationChanged, Variant (kUsageChanged), description->getCategory ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PluginPresentation::isSystemScalingEnabled (UIDRef cid) const
{
	const PlugInfo* info = lookup (cid);
	return info && info->isSystemScalingEnabled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PluginPresentation::getAttribute (Variant& value, UIDRef cid, StringID attrId) const
{
	const PlugInfo* info = lookup (cid);
	return info && info->getAttributes ().getAttribute (value, attrId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::setAttribute (UIDRef cid, StringID attrId, VariantRef value)
{
	getPlugInfo (cid, true)->getAttributes ().setAttribute (attrId, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::removeAttribute (UIDRef cid, StringID attrId)
{
	if(auto info = getPlugInfo (cid, false))
		info->getAttributes ().remove (attrId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::setSystemScalingEnabled (UIDRef cid, tbool state)
{
	getPlugInfo (cid, true)->isSystemScalingEnabled (state != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PluginPresentation::makeLegalFolderPath (StringRef path)
{
	return SortFolderList::makeLegalFolderPath (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PluginPresentation::getInitialSortPath (UIDRef cid)
{
	if(const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid))
	{
		auto getFirstPart = [] (StringRef path)
		{
			String first = path;
			int index = path.index (Url::strPathChar);
			if(index > 0)
				first.truncate (index);
			return first;
		};

		auto stripPluginType = [] (String& path)
		{
			int index = path.index (Url::strPathChar);
			if(index >= 0)
				path.remove (0, index + 1);
			else // type only, no category
				path.empty ();
		};

		// try class folder first
		Variant v;
		description->getClassAttribute (v, Meta::kClassFolder);
		String path = v.asString ();

		// strip sub type from folder
		if(getFirstPart (description->getSubCategory ()) == getFirstPart (path))
			stripPluginType (path);

		if(path.isEmpty ())
		{
			// fall back to sub category
			path = description->getSubCategory ();
			stripPluginType (path);
		}

		//CCL_PRINTF ("Initial sort path for '%s' is '%s'\n", MutableCString (description->getName ()).str (), MutableCString (path).str ())
		return makeLegalFolderPath (path);
	}
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CCL_API PluginPresentation::getSortPath (UIDRef cid) const
{
	if(const PlugInfo* info = lookup (cid))
		return info->getSortPath ();

	return getInitialSortPath (cid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::setSortPath (UIDRef cid, StringRef path)
{
	getPlugInfo (cid, true)->setSortPath (makeLegalFolderPath (path));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SortFolderList* PluginPresentation::getSortFolderList (CategoryRef category) const
{
	return sortFolders.getSortFolderList (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API PluginPresentation::getSortFolders (CategoryRef category) const
{
	return getSortFolderList (category)->newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PluginPresentation::hasSortFolder (CategoryRef category, StringRef path) const
{
	return hasFolderInternal (SortFolderTraits (*this), category, path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PluginPresentation::getParentSortFolder (StringRef path)
{
	return SortFolderList::getParentFolder (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::addSortFolder (CategoryRef category, StringRef path)
{
	getSortFolderList (category)->addOnce (makeLegalFolderPath (path));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::removeSortFolder (CategoryRef category, StringRef path)
{
	removeFolderInternal (SortFolderTraits (*this), category, path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::moveSortFolder (CategoryRef category, StringRef oldPath, StringRef newPath)
{
	moveFolderInternal (SortFolderTraits (*this), category, oldPath, newPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::renameSortFolder (CategoryRef category, StringRef path, StringRef newName)
{
	renameFolderInternal (SortFolderTraits (*this), category, path, newName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SortFolderList* PluginPresentation::getFavoriteFolderList (CategoryRef category) const
{
	return favoriteFolders.getSortFolderList (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API PluginPresentation::getFavoriteFolders (CategoryRef category) const
{
	return getFavoriteFolderList (category)->newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PluginPresentation::hasFavoriteFolder (CategoryRef category, StringRef path) const
{
	return hasFolderInternal (FavoriteFolderTraits (*this), category, path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::addFavoriteFolder (CategoryRef category, StringRef path)
{
	getFavoriteFolderList (category)->addOnce (makeLegalFolderPath (path));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::removeFavoriteFolder (CategoryRef category, StringRef path)
{
	removeFolderInternal (FavoriteFolderTraits (*this), category, path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::moveFavoriteFolder (CategoryRef category, StringRef oldPath, StringRef newPath)
{
	moveFolderInternal (FavoriteFolderTraits (*this), category, oldPath, newPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PluginPresentation::renameFavoriteFolder (CategoryRef category, StringRef path, StringRef newName)
{
	renameFolderInternal (FavoriteFolderTraits (*this), category, path, newName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PluginPresentation::removeFolderInternal (const FolderTraits& folderTraits, CategoryRef category, StringRef path)
{
	String subFolderPrefix (path);
	subFolderPrefix << Url::strPathChar;

	// move contained plugins to parent folder (or root level)
	String newSortPath (SortFolderList::getParentFolder (path));

	ForEachPlugInClass (category, description)
		UIDRef cid (description.getClassID ());
		String plugPath (folderTraits.getFolder (cid));
		if(plugPath == path || plugPath.startsWith (subFolderPrefix))
			folderTraits.setFolder (cid, newSortPath);
	EndFor

	// remove the sort folder and it's subFolders
	if(SortFolderList* folderList = folderTraits.getFolderList (category))
		folderList->removeFolder (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PluginPresentation::moveFolderInternal (const FolderTraits& folderTraits, CategoryRef category, StringRef oldPath, StringRef _newPath)
{
	String newPath (makeLegalFolderPath (_newPath));

	String subFolderPrefix (oldPath);
	subFolderPrefix << Url::strPathChar;

	// update sort folder of affected plugins
	ForEachPlugInClass (category, description)
		String plugPath (folderTraits.getFolder (description.getClassID ()));
		if(plugPath == oldPath || plugPath.startsWith (subFolderPrefix))
		{
			String newPlugPath (newPath);
			newPlugPath << plugPath.subString (oldPath.length ());

			folderTraits.setFolder (description.getClassID (), makeLegalFolderPath (newPlugPath));
		}
	EndFor

	// update sort folder and all subFolders
	if(SortFolderList* folderList = folderTraits.getFolderList (category))
		folderList->moveSortFolder (oldPath, newPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PluginPresentation::renameFolderInternal (const FolderTraits& folderTraits, CategoryRef category, StringRef path, StringRef newName)
{
	String newPath (SortFolderList::getParentFolder (path));
	if(!newPath.isEmpty ())
		newPath	<< Url::strPathChar;
	newPath << newName;

	moveFolderInternal (folderTraits, category, path, makeLegalFolderPath (newPath));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PluginPresentation::hasFolderInternal (const FolderTraits& folderTraits, CategoryRef category, StringRef path) const
{
	if(folderTraits.getFolderList (category)->contains (path))
		return true;

	ForEachPlugInClass (category, description)
		if(folderTraits.getFolder (description.getClassID ()) == path)
			return true;
	EndFor

	return false;
}

//************************************************************************************************
// PlugInSettingsHelper
//************************************************************************************************

void PlugInSettingsHelper::getRemoveMarkerFile (Url& path)
{
	System::GetSystem ().getLocation (path, System::kAppSettingsFolder);
	path.descend ("remove-plugin-settings-marker");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInSettingsHelper::isRemoveMarkerPresent ()
{
	Url path;
	getRemoveMarkerFile (path);
	return File (path).exists ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInSettingsHelper::makeRemoveMarker (bool state)
{
	Url path;
	getRemoveMarkerFile (path);
	if(state)
	{
		File (path).create ();
	}
	else
	{
		if(File (path).exists ())
			File (path).remove ();
	}
}
