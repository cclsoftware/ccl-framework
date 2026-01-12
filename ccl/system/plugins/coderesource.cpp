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
// Filename    : ccl/system/plugins/coderesource.cpp
// Description : Code Resources
//
//************************************************************************************************

#include "ccl/system/plugins/coderesource.h"

#include "ccl/main/cclmodmain.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/storage/iurl.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/plugins/iclassfactory.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// CodeResource
//************************************************************************************************

DEFINE_CLASS (CodeResource, Object)
DEFINE_CLASS_NAMESPACE (CodeResource, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

CodeResource::CodeResource (IClassFactory* classFactory)
: classFactory (classFactory)
{
	if(classFactory)
		classFactory->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CodeResource::~CodeResource ()
{
	if(classFactory)
		classFactory->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API CodeResource::getType () const
{
	return CodeResourceType::kNative;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IClassFactory* CCL_API CodeResource::getClassFactory ()
{
	return classFactory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* CCL_API CodeResource::getMetaInfo ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CodeResource::getPath (IUrl& path) const
{
	return false;
}

//************************************************************************************************
// CodeResourceLoader
//************************************************************************************************

DEFINE_CLASS (CodeResourceLoader, Object)
DEFINE_CLASS_NAMESPACE (CodeResourceLoader, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API CodeResourceLoader::getType () const
{
	return CodeResourceType::kNative;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CodeResourceLoader::isCodeResource (UrlRef path)
{
	bool result = path.getFileType () == FileTypes::Module ();
	#if CCL_PLATFORM_MAC
	String ext;
	path.getExtension (ext);
	result |= ext.compare ("dylib", false) == 0;
	#endif
	return result;

}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CodeResourceLoader::loadCodeResource (ICodeResource*& codeResource, UrlRef path)
{
	codeResource = nullptr;
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CodeResourceLoader::isKnownLocation (UrlRef path)
{
	return false;
}

//************************************************************************************************
// NativeCodeResource
//************************************************************************************************

NativeCodeResource::NativeCodeResource (IExecutableImage& image)
: image (image)
{
	TRY
	{
		CCLModuleMainProc moduleMain = (CCLModuleMainProc)image.getFunctionPointer ("CCLModuleMain");
		if(moduleMain)
		{
			if(!(*moduleMain) (image.getNativeReference (), kModuleInit))
			{
				CCL_WARN("CCLModuleMain failed \n", 0)
				return;
			}
		}

		CCLGetClassFactoryProc getClassFactory = (CCLGetClassFactoryProc)image.getFunctionPointer ("CCLGetClassFactory");
		if(getClassFactory)
			classFactory = (*getClassFactory) ();
	}
	EXCEPT
	{}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeCodeResource::~NativeCodeResource ()
{
	TRY
	{
		if(classFactory)
			classFactory->release (),
			classFactory = nullptr;

		CCLModuleMainProc moduleMain = (CCLModuleMainProc)image.getFunctionPointer ("CCLModuleMain");
		if(moduleMain)
			(*moduleMain) (image.getNativeReference (), kModuleExit);

		image.release ();
	}
	EXCEPT
	{}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeCodeResource::queryInterface (UIDRef iid, void** ptr)
{
	// make IExecutableImage accessible
	if(iid == ccl_iid<IExecutableImage> ())
		return image.queryInterface (iid, ptr);

	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* CCL_API NativeCodeResource::getMetaInfo ()
{
	return const_cast<IAttributeList*> (image.getMetaInfo ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeCodeResource::getPath (IUrl& path) const
{
	return image.getPath (path);
}

//************************************************************************************************
// NativeCodeLoader
//************************************************************************************************

DEFINE_SINGLETON (NativeCodeLoader)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeCodeLoader::loadCodeResource (ICodeResource*& codeResource, UrlRef path)
{
	IExecutableImage* nativeImage = nullptr;
	tresult result = System::GetExecutableLoader ().loadImage (nativeImage, path);
	if(result == kResultOk)
	{
		NativeCodeResource* nativeResource = NEW NativeCodeResource (*nativeImage);
		if(nativeResource->getClassFactory ())
		{
			codeResource = nativeResource;
			return kResultOk;
		}

		CCL_WARN ("Could not get Class Factory for %s\n", MutableCString (path.getPath ()).str ())
		nativeResource->release ();
		result = kResultFailed;
	}
	codeResource = nullptr;
	return result;
}
