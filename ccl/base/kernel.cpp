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
// Filename    : ccl/base/kernel.cpp
// Description : Library Kernel
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/base/kernel.h"
#include "ccl/base/message.h"

#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/plugins/classfactory.h"

namespace CCL {

//************************************************************************************************
// KernelAttributeProvider
//************************************************************************************************

class KernelAttributeProvider: public ClassAttributeProvider
{
public:
	// ClassAttributeProvider
	bool getClassAttributes (IAttributeList& attributes, UIDRef cid, StringID language) const override
	{
		const MetaClass* mc = Kernel::instance ().getClassRegistry ().findType (cid);
		if(mc && mc->countAttributes () > 0)
		{
			for(int i = 0; i < mc->countAttributes (); i++)
			{
				Variant value;
				mc->getAttributeValue (value, i);
				attributes.setAttribute (mc->getAttributeName (i), value);
			}
			return true;
		}
		return false;
	}
};

//************************************************************************************************
// MetaClassIterator
//************************************************************************************************

class MetaClassIterator: public Unknown,
						 public ListIterator<MetaClass*>,
						 public IUnknownIterator
{
public:
	MetaClassIterator (const LinkedList<MetaClass*>& list)
	: ListIterator<MetaClass*> (list)
	{}

	// IUnknownIterator
	tbool CCL_API done () const override { return ListIterator<MetaClass*>::done (); }
	IUnknown* CCL_API nextUnknown () override { return ListIterator<MetaClass*>::next (); }

	CLASS_INTERFACE (IUnknownIterator, Unknown)
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// MetaClassRegistry::HashEntry
//************************************************************************************************

int MetaClassRegistry::HashEntry::getHashCode (const MetaClassRegistry::HashEntry& e, int size)
{
	return Core::CStringFunctions::hashCFSIndex (e.className) % size;
}

//************************************************************************************************
// MetaClassRegistry
//************************************************************************************************

MetaClassRegistry::MetaClassRegistry ()
: libName (nullptr),
  classNameTable (100, HashEntry::getHashCode)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaClassRegistry::remove (MetaClassRef _metaClass)
{
	MetaClass* metaClass = const_cast<MetaClass*> (&_metaClass);
	if(!classes.remove (metaClass))
		return false;
	classNameTable.remove (HashEntry (metaClass->getPersistentName (), metaClass));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const MetaClass* MetaClassRegistry::findType (CStringPtr persistentName) const
{
	HashEntry temp (persistentName);
	return classNameTable.lookup (temp).metaClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const MetaClass* MetaClassRegistry::findType (UIDRef cid) const
{
	ListForEach (classes, MetaClass*, mc)
		if(mc->getClassID ().isValid () && mc->getClassID ().equals (cid))
			return mc;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* MetaClassRegistry::createObject (CStringPtr persistentName) const
{
	const MetaClass* mc = findType (persistentName);
	return mc ? mc->createObject () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* MetaClassRegistry::createObject (UIDRef cid) const
{
	const MetaClass* mc = findType (cid);
	return mc ? mc->createObject () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr MetaClassRegistry::getLibraryName () const
{
	return libName ? libName : "Native Classes";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API MetaClassRegistry::newTypeIterator () const
{
	return NEW MetaClassIterator (classes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API MetaClassRegistry::newEnumIterator () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MetaClassRegistry::getTypeDetails (ITypeInfoDetails& result, const ITypeInfo& typeInfo) const
{
	// TODO: this needs some cleanup here...

	if(const MetaClass::PropertyDefinition* propertyNames = typeInfo.getPropertyNames ())
		for(int i = 0; propertyNames[i].name != nullptr; i++)
		{
			const ITypeInfo::PropertyDefinition& propDef = propertyNames[i];

			Model::MemberDescription member (propDef.name, propDef.type, propDef.typeName);
			if(member.typeName.isEmpty () && propDef.typeInfo)
				member.typeName = propDef.typeInfo->getClassName ();

			result.addMember (member);
		}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ITypeInfo* MetaClassRegistry::findTypeInfo (CStringPtr name) const
{
	return findType (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IEnumTypeInfo* MetaClassRegistry::findEnumTypeInfo (CStringPtr name) const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MetaClassRegistry::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (ITypeLibrary)
	QUERY_UNKNOWN (ITypeLibrary)
	*ptr = nullptr;
	return kResultNoInterface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API MetaClassRegistry::retain ()
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API MetaClassRegistry::release ()
{
	return 1;
}

//************************************************************************************************
// DeferredDestroyer
//************************************************************************************************

static const CString kDeferDestruction (CSTR ("deferDestruction"));

//////////////////////////////////////////////////////////////////////////////////////////////////

#if CCL_DEBUG_INTERNAL
void CCL_API DeferredDestroyer::notify (ISubject* subject, MessageRef msg)
{
	#if DEBUG_LOG
	if(msg == kDeferDestruction)
	{
		IUnknown* unk = msg[0].asUnknown ();
		Object* obj = unknown_cast<Object> (unk);
		CCL_PRINTF ("Deferred destruction of %s\n", obj ? obj->myClass ().getPersistentName () : "IUnknown");
	}
	#endif
}
#endif

//************************************************************************************************
// MetaClassReplaceScope
//************************************************************************************************

MetaClassReplaceScope::MetaClassReplaceScope (CStringPtr className, MetaClassRef newClass)
: classes (Kernel::instance ().getClassRegistry ()),
  newClass (newClass),
  oldClass (nullptr)
{
	oldClass = classes.findType (className);

	if(oldClass == &newClass)
		oldClass = nullptr;
	else if(oldClass)
	{
		// remove the old class
		classes.remove (*oldClass);

		// set temporary class name, resort in class registry
		ASSERT (newClass.getPersistentName () == static_cast<const AbstractTypeInfo&> (newClass).getClassName ()) // no explicit persistentName
		setClassName (newClass, className);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClassReplaceScope::~MetaClassReplaceScope ()
{
	if(oldClass)
	{
		// restore old state
		setClassName (newClass, nullptr);
		classes.append (ccl_const_cast (oldClass));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetaClassReplaceScope::setClassName (MetaClassRef newClass, CStringPtr className)
{
	// set name and resort in class registry
	classes.remove (newClass);
	ccl_const_cast (newClass).setPersistentName (className);
	classes.append (ccl_const_cast (&newClass));
}

//************************************************************************************************
// Kernel::KernelDestroyer
//************************************************************************************************

struct Kernel::KernelDestroyer
{
	~KernelDestroyer ()
	{
		if(theInstance)
		{
			theInstance->destroy ();
		}
	}
};

//************************************************************************************************
// Kernel
//************************************************************************************************

Kernel* Kernel::theInstance = nullptr;
bool Kernel::destroyed = false;
Kernel::KernelDestroyer Kernel::theKernelDestroyer;

//////////////////////////////////////////////////////////////////////////////////////////////////

Kernel& Kernel::instance ()
{
	ASSERT (destroyed == false)
	if(!theInstance)
		theInstance = NEW Kernel;
	return *theInstance;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Kernel::Kernel ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Kernel::addObject (Object* object, bool globalScope)
{
	#if DEBUG_LOG
	MetaClassRef typeId = object->myClass ();
	CCL_PRINTF ("Kernel adding object %s %p\n", typeId.getPersistentName () ? typeId.getPersistentName () : "(Meta class not yet initialized!)", object)
	#endif
	if(globalScope || initialized == false)
		preInitObjects.append (object);
	else
		objects.append (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Kernel::deferDestruction (IUnknown* object)
{
	if(object)
	{
		(NEW Message (kDeferDestruction, object))->post (&destroyer);
		object->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClassRegistry& Kernel::getClassRegistry ()
{
	return classes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Kernel::cleanupObjects (CleanupObjectList& list)
{
	if(list.isEmpty () == false)
	{
		ListForEachReverse (list, Object*, obj)
			CCL_PRINTF ("Kernel removing object %s %p\n", obj->myClass ().getPersistentName (), obj)
			obj->release ();
		EndFor
		list.removeAll ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Kernel::initialize ()
{
	initialized = true;
	ListForEach (initializer, KernelInitializer, i)
		if(!i.func ())
		{
			CCL_PRINTF ("Kernel::initialize failed at: %s\n", i.name);
			ASSERT (0)
			return false;
		}
		else
		{
			CCL_PRINTF ("Kernel::initialize done: %s\n", i.name);
			initializerDone.append (i);
		}
	EndFor
	initializer.removeAll ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Kernel::destroy ()
{
	ASSERT (destroyed == false)
	if(destroyed == false)
	{
		// cleanup remaining objects
		cleanupObjects (objects);
		cleanupObjects (preInitObjects);

		destroyed = true;
		theInstance = nullptr;
		delete this;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Kernel::terminate ()
{
	ListForEachReverse (terminator, KernelTerminator, t)
		t.func ();
		terminatorDone.prepend (t);
	EndFor
	terminator.removeAll ();

	// cleanup objects created during initialize
	cleanupObjects (objects);

	// allow re-initialzation
	initializer.swapContent (initializerDone);
	terminator.swapContent (terminatorDone);
	destroyer.cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Kernel::initializeLevel (int level)
{
	initialized = true;
	while(!initializer.isEmpty ())
	{
		if(initializer.getFirst ().level >= level)
			break;

		KernelInitializer i = initializer.removeFirst ();
		if(!i.func ())
			return false;
		initializerDone.append (i);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Kernel::terminateLevel (int level)
{
	while(!terminator.isEmpty ())
	{
		if(terminator.getLast ().level < level)
			break;

		KernelTerminator t = terminator.removeLast ();
		t.func ();
		terminatorDone.prepend (t);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace KernelHelper
{
	static void getMetaClassCategories (String& category, String& subCategory, MetaClassRef mc)
	{
		CString categoryName (mc.getCategoryName ());
		if(categoryName.isEmpty ())
			categoryName = mc.getNamespaceName ();

		if(!categoryName.isEmpty ())
		{
			int index = categoryName.index (":");
			if(index != -1)
			{
				category = CCLSTR (categoryName.subString (0, index));
				subCategory = CCLSTR (categoryName.subString (index + 1));
			}
			else
			{
				category = CCLSTR (categoryName);
				subCategory.empty ();
			}
		}
		else
		{
			category = CCLSTR ("CCL");
			subCategory.empty ();
		}
	}

	static void assignAttributeProvider (ClassFactory& factory)
	{
		static KernelAttributeProvider theAttributeProvider;
		ASSERT (factory.getAttributeProvider () == nullptr || factory.getAttributeProvider () == &theAttributeProvider)
		factory.setAttributeProvider (&theAttributeProvider);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Kernel::registerPublicClasses (ClassFactory& factory, CStringPtr categoryName, ClassFilter* filter)
{
	// assign attribute provider
	KernelHelper::assignAttributeProvider (factory);

	String inputCategory (categoryName ? CCLSTR (categoryName) : CCLSTR (NAMESPACE_CCL));

	int count = 0;
	ListForEach (classes.getClasses (), MetaClass*, mc)
		if(mc->getClassID ().isValid ())
		{
			String category;
			String subCategory;
			KernelHelper::getMetaClassCategories (category, subCategory, *mc);
			if(category == inputCategory)
			{
				int flags = mc->isSingleton () ? ClassDesc::kSingleton : 0;
				ClassDesc description (mc->getClassID (), category, mc->getPersistentName (), subCategory, mc->getDescription (), flags);
				if(filter && !filter->matches (description))
					continue;

				factory.registerClass (description, MetaClass::createInstance, mc);
				count++;
			}
		}
	EndFor
	return count > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Kernel::registerClass (ClassFactory& factory, MetaClassRef mc)
{
	// assign attribute provider
	KernelHelper::assignAttributeProvider (factory);

	ASSERT (mc.getClassID ().isValid ())

	String category;
	String subCategory;
	KernelHelper::getMetaClassCategories (category, subCategory, mc);
	int flags = mc.isSingleton () ? ClassDesc::kSingleton : 0;
	ClassDesc description (mc.getClassID (), category, mc.getPersistentName (), subCategory, mc.getDescription (), flags);

	factory.registerClass (description, MetaClass::createInstance, const_cast<MetaClass*> (&mc));
}

//************************************************************************************************
// KernelInitializer
//************************************************************************************************

KernelInitializer::KernelInitializer (bool (*func)(), CStringPtr name, int level)
: func (func),
  level (level),
  name (name)
{
	Kernel::instance ().initializer.addSorted (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

KernelInitializer::KernelInitializer ()
: func (nullptr),
  level (0),
  name (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool KernelInitializer::operator > (const KernelInitializer& i) const
{
	return level > i.level;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool KernelInitializer::operator == (const KernelInitializer& i) const
{
	return func == i.func && level == i.level;
}

//************************************************************************************************
// KernelTerminator
//************************************************************************************************

KernelTerminator::KernelTerminator (void (*func)(), int level)
: func (func),
  level (level)
{
	Kernel::instance ().terminator.addSorted (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

KernelTerminator::KernelTerminator ()
: func (nullptr),
  level (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool KernelTerminator::operator > (const KernelTerminator& i) const
{
	return level > i.level;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool KernelTerminator::operator == (const KernelTerminator& i) const
{
	return func == i.func && level == i.level;
}

