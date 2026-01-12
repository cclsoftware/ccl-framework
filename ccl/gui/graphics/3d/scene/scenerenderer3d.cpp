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
// Filename    : ccl/gui/graphics/3d/scene/scenerenderer3d.cpp
// Description : 3D Scene Renderer
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/graphics/3d/scene/scenerenderer3d.h"
#include "ccl/gui/graphics/3d/bufferallocator3d.h"
#include "ccl/gui/graphics/3d/shader/shaderreflection3d.h"

#include "ccl/public/gui/graphics/3d/stockshader3d.h"

namespace CCL {

//************************************************************************************************
// IRenderOperation3D
//************************************************************************************************

interface IRenderOperation3D
{
	virtual tresult processNode (SceneNode3D& node) = 0;
};

//************************************************************************************************
// CreateShaderParameterSetsOperation
//************************************************************************************************

class CreateShaderParameterSetsOperation: public IRenderOperation3D
{
public:
	CreateShaderParameterSetsOperation (SceneRenderer3D& renderer);
	
	// IRenderOperation3D
	tresult processNode (SceneNode3D& node) override;

private:
	SceneRenderer3D& renderer;
	uint32 vertexShaderParametersSize;
};

//************************************************************************************************
// UpdateTexturesOperation
//************************************************************************************************

class UpdateTexturesOperation: public IRenderOperation3D
{
public:
	UpdateTexturesOperation (SceneRenderer3D& renderer);

	// IRenderOperation3D
	tresult processNode (SceneNode3D& node) override;

private:
	SceneRenderer3D& renderer;
};

//************************************************************************************************
// FillBuffersOperation3D
//************************************************************************************************

class FillBuffersOperation3D: public IRenderOperation3D
{
public:
	FillBuffersOperation3D (SceneRenderer3D& renderer);

	// IRenderOperation3D
	tresult processNode (SceneNode3D& node) override;

private:
	SceneRenderer3D& renderer;
};

//************************************************************************************************
// DiscardResourcesOperation3D
//************************************************************************************************

class DiscardResourcesOperation3D: public IRenderOperation3D
{
public:
	// IRenderOperation3D
	tresult processNode (SceneNode3D& node) override;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// CreateShaderParameterSetsOperation
//************************************************************************************************

CreateShaderParameterSetsOperation::CreateShaderParameterSetsOperation (SceneRenderer3D& renderer)
: renderer (renderer),
  vertexShaderParametersSize (0)
{
	if(auto* info = unknown_cast<ShaderTypeInfo3D> (renderer.getVertexShaderParameterTypeInfo ()))
		vertexShaderParametersSize = info->getStructSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CreateShaderParameterSetsOperation::processNode (SceneNode3D& node)
{
	IBufferAllocator3D* allocator = renderer.getAllocator ();
	if(allocator == nullptr)
		return kResultFailed;

	if(ModelNode3D* modelNode = ccl_cast<ModelNode3D> (&node))
	{
		if(Model3D* model = modelNode->getModel ())
		{
			for(int i = 0; i < model->getGeometryCount (); i++)
			{
				ASSERT (renderer.findShaderParameters (modelNode, i) == nullptr)
				
				SceneRenderer3D::ShaderParameterItem shaderParameters (&renderer, modelNode, i);
				
				AutoPtr<IShaderParameterSet3D> parameterSet = Native3DGraphicsFactory::instance ().createShaderParameterSet ();

				if(Native3DShaderParameterSet* parameters = unknown_cast<Native3DShaderParameterSet> (parameterSet))
				{
					parameters->setPixelShaderParameters (kLightParameters, renderer.getLightParameterBuffer ());
					
					shaderParameters.vertexParameterBuffer = allocator->allocateBuffer (IGraphicsBuffer3D::kConstantBuffer, kBufferUsageDynamic, 1, vertexShaderParametersSize);
					if(!shaderParameters.vertexParameterBuffer)
						return kResultOutOfMemory;
					parameters->setVertexShaderParameters (kTransformParameters, shaderParameters.vertexParameterBuffer);

					if(Material3D* material = unknown_cast<Material3D> (model->getMaterialAt (i)))
					{
						SceneRenderer3D::MaterialParameterItem* materialParameters = renderer.findMaterialParameters (material);
						if(materialParameters == nullptr)
						{
							renderer.getMaterialParameters ().add ({ material });
							materialParameters = &renderer.getMaterialParameters ().last ();
						}

						IGraphicsShader3D* pixelShader = material->getPixelShader ();
						if(!materialParameters->materialParameterBuffer.isValid ())
						{
							auto* info = unknown_cast<ShaderTypeInfo3D> (pixelShader ? pixelShader->getBufferTypeInfo (kMaterialParameters) : nullptr);

							materialParameters->materialParameterBuffer = info ? allocator->allocateBuffer (IGraphicsBuffer3D::kConstantBuffer, kBufferUsageDynamic, 1, info->getStructSize ()) : nullptr;
							if(!materialParameters->materialParameterBuffer)
								return kResultOutOfMemory;
						}

						parameters->setPixelShaderParameters (kMaterialParameters, materialParameters->materialParameterBuffer);
						
						bool requiresTextureCoordinates = material->requiresTextureCoordinates ();
						
						BaseGeometry3D* geometry = unknown_cast<BaseGeometry3D> (model->getGeometryAt (i));
						bool hasTextureCoordinates = geometry->getTextureCoords () != nullptr;
						
						SceneRenderer3D::PipelineItem* plItem = renderer.findPipeline (geometry, material);
						if(plItem == nullptr)
						{
							renderer.getPipelines ().add ({ material->getPixelShader (), geometry, material });
							plItem = &renderer.getPipelines ().last ();

							plItem->pipeline = Native3DGraphicsFactory::instance ().createPipeline ();
							if(plItem->pipeline)
							{
								plItem->pipeline->setPrimitiveTopology (geometry->getPrimitiveTopology ());
								#if (0 && DEBUG)
								plItem->pipeline->setFillMode (kFillModeWireframe);
								#endif
								if(get_flag<int> (plItem->flags, SceneRenderer3D::PipelineItem::kIsBillboard))
								{
									plItem->pipeline->setVertexShader (renderer.getBillboardVertexShader ());
									plItem->pipeline->setVertexFormat (renderer.getBillboardVertexFormat ());
								}
								else
								{
									if(hasTextureCoordinates)
									{
										if(requiresTextureCoordinates)
											plItem->pipeline->setVertexShader (renderer.getVertexShaderPNT ());
										else
											plItem->pipeline->setVertexShader (renderer.getConvertingVertexShader ());
										plItem->pipeline->setVertexFormat (renderer.getVertexFormatPNT ());
									}
									else
									{
										ASSERT (requiresTextureCoordinates == false)
										plItem->pipeline->setVertexShader (renderer.getVertexShaderPN ());
										plItem->pipeline->setVertexFormat (renderer.getVertexFormatPN ());
									}
								}
								plItem->pipeline->setPixelShader (pixelShader);
								bool depthTestEnabled = true;
								bool depthWriteEnabled = material->getMaterialHint () != kGraphicsContentTranslucent;
								plItem->pipeline->setDepthTestParameters ({depthTestEnabled, depthWriteEnabled, material->getDepthBias ()});
							}
						}
					}
					
					shaderParameters.parameterSet = return_shared (parameters);
					renderer.getShaderParameters ().addSorted (shaderParameters);
				}
			}
		}
	}

	return kResultOk;
}

//************************************************************************************************
// UpdateTexturesOperation
//************************************************************************************************

UpdateTexturesOperation::UpdateTexturesOperation (SceneRenderer3D& renderer)
: renderer (renderer)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult UpdateTexturesOperation::processNode (SceneNode3D& node)
{
	if(ModelNode3D* modelNode = ccl_cast<ModelNode3D> (&node))
	{
		if(Model3D* model = modelNode->getModel ())
		{
			for(int i = 0; i < model->getGeometryCount (); i++)
			{
				SceneRenderer3D::ShaderParameterItem* shaderParameters = renderer.findShaderParameters (modelNode, i);
				if(shaderParameters == nullptr || !shaderParameters->parameterSet.isValid ())
					continue;

				if(TextureMaterial3D* textureMaterial = unknown_cast<TextureMaterial3D> (model->getMaterialAt (i)))
				{
					for(int i = 0; i < Native3DShaderParameterSet::kMaxTextureCount; i++)
					{
						IGraphicsTexture2D* texture = textureMaterial->getGraphicsTexture (i);
						shaderParameters->parameterSet->setTexture (i, texture);
					}
				}
			}
		}
	}

	return kResultOk;
}

//************************************************************************************************
// FillBuffersOperation3D
//************************************************************************************************

FillBuffersOperation3D::FillBuffersOperation3D (SceneRenderer3D& renderer)
: renderer (renderer)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult FillBuffersOperation3D::processNode (SceneNode3D& node)
{
	IBufferAllocator3D* allocator = renderer.getAllocator ();
	if(allocator == nullptr)
		return kResultFailed;

	if(ModelNode3D* modelNode = ccl_cast<ModelNode3D> (&node))
	{
		if(Model3D* model = modelNode->getModel ())
		{
			for(int i = 0; i < model->getGeometryCount (); i++)
			{
				if(UnknownPtr<IGraphicsResource3D> graphicsResource = model->getGeometryAt (i))
					if(!graphicsResource->isGPUAccessible ())
						graphicsResource->upload (*allocator);
			}
		}
	}

	return kResultOk;
}

//************************************************************************************************
// DiscardResourcesOperation3D
//************************************************************************************************

tresult DiscardResourcesOperation3D::processNode (SceneNode3D& node)
{
	if(ModelNode3D* modelNode = ccl_cast<ModelNode3D> (&node))
	{
		if(Model3D* model = modelNode->getModel ())
		{
			for(int i = 0; i < model->getGeometryCount (); i++)
			{
				if(UnknownPtr<IGraphicsResource3D> graphicsResource = model->getGeometryAt (i))
					graphicsResource->discard ();
			}
		}
	}

	return kResultOk;
}

//************************************************************************************************
// SceneRenderer3D
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SceneRenderer3D, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneRenderer3D::SceneRenderer3D ()
: needsUpdate (false),
  needsSceneUpdate (true),
  multisamplingFactor (kDefaultMultisamplingFactor)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SceneRenderer3D::createContent (IGraphicsFactory3D& factory)
{
	if(!scene.isValid ())
		return kResultFailed;

	allocator = NEW BufferAllocator3D;

	vertexShaderPN = factory.createStockShader (IGraphicsShader3D::kVertexShader, StockShaders::kVertexShaderPN);
	ASSERT (vertexShaderPN)
	if(!vertexShaderPN)
		return kResultFailed;

	vertexShaderPNT = factory.createStockShader (IGraphicsShader3D::kVertexShader, StockShaders::kVertexShaderPNT);
	ASSERT (vertexShaderPNT)
	if(!vertexShaderPNT)
		return kResultFailed;

	convertingVertexShader = factory.createStockShader (IGraphicsShader3D::kVertexShader, StockShaders::kConvertingVertexShader);
	ASSERT (convertingVertexShader)
	if(!convertingVertexShader)
		return kResultFailed;

	billboardVertexShader = factory.createStockShader (IGraphicsShader3D::kVertexShader, StockShaders::kBillboardVertexShader);
	ASSERT (billboardVertexShader)
	if(!billboardVertexShader)
		return kResultFailed;
	
	vertexFormatPN = factory.createVertexFormat (VertexPN::kDescription, ARRAY_COUNT (VertexPN::kDescription), vertexShaderPN);
	ASSERT (vertexFormatPN)
	if(!vertexFormatPN)
		return kResultFailed;
	
	vertexFormatPNT = factory.createVertexFormat (VertexPNT::kDescription, ARRAY_COUNT (VertexPNT::kDescription), vertexShaderPNT);
	ASSERT (vertexFormatPNT)
	if(!vertexFormatPNT)
		return kResultFailed;

	billboardVertexFormat = factory.createVertexFormat (VertexPT::kDescription, ARRAY_COUNT (VertexPT::kDescription), billboardVertexShader);
	ASSERT (billboardVertexFormat)
	if(!billboardVertexFormat)
		return kResultFailed;

	setVertexShaderParameterTypeInfo (vertexShaderPN->getBufferTypeInfo (kTransformParameters));
	
	vertexShaderWriter = factory.createShaderBufferWriter ();
	vertexShaderWriter->setBufferTypeInfo (vertexShaderParameterTypeInfo);
	
	materialParameterWriter = factory.createShaderBufferWriter ();

	needsSceneUpdate = true;
	needsUpdate = true;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SceneRenderer3D::releaseContent ()
{
	DiscardResourcesOperation3D discardOp;
	walkSceneNodes (scene, discardOp);

	vertexShaderWriter.release ();
	materialParameterWriter.release ();
	lightParameterWriter.release ();
	allocator.release ();
	
	pipelines.removeAll ();
	shaderParameters.removeAll ();
	materialParameters.removeAll ();
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SceneRenderer3D::renderContent (IGraphics3D& graphics)
{
	if(!scene || !activeCamera)
		return kResultFalse;

	if(needsUpdate)
		update ();

	Transform3DRef viewTransform = activeCamera->getViewTransform ();
	Transform3DRef projectionTransform = activeCamera->getProjectionTransform ();
	PointF3D cameraPosition;
	activeCamera->getWorldTransform ().getTranslation (cameraPosition);
	
	// CCL_PRINTF ("%s\n", "------------------------------------------")
	// CCL_PRINTF ("%s\n", "start scene rendering")

	for(const ShaderParameterItem& item : shaderParameters)
	{
		const auto& node = item.node;
		if(node == nullptr)
			continue;
		
		BaseGeometry3D* geometry = item.getGeometry ();
		if(geometry == nullptr)
			continue;

		// update vertex shader parameters
		vertexShaderWriter->setBuffer (item.vertexParameterBuffer);
		IShaderValue3D& vertexShaderParameters = vertexShaderWriter->asValue ();
		Transform3D modelViewMatrix = viewTransform * node->getWorldTransform ();
		Transform3D normalMatrix = node->getInverseWorldTransform ();
		normalMatrix.transpose ();
		normalMatrix.resetTranslation ();
		vertexShaderParameters[ParamName3D::kModelMatrix] = node->getWorldTransform ();
		vertexShaderParameters[ParamName3D::kModelViewMatrix] = modelViewMatrix;
		vertexShaderParameters[ParamName3D::kProjectionMatrix] = projectionTransform;
		vertexShaderParameters[ParamName3D::kNormalMatrix] = normalMatrix;
		vertexShaderParameters[ParamName3D::kCameraPosition] = PointF4D (cameraPosition, 1.f);
		vertexShaderWriter->setBuffer (nullptr);

		// update pixel shader parameters
		Material3D* material = item.getMaterial ();
		MaterialParameterItem* materialParameters = findMaterialParameters (material);
		IGraphicsShader3D* pixelShader = material ? material->getPixelShader () : nullptr;
		if(materialParameters && pixelShader)
		{
			materialParameterWriter->setBufferTypeInfo (pixelShader->getBufferTypeInfo (kMaterialParameters));
			materialParameterWriter->setBuffer (materialParameters->materialParameterBuffer);
			IShaderValue3D& pixelShaderParameters = materialParameterWriter->asValue ();
			material->getShaderParameters (pixelShaderParameters);
			materialParameterWriter->setBuffer (nullptr);
		}
		
		updateLightParameters ();

		PipelineItem* pipelineItem = material ? findPipeline (geometry, material) : nullptr;
		if(pipelineItem && pipelineItem->pipeline)
		{
			graphics.setPipeline (pipelineItem->pipeline);
			graphics.setShaderParameters (item.parameterSet);
			graphics.drawGeometry (geometry);

			// CCL_PRINTF ("rendering geometry with %d vertices\n", geometry->getVertexCount ())
		}
	}
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SceneRenderer3D::getContentProperty (Variant& value, ContentProperty3D propertyId) const
{
	switch(propertyId)
	{
	case kContentHint: value = kGraphicsContentTranslucent; return kResultOk;
	case kBackColor: value = Colors::kTransparentBlack; return kResultOk;
	case kMultisampling: value = multisamplingFactor; return kResultOk;
	default: return kResultFailed;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult SceneRenderer3D::walkSceneNodes (SceneNode3D* node, IRenderOperation3D& operation)
{
	tresult result = operation.processNode (*node);

	if(result == kResultFalse)
		return kResultOk; // The call succeeded, but don't process the child nodes
	else if(result != kResultOk)
		return result;

	if(ContainerNode3D* container = ccl_cast<ContainerNode3D> (node))
	{
		for(SceneNode3D* childNode : iterate_as<SceneNode3D> (container->getChildNodes ()))
		{
			result = walkSceneNodes (childNode, operation);
			if(result != kResultOk)
				break;
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneRenderer3D::updateTextures ()
{
	UpdateTexturesOperation operation (*this);
	walkSceneNodes (scene, operation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneRenderer3D::updateModelNodes ()
{
	if(!allocator.isValid ())
		return;
	
	// TODO: only update nodes that actually changed
	shaderParameters.removeAll ();
	materialParameters.removeAll ();

	FillBuffersOperation3D fillOp (*this);
	walkSceneNodes (scene, fillOp);
		
	CreateShaderParameterSetsOperation parameterSetsOp (*this);
	walkSceneNodes (scene, parameterSetsOp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneRenderer3D::updatePointLights ()
{
	pointLights.removeAll ();
	scene->collectNodesOfType<PointLight3D> (pointLights);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneRenderer3D::updateLightParameters ()
{
	if(!lightParameterWriter.isValid () || !lightParameterBuffer.isValid ())
	{
		if(!allocator.isValid ())
			return;
		
		AutoPtr<IGraphicsShader3D> pixelShader = Native3DGraphicsFactory::instance ().createStockShader (IGraphicsShader3D::kPixelShader, StockShaders::kSolidColorMaterialShader);
		ASSERT (pixelShader)
		if(!pixelShader.isValid ())
			return;
		
		lightParameterWriter = Native3DGraphicsFactory::instance ().createShaderBufferWriter ();
		
		setLightParameterTypeInfo (pixelShader->getBufferTypeInfo (kLightParameters));
		if(ShaderTypeInfo3D* info = unknown_cast<ShaderTypeInfo3D> (lightParameterTypeInfo))
			lightParameterBuffer = allocator->allocateBuffer (IGraphicsBuffer3D::kConstantBuffer, kBufferUsageDynamic, 1, info->getStructSize ());
		
		if(!lightParameterBuffer.isValid ())
			return;
		
		lightParameterWriter->setBufferTypeInfo (lightParameterTypeInfo);
	}
	
	lightParameterWriter->setBuffer (lightParameterBuffer);
	
	IShaderValue3D& lightParameters = lightParameterWriter->asValue ();

	// Ambient light
	auto& shaderAmbientLight = lightParameters[ParamName3D::kAmbientLight];
	if(AmbientLight3D* ambientLight = scene->getFirstOfType<AmbientLight3D> ())
		shaderAmbientLight[ParamName3D::kLightColor] = ColorF (ambientLight->getLightColor ());
	else
		shaderAmbientLight[ParamName3D::kLightColor] = Colors::kTransparentBlackF;

	// Directional light
	auto& shaderDirectionalLight = lightParameters[ParamName3D::kDirectionalLight];
	if(DirectionalLight3D* directionalLight = scene->getFirstOfType<DirectionalLight3D> ())
	{
		PointF3D direction;
		directionalLight->getWorldTransform ().getRotation (direction);
		shaderDirectionalLight[ParamName3D::kLightDirection] = PointF4D (direction, 0.0f);
		shaderDirectionalLight[ParamName3D::kLightColor] = ColorF (directionalLight->getLightColor ());
	}
	else
		shaderDirectionalLight[ParamName3D::kLightColor] = Colors::kTransparentBlackF;

	// Point lights
	for(int i = 0; i < StockShaders::kMaxPointLightCount; i++)
	{
		auto& shaderPointLight = lightParameters[ParamName3D::kPointLight][i];
		if(i < pointLights.count ())
		{
			auto pointLight = static_cast<PointLight3D*> (pointLights[i]);
			PointF3D position;
			pointLight->getWorldTransform ().getTranslation (position);
			shaderPointLight[ParamName3D::kLightPosition] = PointF4D (position, 0.0f);
			shaderPointLight[ParamName3D::kLightColor] = ColorF (pointLight->getLightColor ());
			shaderPointLight[ParamName3D::kPointLightConstantTerm] = Variant (pointLight->getConstantTerm ());
			shaderPointLight[ParamName3D::kPointLightLinearFactor] = Variant (pointLight->getLinearFactor ());
			shaderPointLight[ParamName3D::kPointLightQuadraticFactor] = Variant (pointLight->getQuadraticFactor ());
		}
		else
		{
			shaderPointLight[ParamName3D::kLightColor] = Colors::kTransparentBlackF;
			shaderPointLight[ParamName3D::kPointLightConstantTerm] = Variant (0.f);
			shaderPointLight[ParamName3D::kPointLightLinearFactor] = Variant (0.f);
			shaderPointLight[ParamName3D::kPointLightQuadraticFactor] = Variant (0.f);
		}
	}
	
	lightParameterWriter->setBuffer (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneRenderer3D::updateScene ()
{
	updatePointLights ();
	updateLightParameters ();
	updateModelNodes ();

	needsSceneUpdate = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneRenderer3D::update ()
{
	if(needsSceneUpdate)
		updateScene ();
	updateTextures ();
	shaderParameters.sort ();

	needsUpdate = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneRenderer3D::sceneChanged ()
{
	needsSceneUpdate |= scene->childrenChanged ();
	needsUpdate = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneRenderer3D::MaterialParameterItem* SceneRenderer3D::findMaterialParameters (Material3D* material) const
{
	return materialParameters.findIf ([&] (const MaterialParameterItem& item) { return item.material == material; });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneRenderer3D::ShaderParameterItem* SceneRenderer3D::findShaderParameters (ModelNode3D* node, int geometryIndex) const
{
	return shaderParameters.findIf ([&] (const ShaderParameterItem& item) { return item.node == node && item.geometryIndex == geometryIndex; });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SceneRenderer3D::PipelineItem* SceneRenderer3D::findPipeline (BaseGeometry3D* geometry, Material3D* material) const
{
	if(geometry == nullptr || material == nullptr)
		return nullptr;

	int flags = SceneRenderer3D::PipelineItem::getFlags (geometry, material);
	PrimitiveTopology3D topology = geometry->getPrimitiveTopology ();

	return pipelines.findIf ([&] (const PipelineItem& item)
	{
		return item.pixelShader == material->getPixelShader ()
			&& item.topology == topology
			&& item.flags == flags
			&& item.depthBias == material->getDepthBias ();
	});
}

//************************************************************************************************
// SceneRenderer3D::MaterialParameterItem
//************************************************************************************************

SceneRenderer3D::MaterialParameterItem::MaterialParameterItem (Material3D* material)
: material (material)
{}

//************************************************************************************************
// SceneRenderer3D::ShaderParameterItem
//************************************************************************************************

SceneRenderer3D::ShaderParameterItem::ShaderParameterItem (SceneRenderer3D* renderer, ModelNode3D* node, int geometryIndex)
: node (node),
  geometryIndex (geometryIndex),
  renderer (renderer)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SceneRenderer3D::ShaderParameterItem::operator == (const ShaderParameterItem& rhs) const
{
	return node == rhs.node && geometryIndex == rhs.geometryIndex;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SceneRenderer3D::ShaderParameterItem::operator > (const ShaderParameterItem& rhs) const
{
	Material3D* lhsMaterial = getMaterial ();
	Material3D* rhsMaterial = rhs.getMaterial ();

	if(lhsMaterial == nullptr)
		return false;

	if(rhsMaterial == nullptr)
		return true;

	if(lhsMaterial->getMaterialHint () == kGraphicsContentTranslucent && rhsMaterial->getMaterialHint () != kGraphicsContentTranslucent)
		return true;

	if(lhsMaterial->getMaterialHint () != kGraphicsContentTranslucent && rhsMaterial->getMaterialHint () == kGraphicsContentTranslucent)
		return false;

	if(lhsMaterial->getMaterialHint () == kGraphicsContentTranslucent && rhsMaterial->getMaterialHint () == kGraphicsContentTranslucent && renderer->getActiveCamera () != nullptr)
	{
		// sort transparent nodes by distance to the camera, so that nearest objects are drawn last
		if(node.isValid () && rhs.node.isValid ())
		{
			PointF3D lhsPosition;
			PointF3D rhsPosition;
			node->getWorldTransform ().getTranslation (lhsPosition);
			rhs.node->getWorldTransform ().getTranslation (rhsPosition);

			PointF3D cameraPosition;
			renderer->getActiveCamera ()->getWorldTransform ().getTranslation (cameraPosition);

			int lhsDistance = (cameraPosition - lhsPosition).lengthSquared ();
			int rhsDistance = (cameraPosition - rhsPosition).lengthSquared ();

			if(lhsDistance > rhsDistance)
				return false;
			if(lhsDistance < rhsDistance)
				return true;

			if(lhsDistance == rhsDistance)
			{
				if(lhsMaterial->getDepthBias () > rhsMaterial->getDepthBias ())
					return false;
				if(lhsMaterial->getDepthBias () < rhsMaterial->getDepthBias ())
					return true;
			}
		}
	}

	return lhsMaterial > rhsMaterial;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Material3D* SceneRenderer3D::ShaderParameterItem::getMaterial () const
{
	UnknownPtr<IModel3D> model (node ? node->getModelData () : nullptr);
	return model ? unknown_cast<Material3D> (model->getMaterialAt (geometryIndex)) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BaseGeometry3D* SceneRenderer3D::ShaderParameterItem::getGeometry () const
{
	UnknownPtr<IModel3D> model (node ? node->getModelData () : nullptr);
	return model ? unknown_cast<BaseGeometry3D> (model->getGeometryAt (geometryIndex)) : nullptr;
}

//************************************************************************************************
// SceneRenderer3D::PipelineItem
//************************************************************************************************

SceneRenderer3D::PipelineItem::PipelineItem (IGraphicsShader3D* pixelShader, BaseGeometry3D* geometry, Material3D* material)
: pixelShader (pixelShader),
  topology (geometry ? geometry->getPrimitiveTopology () : kPrimitiveTopologyTriangleList),
  flags (getFlags (geometry, material)),
  depthBias (material ? material->getDepthBias () : 0.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SceneRenderer3D::PipelineItem::getFlags (BaseGeometry3D* geometry, Material3D* material)
{
	int flags = 0;
	if(ccl_cast<Billboard3D> (geometry) != nullptr)
		flags |= kIsBillboard;
	return flags;
}
