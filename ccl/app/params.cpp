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
// Filename    : ccl/app/params.cpp
// Description : Parameter classes
//
//************************************************************************************************

#include "ccl/app/params.h"

#include "ccl/base/kernel.h"
#include "ccl/base/boxedtypes.h"

#include "ccl/public/base/irecognizer.h"
#include "ccl/public/base/iextensible.h"
#include "ccl/public/base/iformatter.h"
#include "ccl/public/math/mathprimitives.h"

#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/gui/graphics/color.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/guiservices.h"

#include "core/public/coreinterpolator.h"

using namespace CCL;

//************************************************************************************************
// StructuredParameter
//************************************************************************************************

StructuredParameter::~StructuredParameter ()
{
	removeSubParameters ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StructuredParameter::addSubParameter (IParameter* p)
{
	p->retain ();
	parameters.append (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StructuredParameter::removeSubParameters ()
{
	ListForEach (parameters, IParameter*, p)
		p->release ();
	EndFor
	parameters.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StructuredParameter::prepareStructure ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StructuredParameter::cleanupStructure ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API StructuredParameter::countSubParameters () const
{
	return parameters.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API StructuredParameter::getSubParameter (int index) const
{
	return parameters.at (index);
}

//************************************************************************************************
// CommandParam
//************************************************************************************************

DEFINE_CLASS (CommandParam, Parameter)
DEFINE_CLASS_NAMESPACE (CommandParam, NAMESPACE_CCL)
DEFINE_CLASS_UID (CommandParam, 0xe046bde8, 0xd9cd, 0x4a16, 0x94, 0x6a, 0xf0, 0xe5, 0x2e, 0xcd, 0xc6, 0xb3)

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandParam::CommandParam (StringID name, StringID commandCategory, StringID commandName)
: Parameter (name),
  commandCategory (commandCategory),
  commandName (commandName)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API CommandParam::getType () const
{
	return kCommand;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API CommandParam::getCommandCategory () const
{
	return commandCategory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API CommandParam::getCommandName () const
{
	return commandName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandParam::setCommand (StringID category, StringID name)
{
	commandCategory = category;
	commandName = name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CommandParam::checkEnabled ()
{
	bool state = interpretCommand (CommandMsg::kCheckOnly);
	enable (state);
	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CommandParam::performUpdate () 
{ 
	interpretCommand (); 
	setValue (getMin ()); // don't toggle value
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CommandParam::interpretCommand (int flags)
{
	CommandMsg msg (commandCategory, commandName, asUnknown (), flags);

	bool result = false;
	UnknownPtr<ICommandHandler> handler (controller);
	if(handler)
		result = handler->interpretCommand (msg) != 0;
	else
	{
		// it's safer to defer command in most cases...
		if(!msg.checkOnly ())
		{
			System::GetCommandTable ().performCommand (msg, true);
			result = true;
		}
		else
			result = System::GetCommandTable ().performCommand (msg) ? true : false;		
	}
	return result;
}

//************************************************************************************************
// ScrollParam
//************************************************************************************************

DEFINE_CLASS (ScrollParam, IntParam)
DEFINE_CLASS_NAMESPACE (ScrollParam, NAMESPACE_CCL)
DEFINE_CLASS_UID (ScrollParam, 0x9ba1808b, 0xf2b8, 0x4cb4, 0x85, 0x2, 0x2f, 0xd3, 0xda, 0xb4, 0x1b, 0x14)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScrollParam::ScrollParam (int max, StringID name)
: IntParam (0, max, name),
  pageSize (0.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ScrollParam::getType () const
{
	return kScroll;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScrollParam::setRange (int range, float _pageSize)
{
	if(range != max || _pageSize != pageSize)
	{
		bool update = pageSize > 1 != _pageSize > 1 || max != range;
		
		min = 0;
		max = range;
		pageSize = _pageSize;
		
		if(value > max)
			value = max;

		if(update)
			deferChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScrollParam::setPageSize (float _pageSize)
{
	if(_pageSize != pageSize)
	{
		bool update = !(pageSize > 1 && _pageSize > 1);
		pageSize = _pageSize;
	
		if(update)
			deferChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API ScrollParam::getPageSize () const
{ 
	return pageSize; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScrollParam::canScroll () const
{
	return pageSize > 0.f && pageSize < 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ScrollParam::getStepSize () const
{
	return ccl_max (1, int ((max - min) * pageSize / 12));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScrollParam::increment ()
{
	setValue (value + getStepSize (), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScrollParam::decrement ()
{
	setValue (value - getStepSize (), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScrollParam::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "numPages")
	{
		var = pageSize == 0 ? 1 : ccl_round<0> (1.f / pageSize);
		return true;
	}
	else if(propertyId == "currentPage")
	{
		int page = pageSize == 0 ? 0 : int(getNormalized () / pageSize);
		var = page;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// ColorParam
//************************************************************************************************

DEFINE_CLASS (ColorParam, Parameter)
DEFINE_CLASS_NAMESPACE (ColorParam, NAMESPACE_CCL)
DEFINE_CLASS_UID (ColorParam, 0x8167ae15, 0x651, 0x489a, 0x89, 0x84, 0xcb, 0x24, 0x2a, 0x1e, 0xa9, 0x8d)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorParam::ColorParam (StringID name)
: Parameter (name),
  colorValue (NEW Color)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorParam::~ColorParam ()
{
	if(palette)
		ISubject::removeObserver (palette, this);

	delete colorValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorParam::setPalette (IPalette* p)
{
	if(palette)
		ISubject::removeObserver (palette, this);

	PaletteProvider::setPalette (p);

	if(palette)
		ISubject::addObserver (palette, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ColorParam::getType () const
{
	return kColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Color& CCL_API ColorParam::getColor (Color& color) const
{
	color = *colorValue;
	return color;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorParam::setColor (const Color& color, tbool update)
{
	if(color != *colorValue)
	{
		*colorValue = color;
		deferChanged ();

		if(update)
			performUpdate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorParam::isBitSet (int index) const
{
	uint32 data =  (uint32)*colorValue;
	return data & (1 << index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorParam::setBit (int index, bool state)
{
	uint32 data = (uint32)*colorValue;
	if(state)
		data |= (1 << index);
	else
		data &= ~(1 << index);
	setColor (Color ().fromInt (data), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API ColorParam::getValue () const
{
	uint32 colorCode = *colorValue;
	return Variant ((int)colorCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorParam::setValue (VariantRef value, tbool update)
{
	resetPriority ();
	uint32 colorCode = value.parseInt ();
	Color color = Color::fromInt (colorCode);
	setColor (color, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorParam::canIncrement () const
{
	return UnknownPtr<IColorPalette> (palette).isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorParam::increment ()
{
	UnknownPtr<IColorPalette> colorPalette (palette);
	if(colorPalette)
		setColor (colorPalette->getNextColor (*colorValue, false), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorParam::decrement ()
{
	UnknownPtr<IColorPalette> colorPalette (palette);
	if(colorPalette)
		setColor (colorPalette->getPrevColor (*colorValue, false), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorParam::getString (String& string, const Variant& value) const
{
	uint32 colorCode = value.asInt ();
	Color color = Color::fromInt (colorCode);
	Colors::toString (color, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorParam::toString (String& string) const
{
	Colors::toString (*colorValue, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorParam::fromString (StringRef string, tbool update)
{
	Color color;
	if(Colors::fromString (color, string))
		setColor (color, update != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static const uint32 kColorMax = 0xFFFFFF;

float CCL_API ColorParam::getValueNormalized (VariantRef value) const
{
	uint32 colorCode = value.asInt () & kColorMax;
	float f = (float)colorCode / (float)kColorMax;
	return f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API ColorParam::getValuePlain (float valueNormalized) const
{
	float f = valueNormalized * (float)kColorMax;
	uint32 colorCode = ((uint32)f) & kColorMax;
	return Variant ((int)colorCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorParam::getProperty (Variant& var, MemberID propertyId) const
{
	if(PaletteProvider::getProperty (var, propertyId))
		return true;
	else
		return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorParam::setProperty (MemberID propertyId, VariantRef var)
{
	if(PaletteProvider::setProperty (propertyId, var))
		return true;
	else
		return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
void CCL_API ColorParam::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		if(isMutable () && isEqualUnknown (subject, palette))
			rangeChanged ();
	}
	SuperClass::notify (subject, msg);
}

//************************************************************************************************
// ImageProvider
//************************************************************************************************

DEFINE_CLASS (ImageProvider, Parameter)
DEFINE_CLASS_NAMESPACE (ImageProvider, NAMESPACE_CCL)
DEFINE_CLASS_UID (ImageProvider, 0xa0b92148, 0xa412, 0x4449, 0x9c, 0x80, 0x3e, 0x6e, 0x63, 0xa3, 0x46, 0x94)

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageProvider::ImageProvider (StringID name)
: Parameter (name),
  imageValue (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageProvider::~ImageProvider ()
{
	if(imageValue)
		imageValue->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ImageProvider::getType () const
{
	return kImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ImageProvider::getImage () const
{
	return imageValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ImageProvider::setImage (IImage* image, tbool update)
{
	if(image != imageValue)
	{
		take_shared (imageValue, image);
		deferChanged ();

		if(update)
			performUpdate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API ImageProvider::getValue () const
{
	Variant v (imageValue);
	v.share ();
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ImageProvider::setValue (VariantRef value, tbool update)
{
	resetPriority ();
	UnknownPtr<IImage> image (value.asUnknown ());
	setImage (image, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ImageProvider::getString (String& string, const Variant& value) const
{
	string.empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ImageProvider)
	DEFINE_METHOD_ARGS ("setImage", "image: Object, update: bool")
END_METHOD_NAMES (ImageProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ImageProvider::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "setImage")
	{
		UnknownPtr<IImage> image (msg[0].asUnknown ());
		bool update = msg.getArgCount () > 1 ? msg[1].asBool () : false;
		setImage (image, update);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// TextModelProvider
//************************************************************************************************

DEFINE_CLASS (TextModelProvider, Parameter)
DEFINE_CLASS_UID (TextModelProvider, 0x19E52C6D, 0xF51F, 0x46D3, 0xA5, 0x58, 0xD7, 0xA5, 0xFE, 0x61, 0xC9, 0x60)

//////////////////////////////////////////////////////////////////////////////////////////////////

TextModelProvider::TextModelProvider (StringID name)
: Parameter (name),
  textModel (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextModelProvider::TextModelProvider (const TextModelProvider& p)
: Parameter (p),
  textModel (p.textModel)
{
	if(textModel)
		textModel->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextModelProvider::~TextModelProvider ()
{
	if(textModel)
		textModel->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextModelProvider::setTextModel (ITextModel* model, tbool update)
{
	if(model != textModel)
	{
		take_shared (textModel, model);
		deferChanged ();

		if(update)
			performUpdate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextModel* CCL_API TextModelProvider::getTextModel ()
{
	return textModel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TextModelProvider::getType () const
{
	return kTextModel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API TextModelProvider::getValue () const
{
	Variant v (textModel);
	v.share ();
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextModelProvider::setValue (VariantRef value, tbool update)
{
	resetPriority ();
	UnknownPtr<ITextModel> model (value.asUnknown ());
	setTextModel (model, update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextModelProvider::toString (String& string) const
{
	if(textModel)
		textModel->toParamString (string);
	else
		string.empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextModelProvider::fromString (StringRef string, tbool update)
{
	if(textModel)
	{
		textModel->fromParamString (string);
		deferChanged ();

		if(update)
			performUpdate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextModelProvider::canIncrement () const
{
	return false;
}

//************************************************************************************************
// ListParam
//************************************************************************************************

DEFINE_CLASS (ListParam, IntParam)
DEFINE_CLASS_NAMESPACE (ListParam, NAMESPACE_CCL)
DEFINE_CLASS_UID (ListParam, 0x6e4557d2, 0x8482, 0x469e, 0xb5, 0xb0, 0xe6, 0xc9, 0x2, 0xb1, 0xd3, 0x53)

//////////////////////////////////////////////////////////////////////////////////////////////////

ListParam::ListParam (StringID name)
: IntParam (0, -1, name)
{
	list.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListParam::ListParam (const ListParam& p)
: IntParam (p)
{
	list.objectCleanup ();

	ForEach (p.list, Object, obj)
		list.add (obj->clone ());
	EndFor

	ASSERT (max == list.count () - 1)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListParam::isEmpty () const
{
	return list.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ListParam::appendString (StringRef string, int index)
{
	appendObject (NEW Boxed::String (string), index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ListParam::appendValue (VariantRef value, int index)
{
	appendObject (NEW Boxed::Variant (value), index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ListParam::appendValue (VariantRef value, StringRef string, int index)
{
	appendObject (NEW Boxed::VariantWithName (value, string), index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ListParam::getValueIndex (VariantRef value) const
{
	return list.index (Boxed::Variant (value));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ListParam::getNearestValueIndex (VariantRef _value) const
{
	if(list.isEmpty ())
		return -1;

	int nearestListIndex = 0;
	double minDiff = 0;
	double value = _value.asDouble ();
	for(int i = 0, count = list.count (); i < count; i++)
	{
		double listValue = getValueAt (i).asDouble ();				
		double diff = ccl_abs (listValue - value);
		if(i == 0)
			minDiff = diff;
		else
		{
			if(diff < minDiff)
			{
				minDiff = diff;
				nearestListIndex = i;
			}
		}
		if(minDiff == 0.0)
			break;
	}					
	return nearestListIndex;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API ListParam::getValueAt (int index) const
{
	Object* obj = list.at (index);
	if(obj == nullptr)
		return Variant ();
	
	if(Boxed::Variant* vObj = ccl_cast<Boxed::Variant> (obj))
		return (VariantRef)*vObj;

	if(Boxed::String* str = ccl_cast<Boxed::String> (obj))
	{
		Variant var (*str, true);
		return var;
	}

	Variant var (obj->asUnknown ());
	var.share ();
	return var;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListParam::setValueAt (int index, VariantRef value)
{
	Boxed::Variant* vObj = getObject<Boxed::Variant> (index);
	if(vObj == nullptr)
	{
		Boxed::String* vString = getObject<Boxed::String> (index);
		if(vString == nullptr)
			return false;

		*vString = value.asString ();
		deferChanged ();
		return true;
	}

	*vObj = value;
	deferChanged ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API ListParam::getSelectedValue () const
{
	if(list.count () == 0)
		return Variant ();
	
	int index = getValue ();
	return getValueAt (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListParam::selectValue (VariantRef value, tbool update)
{
	int index = getValueIndex (value);
	if(index < 0)
		return false;
	
	setValue (index, update);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListParam::selectNearestValue (VariantRef value, tbool update)
{
	int index = getNearestValueIndex (value);
	if(index < 0)
		return false;
	
	setValue (index, update);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ListParam::setMax (VariantRef _newMax)
{
	int newMax = _newMax;
	if(newMax != max)
	{
		bool emptyInvolved = max < 0 || newMax < 0;

		max = newMax;

		if(value > max)
		{
			setValue (max);

			// when changing to or from empty list (min == 0, max == -1, value == 0),
			// no signal would be sent when value 0 silently changes it's meaning
			// (-1 would be a better choice for the empty case)
			if(emptyInvolved)
				deferChanged ();
		}
		else
			deferChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListParam::isSeparatorAt (int index) const
{
	String string;
	getString (string, index);
	return string == IMenu::strSeparator;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ListParam::increment ()
{
	int max = getMax ().asInt ();
	int index = getValue ().asInt ();
	while(++index <= max)
		if(!isSeparatorAt (index))
		{
			setValue (index, true);
			break;
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ListParam::decrement ()
{
	int index = getValue ().asInt ();
	while(--index <= max)
		if(!isSeparatorAt (index))
		{
			setValue (index, true);
			break;
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ListParam::removeAll ()
{
	list.removeAll ();
	setMax (-1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ListParam::removeAt (int index)
{
	Object* obj = list.at (index); 
	list.remove (obj);
	obj->release ();

	int max = list.count () - 1;
	setMax (max);

	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListParam::appendObject (Object* obj, int index)
{
	if(index >= 0)
	{
		if(!list.insertAt (index, obj))
			list.add (obj);
	}
	else
		list.add (obj);

	int max = list.count () - 1;
	setMax (max);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListParam::getObjectIndex (const Object* object) const
{
	return list.index (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListParam::getObjectIndex (const Object& object) const
{
	return list.index (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* ListParam::getSelectedObject () const
{
	if(list.count () == 0)
		return nullptr;

	int index = getValue ();
	return list.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListParam::selectObject (Object* object, tbool update)
{
	int index = getObjectIndex (object);
	if(index < 0)
		return false;
	
	setValue (index, update);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ListParam::getType () const
{
	return kList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ListParam::getString (String& string, VariantRef value) const
{
	if(list.isEmpty () && formatter != nullptr)
	{
		// used when list is defined by formatter
		SuperClass::getString (string, value);
	}
	else
	{	
		int idx = (int)value;
		Object* obj = list.at (idx);
		if(obj)
		{
			if(formatter)
			{
				tbool result = false;
				ASSERT (formatter->isNormalized () == 0)

				Boxed::Variant* vObj = ccl_cast<Boxed::Variant> (obj);
				if(vObj)
					result = formatter->printString (string, *vObj);
				else
					result = formatter->printString (string, Variant (obj->asUnknown ()));

				if(result)
					return;
			}

			obj->toString (string);
		}
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ListParam::fromString (StringRef string, tbool update)
{
	if(formatter)
	{
		if(list.isEmpty () || formatter->isNormalized ())
		{
			Parameter::fromString (string, update);
			return;
		}
		else
		{
			Variant v;
			if(formatter->scanString (v, string))
			{
				int foundIndex = getNearestValueIndex (v);
				if(foundIndex != -1)
					setValue (foundIndex, update);
				else			
					setValue (v, update);
				return;
			}
		}
	}
	
	int idx = 0;
	ForEach (list, Object, obj)
		Boxed::String* strObj = ccl_cast<Boxed::String> (obj);
		if(strObj)
		{
			if(*strObj == string)
			{
				setValue (idx, update);
				return;
			}
		}
		else
		{
			String str;
			obj->toString (str, 0);
			if(str == string)
			{
				setValue (idx, update);
				return;
			}
		}
		idx++;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ListParam)
	DEFINE_METHOD_ARGS ("appendString", "str: string")
	DEFINE_METHOD_ARGS ("appendValue", "value: variant")
	DEFINE_METHOD_NAME ("removeAll")
	DEFINE_METHOD_ARGR ("getValueAt", "index: int", "variant")
	DEFINE_METHOD_ARGR ("getSelectedValue", "", "variant")
	DEFINE_METHOD_ARGS ("selectValue", "value: variant, update: bool = false")
END_METHOD_NAMES (ListParam)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListParam::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "appendString")
	{
		appendString (msg[0].asString ());
		return true;
	}
	else if(msg == "appendValue")
	{
		appendValue (msg[0]);
		return true;
	}
	else if(msg == "removeAll")
	{
		removeAll ();
		return true;
	}
	else if(msg == "getValueAt")
	{
		returnValue = getValueAt (msg[0].asInt ());
		returnValue.share ();
		return true;
	}
	else if(msg == "getSelectedValue")
	{
		returnValue = getSelectedValue ();
		returnValue.share ();
		return true;
	}
	else if(msg == "selectValue")
	{
		selectValue (msg[0], msg.getArgCount () > 1 ? msg[1].asBool () : false);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// MenuParam
//************************************************************************************************

DEFINE_CLASS (MenuParam, ListParam)
DEFINE_CLASS_NAMESPACE (MenuParam, NAMESPACE_CCL)
DEFINE_CLASS_UID (MenuParam, 0x5B640B62, 0x3BD9, 0x48F3, 0x8D, 0xCA, 0xAF, 0xC3, 0xCE, 0x91, 0xEA, 0x04)

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuParam::MenuParam (StringID name)
: ListParam (name)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MenuParam::extendMenu (IMenu& menu, StringID name)
{
	UnknownPtr<IObserver> observer (getController ());
	if(observer)
	{
		Message msg (kExtendMenu, &menu);
		observer->notify (this, msg);
	}
}

//************************************************************************************************
// CustomizedMenuParam
//************************************************************************************************

CustomizedMenuParam::CustomizedMenuParam (StringID name, StringID menuType)
: MenuParam (name),
  menuType (menuType)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CustomizedMenuParam::setMenuType (StringID type)
{
	menuType = type;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API CustomizedMenuParam::getMenuType () const
{
	return menuType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CustomizedMenuParam::onMenuKeyDown (const KeyEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CustomizedMenuParam::buildMenu (IMenu& menu, IParameterMenuBuilder& builder)
{
	return false;
}

//************************************************************************************************
// PaletteProvider
//************************************************************************************************

PaletteProvider::PaletteProvider (IPalette* palette)
: palette (palette)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPalette* CCL_API PaletteProvider::getPalette () const
{
	return palette;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PaletteProvider::setPalette (IPalette* p)
{
	palette = p;

	auto param = unknown_cast<Parameter> (this);
	if(param && param->isMutable ())
		param->rangeChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PaletteProvider::getProperty (Variant& var, StringID propertyId) const
{
	if(propertyId == "palette")
	{
		var = getPalette ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PaletteProvider::setProperty (StringID propertyId, VariantRef var)
{
	if(propertyId == "palette")
	{
		UnknownPtr<IPalette> palette (var);
		setPalette (palette);
		return true;
	}
	return false;
}

//************************************************************************************************
// PaletteParam
//************************************************************************************************

DEFINE_CLASS (PaletteParam, ListParam)
DEFINE_CLASS_NAMESPACE (PaletteParam, NAMESPACE_CCL)
DEFINE_CLASS_UID (PaletteParam, 0x77b397ac, 0xfcf6, 0x441f, 0x8d, 0x53, 0xfa, 0x4a, 0x36, 0x4e, 0x31, 0x22)

//////////////////////////////////////////////////////////////////////////////////////////////////

PaletteParam::PaletteParam (StringID name, IPalette* palette)
: ListParam (name),
  PaletteProvider (palette)
{
	setMax (palette ? (palette->getCount () - 1) : -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PaletteParam::setPalette (IPalette* palette)
{
	setMax (palette ? (palette->getCount () - 1) : -1);
	PaletteProvider::setPalette (palette);
}

//************************************************************************************************
// StringParam
//************************************************************************************************

DEFINE_CLASS (StringParam, Parameter)
DEFINE_CLASS_NAMESPACE (StringParam, NAMESPACE_CCL)
DEFINE_CLASS_UID (StringParam, 0xaf7656dc, 0xdd3e, 0x47b7, 0xa7, 0x63, 0x74, 0x6e, 0x1b, 0xc2, 0xc7, 0xb3)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringParam::StringParam (StringID name)
: Parameter (name)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringParam::StringParam (const StringParam& p)
: Parameter (p)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef StringParam::getString () const
{
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef StringParam::getDefaultString () const
{
	return defaultString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API StringParam::getType () const
{
	return kString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API StringParam::getValue () const
{
	Variant v (string);
	v.share ();
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StringParam::setValue (VariantRef v, tbool update)
{
	resetPriority ();
	if(v.getType () == Variant::kString)
		fromString (v.asString(), update);
	/*else - no, has unwanted side effects!
	{
		String str;
		v.toString (str);
		fromString (str, update);
	}*/
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API StringParam::getDefaultValue () const
{
	Variant v (defaultString);
	v.share ();
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StringParam::setDefaultValue (VariantRef value)
{
	value.toString (defaultString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant	CCL_API StringParam::getMax () const
{
	return getMin ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StringParam::getString (String& _string, VariantRef value) const
{
	// TODO: formatter?
	_string = string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StringParam::fromString (StringRef str, tbool update)
{
	// TODO: formatter?
	if(string != str)
	{
		if(update)
			checkSignalFirst ();

		string = str;
		deferChanged ();

		if(update)
			performUpdate ();
	}
	else if(isSignalAlways ())
	{
		deferChanged ();
		if(update)
			performUpdate ();
	}
}

//************************************************************************************************
// FloatParam
//************************************************************************************************

DEFINE_CLASS (FloatParam, Parameter)
DEFINE_CLASS_NAMESPACE (FloatParam, NAMESPACE_CCL)
DEFINE_CLASS_UID (FloatParam, 0xf548b970, 0xe58b, 0x43de, 0xa9, 0xfe, 0x12, 0x1d, 0x43, 0x78, 0xbc, 0xc5)

//////////////////////////////////////////////////////////////////////////////////////////////////

FloatParam::FloatParam (double min, double max, StringID name)
: Parameter (name),
  min (min),
  max (max),
  value (min),
  defaultValue (min),
  precision (100)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FloatParam::FloatParam (const FloatParam& p)
: Parameter (p),
  min (p.min),
  max (p.max),
  value (p.value),
  defaultValue (p.defaultValue),
  precision (p.precision)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FloatParam::getType () const
{
	return kFloat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API FloatParam::getValue () const
{
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API FloatParam::boundValue (VariantRef v) const
{
	double value = v.asDouble ();

	if(value > max)
		value = max;
	if(value < min)
		value = min;

	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API FloatParam::getValueNormalized (VariantRef value) const
{
	return float(ccl_normalize<double> (value, min, max));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API FloatParam::getValuePlain (float valueNormalized) const
{
	return ccl_fromNormalized<double> (valueNormalized, min, max);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FloatParam::setValue (VariantRef v, tbool update)
{
	resetPriority ();

	Variant newValue = boundValue (v);
	if(newValue.asDouble () != value)
	{
		if(update)
			checkSignalFirst ();

		value = newValue;
		deferChanged ();

		if(update)
			performUpdate ();
	}
	else if(isSignalAlways ())
	{
		deferChanged ();
		if(update)
			performUpdate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant	CCL_API FloatParam::getMin () const
{
	return min;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant	CCL_API FloatParam::getMax () const
{
	return max;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FloatParam::setMin (VariantRef _newMin)
{
	double newMin = _newMin;
	if(newMin != min)
	{
		min = newMin;
		
		// check maximum
		//if(max < newMin)
		//	max = newMin;

		if(value < min)
			setValue (value);
		else
			deferChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FloatParam::setMax (VariantRef _newMax)
{
	double newMax = _newMax;
	if(newMax != max)
	{
		max = newMax;

		// check minimum
		//if(min > max)
		//	min = max;

		if(value > max)
			setValue (max);
		else
			deferChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API FloatParam::getDefaultValue () const
{
	return defaultValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FloatParam::setDefaultValue (VariantRef value)
{
	defaultValue = value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FloatParam::getPrecision () const
{
	return precision;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FloatParam::setPrecision (int precision)
{
	ASSERT (precision != 0)
	if(precision == 0)
		return false;

	this->precision = precision;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void IncDecNormalized (Parameter& param, int sign)
{
	ASSERT (sign != 0)
	ASSERT (param.getPrecision () != 0)

	NormalizedValue normalized (&param);
	double oldValue = normalized.get ();
	
	// check if min/max already reached
	if((sign > 0 && oldValue >= 1.) || (sign < 0 && oldValue <= 0.))
	{
		if(param.isWrapAround ())
		{
			if(sign > 0)
				normalized.set (0.0, true);
			else
				normalized.set (1.0, true);					
		}		
		return;
	}

	double maxStepValue = param.getPrecision ();
	double delta = 1 / maxStepValue;
	delta *= sign;

	// try multiple times in case curve snaps to min/max
	for(int i = 1; i <= 2; i++)
	{
		double newValue = ccl_bound<double> (oldValue + i * delta, 0, 1);
		double stepValue = round (newValue * maxStepValue); // avoid cumulative errors
		normalized.set (stepValue / maxStepValue, true);

		double test = normalized.get ();
		if(test != oldValue)
			break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FloatParam::increment ()
{
	IncDecNormalized (*this, +1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FloatParam::decrement ()
{
	IncDecNormalized (*this, -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FloatParam::getString (String& string, VariantRef value) const
{
	string.empty ();

	if(formatter)
	{
		if(formatter->isNormalized ())
			formatter->printString (string, getValueNormalized (value));
		else
			formatter->printString (string, value);
	}
	else
	{
		if(value.getType () == Variant::kFloat)
		{
			int digits = 2;
			//int digits = ccl_digits_of (getPrecision ()) - 1;
			string.appendFloatValue ((double)value, digits);
		}
		else
			value.toString (string);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FloatParam::fromString (StringRef string, tbool update)
{
	if(formatter)
		Parameter::fromString (string, update);
	else
	{
		double f = 0.;
		if(string.getFloatValue (f))
			setValue (f, update);
	}
}

//************************************************************************************************
// IntParam
//************************************************************************************************

DEFINE_CLASS (IntParam, Parameter)
DEFINE_CLASS_NAMESPACE (IntParam, NAMESPACE_CCL)
DEFINE_CLASS_UID (IntParam, 0x3ee3eb3d, 0x4a73, 0x4d7d, 0x90, 0x4, 0xfc, 0xfb, 0xe8, 0x19, 0x6, 0x9f)

//////////////////////////////////////////////////////////////////////////////////////////////////

IntParam::IntParam (int min, int max, StringID name)
: Parameter (name),
  min (min),
  max (max),
  value (min),
  defaultValue (min)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IntParam::IntParam (const IntParam& p)
: Parameter (p),
  min (p.min),
  max (p.max),
  value (p.value),
  defaultValue (p.defaultValue)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API IntParam::getType () const
{
	return kInteger;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API IntParam::getValue () const
{
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IntParam::setValue (VariantRef v, tbool update)
{
	resetPriority ();
	int newValue = boundValue (v);
	if(newValue != value)
	{
		if(update)
			checkSignalFirst ();

		value = newValue;
		deferChanged ();

		if(update)
			performUpdate ();
	}
	else if(isSignalAlways ())
	{
		deferChanged ();
		if(update)
			performUpdate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant	CCL_API IntParam::getMin () const
{
	return min;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant	CCL_API IntParam::getMax () const
{
	return max;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IntParam::setMin (VariantRef _newMin)
{
	int newMin = _newMin;
	if(newMin != min)
	{
		min = newMin;
		
		// check maximum
		//if(max < newMin)
		//	max = newMin;

		if(value < min)
			setValue (value);
		else
			deferChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IntParam::setMax (VariantRef _newMax)
{
	int newMax = _newMax;
	if(newMax != max)
	{
		max = newMax;

		// check minimum
		//if(min > max)
		//	min = max;

		if(value > max)
			setValue (max);
		else
			deferChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API IntParam::getDefaultValue () const
{
	return defaultValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IntParam::setDefaultValue (VariantRef value)
{
	defaultValue = value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API IntParam::getPrecision () const
{
	return ccl_max (1, max - min);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API IntParam::boundValue (VariantRef v) const
{
	int value = v.asInt ();

	if(value > max)
		value = max;
	if(value < min)
		value = min;

	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API IntParam::getValueNormalized (VariantRef value) const
{
	int range = max - min;
	if(range == 0)
		return 0;

	return (float)(value.asInt () - min) / (float)range;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API IntParam::getValuePlain (float valueNormalized) const
{
	int range = max - min;
	Variant value = (int)(valueNormalized * range  + 0.5f) + (int)min;
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IntParam::getString (String& string, VariantRef value) const
{
	string.empty ();

	if(formatter)
	{
		if(formatter->isNormalized ())
			formatter->printString (string, getValueNormalized (value));
		else
			formatter->printString (string, value);
	}
	else
	{
		value.toString (string);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API IntParam::fromString (StringRef string, tbool update)
{
	if(formatter)
		Parameter::fromString (string, update);
	else
	{
		int64 i = 0;
		if(string.getIntValue (i))
			setValue (i, update);
	}
}

//************************************************************************************************
// Parameter
//************************************************************************************************

Parameter* Parameter::createInstance (StringID className)
{
	AutoPtr<Object> obj = Kernel::instance ().getClassRegistry ().createObject (className);
	Parameter* param = ccl_cast<Parameter> (obj);
	ASSERT (param != nullptr)
	if(param)
		obj.detach ();
	return param;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* Parameter::createIdentity (IParameter* This)
{
	if(This && This->getController ())
	{
		UnknownPtr<IResolver> resolver (This->getController ());

		// try as extension
		if(!resolver)
			resolver = IExtensible::getExtensionI<IResolver> (This->getController ());
	
		if(resolver)
			return resolver->resolve (This);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::restoreValue (IParameter* p, VariantRef value, bool update)
{
	if(p->isStoreListValue ())
	{
		UnknownPtr<IListParameter> list (p);
		if(list)
		{
			list->selectNearestValue (value, update);
			return;
		}	
	}
	
	switch(p->getType ())
	{
	case IParameter::kString : 
		// ensure that numeric values are converted to a string
		p->fromString (VariantString (value), update);
		break;

	case IParameter::kFloat :
		if(value.isNumeric ())
			p->setValue (value, update);
		else
		{
			double fValue = 0.;
			VariantString string (value);
			if(string.getFloatValue (fValue))
				p->setValue (fValue, update);
			else
				p->fromString (string, update); // uses formatter
		}
		break;

	case IParameter::kToggle :
	case IParameter::kInteger :
	case IParameter::kList :
		if(value.isNumeric ())
			p->setValue (value, update);
		else
		{
			int64 iValue = 0;
			VariantString string (value);
			if(string.getIntValue (iValue))
				p->setValue (iValue, update);
			else
				p->fromString (string, update); // uses formatter or string list
		}
		break;

	default :
		p->setValue (value, update);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (Parameter, Object)
DEFINE_CLASS_NAMESPACE (Parameter, NAMESPACE_CCL)
DEFINE_CLASS_UID (Parameter, 0xb7856683, 0x5f77, 0x4c4c, 0xa1, 0xd, 0x43, 0x50, 0xe2, 0x51, 0xbe, 0x66)

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter::Parameter (StringID name)
: flags (0),
  visualState (0),
  name (name),
  controller (nullptr),
  tag (-1),
  curve (nullptr),
  formatter (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter::Parameter (const Parameter& p)
: flags (p.flags),
  visualState (p.visualState),
  name (p.name),
  controller (nullptr),
  tag (p.tag),
  curve (nullptr),
  formatter (nullptr)
{
	take_shared<IFormatter> (formatter, p.formatter);
	take_shared (curve, p.curve);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter::~Parameter ()
{
	Object::signal (Message (kDestroyed));
	cancelSignals ();

	if(formatter)
		formatter->release ();

	if(curve)
		curve->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API Parameter::getOriginal ()
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Parameter::createIdentity ()
{
	return createIdentity (getOriginal ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::signal (MessageRef msg)
{
	deferSignal (NEW Message (msg));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::deferChanged ()
{
	if(getFlag (kFeedback))
	{
		UnknownPtr<IObserver> observer (controller);
		if(observer)
			observer->notify (this, Message (kChanged));
	}

	Object::deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::rangeChanged ()
{
	if(getFlag (kFeedback))
	{
		UnknownPtr<IObserver> observer (controller);
		if(observer)
			observer->notify (this, Message (kRangeChanged));
	}

	Object::signal (Message (kRangeChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Parameter::checkSignalFirst ()
{
	if(isSignalFirst () || (canUndo () && getFlag (kEditing) == false))
		performUpdate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::beginEdit ()
{
	setFlag (kEditing, true);

	if(controller)
		controller->paramEdit (this, true);

	Object::signal (Message (kBeginEdit));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::endEdit ()
{
	setFlag (kEditing, false);

	if(controller)
		controller->paramEdit (this, false);

	Object::signal (Message (kEndEdit));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::performUpdate ()
{
	if(controller)
		controller->paramChanged (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Parameter::getType () const
{
	return kToggle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API Parameter::getName () const
{
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::setName (StringID _name)
{
	name = _name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Parameter::isEnabled () const
{
	return !getFlag (kDisabled);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::enable (tbool state)
{
	if(state != isEnabled ())
	{
		setFlag (kDisabled, !state);
		deferChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Parameter::getState (int mask) const
{
	return getFlag (mask);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::setState (int mask, tbool _state)
{
	bool state = _state != 0;
	bool oldState = getFlag (mask);
	if(state != oldState)
	{
		setFlag (mask, state);

		if(mask & (kBipolar|kOutOfRange|kReverse))
			deferChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Parameter::getVisualState () const
{
	return visualState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::setVisualState (int state)
{
	if(state != visualState)
	{
		visualState = state;
		deferChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::connect (IParamObserver* _controller, int _tag)
{
	controller = _controller;
	tag = _tag;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Parameter::getTag () const
{
	return tag;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Parameter::getController () const
{
	return controller;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API Parameter::getValue () const
{
	return getFlag (kToggleOn) ? 1 : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::setValue (VariantRef value, tbool update)
{
	resetPriority ();
	bool toggleOn = boundValue (value).asInt () != 0;

	if(toggleOn != getFlag (kToggleOn))
	{
		if(update)
			checkSignalFirst ();

		setFlag (kToggleOn, toggleOn);
		deferChanged ();

		if(update)
			performUpdate ();
	}
	else if(isSignalAlways ())
	{
		deferChanged ();
		if(update)
			performUpdate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::takeValue (const IParameter& param, tbool update)
{
	if(getType () != kString)
	{
		// TODO: what about curve???
		if(getMin () == param.getMin () && getMax () == param.getMax ())
			setValue (param.getValue (), update);
		else
			setNormalized (param.getNormalized (), update);
	}
	else
		setValue (param.getValue (), update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant	CCL_API Parameter::getMin () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant	CCL_API Parameter::getMax () const
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::setMin (VariantRef min)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::setMax (VariantRef max)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API Parameter::getDefaultValue () const
{
	return getFlag (kDefaultOn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::setDefaultValue (VariantRef value)
{
	setFlag (kDefaultOn, value.asBool ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API Parameter::boundValue (VariantRef v) const
{
	int value = v.asInt ();

	if(value > 1)
		value = 1;
	if(value < 0)
		value = 0;

	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API Parameter::getValueNormalized (VariantRef value) const
{
	return float(ccl_normalize<double> (value, getMin (), getMax ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant CCL_API Parameter::getValuePlain (float valueNormalized) const
{
	return ccl_fromNormalized<double> (valueNormalized, getMin (), getMax ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API Parameter::getNormalized () const
{
	return getValueNormalized (getValue ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::setNormalized (float value, tbool update)
{
	setValue (getValuePlain (value), update);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Parameter::canIncrement () const
{
	return getType () != kString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Parameter::getPrecision () const
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Parameter::setPrecision (int precision)
{
	return precision == 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::increment ()
{
	if(getValue ().asInt () < getMax ().asInt ())
		setValue ((int64)getValue () + 1, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::decrement ()
{
	if(getValue ().asInt () > getMin ().asInt ())
		setValue ((int64)getValue () - 1, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParamCurve* CCL_API Parameter::getCurve () const
{
	return curve;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::setCurve (IParamCurve* c)
{
	if(c == curve)
		return;

	take_shared (curve, c);
	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFormatter* CCL_API Parameter::getFormatter () const
{
	return formatter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::setFormatter (IFormatter* f)
{
	if(f == formatter)
		return;

	take_shared<IFormatter> (formatter, f);
	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::getString (String& string, VariantRef value) const
{
	if(formatter)
	{
		if(formatter->isNormalized ())
			formatter->printString (string, getValueNormalized (value));
		else
			formatter->printString (string, value);
	}
	else
		value.toString (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::toString (String& string) const
{
	getString (string, getValue ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Parameter::fromString (StringRef string, tbool update)
{
	if(formatter)
	{
		Variant v;
		if(formatter->isNormalized ())
		{
			if(formatter->scanString (v, string))
				setNormalized (v, update);
		}
		else
		{
			if(formatter->scanString (v, string))
				setValue (v, update);
		}
	}
	else
	{
		Variant value;
		value.fromString (string);
		if((int)value == 1)
			setValue (1, update);
		else
			setValue (0, update);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (Parameter)
	DEFINE_PROPERTY_TYPE ("type", ITypeInfo::kInt)
	DEFINE_PROPERTY_TYPE ("value", ITypeInfo::kVariant)
	DEFINE_PROPERTY_TYPE ("min", ITypeInfo::kVariant)
	DEFINE_PROPERTY_TYPE ("max", ITypeInfo::kVariant)
	DEFINE_PROPERTY_TYPE ("default", ITypeInfo::kVariant)
	DEFINE_PROPERTY_TYPE ("name", ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("string", ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("enabled", ITypeInfo::kBool)
	DEFINE_PROPERTY_TYPE ("signalAlways", ITypeInfo::kBool)
	DEFINE_PROPERTY_TYPE ("reverse", ITypeInfo::kBool)
END_PROPERTY_NAMES (Parameter)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Parameter::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "type")
	{
		var = getType ();
		return true;
	}
	else if(propertyId == "value")
	{
		var = getValue ();
		return true;
	}
	else if(propertyId == "min")
	{
		var = getMin ();
		return true;
	}
	else if(propertyId == "max")
	{
		var = getMax ();
		return true;
	}
	else if(propertyId == "default")
	{
		var = getDefaultValue ();
		return true;
	}
	else if(propertyId == "name")
	{	
		String temp (getName ());
		var = temp;
		var.share ();
		return true;
	}
	else if(propertyId == "string")
	{	
		String temp;
		toString (temp);
		var = temp;
		var.share ();
		return true;
	}
	else if(propertyId == "enabled")
	{
		var = isEnabled ();
		return true;
	}
	else if(propertyId == "signalAlways")
	{
		var = isSignalAlways ();
		return true;
	}
	else if(propertyId == "reverse")
	{
		var = isReverse ();
		return true;
	}
	return Object::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Parameter::setProperty (MemberID propertyId, VariantRef var)
{
	if(propertyId == "value")
	{
		restoreValue (this, var); // use restoreValue() to handle variant types
		return true;
	}
	else if(propertyId == "name")
	{
		MutableCString name (var.asString ());
		setName (name);
		return true;
	}
	else if(propertyId == "min")
	{
		setMin (var);
		return true;
	}
	else if(propertyId == "max")
	{
		setMax (var);
		return true;
	}
	else if(propertyId == "default")
	{
		setDefaultValue (var);
		return true;
	}
	else if(propertyId == "string")
	{
		fromString (var.asString ());
		return true;
	}
	else if(propertyId == "enabled")
	{
		enable (var.asBool ());
		return true;
	}
	else if(propertyId == "signalAlways")
	{
		setSignalAlways (var.asBool ());
		return true;
	}
	else if(propertyId == "reverse")
	{
		setReverse (var.asBool ());
		return true;
	}

	return Object::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Parameter)
	DEFINE_METHOD_ARGS ("setValue", "value: variant, update: bool = false")
	DEFINE_METHOD_ARGS ("fromString", "str: string, update: bool = false")
	DEFINE_METHOD_ARGS ("setNormalized", "value: float, upate: bool = false")
	DEFINE_METHOD_ARGR ("getNormalized", "", "float")
	DEFINE_METHOD_ARGS ("setFormatter", "formatter: Formatter")
	DEFINE_METHOD_ARGS ("setCurve", "curve: string")
	DEFINE_METHOD_ARGR ("isType", "type: string", "bool")
	DEFINE_METHOD_ARGS ("setSignalAlways", "state: bool = true")
END_METHOD_NAMES (Parameter)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Parameter::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "setValue")
	{
		restoreValue (this, msg[0], msg.getArgCount () > 1 ? msg[1].asBool () : false); // use restoreValue() to handle variant types
		return true;
	}
	else if(msg == "fromString")
	{
		fromString (msg[0].asString (), msg.getArgCount () > 1 ? msg[1].asBool () : false);
		return true;
	}
	else if(msg == "setNormalized")
	{
		setNormalized (msg[0].asFloat (), msg.getArgCount () > 1 ? msg[1].asBool () : false);
		return true;
	}
	else if(msg == "getNormalized")
	{
		returnValue = getNormalized ();
		return true;
	}
	else if(msg == "setFormatter")
	{
		UnknownPtr<IFormatter> formatter (msg[0].asUnknown ());
		setFormatter (formatter);
		return true;
	}
	else if(msg == "setCurve")
	{
		MutableCString curveName;
		if(msg.getArgCount () == 0)
		{
			returnValue = false;
			return true;
		}
		
		msg[0].toCString (curveName);

		setCurve (ParamCurveFactory::instance().create (curveName));
		return true;
	}
	else if(msg == "isType")
	{
		MutableCString typeName (msg[0].asString ());
		const MetaClass* type = Kernel::instance ().getClassRegistry ().findType (typeName);
		returnValue = type && canCast (*type);
		return true;
	}
	else if(msg == "setSignalAlways")
	{
		bool state = msg.getArgCount () > 0 ? msg[0].asBool () : true;
		setSignalAlways (state);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ParamCurve
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ParamCurve, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API ParamCurve::getRelativeValue (double startValue, double endValue, double linearValue) const
{
	double delta = endValue - startValue;
	double v = ccl_bound<double> (linearValue + delta, 0, 1);
	//if(ccl_equals (v, endValue, 0.01))
	//	v = endValue;
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API ParamCurve::getFactoryName () const
{
	return nullptr;
}

//************************************************************************************************
// ConcaveCurve
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ConcaveCurve, ParamCurve)

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API ConcaveCurve::displayToNormalized (double linearValue) const
{
	return linearValue * linearValue; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API ConcaveCurve::normalizedToDisplay (double curveValue) const
{
	return pow (curveValue, .5);
}

//************************************************************************************************
// ConvexCurve
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ConvexCurve, ParamCurve)

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API ConvexCurve::displayToNormalized (double linearValue) const
{
	linearValue = 1 - linearValue;
	return 1 - linearValue * linearValue; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API ConvexCurve::normalizedToDisplay (double curveValue) const
{
	return 1 - pow (1 - curveValue, .5);
}

//************************************************************************************************
// InterpolatorCurve
//************************************************************************************************

DEFINE_CLASS_HIDDEN (InterpolatorCurve, ParamCurve)

//////////////////////////////////////////////////////////////////////////////////////////////////

InterpolatorCurve::InterpolatorCurve (Core::Interpolator* _interpolator)
: interpolator (nullptr),
  normalizer (nullptr)
{
	if(_interpolator)
		setInterpolator (_interpolator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

InterpolatorCurve::~InterpolatorCurve ()
{
	setInterpolator (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InterpolatorCurve::setInterpolator (Core::Interpolator* _interpolator)
{
	delete interpolator;
	delete normalizer;

	interpolator = _interpolator;
	normalizer = nullptr;

	if(interpolator)
		normalizer = NEW Core::LinearInterpolator (interpolator->getMinRange (), interpolator->getMaxRange ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float InterpolatorCurve::getMinRange () const
{
	return interpolator ? interpolator->getMinRange () : 0.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float InterpolatorCurve::getMaxRange () const
{
	return interpolator ? interpolator->getMaxRange () : 0.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float InterpolatorCurve::getMidRange () const
{
	return interpolator ? interpolator->getMidRange () : 0.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void InterpolatorCurve::setRange (float minRange, float maxRange, float midRange)
{
	if(interpolator)
		interpolator->setRange (minRange, maxRange, midRange);
	if(normalizer)
		normalizer->setRange (interpolator ? interpolator->getMinRange () : minRange, interpolator ? interpolator->getMaxRange () : maxRange);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
double CCL_API InterpolatorCurve::displayToNormalized (double linearValue) const
{
	// Conversion used by NormalizedValue::set(), i.e. from display to parameter range
	// p->setNormalized (curve->getCurveValue ())

	if(interpolator)
	{
		const float value = interpolator->normalizedToRange (float (linearValue));
		return double (normalizer->rangeToNormalized (value));
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API InterpolatorCurve::normalizedToDisplay (double curveValue) const
{
	// Conversion used by NormalizedValue::get(), i.e. from parameter range to display
	// curve->normalizedToDisplay (p->getNormalized ())
	
	if(interpolator)
	{
		const float value = normalizer->normalizedToRange (float (curveValue));
		return double (interpolator->rangeToNormalized (value));
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double CCL_API InterpolatorCurve::getRelativeValue (double startValue, double endValue, double linearValue) const
{
	#if 1
	if(interpolator)	
	{
		float startRange = interpolator->normalizedToRange ((float)startValue);
		float endRange = interpolator->normalizedToRange ((float)endValue);
		float deltaRange = endRange - startRange;
		float newRange = interpolator->normalizedToRange ((float)linearValue) + deltaRange;
		return interpolator->rangeToNormalized (newRange);
	}
	#endif

	return ParamCurve::getRelativeValue (startValue, endValue, linearValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API InterpolatorCurve::getFactoryName () const
{
	return interpolator ? interpolator->getName () : nullptr;
}

//************************************************************************************************
// ParamCurveFactory
//************************************************************************************************

ParamCurveFactory& ParamCurveFactory::instance ()
{
	static ParamCurveFactory theFactory;
	return theFactory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParamCurve* ParamCurveFactory::create (StringID name)
{
	VectorForEach (classes, CurveClass, c)
		if(c.name == name)
			return c.createFunc ();
	EndFor

	// try core interpolator factory
	if(Core::Interpolator* interpolator = Core::InterpolatorFactory::create (name))
		return NEW InterpolatorCurve (interpolator);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamCurveFactory::add (StringID name, CreateFunc createFunc)
{
	classes.add (CurveClass (name, createFunc));
}
