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
// Filename    : ccl/gui/graphics/3d/scene/scenerenderer3d.h
// Description : 3D Scene Renderer
//
//************************************************************************************************

#ifndef _ccl_scenerenderer3d_h
#define _ccl_scenerenderer3d_h

#include "ccl/gui/graphics/3d/scene/scene3d.h"

namespace CCL {

interface IRenderOperation3D;

//************************************************************************************************
// SceneRenderer3D
//************************************************************************************************

class SceneRenderer3D: public Object,
					   public IGraphicsContent3D,
					   public ISceneRenderer3D
{
public:
	DECLARE_CLASS (SceneRenderer3D, Object)

	SceneRenderer3D ();

	PROPERTY_SHARED_AUTO (Scene3D, scene, Scene)
	PROPERTY_SHARED_AUTO (Camera3D, activeCamera, ActiveCamera)

	PROPERTY_AUTO_POINTER (IShaderBufferWriter3D, materialParameterWriter, MaterialParameterWriter)
	PROPERTY_AUTO_POINTER (IShaderBufferWriter3D, lightParameterWriter, LightParameterWriter)
	PROPERTY_AUTO_POINTER (IShaderBufferWriter3D, vertexShaderWriter, VertexShaderWriter)
	PROPERTY_SHARED_AUTO (ITypeInfo, vertexShaderParameterTypeInfo, VertexShaderParameterTypeInfo)
	PROPERTY_SHARED_AUTO (ITypeInfo, lightParameterTypeInfo, LightParameterTypeInfo)
	PROPERTY_AUTO_POINTER (IBufferSegment3D, lightParameterBuffer, LightParameterBuffer)
	PROPERTY_AUTO_POINTER (IVertexFormat3D, vertexFormatPN, VertexFormatPN)
	PROPERTY_AUTO_POINTER (IVertexFormat3D, vertexFormatPNT, VertexFormatPNT)
	PROPERTY_AUTO_POINTER (IVertexFormat3D, billboardVertexFormat, BillboardVertexFormat)
	PROPERTY_AUTO_POINTER (IGraphicsShader3D, vertexShaderPN, VertexShaderPN)
	PROPERTY_AUTO_POINTER (IGraphicsShader3D, vertexShaderPNT, VertexShaderPNT)
	PROPERTY_AUTO_POINTER (IGraphicsShader3D, convertingVertexShader, ConvertingVertexShader)
	PROPERTY_AUTO_POINTER (IGraphicsShader3D, billboardVertexShader, BillboardVertexShader)

	PROPERTY_AUTO_POINTER (IBufferAllocator3D, allocator, Allocator)

	struct MaterialParameterItem;
	struct ShaderParameterItem;
	struct PipelineItem;
	
	void sceneChanged ();

	Vector<MaterialParameterItem>& getMaterialParameters () { return materialParameters; }
	Vector<PipelineItem>& getPipelines () { return pipelines; }
	Vector<ShaderParameterItem>& getShaderParameters () { return shaderParameters; }
	MaterialParameterItem* findMaterialParameters (Material3D* material) const;
	ShaderParameterItem* findShaderParameters (ModelNode3D* node, int geometryIndex) const;
	PipelineItem* findPipeline (BaseGeometry3D* geometry, Material3D* material) const;

	// IGraphicsContent3D
	tresult CCL_API createContent (IGraphicsFactory3D& factory) override;
	tresult CCL_API releaseContent () override;
	tresult CCL_API renderContent (IGraphics3D& graphics) override;
	tresult CCL_API getContentProperty (Variant& value, ContentProperty3D propertyId) const override;

	// ISceneRenderer3D
	int CCL_API getMultisamplingFactor () const override { return multisamplingFactor; }
	tresult CCL_API setMultisamplingFactor (int factor) override { multisamplingFactor = factor; return kResultOk; }

	CLASS_INTERFACE2 (IGraphicsContent3D, ISceneRenderer3D, Object)

	static constexpr int kDefaultMultisamplingFactor = 4;

private:
	Vector<MaterialParameterItem> materialParameters;
	Vector<ShaderParameterItem> shaderParameters;
	Vector<PipelineItem> pipelines;
	
	ObjectArray pointLights;

	bool needsSceneUpdate;
	bool needsUpdate;

	int multisamplingFactor;
	
	tresult walkSceneNodes (SceneNode3D* node, IRenderOperation3D& operation);
	
	void updateModelNodes ();
	void updatePointLights ();
	void updateLightParameters ();
	void updateScene ();
	void update ();

	void updateTextures ();
	
	// ISceneRenderer3D
	IScene3D* CCL_API getIScene () const override { return scene; }
	ICamera3D* CCL_API getActiveICamera () const override { return getActiveCamera (); }
	tresult CCL_API setActiveICamera (ICamera3D* camera) override { setActiveCamera (unknown_cast<Camera3D> (camera)); return kResultOk; }
};

//************************************************************************************************
// SceneRenderer3D::MaterialParameterItem
//************************************************************************************************

struct SceneRenderer3D::MaterialParameterItem
{
	SharedPtr<Material3D> material;
	AutoPtr<IBufferSegment3D> materialParameterBuffer;

	MaterialParameterItem (Material3D* material = nullptr);
};

//************************************************************************************************
// SceneRenderer3D::ShaderParameterItem
//************************************************************************************************

struct SceneRenderer3D::ShaderParameterItem
{
	SharedPtr<ModelNode3D> node;
	int geometryIndex;
	AutoPtr<IShaderParameterSet3D> parameterSet;
	AutoPtr<IBufferSegment3D> vertexParameterBuffer;
	SceneRenderer3D* renderer;

	ShaderParameterItem (SceneRenderer3D* renderer = nullptr, ModelNode3D* node = nullptr, int geometryIndex = 0);

	bool operator == (const ShaderParameterItem& other) const;
	bool operator > (const ShaderParameterItem& other) const;

	Material3D* getMaterial () const;
	BaseGeometry3D* getGeometry () const;
};

//************************************************************************************************
// SceneRenderer3D::PipelineItem
//************************************************************************************************

struct SceneRenderer3D::PipelineItem
{
	enum PipelineFlags
	{
		kIsBillboard = 1 << 0,
		kIsTranslucent = 1 << 1
	};

	IGraphicsShader3D* pixelShader;
	PrimitiveTopology3D topology;
	AutoPtr<IGraphicsPipeline3D> pipeline;
	float depthBias;
	int flags;

	PipelineItem (IGraphicsShader3D* pixelShader = nullptr, BaseGeometry3D* geometry = nullptr, Material3D* material = nullptr);
	static int getFlags (BaseGeometry3D* geometry, Material3D* material);
};

} // namespace CCL

#endif // _ccl_scenerenderer3d_h
