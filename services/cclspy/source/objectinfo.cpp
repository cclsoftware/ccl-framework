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
// Filename    : objectinfo.cpp
// Description : Object info & item model
//
//************************************************************************************************

#include "objectinfo.h"

#include "ccl/app/params.h"
#include "ccl/app/components/colorpicker.h"

#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/framework/iclipboard.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/gui/graphics/iuivalue.h"
#include "ccl/public/guiservices.h"

using namespace CCL;
using namespace Spy;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Spy")
	XSTRING (CopyName, "Copy Name")
	XSTRING (CopyValue, "Copy Value")
END_XSTRINGS

//************************************************************************************************
// PropertyList
//************************************************************************************************

PropertyList::Property* PropertyList::getProperty (StringID id) const
{
	ArrayForEachFast (properties, Property, p)
		if(p->getID () == id)
			return p;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PropertyList::setProperty (StringID id, VariantRef value, PropertyHandler* handler)
{
	Property* p = getProperty (id);
	if(!p)
	{
		p = NEW Property (id);
		properties.add (p);
	}

	p->set (value);
	p->setHandler (handler);
}

//************************************************************************************************
// PropertyList::Property
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PropertyList::Property, Attribute)

//************************************************************************************************
// ObjectInfo
//************************************************************************************************

ObjectInfo::ObjectInfo (IUnknown* object)
: object (object),
  subject (nullptr)
{
	// observe views, share other objects
	UnknownPtr<IView> view (object);
	if(view)
	{
		subject = UnknownPtr<ISubject> (view);
		if(subject)
			subject->addObserver (this);
	}

	if(!subject)
		object->retain ();

	groups.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectInfo::~ObjectInfo ()
{
	if(subject)
		subject->removeObserver (this);
	else if(object)
		object->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ObjectInfo::notify (ISubject* s, MessageRef msg)
{
	if(subject && subject == s && msg == kDestroyed)
	{
		subject->removeObserver (this);
		subject = nullptr;
		object = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PropertyList* ObjectInfo::getGroupAt (int index)
{
	return (PropertyList*)groups.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PropertyList* ObjectInfo::getGroup (StringID name, bool create)
{
	ForEach (groups, PropertyList, g)
		if(g->getName () == name)
			return g;
	EndFor

	if(create)
	{
		PropertyList* g = NEW PropertyList (name);
		groups.add (g);
		return g;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectInfo::addProperty (StringID path, VariantRef value, PropertyHandler* handler)
{
	MutableCString groupName;
	MutableCString key;
	int index = path.index ('/');
	if(index >= 0)
	{
		groupName = path;
		groupName.truncate (index);
		key = path.subString (index + 1);
	}
	else
		key = path;

	PropertyList* g = getGroup (groupName);
	g->setProperty (key, value, handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectInfo::addObjectProperty (IUnknown* object, MemberID propertyId, StringID path, PropertyHandler* handler)
{
	if(!object)
		object = this->object;

	UnknownPtr<IObject> iObject (object);
	if(iObject)
	{
		Variant var;
		if(iObject->getProperty (var, propertyId))
			addProperty (path.isEmpty () ? propertyId : path, var, handler);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectInfo::addObjectProperty (MemberID propertyId, PropertyHandler* handler)
{
	addObjectProperty (nullptr, propertyId, nullptr, handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ObjectInfo::getPropertyString (StringID path)
{
	String str;
	PropertyList* g = getGroupAt (0);
	if(g)
	{
		if(PropertyList::Property* p = g->getProperty (path))
		{
			if(p->getHandler ())
				p->getHandler ()->toString (str, p->getValue ());
			else
				p->getValue ().toString (str);
		}
	}
	return str;
}

//************************************************************************************************
// PropertiesItemModel
//************************************************************************************************

PropertiesItemModel::PropertiesItemModel ()
{}

/////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PropertiesItemModel::countFlatItems ()
{
	return properties ? properties->countProperties () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PropertiesItemModel::getItemTitle (String& title, CCL::ItemIndexRef index)
{
	if(properties)
		if(PropertyList::Property* p = properties->getPropertyAt (index.getIndex ()))
		{
			title.empty ();
			title.appendASCII (p->getID ());
			return true;
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PropertiesItemModel::createColumnHeaders (IColumnHeaderList& list)
{
	list.addColumn (80, CCLSTR ("Property"), nullptr, 0, IColumnHeaderList::kSizable);
	list.addColumn (200, CCLSTR ("Value"), nullptr, 0, IColumnHeaderList::kSizable);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PropertiesItemModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	if(properties)
	{
		PropertyList::Property* prop = properties->getPropertyAt (index.getIndex ());
		if(!prop)
			return false;

		switch(column)
		{
			case kKey:
			{
				MutableCString name (prop->getID ());
				bool bold = false;
				if(name.startsWith ("@"))
				{
					name = name.subString (1);
					bold = true;
				}

				Font font (info.style.font);
				font.isBold (bold);					
				info.graphics.drawString (info.rect, String (name), font, info.style.textBrush, Alignment::kLeft|Alignment::kVCenter);
				break;
			}

			case kValue:
			{
				const_cast<Rect&> (info.rect).right = info.view->getSize ().getWidth (); // claim the whole remaining width
				SolidBrush textBrush (info.style.textBrush);
				String string;
				Variant value (prop->getValue ());
				if(PropertyHandler* handler = prop->getHandler ())
				{
					if(handler->draw (value, info))
						break;

					handler->toString (string, value);
					int cap = handler->getEditCapability (value);
					if(cap == PropertyHandler::kObjectLink)
						textBrush = SolidBrush (Color (0xCC, 0x99, 0x00));
					else if (cap == PropertyHandler::kCustomLink)
						textBrush = SolidBrush (Colors::kGreen);
					else if(cap != PropertyHandler::kNoEdit)
						textBrush = SolidBrush (Colors::kBlue);
				}

				if(string.isEmpty ())
				{
					if(value.getType () == Variant::kObject)
					{
						UnknownPtr<IObject> object (value.object);
						if(object)
							string = object->getTypeInfo ().getClassName ();
						else
							string = "(Unknown)";
					}
					else
						value.toString (string);
				}
				info.graphics.drawString (info.rect, string, info.style.font, textBrush, Alignment::kLeft|Alignment::kVCenter);
				break;
			}
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PropertiesItemModel::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	PropertyList::Property* prop = properties->getPropertyAt (index.getIndex ());
	if(prop && prop->getHandler () && column == kValue)
	{
		Variant value (prop->getValue ());
		int cap = prop->getHandler ()->getEditCapability (value);
		if(cap != PropertyHandler::kNoEdit)
		{
			if(cap == PropertyHandler::kStringEdit)
			{
				String string;
				prop->getHandler ()->toString (string, value);
					
				Promise p (editString (string, info.rect, info));
				p.then ([this, prop] (IAsyncOperation& operation)
				{
					if(operation.getState () == IAsyncInfo::kCompleted)
						signal (Message ("editProperty", prop->asUnknown (), operation.getResult ()));
				});
			}
			else if(cap == PropertyHandler::kNumericEdit)
			{
				const int kMaxValue = 5000; // TODO: range is unclear here, ValueBox behaves bad when range is too large
				
				AutoPtr<IParameter> editParam;
				if(value.isFloat ())
					editParam = NEW FloatParam (double (-kMaxValue), double (kMaxValue));
				else
					editParam = NEW IntParam (-kMaxValue, kMaxValue);
				editParam->setValue (value);

				Promise p (editValue (editParam, info));
				p.then ([this, prop] (IAsyncOperation& operation)
				{
					if(operation.getState () == IAsyncInfo::kCompleted)
						signal (Message ("editProperty", prop->asUnknown (), operation.getResult ()));
				});
			}
			else if(cap == PropertyHandler::kColorEdit)
			{
				Color c;
				String string;
				prop->getHandler ()->toString (string, value);
				Colors::fromString (c, string);

				AutoPtr<ColorParam> colorParam = NEW ColorParam;
				colorParam->setColor (c);
				AutoPtr<ColorPicker> cp = NEW ColorPicker (colorParam);
				if(cp->popup (nullptr, true))
				{
					colorParam->getColor (c);
					AutoPtr<IUIValue> value = GraphicsFactory::createValue ();
					value->fromColor (c);
					signal (Message ("editProperty", prop->asUnknown (), value));
				}
			}
			else
			{
				PropertyHandler::EditContext context (info);
				if(prop->getHandler ()->edit (value, context))
				{
					if(context.objectToInspect)
						signal (Message ("inspectObject", (IUnknown*)context.objectToInspect));
				}
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PropertiesItemModel::appendItemMenu (IContextMenu& menu, ItemIndexRef index, const IItemSelection& selection)
{
	menu.addCommandItem (XSTR (CopyName), "Property", "Copy Name",
						CommandDelegate<PropertiesItemModel>::make (this, &PropertiesItemModel::onPropertyCommand, index.getIndex ()));
	menu.addCommandItem (XSTR (CopyValue), "Property", "Copy Value",
						CommandDelegate<PropertiesItemModel>::make (this, &PropertiesItemModel::onPropertyCommand, index.getIndex ()));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PropertiesItemModel::onPropertyCommand (CmdArgs args, VariantRef data)
{
	int row = data;
	if(args.category == "Property")
	{
		if(args.name == "Copy Name" || args.name == "Copy Value")
		{
			if(PropertyList::Property* prop = properties->getPropertyAt (row))
			{
				Variant value (prop->getValue ());
				MutableCString name (prop->getID ());
				if(!args.checkOnly ())
				{
					String text;
					if(args.name == "Copy Name")
					{
						if(name.startsWith ("@"))
							text = String (name.subString (1));
						else
							text = String (name);
					}
					else
						text = VariantString (value);

					System::GetClipboard ().setText (text);
				}
				return true;
			}
		}
	}
	return false;
}
