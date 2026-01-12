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
// Filename    : modelimporter.h
// Description : Model Importer
//
//************************************************************************************************

#ifndef _modelimporter_h
#define _modelimporter_h

#include "ccl/base/object.h"

#include "ccl/public/storage/filetype.h"
#include "ccl/public/plugins/pluginst.h"
#include "ccl/public/gui/graphics/3d/imodel3d.h"

namespace CCL {

//************************************************************************************************
// ModelImporter
//************************************************************************************************

class ModelImporter: public Object,
					 public IModelImporter3D,
					 public PluginInstance
{
public:
	DECLARE_CLASS (ModelImporter, Object)

	ModelImporter (const FileType& fileType = FileType ());

	// IModelImporter3D
	const FileType& CCL_API getFileType () const override;
	tresult CCL_API importModel (IModel3D& model, UrlRef path) override;

	CLASS_INTERFACE2 (IModelImporter3D, IPluginInstance, Object)

protected:
	FileType fileType;
};

//************************************************************************************************
// OBJImporter
// https://de.wikipedia.org/wiki/Wavefront_OBJ
//************************************************************************************************

class OBJImporter: public ModelImporter
{
public:
	OBJImporter ();

	static ClassDesc getDescription ();
};

} // namespace CCL

#endif // _modelimporter_h
