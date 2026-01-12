//************************************************************************************************
//
// CCL Spy
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
// Filename    : objecttablebrowser.cpp
// Description : Object table browser
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "objecttablebrowser.h"
#include "objectinfo.h"

#include "ccl/app/component.h"
#include "ccl/public/app/idocument.h"

#include "ccl/base/message.h"
#include "ccl/base/trigger.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/extras/modeling/classrepository.h"

#include "ccl/public/base/itypelib.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/app/irootcomponent.h"
#include "ccl/public/app/documentlistener.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/collections/variantvector.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/plugins/itypelibregistry.h"
#include "ccl/public/plugservices.h"

namespace Spy {

//************************************************************************************************
// ParamListExtractor
//************************************************************************************************

class ParamListExtractor: public CCL::Model::ClassRepositoryBuilder::IExtractor
{
public:
	ParamListExtractor (bool publicOnly = false)
	: publicOnly (publicOnly) 
	{}

	PROPERTY_BOOL (publicOnly, PublicOnly)

	// IExtractor
	void extract (CCL::Model::ObjectElement& element, CCL::IObjectNode& object) const override;
};

//************************************************************************************************
// ExtractModelSettings
//************************************************************************************************

class ExtractModelSettings
{
public:
	ExtractModelSettings (CCL::StringRef nameString);

	bool runDialog ();

	CCL::String getName () const { CCL::String string; name->toString (string); return string; }
	bool isPublicOnly () const { return publicOnly->getValue ().asBool (); }
	bool isScriptableOnly () const { return scriptableOnly->getValue ().asBool (); }
	bool isDeep () const { return deep->getValue ().asBool (); }

protected:
	CCL::ParamContainer params;
	CCL::IParameter* name;
	CCL::IParameter* publicOnly;
	CCL::IParameter* scriptableOnly;
	CCL::IParameter* deep;
};

} // namespace Spy

using namespace CCL;
using namespace Spy;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Spy")
	XSTRING (Refresh, "Refresh")
	XSTRING (ExtractModel, "Extract Model")
	XSTRING (ObjectNotFound, "Object not found!")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

static IObject* getPropertyByPath (StringID _propertyPath)
{
	MutableCString propertyPath (_propertyPath);
	UnknownPtr<IObject> anchor (&System::GetScriptingManager ().getHost ());

	IObject* object = anchor;
	if(!propertyPath.isEmpty ())
	{
		propertyPath += ".unused"; // we want the property holder
		object = Property (anchor, propertyPath).getHolder ();
	}
	return object;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static ITypeLibrary* getTypeLibByName (StringRef _name)
{
	MutableCString name (_name);
	return System::GetTypeLibRegistry ().findTypeLib (name);
}

//************************************************************************************************
// ParamListExtractor
//************************************************************************************************

void ParamListExtractor::extract (Model::ObjectElement& element, IObjectNode& object) const
{
	UnknownPtr<IController> controller = &object;
	if(!controller)
		return;

	for(int i = 0, count = controller->countParameters (); i < count; i++)
		if(IParameter* p = controller->getParameterAt (i))
		{
			if(publicOnly && !p->isPublic ())
				continue;

			CCL::DataType type = ITypeInfo::kVoid;
			switch(p->getType ())
			{
			case IParameter::kToggle : type = ITypeInfo::kBool; break;
			case IParameter::kInteger : type = ITypeInfo::kInt; break;
			case IParameter::kFloat : type = ITypeInfo::kFloat; break;
			default : type = ITypeInfo::kString;
			}
			element.addProperty (NEW Model::Property (p->getName (), type)); // TODO: data type???
		}
}

//************************************************************************************************
// ExtractModelSettings
//************************************************************************************************

ExtractModelSettings::ExtractModelSettings (StringRef nameString)
{
	name = params.addString ("name");
	name->fromString (nameString);
	publicOnly = params.addParam ("publicOnly");
	scriptableOnly = params.addParam ("scriptableOnly");
	deep = params.addParam ("deep");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtractModelSettings::runDialog ()
{
	IView* view = RootComponent::instance ().getTheme ()->createView ("ExtractModelSettings", ccl_as_unknown (params));
	return view ? DialogBox ()->runDialog (view) == DialogResult::kOkay : false;
}

//************************************************************************************************
// ObjectItem
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ObjectItem, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectItem::ObjectItem (ObjectType type, StringRef title)
: type (type),
  subType (kNone),
  title (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectItem::assign (IUnknown* obj)
{
	String title;
	UnknownPtr<IRootComponent> rootComponent (obj);
	if(rootComponent)
	{
		IRootComponent::Description description;
		rootComponent->getDescription (description);
		title = description.appID;
		//title << " - " << description.appTitle << "";
	
		setSubType (kModule);
	}
	else
	{
		UnknownPtr<IObjectNode> iNode (obj);
		if(iNode)
			title = iNode->getObjectID ();

		//UnknownPtr<IObject> iObject (obj);
		//if(iObject)
		//	title << " [" << iObject->getTypeInfo ().getClassName () << "]";
	}

	setTitle (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectItem::isStructuralItem () const
{
	return !(type == kGeneric || type == kProperty || type == kTypeLib);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ObjectItem::getAliveObject () const
{
	IUnknown* object = nullptr;
	if(type == kGeneric)
		object = System::GetObjectTable ().getObjectByUrl (Url (address));
	else if(type == kProperty)
		object = getPropertyByPath (MutableCString (address));
	else if(type == kTypeLib)
		object = getTypeLibByName (address);
	return object;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectItem::getProperties (PropertyList& propertyList)
{
	IUnknown* object = getAliveObject ();
	if(object == nullptr)
	{
		if(!isStructuralItem ())
			propertyList.setProperty ("Class", XSTR (ObjectNotFound));
		return;
	}

	UnknownPtr<IObject> iObject (object);
	if(iObject)
		propertyList.setProperty ("Class", String (CString (iObject->getTypeInfo ().getClassName ())));

	if(!address.isEmpty ())
	{
		String string (address);
		if(type == kGeneric)
			string.prepend (CCLSTR ("object"));
		else if(type == kProperty)
			string.prepend (CCLSTR ("Host."));

		propertyList.setProperty ("Address", string);
	}

	if(subType == kModule)
	{
		IRootComponent::Description description;
		UnknownPtr<IRootComponent> rootComponent (object);
		if(rootComponent)
			rootComponent->getDescription (description);

		propertyList.setProperty ("Title", description.appTitle);
		if(!description.appVersion.isEmpty ())
			propertyList.setProperty ("Version", description.appVersion);
		propertyList.setProperty ("Vendor", description.appVendor);
	}

	// scriptable methods + properties
	if(iObject)
	{
		Model::PropertyCollection props;
		iObject->getPropertyNames (props);

		for(int i = 0; i < props.count (); i++)
		{
			if(Model::Property* prop = props.getProperty (i))
			{
				StringID name (prop->getName ());

				Variant value;
				iObject->getProperty (value, name);
				if(value.isObject ())
					continue;

				propertyList.setProperty (name, VariantString (value));
			}
		}

		for(const ITypeInfo* typeInfo = &iObject->getTypeInfo (); typeInfo; typeInfo = typeInfo->getParentType ())
			if(const ITypeInfo::MethodDefinition* methodNames = typeInfo->getMethodNames ())
				for(int i = 0; methodNames[i].name != nullptr; i++)
				{
					MutableCString name = "@"; // should be drawn bold
					name += methodNames[i].name;

					String value = "function (";

					auto addType = [&value] (const Model::Variable& arg)
					{
						if(arg.getType () != ITypeInfo::kVoid)
							value << ": " << arg.getTypeDescription ();
					};

					Model::Method method (methodNames[i].name);
					method.assign (methodNames[i]);
					{
						bool isFirst = true;
						for(auto argument : iterate_as<Model::MethodArgument> (method.getArguments ()))
						{
							if(isFirst)
								isFirst = false;
							else
								value << ", ";

							value << argument->getName ();
							addType (*argument);

							if(!argument->getDefaultValue ().isEmpty ())
								value << " = " << argument->getDefaultValue ();
						}
					}
					value << ")";
					addType (method.getReturnValue ());
						
					propertyList.setProperty (name, value);
				}
	}

	// controller parameters
	UnknownPtr<IController> controller (object);
	if(controller)
	{
		for(int i = 0, count = controller->countParameters (); i < count; i++)
			if(IParameter* p = controller->getParameterAt (i))
			{
				String value;
				p->toString (value);
				propertyList.setProperty (p->getName (), value);
			}
	}
}

//************************************************************************************************
// ObjectTableItemModel
//************************************************************************************************

ObjectTableItemModel::ObjectTableItemModel (ObjectTableBrowser& browser)
: objectTable (System::GetObjectTable ()),
  browser (browser),
  itemView (nullptr),
  rootItem (*NEW ObjectItem (ObjectItem::kTreeRoot, String ("Objects"))),
  documentManager (DocumentListenerFactory::getDocumentManager ())
{
	ISubject::addObserver (&objectTable, this);

	if(documentManager)
		ISubject::addObserver (documentManager, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectTableItemModel::~ObjectTableItemModel ()
{
	ISubject::removeObserver (&objectTable, this);

	if(documentManager)
		ISubject::removeObserver (documentManager, this);

	rootItem.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ObjectTableItemModel::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged && isEqualUnknown (subject, &objectTable))
	{
		//refreshAll ();
	}
	else if(msg == IDocumentManager::kActiveDocumentChanged)
	{
		//refreshAll ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectItem* ObjectTableItemModel::resolve (ItemIndexRef index) const
{
	return unknown_cast<ObjectItem> (index.getObject ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectTableItemModel::refreshAll ()
{
	// discard focused object
	browser.signal (Message ("ObjectFocused", 0));

	// discard the whole tree
	UnknownPtr<ITreeView> treeView (itemView);
	if(treeView)
		treeView->refreshItem (treeView->getRootItem ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectTableItemModel::getRootItem (ItemIndex& index)
{
	index = ItemIndex (rootItem.asUnknown ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectTableItemModel::getSubItems (IUnknownList& subItems, ItemIndexRef index) 
{ 
	ObjectItem* item = resolve (index);
	if(item == nullptr)
		return false;

	switch(item->getType ())
	{
	case ObjectItem::kTreeRoot :
		subItems.add (ccl_as_unknown (NEW ObjectItem (ObjectItem::kObjectTable, String ("Globals"))));
		subItems.add (ccl_as_unknown (NEW ObjectItem (ObjectItem::kDocumentManager, String ("ActiveDocument"))));
		subItems.add (ccl_as_unknown (NEW ObjectItem (ObjectItem::kProperty, String ("Host"))));
		subItems.add (ccl_as_unknown (NEW ObjectItem (ObjectItem::kTypeRegistry, String ("TypeLibs"))));
		break;

	case ObjectItem::kObjectTable :
		for(int i = 0, count = objectTable.countObjects (); i < count; i++)
			if(IUnknown* obj = objectTable.getObjectByIndex (i))
			{
				ObjectItem* subItem = NEW ObjectItem (ObjectItem::kGeneric);
				subItem->assign (obj);

				String objectName (objectTable.getObjectName (i));

				if(subItem->getTitle ().isEmpty ())
					subItem->setTitle (objectName);

				Url url;
				url.setHostName (objectName);
				subItem->setAddress (UrlFullString (url));
					
				subItems.add (subItem->asUnknown ());
			}
		break;

	case ObjectItem::kTypeRegistry :
		IterForEachUnknown (System::GetTypeLibRegistry ().newIterator (), unk)
			UnknownPtr<ITypeLibrary> typeLib (unk);
			if(typeLib)
			{
				ObjectItem* subItem = NEW ObjectItem (ObjectItem::kTypeLib);
				subItem->setTitle (typeLib->getLibraryName ());
				subItem->setAddress (typeLib->getLibraryName ()); //???
				subItems.add (subItem->asUnknown ());
			}
		EndFor
		break;

	case ObjectItem::kDocumentManager :
		if(documentManager)
			if(IDocument* doc = documentManager->getActiveIDocument ())
			{
				ObjectItem* subItem = NEW ObjectItem (ObjectItem::kGeneric);
				subItem->assign (doc->getController ());
				subItem->setAddress (UrlFullString (Url ("://hostapp/DocumentManager/ActiveDocument")));
				subItems.add (subItem->asUnknown ());
			}
		break;

	case ObjectItem::kGeneric :
		{
			Url parentUrl (item->getAddress ());
			UnknownPtr<IObjectNode> parent = objectTable.getObjectByUrl (parentUrl);
			if(parent)
			{
				// childs
				for(int i = 0, count = parent->countChildren (); i < count; i++)
					if(IObjectNode* child = parent->getChild (i))
					{
						String id (child->getObjectID ());
						ASSERT (!id.isEmpty ())
						if(id.isEmpty ())
							continue;

						Url childUrl (parentUrl);
						childUrl.descend (id);
					
						ObjectItem* subItem = NEW ObjectItem (ObjectItem::kGeneric);
						subItem->assign (child);
						subItem->setAddress (UrlFullString (childUrl));
						subItems.add (subItem->asUnknown ());
					}

				// delegates
				VariantStringVector delegates;
				if(parent->getChildDelegates (delegates))
					VectorForEach (delegates, String, id)
						if(IObjectNode* child = parent->findChild (id))
						{
							Url childUrl (parentUrl);
							childUrl.descend (id);

							ObjectItem* subItem = NEW ObjectItem (ObjectItem::kGeneric);
							subItem->setSubType (ObjectItem::kDelegate);
							//subItem->assign (child);
							subItem->setTitle (id);
							subItem->setAddress (UrlFullString (childUrl));
							subItems.add (subItem->asUnknown ());
						}
					EndFor
			}
		}
		break;

	case ObjectItem::kProperty :
		{
			UnknownPtr<IObject> parent = getPropertyByPath (MutableCString (item->getAddress ()));
			if(parent)
			{
				Model::PropertyCollection props;
				parent->getPropertyNames (props);

				for(int i = 0; i < props.count (); i++)
				{
					MutableCString name = props.at (i);

					Variant value;
					parent->getProperty (value, name);
					if(!value.isObject ())
						continue;

					String childAddres = item->getAddress ();
					if(!childAddres.isEmpty ())
						childAddres << ".";
					childAddres << name;

					ObjectItem* subItem = NEW ObjectItem (ObjectItem::kProperty, String (name));
					subItem->setAddress (childAddres);
					subItems.add (subItem->asUnknown ());
				}
			}
		}
		break;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectTableItemModel::canExpandItem (ItemIndexRef index) 
{
	ObjectItem* item = resolve (index);
	if(item == nullptr)
		return false;

	if(item->getType () == ObjectItem::kGeneric)
	{
		UnknownPtr<IObjectNode> parent = objectTable.getObjectByUrl (Url (item->getAddress ()));
		if(parent)
		{
			if(parent->countChildren () > 0)
				return true;

			VariantStringVector delegates;
			parent->getChildDelegates (delegates);
			return !delegates.isEmpty ();
		}
		return false;
	}
	else if(item->getType () == ObjectItem::kProperty)
	{
		IObject* parent = getPropertyByPath (MutableCString (item->getAddress ()));
		if(parent)
		{
			Model::PropertyCollection props;
			parent->getPropertyNames (props);

			for(int i = 0; i < props.count (); i++)
			{
				MutableCString name = props.at (i);

				Variant value;
				parent->getProperty (value, name);
				if(value.isObject ())
					return true;
			}
			return false;
		}
	}
	else if(item->getType () == ObjectItem::kTypeLib)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectTableItemModel::getItemTitle (String& title, CCL::ItemIndexRef index)
{
	ObjectItem* item = resolve (index);
	if(item == nullptr)
		return false;

	title = item->getTitle ();
	if(title.isEmpty ())
		title = CCLSTR ("(Unnamed)");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ObjectTableItemModel::getItemIcon (ItemIndexRef index)
{
	ObjectItem* item = resolve (index);
	if(item == nullptr)
		return nullptr;

	if(item->getIcon () == nullptr)
	{
		MutableCString iconName ("icon:");
		if(item->getType () == ObjectItem::kProperty)
			iconName += "Script";
		else if(item->getType () == ObjectItem::kTypeRegistry || item->getType () == ObjectItem::kTypeLib)
			iconName += "Type";
		else
		{
			if(item->getSubType () == ObjectItem::kDelegate)
				iconName += "Delegate";
			else if(item->getSubType () == ObjectItem::kModule)
				iconName += "Module";
			else
				iconName += "Object";
		}

		IImage* icon = RootComponent::instance ().getTheme ()->getImage (iconName);
		item->setIcon (icon);
	}

	return item->getIcon ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ObjectTableItemModel::viewAttached (IItemView* itemView)
{
	this->itemView = itemView;
	//UnknownPtr<ITreeView> treeView (itemView);
	//if(treeView)
	//	treeView->expandItem (treeView->getRootItem ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ObjectTableItemModel::viewDetached (IItemView* itemView)
{
	if(itemView == this->itemView)
		this->itemView = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectTableItemModel::onItemFocused (ItemIndexRef index)
{
	ObjectItem* item = resolve (index);
	browser.signal (Message ("ObjectFocused", item->asUnknown ()));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectTableItemModel::appendItemMenu (IContextMenu& menu, ItemIndexRef index, const IItemSelection& selection)
{
	ObjectItem* item = resolve (index);
	if(!item)
		return false;

	if(!item->isStructuralItem ())
	{
		menu.addCommandItem (CommandWithTitle ("Object", "Extract Model", XSTR (ExtractModel)),
							CommandDelegate<ObjectTableItemModel>::make (this, &ObjectTableItemModel::onItemCommand, item->asUnknown ()), true);
		menu.addSeparatorItem ();
	}

	menu.addCommandItem (XSTR (Refresh), "Object", "Refresh",
						CommandDelegate<ObjectTableItemModel>::make (this, &ObjectTableItemModel::onItemCommand, item->asUnknown ()));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectTableItemModel::onItemCommand (CmdArgs args, VariantRef data)
{
	ObjectItem* item = unknown_cast<ObjectItem> (data.asUnknown ());

	if(item && args.category == "Object")
	{
		if(args.name == "Refresh")
		{
			if(!args.checkOnly ())
			{
				UnknownPtr<ITreeView> treeView (itemView);
				if(treeView && item && treeView->getRootItem ())
					if(ITreeItem* treeItem = treeView->getRootItem ()->findItem (item->asUnknown ()))
					{
						treeView->refreshItem (treeItem);

						browser.signal (Message ("ObjectFocused", 0));
						browser.signal (Message ("ObjectFocused", item->asUnknown ()));
						return true;
					}

				refreshAll ();
			}
			return true;
		}
		else if(args.name == "Extract Model")
		{
			if(IUnknown* object = item->getAliveObject ())
			{
				if(!args.checkOnly ())
				{
					ExtractModelSettings settings (item->getTitle ());
					if(!settings.runDialog ())
						return true;

					String name (settings.getName ());

					AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);
					fs->addFilter (Model::ClassRepository::getFileType ());
					fs->setFileName (name);
					if(fs->run (IFileSelector::kSaveFile))
					{
						UrlRef path = *fs->getPath ();
						Model::ClassRepository repository;
						Model::ClassRepositoryBuilder builder (repository);

						if(item->getType () == ObjectItem::kTypeLib)
						{
							UnknownPtr<ITypeLibrary> typeLib = object;
							ASSERT (typeLib)
							if(typeLib)
							{
								Model::TypeInfoFilter filter (settings.isScriptableOnly ());
								builder.build (*typeLib, &filter);
							}
						}
						else if(item->getType () == ObjectItem::kGeneric)
						{
							UnknownPtr<IObjectNode> iNode = object;
							ASSERT (iNode)
							if(iNode)
								builder.build (MutableCString (name), *iNode, ParamListExtractor (settings.isPublicOnly ()), settings.isDeep ());
						}
						else if(item->getType () == ObjectItem::kProperty)
						{
							UnknownPtr<IObject> iObject = object;
							ASSERT (iObject)
							if(iObject)
								builder.build (MutableCString (name), *iObject, settings.isDeep ());
						}

						repository.setName (MutableCString (name));
						repository.saveToFile (path);
					}
				}
				return true;
			}
		}
	}
	return false;
}

//************************************************************************************************
// ObjectTableBrowser
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ObjectTableBrowser, ObjectNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectTableBrowser::ObjectTableBrowser ()
: ObjectNode (CCLSTR ("ObjectTableBrowser"))
{
	objectTableModel = NEW ObjectTableItemModel (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ObjectTableBrowser::getObject (StringID name, UIDRef classID)
{
	if(classID == ccl_iid<IItemModel> ())
	{
		if(name == "ObjectTable")
		{
			if(objectTableModel)
				return objectTableModel->asUnknown ();
		}
	}
	return nullptr;
}
