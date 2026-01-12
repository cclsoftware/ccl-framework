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
// Filename    : modelimporter.cpp
// Description : Model Importer
//
//************************************************************************************************

#include "modelimporter.h"
#include "assimpiosystem.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/storage/iurl.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/translation.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (OBJFile, "3D Object")
END_XSTRINGS

//************************************************************************************************
// ModelImporter
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ModelImporter, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ModelImporter::ModelImporter (const FileType& fileType)
: fileType (fileType)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API ModelImporter::getFileType () const
{
	return fileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ModelImporter::importModel (IModel3D& model, UrlRef path)
{
	AssimpIOSystem handler;
	Assimp::Importer importer;
	importer.SetIOHandler (&handler);

	String strPath;
	path.getUrl (strPath);

	// The lifetime of the returned scene is tied to the lifetime of the importer.
	const aiScene* scene = importer.ReadFile (MutableCString (strPath, Text::kUTF8),
		aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_SortByPType | aiProcess_MakeLeftHanded);

	// Explicitly unlink the IO handler. Otherwise, Assimp will try to free it.
	importer.SetIOHandler (nullptr);

	if(scene)
	{
		// Cameras
		for(unsigned int i = 0; i < scene->mNumCameras; i++)
		{
		}

		// Lights
		for(unsigned int i = 0; i < scene->mNumLights; i++)
		{
		}

		// Materials
		for(unsigned int i = 0; i < scene->mNumMaterials; i++)
		{
		}

		// Meshes
		for(unsigned int i = 0; i < scene->mNumMeshes; i++)
		{
			const aiMesh* mesh = scene->mMeshes[i];

			AutoPtr<IGeometry3D> geometry = model.createGeometry ();
			geometry->setPrimitiveTopology (kPrimitiveTopologyTriangleList);

			const PointF3D* positions = nullptr;
			const PointF3D* normals = nullptr;
			const PointF* textureCoords = nullptr;

			if(mesh->HasPositions ())
				positions = reinterpret_cast<const PointF3D*>(mesh->mVertices);

			if(mesh->HasNormals ())
				normals = reinterpret_cast<const PointF3D*>(mesh->mNormals);

			if(mesh->HasTextureCoords (0))
				textureCoords = reinterpret_cast<const PointF*>(mesh->mTextureCoords[0]);

			geometry->setVertexData (positions, normals, textureCoords, mesh->mNumVertices);

			if(mesh->HasFaces ())
			{
				Vector<uint32> indices;
				indices.resize (mesh->mNumFaces * 3);

				for(unsigned int j = 0; j < mesh->mNumFaces; j++)
				{
					aiFace &face = mesh->mFaces[j];
					ASSERT (face.mNumIndices == 3)

					indices.add (face.mIndices[0]);
					indices.add (face.mIndices[1]);
					indices.add (face.mIndices[2]);
				}

				geometry->setIndices (indices.getItems (), indices.count ());
			}

			model.addGeometry (geometry.detach ());
		}

		// Textures
		for(unsigned int i = 0; i < scene->mNumMeshes; i++)
		{
		}

		return kResultOk;
	}

	return kResultFailed;
}
//************************************************************************************************
// OBJImporter
//************************************************************************************************

ClassDesc OBJImporter::getDescription ()
{
	return 
	{
		UID (0x6f12ca6e, 0x223c, 0x4844, 0xa7, 0x9f, 0x7f, 0x74, 0x8c, 0xa8, 0x65, 0xf), 
		PLUG_CATEGORY_MODELIMPORTER3D,
		XSTR_REF (OBJFile).getKey ()
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////

OBJImporter::OBJImporter ()
: ModelImporter ({nullptr, "obj", "text"})
{
	FileTypes::init (fileType, XSTR (OBJFile));
}

