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
// Filename    : ccl/gui/skin/skinelement.cpp
// Description : Skin Element class
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/skin/skinelement.h"
#include "ccl/gui/skin/skinattributes.h"
#include "ccl/gui/skin/skinmodel.h"

#include "ccl/base/singleton.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/configuration.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/text/itranslationtable.h"
#include "ccl/public/gui/framework/styleflags.h"
#include "ccl/public/gui/framework/skinxmldefs.h"

#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ilogger.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

namespace CCL {
namespace SkinElements {

//************************************************************************************************
// SkinElementLibrary
//************************************************************************************************

class SkinElementLibrary: public TypeLibrary,
						  public Singleton<SkinElementLibrary>
{
public:
	SkinElementLibrary ();

	const StyleDef* getStyleDef (CStringRef name) const;
};

} // namespace SkinElements
} // namespace CCL

using namespace CCL;
using namespace SkinElements;

//************************************************************************************************
// Skin Warnings
//************************************************************************************************

static const Configuration::BoolValue skinWarningsEnabled ("GUI.Skin", "skinWarningsEnabled", false);

void CCL::SkinWarning (const SkinElements::Element* element, CStringRef _warning)
{
	String warning (_warning);

	if(element)
	{
		CString elementName;
		if(!element->getName ().isEmpty ())
			elementName = element->getName ();
		else
			elementName = element->myClass ().getPersistentName ();

		String context;
		context << element->getFileName () << ":" << element->getLineNumber ();
		context << " '" << elementName << "': ";

		warning.prepend (context);
	}

	warning.prepend ("[Skin] ");

	if(skinWarningsEnabled)
	{
		Alert::Event e (warning, Alert::kWarning);
		/*if(element)
		{
			e.fileName = String (element->getFileName ());
			e.lineNumber = element->getLineNumber ();
		}*/
		System::GetLogger ().reportEvent (e);
	}

	#if DEBUG
	Debugger::println (warning);
	#endif

}

//************************************************************************************************
// SkinElementLibrary
//************************************************************************************************

DEFINE_SINGLETON (SkinElementLibrary)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinElementLibrary::SkinElementLibrary ()
: TypeLibrary (CCL_SKIN_TYPELIB_NAME)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

const StyleDef* SkinElementLibrary::getStyleDef (CStringRef name) const
{
	const Enumeration* e = (const Enumeration*)findEnum (name);
	if(e)
		return e->def;
	return nullptr;
}

//************************************************************************************************
// MetaElement
//************************************************************************************************

TypeLibrary& MetaElement::getTypeLibrary ()
{
	return SkinElementLibrary::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Element* MetaElement::createElement (CStringRef name)
{
	const MetaElement* me = static_cast<const MetaElement*> (SkinElementLibrary::instance ().findType (name, kTagsCaseSensitive));
	if(me)
		return me->createElement ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaElement::MetaElement (CStringPtr name, Object* (*creator)(), const MetaElement* parentClass,
						  bool isAbstract, CStringPtr groupName)
: TypeInfoWithMembers (name, parentClass),
  creator (creator),
  isAbstract (isAbstract),
  groupName (groupName)
{
	SkinElementLibrary::instance ().addType (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Element* MetaElement::createElement () const
{
	return (Element*)(*creator) ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MetaElement::getClassFlags () const
{
	return isAbstract ? kAbstract : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaElement::getDetails (ITypeInfoDetails& details) const
{
	TypeInfoWithMembers::getDetails (details);

	if(members == nullptr)
	{
		AutoPtr<Element> element = createElement ();
		ASSERT (element)
		if(!element)
			return false;

		MutableSkinAttributes attr;
		attr.setVerbose (true);
		element->getAttributes (attr);

		ForEachAttribute (attr.getAttributes (), name, value)
			DataType dt = ITypeInfo::kVoid;
			switch(value.getType ())
			{
			case Variant::kInt : dt = ITypeInfo::kInt; break;
			case Variant::kFloat : dt = ITypeInfo::kFloat; break;
			case Variant::kString : dt = ITypeInfo::kString; break;
			case Variant::kObject : dt = ITypeInfo::kObject; break;
			}
			details.addMember (Model::MemberDescription (name, dt));
		EndFor
	}

	if(groupName)
		details.setAttribute (Model::kClassDocGroup, groupName);

	return true;
}

//************************************************************************************************
// Enumeration
//************************************************************************************************

const StyleDef* Enumeration::getStyleDef (CStringRef name)
{
	return SkinElementLibrary::instance ().getStyleDef (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Enumeration::Enumeration (CStringPtr name, CStringPtr parentName, const StyleDef* def)
: EnumTypeInfo (name, parentName),
  def (def),
  count (-1)
{
	SkinElementLibrary::instance ().addEnum (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Enumeration::getEnumeratorCount () const
{
	if(count == -1)
	{
		count = 0;
		if(const StyleDef* ptr = def)
		{
			while(ptr->name)
			{
				count++;
				ptr++;
			}
		}
	}

	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Enumeration::getEnumerator (MutableCString& name, Variant& value, int index) const
{
	ASSERT (index >= 0 && index < getEnumeratorCount ())
	if(index < 0 || index >= getEnumeratorCount ())
		return false;

	name = def[index].name;
	value = def[index].value;
	return true;
}

//************************************************************************************************
// Element
//************************************************************************************************

BEGIN_SKIN_ELEMENT_BASE_WITH_MEMBERS (Element, ObjectArray, TAG_ELEMENT, DOC_GROUP_GENERAL)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_NAME, TYPE_STRING) ///< The name of an element plays different roles, depending on the class of the element. 
	ADD_SKIN_ELEMENT_MEMBER (ATTR_COMMENT, TYPE_STRING) ///< Optional comment for developers and tool support
END_SKIN_ELEMENT_WITH_MEMBERS (Element)

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_VCALL compareElementAndId (const void *key, const void *p)
{
	Element** e = (Element**)p;
	CString* id = (CString*)key;
	return id->compare ((**e).getName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Element::isSkinWarningsEnabled ()
{
	return skinWarningsEnabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Element::Element (CStringRef name)
: name(name),
  parent (nullptr),
  sorted (false)
{
	objectCleanup (true);

	#if DEBUG
	lineNumber = 0;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Element::clone (ISkinElement*& _element) const
{
	auto element = static_cast<Element*> (clone ());
	if(element) // don't copy source file
		element->setSourceFile (String::kEmpty);
	_element = element;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Element::setName (StringRef name)
{
	this->name = name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Element::addChild (Element* e, int index)
{ 
	ASSERT (e->getParent () == nullptr || e->getParent () == this)
	if(sorted)
		addSorted (e);
	else
	{
		if(index >= 0)
		{
			if(!insertAt (index, e))
				add (e);
		}
		else
			add (e);
	}
	e->setParent (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Element::removeChild (Element* e)
{
	ASSERT (e->getParent () == this)
	remove (e); 
	e->setParent (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Element::addChildElement (ISkinElement* childElement, int index)
{
	auto* e = unknown_cast<Element> (childElement);
	ASSERT (e)
	if(!e)
		return false;

	addChild (e, index);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Element::removeChildElement (ISkinElement* childElement, int* oldIndex)
{
	auto* e = unknown_cast<Element> (childElement);
	ASSERT (e)
	if(!e)
		return false;
	
	if(oldIndex)
		*oldIndex = index (e);

	removeChild (e);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Element* Element::getParent () const
{
	return parent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Element* Element::findElement (CStringRef id) const
{
	if(sorted && total > 0)
	{
		Element** p = (Element**)::bsearch (&id, items, total, sizeof(Element*), compareElementAndId);
		if(p)
			return *p;
	}

	ArrayForEach (*this, Element, child)
		if(child->getName () == id)
			return child;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Element* Element::findElement (MetaClassRef typeId) const
{ 
	ArrayForEach (*this, Element, child)
		if(child->canCast (typeId))
			return child;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Element::compare (const Object& obj) const
{
	Element& e = (Element&)obj;
	return -e.getName ().compare (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Element* Element::getElement (int index) const
{ 
	return (Element*)at (index); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Element* Element::getParent (MetaClassRef typeId) const
{
	Element* e = getParent ();
	while(e)
	{
		if(e->canCast (typeId))
			return e;
		e = e->getParent ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Element::setParent (Element* newParent)
{
	ASSERT (newParent == nullptr || parent == nullptr || parent == newParent)
	parent = newParent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Element* Element::findElement (CStringRef name, MetaClassRef typeId) const
{
	Element* e = findElement (name);
	return e && e->canCast (typeId) ? e : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Element::mergeElements (Element& other)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Element::takeElements (Element& source)
{
	if(source.isEmpty ()) // nothing to do
		return;

	ObjectList temp;
	temp.add (source, Container::kShare);
	source.removeAll ();

	ForEach (temp, Element, e)
		e->setParent (nullptr);
	
		Element* existing = !e->getName ().isEmpty () ? findElement (e->getName ()) : nullptr;
		if(existing)
		{
			if(existing->mergeElements (*e))
			{
				CCL_PRINTF ("merged %s \"%s\" (%s:%d)\n", e->myClass ().getPersistentName (), e->getName ().str (), e->getFileName ().str (), e->getLineNumber ());
				e->release ();
				continue;
			}
			else
			{
				MutableCString declarations;
				declarations.appendFormat ("(First declaration: %s:%d, ", existing->getFileName ().str (), existing->getLineNumber ());
				declarations.appendFormat ("Second declaration: %s:%d)", e->getFileName ().str (), e->getLineNumber ());
				
				if(!e->isOverrideEnabled ())
				{
					SKIN_WARNING (nullptr, "Element '%s' already exists in '%s'! %s", e->getName ().str (), myClass ().getPersistentName (), declarations.str ())
					
					if(existing->isOverrideEnabled ()) // don't replace existing override with non-override style
						continue;
				}
				removeChild (existing);
				existing->release ();
			}
		}
		addChild (e);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Element::setAttributes (const SkinAttributes& a)
{
	setName (a.getString (ATTR_NAME));
	setComment (a.getString (ATTR_COMMENT));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Element::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_NAME, getName ());
	if(!comment.isEmpty ())
		a.setString (ATTR_COMMENT, comment);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Element::getAttributes (IAttributeList& attributes) const
{
	MutableSkinAttributes attr;
	getAttributes (attr);
	attributes.copyFrom (attr.getAttributes ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Element::setAttributes (const IAttributeList& attributes)
{
	MutableSkinAttributes attr;
	attr.getAttributes ().copyFrom (attributes);
	setAttributes (attr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Element::getAttributeValue (Variant& value, StringID name) const
{
	MutableSkinAttributes attr;
	getAttributes (attr);
	return attr.getAttributes ().getAttribute (value, name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Element::setAttributeValue (StringID name, VariantRef value, int index)
{
	MutableSkinAttributes attr;
	getAttributes (attr);
	attr.getAttributes ().setAttribute (name, value);
	setAttributes (attr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Element::removeAttribute (StringID name, int* oldIndex)
{
	MutableSkinAttributes attr;
	getAttributes (attr);
	if(oldIndex)
		*oldIndex = attr.getAttributes ().getAttributeIndex (name);
	if(!attr.getAttributes ().remove (name))
		return false;
	setAttributes (attr); // note that attribute might not acutally be removed here
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Element::loadFinished ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Element::isOverrideEnabled () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISkinContext* Element::getSkinContext () const
{
	return parent ? parent->getSkinContext () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileSystem* Element::getFileSystem () const
{
	ISkinContext* c = getSkinContext ();
	IFileSystem* fs = c ? c->getFileSystem () : nullptr;
	ASSERT (fs != nullptr)
	return fs;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url& Element::makeSkinUrl (Url& url, StringRef path, bool isFolder) const
{
	url.setProtocol (CCLSTR ("skin"));

	ISkinContext* c = getSkinContext ();
	ASSERT (c != nullptr)
	if(c)
		url.setHostName (String (c->getSkinID ()));

	url.setPath (path, isFolder ? Url::kFolder : Url::kIgnore);
	return url;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Element* Element::getTranslationScope () const
{
	if(getParent ())
		return getParent ()->getTranslationScope ();
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Element::translate (StringRef text) const
{
	CString scopeName;
	const Element* scopeElement = getTranslationScope ();
	if(scopeElement)
		scopeName = scopeElement->getName ();

	return translateWithScope (scopeName, text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Element::translateWithScope (StringID scopeName, StringRef text) const
{
	String result;
	ISkinContext* c = getSkinContext ();
	ITranslationTable* table = c ? c->getStringTable () : nullptr;
	ASSERT (table != nullptr)
	if(table)
		table->getStringWithUnicodeKey (result, scopeName, text);
	else
		result = text;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Theme* Element::getTheme () const
{
	ISkinContext* c = getSkinContext ();
	Theme* theme = c ? c->getTheme () : nullptr;
	ASSERT (theme != nullptr)
	return theme;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Element::getSourceInfo (String& fileName, int32& lineNumber, IUrl* packageUrl) const
{
	fileName = String (getFileName ());
	lineNumber = getLineNumber ();

	if(packageUrl)
		if(ISkinContext* c = getSkinContext ())
		{
			UnknownPtr<IFileResource> packageResource (c->getPackage ());
			if(packageResource)
				packageUrl->assign (packageResource->getPath ());
		}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Element::setSourceFile (StringRef fileName)
{
	setFileName (MutableCString (fileName));
}
