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
// Filename    : ccl/gui/graphics/3d/nativegraphics3d.h
// Description : Native 3D Graphics classes
//
//************************************************************************************************

#ifndef _ccl_nativegraphics3d_h
#define _ccl_nativegraphics3d_h

#include "ccl/base/singleton.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/text/cstring.h"

#include "ccl/public/gui/graphics/rect.h"
#include "ccl/public/gui/graphics/3d/igraphics3d.h"

namespace CCL {

class Native3DGraphicsFactory;
class Bitmap;

//************************************************************************************************
// Native3DSurface
//************************************************************************************************

class Native3DSurface: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (Native3DSurface, Object)

	Native3DSurface ();
	
	PROPERTY_BOOL (dirty, Dirty)

	IGraphicsContent3D* getContent () const { return content; }
	virtual void setContent (IGraphicsContent3D* _content) { content = _content; }

	const Rect& getSize () const { return size; }
	virtual void setSize (const Rect& _size) { size = _size; }
	
	bool hasClearColor () const;
	Color getClearColor () const;

	virtual void applyMultisampling (int sampleCount) {}

protected:
	SharedPtr<IGraphicsContent3D> content;
	Rect size;
};

//************************************************************************************************
// INative3DSupport
//************************************************************************************************

interface INative3DSupport
{
	virtual Native3DGraphicsFactory& get3DFactory () = 0;

	virtual Native3DSurface* create3DSurface () = 0;
};

//************************************************************************************************
// Native3DVertexFormat
//************************************************************************************************

class Native3DVertexFormat: public Object,
                            public IVertexFormat3D
{
public:
	DECLARE_CLASS_ABSTRACT (Native3DVertexFormat, Object)
	
	CLASS_INTERFACE (IVertexFormat3D, Object)
};

//************************************************************************************************
// Native3DGraphicsBuffer
//************************************************************************************************

class Native3DGraphicsBuffer: public Object,
                              public IGraphicsBuffer3D
{
public:
	DECLARE_CLASS_ABSTRACT (Native3DGraphicsBuffer, Object)

	Native3DGraphicsBuffer (uint32 capacity = 0);

	// IGraphicsBuffer3D
	Type CCL_API getType () const override { return type; }
	IBufferSegment3D* CCL_API createSegment (uint32 count, uint32 stride) override;

	CLASS_INTERFACE (IGraphicsBuffer3D, Object)

protected:
	Type type;
	uint32 offset;
	uint32 capacity;

	virtual bool ensureSegmentAlignment (uint32& byteOffset, uint32& size, uint32 stride) const;
};

//************************************************************************************************
// Native3DTexture2D
//************************************************************************************************

class Native3DTexture2D: public Object,
                         public IGraphicsTexture2D
{
public:
	DECLARE_CLASS_ABSTRACT (Native3DTexture2D, Object)

	virtual bool create (IBitmap* bitmap, TextureFlags3D flags);
	virtual bool create (uint32 width, uint32 height, uint32 bytesPerRow, DataFormat3D format, TextureFlags3D flags, const void* initialData) { return false; }

	CLASS_INTERFACE (IGraphicsTexture2D, Object)

protected:
	static uint32 getMipLevels (uint32 width, uint32 height);
	static int getHighestResolutionIndex (IBitmap* bitmap);
};

//************************************************************************************************
// Native3DGraphicsShader
//************************************************************************************************

class Native3DGraphicsShader: public Object,
                              public IGraphicsShader3D
{
public:
	DECLARE_CLASS_ABSTRACT (Native3DGraphicsShader, Object)

	Native3DGraphicsShader (Type type = kVertexShader);

	UrlRef getPath () const { return path; }

	// IGraphicsShader3D
	Type CCL_API getType () const override { return type; }
	const void* CCL_API getBlobAddress () const override;
	uint32 CCL_API getBlobSize () const override;
	ITypeInfo* CCL_API getBufferTypeInfo (int bufferIndex) override;

	CLASS_INTERFACE (IGraphicsShader3D, Object)

protected:
	Type type;
	Url path;
	AutoPtr<Buffer> blob;
	ObjectArray bufferTypeInfos;
};

//************************************************************************************************
// Native3DShaderParameters
//************************************************************************************************

struct Native3DShaderParameters
{
	int bufferIndex;
	SharedPtr<IBufferSegment3D> segment;

	Native3DShaderParameters (int bufferIndex = 0, IBufferSegment3D* segment = nullptr);

	bool operator > (const Native3DShaderParameters& other) const;
};

//************************************************************************************************
// Native3DShaderParameterSet
//************************************************************************************************

class Native3DShaderParameterSet: public Object,
								  public IShaderParameterSet3D
{
public:
	DECLARE_CLASS_ABSTRACT (Native3DShaderParameterSet, Object)

	Native3DShaderParameterSet ();

	const Vector<Native3DShaderParameters>& getVertexShaderParameters () const;
	const Vector<Native3DShaderParameters>& getPixelShaderParameters () const;
	Native3DShaderParameters* findVertexShaderParameters (int bufferIndex) const;
	Native3DShaderParameters* findPixelShaderParameters (int bufferIndex) const;
	IGraphicsTexture2D* getTexture (int textureIndex) const;
	
	// IShaderParameterSet3D
	tresult CCL_API setVertexShaderParameters (int bufferIndex, IBufferSegment3D* parameters) override;
	tresult CCL_API setPixelShaderParameters (int bufferIndex, IBufferSegment3D* parameters) override;
	tresult CCL_API setTexture (int textureIndex, IGraphicsTexture2D* texture) override;

	CLASS_INTERFACE (IShaderParameterSet3D, Object)
	
protected:
	Vector<Native3DShaderParameters> vertexShaderParameters;
	Vector<Native3DShaderParameters> pixelShaderParameters;

	FixedSizeVector<SharedPtr<IGraphicsTexture2D>, kMaxTextureCount> textures;
};

//************************************************************************************************
// Native3DResourceManager
//************************************************************************************************

class Native3DResourceManager: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (Native3DResourceManager, Object)

	Native3DResourceManager ();

	Native3DGraphicsShader* getShader (UrlRef path, GraphicsShader3DType type);
	Native3DTexture2D* getTexture (Bitmap* bitmap, TextureFlags3D flags);
	void removeAll ();

protected:
	class TextureItem;
	
	ObjectArray shaderList;
	ObjectArray textureList;

	Native3DGraphicsShader* findShader (UrlRef path) const;
	virtual Native3DGraphicsShader* loadShader (UrlRef path, GraphicsShader3DType type) = 0;
	Native3DTexture2D* findTexture (Bitmap* bitmap, TextureFlags3D flags) const;
	virtual Native3DTexture2D* loadTexture (Bitmap* bitmap, TextureFlags3D flags) = 0;
};

//************************************************************************************************
// Native3DGraphicsPipeline
//************************************************************************************************

class Native3DGraphicsPipeline: public Object,
								public IGraphicsPipeline3D
{
public:
	DECLARE_CLASS_ABSTRACT (Native3DGraphicsPipeline, Object)

	CLASS_INTERFACE (IGraphicsPipeline3D, Object)
};

//************************************************************************************************
// Native3DGraphicsFactory
//************************************************************************************************

class Native3DGraphicsFactory: public Object,
                               public IGraphicsFactory3D,
                               public ExternalSingleton<Native3DGraphicsFactory>
{
public:
	DECLARE_CLASS_ABSTRACT (Native3DGraphicsFactory, Object)
	
	// IGraphicsFactory3D
	IShaderBufferWriter3D* CCL_API createShaderBufferWriter () override;

	CLASS_INTERFACE (IGraphicsFactory3D, Object)
};

//************************************************************************************************
// Native3DGraphicsDevice
//************************************************************************************************

class Native3DGraphicsDevice: public Object,
                              public IGraphics3D
{
public:
	DECLARE_CLASS_ABSTRACT (Native3DGraphicsDevice, Object)

	// IGraphics3D
	tresult CCL_API drawGeometry (IGeometry3D* geometry) override;

	CLASS_INTERFACE (IGraphics3D, Object)
};

} // namespace CCL

#endif // _ccl_nativegraphics3d_h
