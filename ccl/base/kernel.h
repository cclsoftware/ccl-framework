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
// Filename    : ccl/base/kernel.h
// Description : Library Kernel
//
//************************************************************************************************

#ifndef _ccl_kernel_h
#define _ccl_kernel_h

#include "ccl/public/base/itypelib.h"

#include "ccl/public/collections/hashtable.h"
#include "ccl/base/collections/objectlist.h"

namespace CCL {

class ClassFactory;
class ClassFilter;

//************************************************************************************************
// MetaClassRegistry
/** Runtime meta class registry.  \ingroup ccl_base  */
//************************************************************************************************

class MetaClassRegistry: public ITypeLibrary
{
public:
	MetaClassRegistry ();

	PROPERTY_VARIABLE (CStringPtr, libName, LibName)

	void append (MetaClass* metaClass);
	bool remove (MetaClassRef metaClass);

	const MetaClass* findType (CStringPtr persistentName) const;
	const MetaClass* findType (UIDRef cid) const;

	Object* createObject (CStringPtr persistentName) const;
	Object* createObject (UIDRef cid) const;

	const LinkedList<MetaClass*>& getClasses () const;

protected:
	struct HashEntry
	{
		CStringPtr className;
		MetaClass* metaClass;

		HashEntry (CStringPtr className = nullptr, MetaClass* metaClass = nullptr)
		: className (className),
		  metaClass (metaClass)
		{}

		bool operator == (const HashEntry& entry)
		{ return ::strcmp (className, entry.className) == 0; }

		static int getHashCode (const HashEntry& e, int size);
	};

	typedef LinkedList<HashEntry> HashEntryList;

	LinkedList<MetaClass*> classes;
	HashTable<HashEntry, HashEntryList> classNameTable;

	// ITypeLibrary
	DECLARE_UNKNOWN
	CStringPtr CCL_API getLibraryName () const override;
	IUnknownIterator* CCL_API newTypeIterator () const override;
	IUnknownIterator* CCL_API newEnumIterator () const override;
	tresult CCL_API getTypeDetails (ITypeInfoDetails& result, const ITypeInfo& typeInfo) const override;
	const ITypeInfo* findTypeInfo (CStringPtr name) const override;
	const IEnumTypeInfo* findEnumTypeInfo (CStringPtr name) const override;
};

//************************************************************************************************
// DeferredDestroyer
//************************************************************************************************

class DeferredDestroyer: public Object
{
public:
	using Object::cancelSignals;
	
	#if CCL_DEBUG_INTERNAL
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	#endif
};

//************************************************************************************************
// MetaClassReplaceScope
/** Temporarily replaces a class given by className with another class in the class registry.
	Objects of the new class can be created by the given name during this scope.
	This can be useful for adjusting the class to be created by name when loading from an archive.*/
//************************************************************************************************

class MetaClassReplaceScope
{
public:
	MetaClassReplaceScope (CStringPtr className, MetaClassRef newClass);
	~MetaClassReplaceScope ();

private:
	MetaClassRegistry& classes;
	MetaClassRef newClass;
	const MetaClass* oldClass;

	void setClassName (MetaClassRef newClass, CStringPtr className);
};

//************************************************************************************************
// Kernel
/** Global initializers and meta classes.  \ingroup ccl_base
   \see CCL_KERNEL_INIT, CCL_KERNEL_INIT_LEVEL, CCL_KERNEL_TERM, CCL_KERNEL_TERM_LEVEL */
//************************************************************************************************

class Kernel
{
public:
	/** Kernel singleton. */
	static Kernel& instance ();

	/** Execute initializers. */
	bool initialize ();

	/** Execute terminators. */
	void terminate ();

	/** Delete kernel */
	void destroy ();

	/** Initialize up to given level. */
	bool initializeLevel (int level);

	/** Terminate down to given level (does not delete kernel). */
	void terminateLevel (int level);

	/** Add object to be deleted with kernel. */
	void addObject (Object* object, bool globalScope);

	/** Defer destruction of object. */
	void deferDestruction (IUnknown* object);

	/** Get class registry. */
	MetaClassRegistry& getClassRegistry ();

	/** Register public classes in factory. */
	bool registerPublicClasses (ClassFactory& factory, CStringPtr categoryName = nullptr, ClassFilter* filter = nullptr);

	/** Register single meta class in factory. */
	void registerClass (ClassFactory& factory, MetaClassRef mc);

private:
	friend struct KernelInitializer;
	friend struct KernelTerminator;

	typedef LinkedList<Object*> CleanupObjectList;

	Kernel ();
	void cleanupObjects (CleanupObjectList& list);

	CleanupObjectList preInitObjects;
	CleanupObjectList objects;

	struct KernelDestroyer;
	static KernelDestroyer theKernelDestroyer;
	static Kernel* theInstance;
	static bool destroyed;
	DeferredDestroyer destroyer;
	MetaClassRegistry classes;
	LinkedList<KernelInitializer> initializer;
	LinkedList<KernelInitializer> initializerDone;
	LinkedList<KernelTerminator> terminator;
	LinkedList<KernelTerminator> terminatorDone;
	bool initialized = false;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// MetaClassRegistry inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void MetaClassRegistry::append (MetaClass* metaClass)
{
	classes.append (metaClass);
	classNameTable.add (HashEntry (metaClass->getPersistentName (), metaClass));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline const LinkedList<MetaClass*>& MetaClassRegistry::getClasses () const
{ return classes; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_kernel_h
