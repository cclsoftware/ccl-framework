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
// Filename    : ccl/base/storage/archive.h
// Description : Archive base class
//
//************************************************************************************************

#ifndef _ccl_archive_h
#define _ccl_archive_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/cclmacros.h"

namespace CCL {

class Object;
class Attributes;
interface IStream;

//************************************************************************************************
// ArchiveType
//************************************************************************************************

enum ArchiveType
{
	kXmlArchive,
	kBinaryArchive,
	kJsonArchive,
	kUBJsonArchive
};

//************************************************************************************************
// Archive
//************************************************************************************************

class Archive
{
public:
	Archive (IStream& stream, Attributes* context = nullptr, StringID saveType = nullptr);
	virtual ~Archive ();

	virtual ArchiveType getArchiveType () const = 0;	///< archive type (XML, JSON, binary, etc.)
	virtual bool isAnonymous () const = 0;				///< true if archive doesn't provide type information
	PROPERTY_VARIABLE (int, flags, Flags)				///< flags specific to archive type

	IStream& getStream ();

	void setContext (Attributes* context);
	Attributes* getContext ();

	PROPERTY_MUTABLE_CSTRING (saveType, SaveType)

	static StringID kSaveTypeUndo;
	static StringID kSaveTypeCopy;
	static StringID kSaveTypePreview; 

	typedef CStringRef ObjectID;

	bool saveObject (ObjectID name, const Object& object);
	bool loadObject (ObjectID name, Object& object);

	virtual bool saveAttributes (ObjectID root, const Attributes& attributes) = 0;
	virtual bool loadAttributes (ObjectID root, Attributes& attributes) = 0;

protected:
	IStream& stream;
	Attributes* context;
};

} // namespace CCL

#endif // _ccl_archive_h
