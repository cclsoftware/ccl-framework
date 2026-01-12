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
// Filename    : ccl/base/storage/binaryarchive.h
// Description : Binary Archive
//
//************************************************************************************************

#ifndef _ccl_binaryarchive_h
#define _ccl_binaryarchive_h

#include "ccl/base/storage/archive.h"

namespace CCL {

class FileType;

//************************************************************************************************
// BinaryArchive
/** Binary storage archive */
//************************************************************************************************

class BinaryArchive: public Archive
{
public:
	BinaryArchive (IStream& stream, Attributes* context = nullptr, StringID saveType = nullptr);

	static const FileType& getFileType ();

	// Archive
	ArchiveType getArchiveType () const override { return kBinaryArchive; }
	bool isAnonymous () const override { return false; }
	bool saveAttributes (ObjectID root, const Attributes& attributes) override;
	bool loadAttributes (ObjectID root, Attributes& attributes) override;

protected:
	bool writeAttributes (const Attributes& attributes);
	bool writeObject (const Object& object);

	bool readAttributes (Attributes& attributes);
	Object* readObject (bool typeNeeded = false);

	class ArchiveString;
};

} // namespace CCL

#endif // _ccl_binaryarchive_h
