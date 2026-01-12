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
// Filename    : ccl/base/storage/fileresource.h
// Description : File Resource
//
//************************************************************************************************

#ifndef _ccl_fileresource_h
#define _ccl_fileresource_h

#include "ccl/base/storage/url.h"

#include "ccl/public/storage/ifileresource.h"

namespace CCL {

//************************************************************************************************
// FileResource macros
//************************************************************************************************

#define DELEGATE_FILERESOURCE_METHODS(Class) \
tbool CCL_API setPath (UrlRef path) override { return Class::setPath (path); } \
UrlRef CCL_API getPath () const override { return Class::getPath (); } \
tbool CCL_API open (int mode = 0) override { return Class::open (mode); } \
tbool CCL_API create (int mode = 0) override { return Class::create (mode); } \
tbool CCL_API close () override { return Class::close (); } \
tbool CCL_API isExisting () const override { return Class::isExisting (); } \
tbool CCL_API isOpen () const override { return Class::isOpen (); } \
tbool CCL_API deletePhysical (int mode = 0) override { return Class::deletePhysical (mode); }

//************************************************************************************************
// FileResource
//************************************************************************************************

class FileResource: public Object,
					public IFileResource
{
public:
	DECLARE_CLASS_ABSTRACT (FileResource, Object)

	FileResource ();
	FileResource (UrlRef path);
	FileResource (const FileResource&);
	~FileResource ();

	// IFileResource
	tbool CCL_API setPath (UrlRef path) override;
	UrlRef CCL_API getPath () const override;
	tbool CCL_API open (int mode = 0) override;
	tbool CCL_API create (int mode = 0) override;
	tbool CCL_API close () override;
	tbool CCL_API isExisting () const override;
	tbool CCL_API isOpen () const override;
	tbool CCL_API deletePhysical (int mode = 0) override;

	CLASS_INTERFACE (IFileResource, Object)

protected:
	Url path;
	int openCount;

	virtual bool openFile (int mode) = 0;
	virtual bool createFile (int mode) = 0;
	virtual bool closeFile () = 0;
};

//************************************************************************************************
// FileStreamResource
//************************************************************************************************

class FileStreamResource: public FileResource
{
public:
	FileStreamResource ();
	FileStreamResource (UrlRef path);
	FileStreamResource (const FileStreamResource&);
	~FileStreamResource ();

	void setIOOptions (int options);
	IStream* getStream ();
	
protected:
	IStream* file;
	int options;

	// FileResource overrides:
	bool openFile (int mode) override;
	bool createFile (int mode) override;
	bool closeFile () override;
};

} // namespace CCL

#endif // _ccl_fileresource_h
