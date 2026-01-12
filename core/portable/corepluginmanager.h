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
// Filename    : core/portable/corepluginmanager.h
// Description : Plug-in Management
//
//************************************************************************************************

#ifndef _corepluginmanager_h
#define _corepluginmanager_h

#include "core/portable/coreattributes.h"
#include "core/portable/coresingleton.h"

#include "core/public/coreplugin.h"
#include "core/public/coreuid.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// CodeResource
//************************************************************************************************

class CodeResource
{
public:
	virtual ~CodeResource () {}
	virtual CStringPtr getResourceName () const = 0;
	virtual const Plugins::ClassInfoBundle* getClassInfoBundle () = 0;
};

//************************************************************************************************
// BuiltInCodeResource
//************************************************************************************************

class BuiltInCodeResource: public CodeResource
{
public:
	BuiltInCodeResource (CStringPtr name, Plugins::GetClassInfoBundleProc entryPoint);

	// CodeResource
	CStringPtr getResourceName () const override;
	const Plugins::ClassInfoBundle* getClassInfoBundle () override;

protected:
	CStringPtr name;
	const Plugins::ClassInfoBundle* classInfoBundle;
};

//************************************************************************************************
// PluginManager
//************************************************************************************************

class PluginManager: public StaticSingleton<PluginManager>
{
public:
	~PluginManager ();

	void addCodeResource (CodeResource* codeResource); // takes ownership!

	const Plugins::ClassInfo* findClass (UIDRef classId) const;

	typedef Vector<const Plugins::ClassInfo*> ClassList;
	void collectClasses (ClassList& classList, CStringPtr classType) const;

protected:
	class BuiltInResource;

	Vector<CodeResource*> codeResources;
};

//************************************************************************************************
// ClassAttributeReader
//************************************************************************************************

class ClassAttributeReader
{
public:
	ClassAttributeReader (const Plugins::ClassInfo& classInfo);

	bool getValue (CString64& value, CStringPtr key) const;

protected:
	const Plugins::ClassInfo& classInfo;
};

//************************************************************************************************
// AuthorizationPolicy
//************************************************************************************************

namespace AuthorizationPolicy
{
	const CStringPtr kTypeID = "__typeid";
	const CStringPtr kSID = "sid";
	const CStringPtr kChildren = "children";
	const CStringPtr kAny = "*";

	// policy item types
	const CStringPtr kResource = "AuthResource";
	const CStringPtr kClient = "AuthClient";
	const CStringPtr kAccessDenied = "AccessDenied";
	const CStringPtr kAccessAllowed = "AccessAllowed";

	const Attributes* findItemOfType (const Attributes& parent, CStringPtr sid, CStringPtr typeId);
	const Attributes* findMatchingItem (const Attributes& parent, CStringPtr sid);
	bool checkAccess (const Attributes& parent, CStringPtr sid);
}

} // namespace Portable
} // namespace Core

#endif // _corepluginmanager_h
