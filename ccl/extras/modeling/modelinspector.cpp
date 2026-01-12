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
// Filename    : ccl/extras/modeling/modelinspector.cpp
// Description : Class Model Inspector
//
//************************************************************************************************

#include "ccl/extras/modeling/modelinspector.h"
#include "ccl/extras/modeling/classrepository.h"

#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/app/signals.h"
#include "ccl/app/controls/listviewmodel.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/popupselectorclient.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// PropertyListModel
//************************************************************************************************

class PropertyListModel: public ListViewModel
{
public:
	DECLARE_CLASS_ABSTRACT (PropertyListModel, ListViewModel)

	PropertyListModel (ElementInspector& inspector);

	DECLARE_STRINGID_MEMBER (kElementSelected)

	enum Columns
	{
		kState,
		kTitle,
		kType,
		kDocumentation
	};

	class PropertyItem: public ListViewItem
	{
	public:
		PropertyItem (Model::Element* element);

		PROPERTY_BOOL (inherited, Inherited)
		PROPERTY_SHARED_AUTO (Model::Element, element, Element)
		PROPERTY_SHARED_AUTO (Model::Enumeration, enumeration, Enumeration)
	};

	void rebuild (Model::Element* element);

	// ListViewModel
	tbool CCL_API createColumnHeaders (IColumnHeaderList& list) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API getItemTooltip (String& tooltip, ItemIndexRef index, int column) override;
	tbool CCL_API onItemFocused (ItemIndexRef index) override;

protected:
	ElementInspector& inspector;

	PropertyItem* addElement (Model::Element* element);

	PropertyItem* resolve (ItemIndexRef index) const override;
	String toDocumentation (PropertyItem& item, bool& inherited) const;
};

//************************************************************************************************
// LinkListModel
//************************************************************************************************

class LinkListModel: public ListViewModel
{
public:
	typedef ListViewModel SuperClass;

	LinkListModel (ElementDocumenter& elementDocumenter);

	enum Columns
	{
		kState,
		kTitle
	};

	void rebuild (const Model::Documentation& documentation);

	// ListViewModel
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override;

protected:
	ElementDocumenter& elementDocumenter;	
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

// XSTRINGS_OFF     hint for xstring tool to skip this section

BEGIN_XSTRINGS ("Modeller")
	XSTRING (Title, "Name")
	XSTRING (Type, "Type")
	XSTRING (Url, "Url")
	XSTRING (Documentation, "Documentation")
	XSTRING (NothingSelected, "No element selected")
	XSTRING (ReturnVerb, "returns")
	XSTRING (ArgumentN, "Arg[%(1)]")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum ElementDocumenterTags
	{
		kTargetName = 100,
		kBriefDescription,
		kDetailedDescription,
		kRemarks,
		kAddLink,
		kRemoveLink
	};

	enum ElementInspectorTags
	{
		kElementName = 100,
		kElementIcon,
		kGroupName,
		kSuperClassName,
		kGotoSuperClass,
		kIsAbstract,
		kIsScriptable
	};
}

//************************************************************************************************
// ElementInspector
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ElementInspector, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ElementInspector::ElementInspector ()
: Component (String ("Inspector")),
  propertyList (nullptr),
  documenter (nullptr),
  inspectedElement (nullptr),
  browser (nullptr)
{
	propertyList = NEW PropertyListModel (*this);
	propertyList->addObserver (this); // TODO: remove dependency, make explicit!

	paramList.addString ("elementName", Tag::kElementName);
	paramList.addImage ("elementIcon", Tag::kElementIcon);
	paramList.addString ("groupName", Tag::kGroupName);
	paramList.addString ("superClassName", Tag::kSuperClassName);	
	paramList.addParam ("gotoSuperClass", Tag::kGotoSuperClass);
	paramList.addParam ("isAbstract", Tag::kIsAbstract)->enable (false);
	paramList.addParam ("isScriptable", Tag::kIsScriptable)->enable (false);

	addComponent (documenter = NEW ElementDocumenter);
	documenter->addObserver (this);

	setInspectedElement (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ElementInspector::~ElementInspector ()
{
	documenter->removeObserver (this);

	propertyList->removeObserver (this);
	propertyList->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ElementInspector::setEnabled (bool state)
{
	documenter->setEnabled (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ElementInspector::getObject (StringID name, UIDRef classID)
{
	if(name == "propertyList")
		return ccl_as_unknown (propertyList);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ElementInspector::setInspectedElement (Model::Element* element, IImage* icon)
{
	inspectedElement = element;

	propertyList->rebuild (element);

	IParameter* nameParam = paramList.byTag (Tag::kElementName);
	if(element)
		nameParam->fromString (element->getEnclosedTitle ());
	else
		nameParam->fromString (XSTR (NothingSelected));

	String groupName, superClassName;
	bool isAbstract = false, isScriptable = false;
	if(Model::Class* theClass = ccl_cast<Model::Class> (element))
	{
		groupName = theClass->getGroupName ();
		superClassName << theClass->getParentName ();
		isAbstract = theClass->isAbstract ();
		isScriptable = theClass->isScriptable ();
	}

	UnknownPtr<IImageProvider> (paramList.byTag (Tag::kElementIcon))->setImage (icon);
	paramList.byTag (Tag::kGroupName)->fromString (groupName);
	paramList.byTag (Tag::kSuperClassName)->fromString (superClassName);	
	paramList.byTag (Tag::kIsAbstract)->setValue (isAbstract);
	paramList.byTag (Tag::kIsScriptable)->setValue (isScriptable);

	documenter->setTargetElement (element);

	signal (Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ElementInspector::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "isClass")
	{
		var = ccl_cast<Model::Class> (inspectedElement) != nullptr;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ElementInspector::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kGotoSuperClass)
	{
		if(Model::Class* c = ccl_cast<Model::Class> (inspectedElement))
			if(!c->getParentName ().isEmpty () && c->getRepository ())
			{
				ASSERT (browser != nullptr)
				browser->notify (this, Message ("RevealClass", String (c->getParentName ()), String (c->getRepository ()->getName ())));
			}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ElementInspector::notify (ISubject* subject, MessageRef msg)
{
	if(msg == PropertyListModel::kElementSelected)
	{
		Model::Element* element = unknown_cast<Model::Element> (msg[0]);
		documenter->setTargetElement (element);
	}
	else if(msg == ElementDocumenter::kElementDirty)
	{
		ASSERT (inspectedElement)
		if(inspectedElement)
			inspectedElement->signal (Message (kChanged));

		if(propertyList->getItemView ())
			ViewBox (propertyList->getItemView ()).invalidate ();

		// TODO: other type of dirty notification!
		SignalSource (Signals::kDocumentManager).signal (Message (Signals::kDocumentDirty));
	}
	else
		SuperClass::notify (subject, msg);
}

//************************************************************************************************
// ElementDocumenter
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ElementDocumenter, Component)
DEFINE_STRINGID_MEMBER_ (ElementDocumenter, kElementDirty, "elementDirty")

//////////////////////////////////////////////////////////////////////////////////////////////////

ElementDocumenter::ElementDocumenter ()
: Component (String ("Documenter")),
  targetElement (nullptr),
  inputEnabled (true)
{
	linkList = NEW LinkListModel (*this);
	addObject ("linkList", linkList);

	paramList.addString ("targetName", Tag::kTargetName);
	paramList.addString ("briefDescription", Tag::kBriefDescription);
	paramList.addString ("detailedDescription", Tag::kDetailedDescription);
	paramList.addString ("remarks", Tag::kRemarks);
	paramList.addParam ("addLink", Tag::kAddLink);
	paramList.addParam ("removeLink", Tag::kRemoveLink);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ElementDocumenter::~ElementDocumenter ()
{
	linkList->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef ElementDocumenter::getLink (int index) const
{
	return targetElement ? targetElement->getDocumentation ().getLinks ().at (index) : String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ElementDocumenter::setLink (int index, StringRef link)
{
	if(targetElement && targetElement->getDocumentation ().setLink (index, link))
	{
		rebuildLinks ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ElementInspector* ElementDocumenter::getInspector () const
{
	return getParentNode<ElementInspector> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ElementDocumenter::rebuildLinks ()
{
	if(targetElement)
		linkList->rebuild (targetElement->getDocumentation ());

	signal (Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ElementDocumenter::setEnabled (bool state)
{
	inputEnabled = state;
	paramList.byTag (Tag::kBriefDescription)->enable (state);
	paramList.byTag (Tag::kDetailedDescription)->enable (state);
	paramList.byTag (Tag::kRemarks)->enable (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ElementDocumenter::setTargetElement (Model::Element* element)
{
	targetElement = element;
	bool enabled = targetElement != nullptr && inputEnabled == true;

	String text[3];
	const int tags[3] = {Tag::kBriefDescription, Tag::kDetailedDescription, Tag::kRemarks};

	String targetName;
	if(targetElement)
	{
		targetName = targetElement->getEnclosedTitle ();

		const Model::Documentation& documentation = targetElement->getDocumentation ();
		text[0] = documentation.getBriefDescription ();
		text[1] = documentation.getDetailedDescription ();
		text[2] = documentation.getRemarks ();
	}

	IParameter* targetParam = paramList.byTag (Tag::kTargetName);
	targetParam->fromString (targetName);

	for(int i = 0; i < ARRAY_COUNT (text); i++)
	{
		IParameter* p = paramList.byTag (tags[i]);
		p->enable (enabled);
		p->fromString (text[i]);
	}

	rebuildLinks ();
	signal (Message (kPropertyChanged)); // hasDetails
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ElementDocumenter::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "numLinks")
	{
		var = targetElement ? targetElement->getDocumentation ().getLinks ().count () : 0;
		return true;
	}
	else if(propertyId == "hasLinks")
	{
		var = targetElement && !targetElement->getDocumentation ().getLinks ().isEmpty ();
		return true;
	}
	else if(propertyId == "hasDetails")
	{
		var = targetElement && !targetElement->getDocumentation ().getDetailedDescription ().isEmpty ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API ElementDocumenter::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name.startsWith ("@link"))
	{
		int index = 0;
		sscanf (name, "@link[%d]", &index);
		if(targetElement)
		{
			String link (targetElement->getDocumentation ().getLinks ().at (index));
			if(link.isEmpty () == false)
			{
				ControlBox linkView (ClassID::LinkView, nullptr, bounds, StyleFlags (0, Styles::kLinkViewAppearanceFitTitle), link);
				linkView.setParameter (nullptr);
				linkView.autoSize ();
				return linkView;
			}
		}
	}
	return SuperClass::createView (name, data, bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ElementDocumenter::signalDirty ()
{
	ASSERT (targetElement)
	signal (Message (kElementDirty, ccl_as_unknown (targetElement)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ElementDocumenter::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kBriefDescription :
	case Tag::kDetailedDescription :
	case Tag::kRemarks :
		if(targetElement)
		{
			String text;
			param->toString (text);
			
			Model::Documentation& documentation = targetElement->getDocumentation ();
			switch(param->getTag ())
			{
			case Tag::kBriefDescription : documentation.setBriefDescription (text); break;
			case Tag::kDetailedDescription : documentation.setDetailedDescription (text); break;
			case Tag::kRemarks : documentation.setRemarks (text); break;
			}

			signalDirty ();
		}
		break;

	case Tag::kAddLink:
		if(targetElement)
		{
			targetElement->getDocumentation ().addLink (String (targetElement->getName ()));
			linkList->rebuild (targetElement->getDocumentation ());
			signalDirty ();
			signal (Message (kPropertyChanged));
		}
		break;

	case Tag::kRemoveLink:
		if(targetElement)
		{
			int index = linkList->getFirstSelectedIndex ();
			if(index >= 0 && targetElement->getDocumentation ().removeLink (index))
			{
				linkList->rebuild (targetElement->getDocumentation ());
				signalDirty ();
				signal (Message (kPropertyChanged));
			}
		}
	}
	return true;
}

//************************************************************************************************
// PropertyListModel::PropertyItem
//************************************************************************************************

PropertyListModel::PropertyItem::PropertyItem (Model::Element* element)
: inherited (false)
{
	ASSERT (element != nullptr)
	setElement (element);
	setTitle (String (element->getName ()));
}

//************************************************************************************************
// PropertyListModel
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PropertyListModel, ListViewModel)
DEFINE_STRINGID_MEMBER_ (PropertyListModel, kElementSelected, "elementSelected")

//////////////////////////////////////////////////////////////////////////////////////////////////

PropertyListModel::PropertyListModel (ElementInspector& inspector)
: inspector (inspector)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PropertyListModel::PropertyItem* PropertyListModel::resolve (ItemIndexRef index) const
{
	return (PropertyItem*)SuperClass::resolve (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PropertyListModel::toDocumentation (PropertyItem& item, bool& inherited) const
{
	const Model::Documentation& documentation = item.getElement ()->getDocumentation ();
	String text (documentation.getBriefDescription ());
	
	// search for member documentation upwards via class inheritance
	if(text.isEmpty () && item.isInherited ())
		if(Model::Member* member = ccl_cast<Model::Member> (item.getElement ()))
		{
			Model::ClassQualifier q (*member);
			q.next (); // start at superclass
			while(const Model::Class* c = q.next ())
				if(Model::Member* baseMember = c->findMember (member->getName ()))
					if(!baseMember->getDocumentation ().getBriefDescription ().isEmpty ())
					{
						text = baseMember->getDocumentation ().getBriefDescription ();
						inherited = true;
						break;
					}
		}

	if(text.isEmpty () && item.getEnumeration ())
	{
		text = item.getEnumeration ()->asString ();
		inherited = true;
	}
	return text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PropertyListModel::rebuild (Model::Element* element)
{
	items.removeAll ();

	if(Model::Class* theClass = ccl_cast<Model::Class> (element))
	{
		addElement (theClass);

		ObjectArray members;
		theClass->getMembers (members, true);
		ForEach (members, Model::Member, member)
			PropertyItem* item = addElement (member);

			const Model::Element* memberType = Model::ClassQualifier::findTypeForMember (*member);
			item->setEnumeration (ccl_cast<Model::Enumeration> (ccl_const_cast (memberType)));

			item->setInherited (Model::ClassQualifier::isInheritedMember (*member));
		EndFor

		/*ForEach (theClass->getMethods (), Model::Method, method)
			addElement (method);
		EndFor*/
	}
	else if(Model::Method* method = ccl_cast<Model::Method> (element))
	{
		addElement (method);
		
		PropertyItem* item = addElement (&method->getReturnValue ());
		item->setTitle (String () << XSTR (ReturnVerb) << ": " << method->getReturnValue ().getTypeDescription ());

		int i = 0;
		ForEach (method->getArguments (), Model::Variable, arg)
			item = addElement (arg);
			String argString;
			argString.appendFormat (XSTR (ArgumentN), i++);
			item->setTitle (String () << argString << ": " << item->getTitle ());
		EndFor
	}
	else if(Model::Enumeration* theEnum = ccl_cast<Model::Enumeration> (element))
	{
		addElement (theEnum);

		ObjectArray enumerators;
		theEnum->getEnumerators (enumerators, true);
		ForEach (enumerators, Model::Enumerator, e)
			addElement (e);
		EndFor
	}
	else if(Model::ObjectElement* object = ccl_cast<Model::ObjectElement> (element))
	{
		ForEach (object->getProperties (), Model::Property, p)
			addElement (p);
		EndFor
	}

	updateColumns ();

	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PropertyListModel::PropertyItem* PropertyListModel::addElement (Model::Element* element)
{
	PropertyItem* item = NEW PropertyItem (element);
	items.add (item);
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PropertyListModel::createColumnHeaders (IColumnHeaderList& list)
{
	int typeHidden = ccl_cast<Model::Enumeration> (inspector.getInspectedElement ()) ? IColumnHeaderList::kHidden : 0;

	list.addColumn (20);	// kState
	list.addColumn (150, XSTR (Title), nullptr, 0, IColumnHeaderList::kSizable);			// kTitle
	list.addColumn (50, XSTR (Type), nullptr, 0, IColumnHeaderList::kSizable|typeHidden);	// kType
	list.addColumn (200, XSTR (Documentation), nullptr, 0, IColumnHeaderList::kSizable);	// kDocumentation
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PropertyListModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	PropertyItem* item = resolve (index);
	if(!item)
		return false;

	switch(column)
	{
	case kState :
		{
			bool hasDocumentation = item->getElement ()->hasDocumentation ();
			if(hasDocumentation)
				info.graphics.fillRect (Rect (info.rect).contract (2), SolidBrush (Colors::kGreen));

			if(item->getEnumeration ())
			{
				const uchar linkArrow[2] = {8599, 0};
				info.graphics.drawString (info.rect, String (linkArrow), info.style.font, info.style.textBrush);
			}
		}
		break;

	case kTitle :
		{
			int fontStyle = 0;
			bool drawDisabled = false;

			bool isClass = item->getElement ()->isClass (ccl_typeid<Model::Class> ());
			bool isEnum = item->getElement ()->isClass (ccl_typeid<Model::Enumeration> ());
			bool isMethod = item->getElement ()->isClass (ccl_typeid<Model::Method> ());
			if(isClass || isEnum || isMethod)
				fontStyle = Font::kBold;
			
			if(item->isInherited ())
			{
				fontStyle = Font::kItalic;
				drawDisabled = true;
			}

			if(item->getEnumeration ())
				fontStyle |= Font::kUnderline;

			drawTitle (info, item->getTitle (), !drawDisabled, fontStyle);
		}
		break;

	case kType:
		{
			String type;
			Model::Variable* var = ccl_cast<Model::Variable> (item->getElement ());
			if(var)
				type = String (var->getTypeName ());
			
			if(type.isEmpty () == false)
			{
				int fontStyle = 0;
				drawTitle (info, type, !item->isInherited (), fontStyle);
			}
		}
		break;

	case kDocumentation :
		{
			bool inherited = false;
			String text (toDocumentation (*item, inherited));
			drawTitle (info, text, !inherited);
		}
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PropertyListModel::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	PropertyItem* item = resolve (index);
	if(!item)
		return false;

	switch(column)
	{
	case kState:
		if(Model::Enumeration* enumeration = item->getEnumeration ())
		{
			ASSERT (inspector.getBrowser () != nullptr)
			if(enumeration->getRepository ())
				inspector.getBrowser ()->notify (&inspector, Message ("RevealEnum", String (enumeration->getName ()), String (enumeration->getRepository ()->getName ())));
			return true;
		}
		break;

	case kType:
		break;

	case kTitle:
		if(Model::Enumeration* enumeration = item->getEnumeration ())
		{
			AutoPtr<ElementInspector> popupInspector = NEW ElementInspector;
			popupInspector->setInspectedElement (enumeration);

			AutoPtr<IPopupSelector> popupSelector (ccl_new<IPopupSelector> (ClassID::PopupSelector));
			ASSERT (popupSelector != nullptr)

			ViewBox itemView (info.view);
			ITheme& theme = itemView.getTheme ();
			popupSelector->setTheme (&theme);

			IView* view = theme.createView ("EnumerationPopup", popupInspector->asUnknown ());
			if(view)
			{
				PopupSizeInfo sizeInfo (info.rect.getLeftBottom (), itemView);
				sizeInfo.canFlipParentEdge (true);

				Rect size (view->getSize ());
				size.setWidth (itemView.getWidth ());
				sizeInfo.sizeLimits.makeValid (size);
				view->setSize (size);

				AutoPtr<SimplePopupSelectorClient> client (NEW SimplePopupSelectorClient);
				if(IWindow* w = itemView.getWindow ())
					client->setSourceController (UnknownPtr<IObjectNode> (w->getController ())); // to help identifying a popup from CCL Spy...
				popupSelector->popup (view, client, sizeInfo);
			}
			return true;
		}
		break;
	}
	return SuperClass::editCell (index, column, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PropertyListModel::getItemTooltip (String& tooltip, ItemIndexRef index, int column)
{
	PropertyItem* item = resolve (index);
	if(!item)
		return false;

	tooltip.empty ();

	switch(column)
	{
	case kTitle :
		if(item->getEnumeration ())
			tooltip = item->getEnumeration ()->getEnclosedTitle ();
		break;

	case kDocumentation :
		{
			bool inherited = false;
			tooltip = toDocumentation (*item, inherited);
		}
		break;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PropertyListModel::onItemFocused (ItemIndexRef index)
{
	Model::Element* element = nullptr;
	if(PropertyItem* item = resolve (index))
		element = item->getElement ();

	signal (Message (kElementSelected, static_cast<IObject*> (element)));
	return true;
}

//************************************************************************************************
// LinkListModel
//************************************************************************************************

LinkListModel::LinkListModel (ElementDocumenter& elementDocumenter)
: elementDocumenter (elementDocumenter)
{
	getColumns ().addColumn (20);	// kState
	getColumns ().addColumn (120, XSTR (Title), kTitleID, 0, IColumnHeaderList::kSizable);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinkListModel::rebuild (const Model::Documentation& documentation)
{
	items.removeAll ();

	VectorForEach (documentation.getLinks (), String, link)
		items.add (NEW ListViewItem (link));
	EndFor

	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinkListModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	ListViewItem* item = resolve (index);
	if(!item)
		return false;

	if(column == kState)
	{
		const uchar linkArrow[2] = {8599, 0};
		info.graphics.drawString (info.rect, String (linkArrow), info.style.font, info.style.textBrush);
		
		return true;
	}
	return SuperClass::drawCell (index, column, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinkListModel::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	SharedPtr<LinkListModel> durableThis (this);
	ListViewItem* item = resolve (index);
	if(!item)
		return false;

	if(column == kState)
	{
		ElementInspector* inspector = elementDocumenter.getInspector ();
		if(inspector && inspector->getBrowser ())
			inspector->getBrowser ()->notify (inspector, Message ("RevealClass", item->getTitle (), String ()));
		
		return true;
	}
	return false;
}
