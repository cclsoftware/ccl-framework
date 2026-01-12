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
// Filename    : ccl/base/objectconverter.cpp
// Description : Object Converter
//
//************************************************************************************************

#include "ccl/base/objectconverter.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/xmlarchive.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// ObjectConverter
//************************************************************************************************

DEFINE_SINGLETON (ObjectConverter)

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectConverter::ObjectConverter ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectConverter::~ObjectConverter ()
{
	ListForEach (filters, IConvertFilter*, filter)
		filter->release ();
	EndFor

	ListForEach (importers, IImportFilter*, filter)
		filter->release ();
	EndFor

	ListForEach (exporters, IExportFilter*, filter)
		filter->release ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ObjectConverter::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IObjectConverter)
	QUERY_INTERFACE (IConvertFilter)
	QUERY_INTERFACE (IImportFilter)
	QUERY_INTERFACE (IExportFilter)
	return Object::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ObjectConverter::registerFilter (IConvertFilter* filter)
{
	ASSERT (filter != nullptr)
	if(filter)
	{
		filters.append (filter);
		filter->retain ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ObjectConverter::unregisterFilter (IConvertFilter* filter)
{
	bool result = filters.remove (filter);
	ASSERT (result == true)
	if(result)
		filter->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ObjectConverter::registerImporter (IImportFilter* filter)
{
	ASSERT (filter != nullptr)
	if(filter)
	{
		importers.append (filter);
		filter->retain ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ObjectConverter::unregisterImporter (IImportFilter* filter)
{
	bool result = importers.remove (filter);
	ASSERT (result == true)
	if(result)
		filter->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ObjectConverter::registerExporter (IExportFilter* filter)
{
	ASSERT (filter != nullptr)
	if(filter)
	{
		exporters.append (filter);
		filter->retain ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ObjectConverter::unregisterExporter (IExportFilter* filter)
{
	bool result = exporters.remove (filter);
	ASSERT (result == true)
	if(result)
		filter->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectConverter::canConvert (IUnknown* object, UIDRef cid) const
{
	if(object == nullptr)
		return false;

	IUnknown* iface = nullptr;
	object->queryInterface (cid, (void**)&iface);
	if(iface)
	{
		iface->release ();
		return true;
	}

	UnknownPtr<IObject> iObject (object);
	if(iObject && iObject->getTypeInfo ().getClassID () == cid)
		return true;

	ListForEach (filters, IConvertFilter*, filter)
		if(filter->canConvert (object, cid))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ObjectConverter::convert (IUnknown* object, UIDRef cid) const
{
	if(!cid.isValid () || object == nullptr)
		return nullptr;

	// check if object already has the requested interface
	IUnknown* iface = nullptr;
	object->queryInterface (cid, (void**)&iface);
	if(iface)
		return iface;

	// check if object is already of requested class
	UnknownPtr<IObject> iObject (object);
	if(iObject && iObject->getTypeInfo ().getClassID () == cid)
	{
		object->retain ();
		return object;
	}

	// try filters
	ListForEach (filters, IConvertFilter*, filter)
		if(filter->canConvert (object, cid))
		{
			IUnknown* result = filter->convert (object, cid);
			if(result)
				return result;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectConverter::canImport (IStream& stream, TypeID type, UIDRef cid) const
{
	ListForEach (importers, IImportFilter*, filter)
		if(filter->canImport (stream, type, cid))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ObjectConverter::import (IStream& stream, TypeID type, UIDRef cid) const
{
	ListForEach (importers, IImportFilter*, filter)
		if(filter->canImport (stream, type, cid))
		{
			IUnknown* result = filter->import (stream, type, cid);
			if(result)
				return result;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ObjectConverter::importText (const void* text, unsigned int byteSize, bool isUnicode)
{
	MemoryStream ms (const_cast<void*> (text), byteSize);

	// try to load as XML archive first
	if(Object* obj = tryXmlImport (ms))
		return ccl_as_unknown (obj);

	return import (ms, isUnicode ? "UNICODE" : "TEXT");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* ObjectConverter::tryXmlImport (IStream& stream)
{
	XmlArchive archive (stream);
	archive.silentOnErrors (true);
	AutoPtr<Attributes> attributes = NEW Attributes;
	if(archive.loadAttributes (XmlArchive::defaultRootTag, *attributes))
	{
		attributes->retain ();
		return attributes;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API ObjectConverter::getIdentity () const
{
	CCL_DEBUGGER ("Must not be called!\n")
	return CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectConverter::getFileType (FileType& fileType) const
{
	CCL_DEBUGGER ("Must not be called!\n")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectConverter::makeFilePromises (IUnknownList& filePromises, IUnknown* object, IUnknown* context) const
{
	// check if object already is a file promise
	UnknownPtr<IFilePromise> fp (object);
	if(fp)
	{
		filePromises.add (fp, true);
		return true;
	}

	// try filters
	tbool result = false;
	ListForEach (exporters, IExportFilter*, filter)
		if(filter->makeFilePromises (filePromises, object, context))
			result = true;
	EndFor
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const LinkedList<IExportFilter*>& ObjectConverter::getExporters () const
{
	return exporters;
}

//************************************************************************************************
// ConvertFilter
//************************************************************************************************

tbool CCL_API ConvertFilter::canConvert (IUnknown* object, UIDRef cid) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ConvertFilter::convert (IUnknown* object, UIDRef cid) const
{
	return nullptr;
}

//************************************************************************************************
// ImportFilter
//************************************************************************************************

tbool CCL_API ImportFilter::canImport (IStream& stream, TypeID type, UIDRef cid) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ImportFilter::import (IStream& stream, TypeID type, UIDRef cid) const
{
	return nullptr;
}

//************************************************************************************************
// ExportFilter
//************************************************************************************************

StringID CCL_API ExportFilter::getIdentity () const
{
	return CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExportFilter::getFileType (FileType& fileType) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExportFilter::makeFilePromises (IUnknownList& filePromises, IUnknown* object, IUnknown* context) const
{
	return false;
}

//************************************************************************************************
// FilePromise
//************************************************************************************************

tbool CCL_API FilePromise::getTitle (String& title) const
{
	return getFileName (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FilePromise::getFileName (String& fileName) const
{
	CCL_NOT_IMPL ("Must be implemented by derived class!\n")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FilePromise::getFileType (FileType& fileType) const
{
	CCL_NOT_IMPL ("Must be implemented by derived class!\n")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FilePromise::getFileSize (int64& fileSize) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FilePromise::getFileTime (CCL::DateTime& fileTime) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FilePromise::getMetaInfo (IAttributeList& a) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FilePromise::isAsync () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FilePromise::createFile (UrlRef destPath, IProgressNotify* progress)
{
	ASSERT (!isAsync ())
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (destPath, IStream::kCreateMode);
	return stream ? createFile (*stream, progress) : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FilePromise::createFile (IStream& stream, IProgressNotify* progress)
{
	CCL_NOT_IMPL ("Must be implemented by derived class!\n")
	return kResultNotImplemented;
}
