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
// Filename    : ccl/public/gui/graphics/3d/doc3d.h
// Description : 3D Graphics Documentation
//
//************************************************************************************************

/**
	\addtogroup gui_graphics3d
	
	There are two entry points for displaying 3D graphics in CCL: CCL::UserView3D and CCL::IScene3D.
	
	CCL::UserView3D can be used to create a custom 3D graphics view. An application developer can implement a new class derived from CCL::IGraphicsContent3D and attach the graphics content to the CCL::UserView3D. 3D graphics objects can be created using CCL::IGraphicsFactory3D, which is passed to the CCL::IGraphicsContent3D in CCL::IGraphicsContent3D::createContent.
	In order to render 3D content, the application developer needs to create shader objects (CCL::IGraphicsShader3D) using the CCL::IGraphicsFactory3D. Shaders are programs that run on the GPU. The most common types of shaders are vertex shaders, which define how the vertices of a 3D geometry are rendered, and pixel shaders, which define the color output. An application developer can either supply custom vertex and pixel shaders or use stock shaders which are part of the framework.
	Once the shader objects have been created, these can be used to configure a pipeline (CCL::IGraphicsPipeline3D), which is used to draw 3D geometries in the CCL::IGraphicsContent3D::renderContent method.

	CCL::IScene3D provides the second, higher level approach to displaying 3D graphics. A scene is a tree of nodes, which in turn contain 3D models, camera objects, different types of lights, etc. It is the framework's responsibility to render a scene. The application developer only needs to provide high level information about the objects that need to be rendered. Scenes can be added to an existing UI using the SceneView3D skin tag.
	In order to display 3D geometries in a scene, the application first needs to add a camera (CCL::ICamera3D), some lights (CCL::ILightSource3D) and model nodes (CCL::IModelNode3D) to the scene. Model nodes contain models (CCL::IModel3D) and have a transform matrix which defines the position, orientation and scale of the model in the scene. Models in turn contain one or many geometries (CCL::IGeometry3D) and a material (CCL::IMaterial3D) for each of these geometries. Geometries contain vertex data, i.e. the geometric shape of a 3D object. Materials define how a geometry is rendered, e.g. using a solid color for each face or applying a texture. Models can either be defined in a skin file or in C++ code.
	
	Each material refers to a specific pixel shader. A solid color material uses a solid color pixel shader. A textured material uses a texture material shader.
	Material instances exist independently of models and can be used for multiple geometries. If a material is used multiple times, the same shader parameters (e.g. material color) are passed to the pixel shader for each geometry it is assigned to.
	
	A single geometry can be assigned to multiple models. Each model's transform matrix is used to determine the position of the object (vertex shader parameters). Each model's materials determine which pixel shaders are used to render its geometries and which shader parameter values are passed to the pixel shader.
	
	Each model needs a unique CCL::IShaderParameterSet3D for each geometry. A shader parameter set is a unique set of buffers which contain shader parameter and texture data, which is used to render a geometry. The GPU backend may render asynchronously, so shader parameter and texture data needs to stay valid across multiple draw calls for different geometries.
	The buffer segment used to store vertex shader parameters inside a CCL::IShaderParameterSet3D needs to be unique for a specific model node, as it stores the transform matrix of the node. It does not contain information about the geometry or the material.
	The buffer segment used to store pixel shader parameters inside a CCL::IShaderParameterSet3D needs to be unique for a specific material. It does not contain information about nodes or geometries.
	The CCL::IShaderParameterSet3D objects are managed by the scene renderer. Application developers don't need to create these objects explicitly when working with a scene.
	
	In order to use custom materials and custom pixel shaders, an application developer can create a new class derived from CCL::IShaderParameterProvider3D, create an instance of CCL::ICustomMaterial3D using `ccl_new` (CCL::ClassID::CustomMaterial3D) and supply a pixel shader and shader parameters through these interfaces. CCL::ClassID::CustomMaterial3D also provides the CCL::ITextureMaterial3D interface. Use this interface if your custom material requires textures.
	
	Custom shaders that are used in a scene need to follow some conventions. See stockshader3d.h for definitions of parameter names, buffer and texture indices that should be used in all shaders.
	
	There is currently no support for custom vertex shaders when working with scenes.
*/
