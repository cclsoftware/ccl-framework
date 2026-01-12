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
// Filename    : ccl/base/objectconverter.h
// Description : Object Converter
//
//************************************************************************************************

#ifndef _ccl_objectconverter_h
#define _ccl_objectconverter_h

#include "ccl/base/singleton.h"

#include "ccl/public/base/iconverter.h"
#include "ccl/public/collections/linkedlist.h"
#include "ccl/public/system/ifileitem.h"

namespace CCL {

//************************************************************************************************
/** Converter singleton. 
	\ingroup ccl_base  */
//************************************************************************************************

class ObjectConverter: public Object,
					   public IObjectConverter,
					   public Singleton<ObjectConverter>
{
public:
	ObjectConverter ();
	~ObjectConverter ();

	// IObjectConverter
	void CCL_API registerFilter (IConvertFilter* filter) override;
	void CCL_API unregisterFilter (IConvertFilter* filter) override;
	void CCL_API registerImporter (IImportFilter* filter) override;
	void CCL_API unregisterImporter (IImportFilter* filter) override;
	void CCL_API registerExporter (IExportFilter* filter) override;
	void CCL_API unregisterExporter (IExportFilter* filter) override;

	// IConvertFilter
	tbool CCL_API canConvert (IUnknown* object, UIDRef cid = kNullUID) const override;
	IUnknown* CCL_API convert (IUnknown* object, UIDRef cid = kNullUID) const override;

	// shortcuts: pass the requested interface or class as template argument
	template<class Iface> static Iface* toInterface (IUnknown* object);
	template<class Class> static Class* toClass (IUnknown* object);
	template<class Iface> static bool canConvertToInterface (IUnknown* object);
	template<class Class> static bool canConvertToClass (IUnknown* object);

	// IImportFilter
	tbool CCL_API canImport (IStream& stream, TypeID type, UIDRef cid = kNullUID) const override;
	IUnknown* CCL_API import (IStream& stream, TypeID type, UIDRef cid = kNullUID) const override;

	IUnknown* importText (const void* text, unsigned int byteSize, bool isUnicode = false);

	// IExportFilter
	tbool CCL_API makeFilePromises (IUnknownList& filePromises, IUnknown* object, IUnknown* context) const override;

	const LinkedList<IExportFilter*>& getExporters () const;

	CLASS_INTERFACES (Object)

protected:
	LinkedList<IConvertFilter*> filters;
	LinkedList<IImportFilter*> importers;
	LinkedList<IExportFilter*> exporters;

	Object* tryXmlImport (IStream& stream);

	// IExportFilter
	StringID CCL_API getIdentity () const override;
	tbool CCL_API getFileType (FileType& fileType) const override;
};

//************************************************************************************************
// ConvertFilter
//************************************************************************************************

class ConvertFilter: public Object,
					 public IConvertFilter
{
public:
	// IConvertFilter
	tbool CCL_API canConvert (IUnknown* object, UIDRef cid = kNullUID) const override;
	IUnknown* CCL_API convert (IUnknown* object, UIDRef cid = kNullUID) const override;

	CLASS_INTERFACE (IConvertFilter, Object)
};

//************************************************************************************************
// ImportFilter
//************************************************************************************************

class ImportFilter: public Object,
					public IImportFilter
{
public:
	// IImportFilter
	tbool CCL_API canImport (IStream& stream, TypeID type, UIDRef cid = kNullUID) const override;
	IUnknown* CCL_API import (IStream& stream, TypeID type, UIDRef cid = kNullUID) const override;

	CLASS_INTERFACE (IImportFilter, Object)
};

//************************************************************************************************
// ExportFilter
//************************************************************************************************

class ExportFilter: public Object,
					public IExportFilter
{
public:
	// IExportFilter
	StringID CCL_API getIdentity () const override;
	tbool CCL_API getFileType (FileType& fileType) const override;
	tbool CCL_API makeFilePromises (IUnknownList& filePromises, IUnknown* object, IUnknown* context) const override;

	CLASS_INTERFACE (IExportFilter, Object)
};

//************************************************************************************************
// FilePromise
//************************************************************************************************

class FilePromise: public Object,
				   public IFilePromise
{
public:
	// IFileDescriptor
	tbool CCL_API getTitle (String& title) const override;
	tbool CCL_API getFileName (String& fileName) const override;
	tbool CCL_API getFileType (FileType& fileType) const override;
	tbool CCL_API getFileSize (int64& fileSize) const override;
	tbool CCL_API getFileTime (DateTime& fileTime) const override;
	tbool CCL_API getMetaInfo (IAttributeList& a) const override;

	// IFilePromise
	tbool CCL_API isAsync () const override;
	tresult CCL_API createFile (UrlRef destPath, IProgressNotify* progress = nullptr) override;

	CLASS_INTERFACE2 (IFileDescriptor, IFilePromise, Object)

	// simplified registration of a filter that delegates to a static create () function
	template<class PromiseClass> static void registerExporter ();
	template<class PromiseClass> class Exporter;

protected:
	virtual tresult CCL_API createFile (IStream& stream, IProgressNotify* progress = nullptr);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ObjectConverter inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Iface> inline Iface* ObjectConverter::toInterface (IUnknown* object)
{
	AutoPtr<IUnknown> unk = ObjectConverter::instance ().convert (object, ccl_iid<Iface> ());
	UnknownPtr<Iface> iface (static_cast<IUnknown*> (unk));
	return iface.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Class> inline Class* ObjectConverter::toClass (IUnknown* object)
{
	ASSERT (ccl_typeid<Class> ().getClassID ().isValid ())
	AutoPtr<IUnknown> unk = ObjectConverter::instance ().convert (object, ccl_typeid<Class> ().getClassID ());
	Class* obj = unknown_cast<Class> (unk);
	return return_shared (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Iface> inline bool ObjectConverter::canConvertToInterface (IUnknown* object)
{
	return ObjectConverter::instance ().canConvert (object, ccl_iid<Iface> ()) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Class> inline bool ObjectConverter::canConvertToClass (IUnknown* object)
{
	ASSERT (ccl_typeid<Class> ().getClassID ().isValid ())
	return ObjectConverter::instance ().canConvert (object, ccl_typeid<Class> ().getClassID ()) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// FilePromise inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class PromiseClass>
class FilePromise::Exporter: public ExportFilter
{
	tbool CCL_API makeFilePromises (IUnknownList& filePromises, IUnknown* object, IUnknown* context) const override
	{
		return PromiseClass::create (filePromises, object, context);
	}
};

template<class PromiseClass> inline void FilePromise::registerExporter ()
{
	ObjectConverter::instance ().registerExporter (AutoPtr<IExportFilter> (NEW FilePromise::Exporter<PromiseClass>));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_objectconverter_h
