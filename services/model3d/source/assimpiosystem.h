//************************************************************************************************
//
// 3D Model Importer Library
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
// Filename    : assimpiosystem.h
// Description : Assimp I/O System
//
//************************************************************************************************

#ifndef _assimpiosystem_h
#define _assimpiosystem_h

#include "assimp/IOSystem.hpp"

namespace CCL {

struct INativeFileSystem;

//************************************************************************************************
// AssimpIOSystem
/** Implementation of Assimp::IOSystem that supports the framework's virtual file system. */
//************************************************************************************************

class AssimpIOSystem: public Assimp::IOSystem
{
public:
	AssimpIOSystem ();

	// IOSystem
	bool Exists (const char* path) const override;
	char getOsSeparator () const override;
	Assimp::IOStream* Open (const char* path, const char* mode) override;
	void Close (Assimp::IOStream* file) override;

private:
	static int parseOpenMode (const char* mode);

	INativeFileSystem& fileSystem;
};

} // namespace CCL

#endif // _assimpiosystem_h
