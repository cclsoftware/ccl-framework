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
// Filename    : ccl/platform/shared/vulkan/vulkan3dsupport.h
// Description : Vulkan 3D Support
//
//************************************************************************************************

#ifndef _vulkan3dsupport_h
#define _vulkan3dsupport_h

#include "ccl/platform/shared/vulkan/vulkanimage.h"

#include "ccl/gui/graphics/3d/nativegraphics3d.h"
#include "ccl/gui/graphics/mutableregion.h"

namespace CCL {

//************************************************************************************************
// Vulkan3DVertexFormat
//************************************************************************************************

class Vulkan3DVertexFormat: public Native3DVertexFormat
{
public:
	DECLARE_CLASS (Vulkan3DVertexFormat, Native3DVertexFormat)

	Vulkan3DVertexFormat ();
	
	PROPERTY_OBJECT (VkPipelineVertexInputStateCreateInfo, vertexInputInfo, VertexInputInfo)
	
	bool create (const VertexElementDescription description[], uint32 count, 
				 const IGraphicsShader3D* shader);
	
private:
	Vector<VkVertexInputAttributeDescription> attributeDescription;
	VkVertexInputBindingDescription bindingDescription;
};

//************************************************************************************************
// Vulkan3DBuffer
//************************************************************************************************

class Vulkan3DBuffer: public Native3DGraphicsBuffer
{
public:
	DECLARE_CLASS (Vulkan3DBuffer, Native3DGraphicsBuffer)

	Vulkan3DBuffer ();
	~Vulkan3DBuffer ();

	PROPERTY_OBJECT (VkBuffer, buffer, Buffer)

	bool create (GraphicsBuffer3DType type, BufferUsage3D usage,
				 uint32 sizeInBytes, uint32 strideInBytes, const void* initialData);
	void destroy ();
	
	// Native3DGraphicsBuffer
	void* CCL_API map () override;
	void CCL_API unmap () override;

private:
	VkBufferCreateInfo bufferInfo;
	VkDeviceMemory memory;
	uint32 memoryAlignment;
	int mapCount;
	void* mappedData;
	
	// Native3DGraphicsBuffer
	bool ensureSegmentAlignment (uint32& byteOffset, uint32& size, uint32 stride) const override;
};

//************************************************************************************************
// Vulkan3DTexture2D
//************************************************************************************************

class Vulkan3DTexture2D: public Native3DTexture2D
{
public:
	DECLARE_CLASS (Vulkan3DTexture2D, Native3DTexture2D)
	
	Vulkan3DTexture2D ();
	
	PROPERTY_OBJECT (VulkanImage, image, Image)
	PROPERTY_VARIABLE (VkSamplerAddressMode, addressMode, AddressMode)

	void destroy ();
	
	// Native3DTexture2D
	tresult CCL_API copyFromBitmap (IBitmap* bitmap) override;
	using Native3DTexture2D::create;
	bool create (uint32 width, uint32 height, uint32 bytesPerRow, DataFormat3D format, TextureFlags3D flags, const void* initialData) override;

private:
	Vulkan3DBuffer stagingBuffer;
	uint32 rowSize;
	bool immutable;
	
	void upload ();
};

//************************************************************************************************
// Vulkan3DShader
//************************************************************************************************

class Vulkan3DShader: public Native3DGraphicsShader
{
public:
	DECLARE_CLASS (Vulkan3DShader, Native3DGraphicsShader)

	Vulkan3DShader ();
	~Vulkan3DShader ();

	static const FileType kFileType;
		
	PROPERTY_OBJECT (VkShaderModule, shader, Shader)
	
	bool create (GraphicsShader3DType type, UrlRef path);
	
	// Native3DGraphicsShader
	ITypeInfo* CCL_API getBufferTypeInfo (int bufferIndex) override;
	
private:
	bool load ();
	void reset ();
};

//************************************************************************************************
// Vulkan3DDescriptorSet
//************************************************************************************************

class Vulkan3DDescriptorSet: public Native3DShaderParameterSet
{
public:
	DECLARE_CLASS (Vulkan3DDescriptorSet, Native3DShaderParameterSet)

	Vulkan3DDescriptorSet ();
	~Vulkan3DDescriptorSet ();
	
	VkDescriptorSet getDescriptorSet () const { return descriptorSet; }
	
	// Native3DShaderParameterSet
	tresult CCL_API setVertexShaderParameters (int bufferIndex, IBufferSegment3D* parameters) override;
	tresult CCL_API setPixelShaderParameters (int bufferIndex, IBufferSegment3D* parameters) override;
	tresult CCL_API setTexture (int textureIndex, IGraphicsTexture2D* texture) override;
	
private:
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
	
	void updateDescriptorSet ();
};

//************************************************************************************************
// Vulkan3DResourceManager
//************************************************************************************************

class Vulkan3DResourceManager: public Native3DResourceManager,
							   public StaticSingleton<Vulkan3DResourceManager>
{
public:
	DECLARE_CLASS (Vulkan3DResourceManager, Native3DResourceManager)

	Vulkan3DResourceManager ();

	void shutdown ();
	VkSampler getSampler (VkSamplerAddressMode addressMode, int textureIndex) const;

	const VulkanImage& getNullImage ();
	
private:
	static const int kNumAddressModes = 4;
	mutable FixedSizeVector<FixedSizeVector<VkSampler, Vulkan3DDescriptorSet::kMaxTextureCount>, kNumAddressModes> samplers;
	Vulkan3DTexture2D nullTexture;
	
	// Native3DResourceManager
	Native3DGraphicsShader* loadShader (UrlRef path, GraphicsShader3DType type) override;
	Native3DTexture2D* loadTexture (Bitmap* bitmap, TextureFlags3D flags) override;
};

//************************************************************************************************
// Vulkan3DPipeline
//************************************************************************************************

class Vulkan3DPipeline: public Native3DGraphicsPipeline
{
public:
	DECLARE_CLASS (Vulkan3DPipeline, Native3DGraphicsPipeline)

	Vulkan3DPipeline ();
	~Vulkan3DPipeline ();
	
	VkPipeline getPipeline (VkRenderPass renderpass, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);
	VkPipelineLayout getLayout () const { return pipelineLayout; }
	
	Vulkan3DShader* getShader (int index) const;

	// IGraphicsPipeline3D
	tresult CCL_API setPrimitiveTopology (PrimitiveTopology3D topology) override;
	tresult CCL_API setFillMode (FillMode3D mode) override;
	tresult CCL_API setVertexFormat (IVertexFormat3D* format) override;
	tresult CCL_API setVertexShader (IGraphicsShader3D* shader) override;
	tresult CCL_API setPixelShader (IGraphicsShader3D* shader) override;
	tresult CCL_API setDepthTestParameters (const DepthTestParameters3D& parameters) override;

private:
	enum ShaderIndex
	{
		kVertexShaderIndex = 0,
		kPixelShaderIndex
	};
	
	struct PipelineItem
	{
		VkRenderPass renderpass;
		VkSampleCountFlagBits sampleCount;
		VkPipeline pipeline;
		
		PipelineItem (VkRenderPass renderpass = VK_NULL_HANDLE, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkPipeline pipeline = VK_NULL_HANDLE)
		: renderpass (renderpass),
		  sampleCount (sampleCount),
		  pipeline (pipeline)
		{}
	};
	Vector<PipelineItem> pipelines;
	
	VkPipelineLayout pipelineLayout;
	Vector<SharedPtr<Vulkan3DShader>> shaderList;
	SharedPtr<Vulkan3DVertexFormat> vertexFormat;
	VkPrimitiveTopology topology;
	VkPolygonMode fillMode;
	bool depthTestEnabled;
	bool depthWriteEnabled;
	float depthBias;
	
	bool changed;

	void reset ();
	VkPipeline createPipeline (VkRenderPass renderpass, VkSampleCountFlagBits sampleCount);
	tresult setShader (int index, IGraphicsShader3D* shader);
};

//************************************************************************************************
// Vulkan3DGraphicsFactory
//************************************************************************************************

class Vulkan3DGraphicsFactory: public Native3DGraphicsFactory
{
public:
	DECLARE_CLASS (Vulkan3DGraphicsFactory, Native3DGraphicsFactory)

	// Native3DGraphicsFactory
	IVertexFormat3D* CCL_API createVertexFormat (const VertexElementDescription* description, uint32 count, const IGraphicsShader3D* shader) override;
	IGraphicsBuffer3D* CCL_API createBuffer (GraphicsBuffer3DType type, BufferUsage3D usage, uint32 sizeInBytes, uint32 strideInBytes, const void* initialData = nullptr) override;
	IGraphicsTexture2D* CCL_API createTexture (IBitmap* bitmap, TextureFlags3D flags) override;
	IGraphicsShader3D* CCL_API createShader (GraphicsShader3DType type, UrlRef filename) override;
	IGraphicsShader3D* CCL_API createStockShader (GraphicsShader3DType type, StringID name) override;
	IGraphicsPipeline3D* CCL_API createPipeline () override;
	IShaderParameterSet3D* CCL_API createShaderParameterSet () override;
};

//************************************************************************************************
// Vulkan3DGraphicsContext
//************************************************************************************************

class Vulkan3DGraphicsContext: public Native3DGraphicsDevice
{
public:
	DECLARE_CLASS (Vulkan3DGraphicsContext, Native3DGraphicsDevice)

	Vulkan3DGraphicsContext ();

	PROPERTY_OBJECT (VkRenderPass, renderpass, Renderpass)
	PROPERTY_OBJECT (VkCommandBuffer, commandBuffer, CommandBuffer)
	PROPERTY_OBJECT (Rect, viewport, Viewport)
	PROPERTY_VARIABLE (VkSampleCountFlagBits, sampleCount, SampleCount)
	
	// Native3DGraphicsContext
	tresult CCL_API setPipeline (IGraphicsPipeline3D* pipeline) override;
	tresult CCL_API setVertexBuffer (IGraphicsBuffer3D* buffer, uint32 stride) override;
	tresult CCL_API setIndexBuffer (IGraphicsBuffer3D* buffer, DataFormat3D format) override;
	tresult CCL_API setShaderParameters (IShaderParameterSet3D* parameters) override;
	tresult CCL_API draw (uint32 startVertex, uint32 vertexCount) override;
	tresult CCL_API drawIndexed (uint32 startIndex, uint32 indexCount, int32 baseVertex) override;

protected:
	SharedPtr<Vulkan3DPipeline> pipeline;
	SharedPtr<Vulkan3DBuffer> vertexBuffer;
	SharedPtr<Vulkan3DBuffer> indexBuffer;
	SharedPtr<Vulkan3DDescriptorSet> shaderParameters;
	
	uint32 bufferStride;
	DataFormat3D indexBufferFormat;

	tresult prepareDrawing ();
	bool bindPipeline ();
	void bindDescriptorSet ();
};

//************************************************************************************************
// Vulkan3DSurface
//************************************************************************************************

class Vulkan3DSurface: public Native3DSurface
{
public:
	DECLARE_CLASS (Vulkan3DSurface, Native3DSurface)
	
	Vulkan3DSurface ();
	~Vulkan3DSurface ();

	virtual bool create (VulkanGPUContext* gpuContext, VkFormat format, float scaleFactor, int imageCount);
	virtual void destroy ();
	virtual bool isValid () const;
	
	virtual void invalidate ();
	
	VkSemaphore render (Vulkan3DGraphicsContext& context);
	
	// Native3DSurface
	void setContent (IGraphicsContent3D* content) override;
	void setSize (const Rect& size) override;
	void applyMultisampling (int sampleCount) override;
	
protected:
	VkSampleCountFlagBits sampleCount;

	Vector<VkCommandBuffer> commandBuffers;
	int currentCommandBuffer;

	VkRenderPass renderpass;
	Vector<VkSemaphore> signalSemaphores;

	VulkanImage colorImage;
	VulkanImage depthImage;
	Vector<VulkanImage> resolveImages;
	Vector<VkFramebuffer> framebuffers;

	Rect viewPortRect;
	float scaleFactor;

	VkImage getResolveImage () const;
	VkFramebuffer getFrameBuffer () const;
	VkCommandBuffer getCommandBuffer () const;
	VkSemaphore getSemaphore () const;
	VkCommandBuffer nextCommandBuffer ();
};

//************************************************************************************************
// Vulkan3DSupport
//************************************************************************************************

class Vulkan3DSupport: public INative3DSupport,
					   public StaticSingleton<Vulkan3DSupport>
{
public:
	void shutdown3D ();

	// INative3DSupport
	Native3DGraphicsFactory& get3DFactory () override;
	Native3DSurface* create3DSurface () override;

protected:
	Vulkan3DGraphicsFactory factory;
};

} // namespace CCL

#endif // _vulkan3dsupport_h

