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
// Filename    : ccl/platform/shared/opengles/opengles3dsupport.cpp
// Description : OpenGLES 3D Support
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "ccl/platform/shared/opengles/opengles3dsupport.h"
#include "ccl/platform/shared/opengles/openglesclient.h"
#include "ccl/platform/shared/opengles/glslshaderreflection.h"

#include "ccl/base/storage/file.h"

#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/3d/shader/shaderreflection3d.h"

#include "ccl/public/gui/graphics/dpiscale.h"

#include "ccl/public/math/mathprimitives.h"

#if CCL_PLATFORM_LINUX
#include "ccl/public/base/ccldefpush.h"
#include <wayland-egl.h> // must be included before EGL headers
#include "ccl/public/base/ccldefpop.h"
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace CCL {

//************************************************************************************************
// OpenGLESFormatMap
//************************************************************************************************

struct OpenGLESFormatMap
{
	DataFormat3D format;
	GLenum type;
	GLint count;
	GLint size;
};
static constexpr OpenGLESFormatMap kOpenGLESFormatMap[] =
{
	{ kR8_Int,             GL_BYTE              , 1 , 1 },
	{ kR8_UInt,            GL_UNSIGNED_BYTE     , 1 , 1 },
	{ kR16_Int,            GL_SHORT             , 1 , 2 },
	{ kR16_UInt,           GL_UNSIGNED_SHORT    , 1 , 2 },
	{ kR32_Int,            GL_INT               , 1 , 4 },
	{ kR32_UInt,           GL_UNSIGNED_INT      , 1 , 4 },
	{ kR32_Float,          GL_FLOAT             , 1 , 4 },
	{ kR8G8_Int,           GL_BYTE              , 2 , 2 },
	{ kR8G8_UInt,          GL_UNSIGNED_BYTE     , 2 , 2 },
	{ kR16G16_Int,         GL_SHORT             , 2 , 4 },
	{ kR16G16_UInt,        GL_UNSIGNED_SHORT    , 2 , 4 },
	{ kR32G32_Int,         GL_INT               , 2 , 8 },
	{ kR32G32_UInt,        GL_UNSIGNED_INT      , 2 , 8 },
	{ kR32G32_Float,       GL_FLOAT             , 2 , 8 },
	{ kR32G32B32_Int,      GL_INT               , 3 ,12 },
	{ kR32G32B32_UInt,     GL_UNSIGNED_INT      , 3 ,12 },
	{ kR32G32B32_Float,    GL_FLOAT             , 3 ,12 },
	{ kR32G32B32A32_Int,   GL_INT               , 4 ,16 },
	{ kR32G32B32A32_UInt,  GL_UNSIGNED_INT      , 4 ,16 },
	{ kR8G8B8A8_UNORM,     GL_RGBA              , 4 , 4 },
	{ kB8G8R8A8_UNORM,     GL_BGRA_EXT          , 4 , 4 }
};

static constexpr GLenum getOpenGLESFormatType (DataFormat3D format)
{
	for(const auto& entry : kOpenGLESFormatMap)
	{
		if(entry.format == format)
			return entry.type;
	}

	return 0;
}

static constexpr GLint getOpenGLESFormatCount (DataFormat3D format)
{
	for(const auto& entry : kOpenGLESFormatMap)
	{
		if(entry.format == format)
			return entry.count;
	}

	return 0;
}

static constexpr GLint getOpenGLESFormatSize (DataFormat3D format)
{
	for(const auto& entry : kOpenGLESFormatMap)
	{
		if(entry.format == format)
			return entry.size;
	}

	return 0;
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// OpenGLES3DVertexFormat
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OpenGLES3DVertexFormat, Native3DVertexFormat)

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLES3DVertexFormat::OpenGLES3DVertexFormat ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLES3DVertexFormat::create (const VertexElementDescription description[], uint32 count, const IGraphicsShader3D* shader)
{
	elements.setCount (count);
	int offset = 0;
	for(uint32 i = 0; i < elements.count (); i++)
	{
		elements[i].index = i;
		elements[i].type = getOpenGLESFormatType (description[i].format);
		elements[i].size = getOpenGLESFormatCount (description[i].format);
		elements[i].offset = offset;

		offset += getOpenGLESFormatSize (description[i].format);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DVertexFormat::apply (uint64 offset, uint32 stride)
{
	for(const auto& element : elements)
	{
		ASSERT (element.size > 0 && element.size <= 4)
		glVertexAttribPointer (element.index, element.size, element.type, GL_FALSE, stride, (void*)(offset + element.offset));
		if(GLenum error = glGetError ())
			CCL_WARN ("Failed to set vertex format for index %d: %x\n", element.index, error)
		glEnableVertexAttribArray (element.index);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int OpenGLES3DVertexFormat::countAttributes () const
{ 
	return elements.count (); 
}

//************************************************************************************************
// OpenGLES3DBuffer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OpenGLES3DBuffer, Native3DGraphicsBuffer)

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLES3DBuffer::OpenGLES3DBuffer ()
: bufferId (0),
  memoryAlignment (1),
  mapCount (0),
  target (GL_ARRAY_BUFFER),
  bufferUsage (GL_STATIC_DRAW),
  usingGPUMemory (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLES3DBuffer::~OpenGLES3DBuffer ()
{
	ASSERT (mapCount == 0)
	destroy ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DBuffer::destroy ()
{
	if(bufferId == 0)
		return;
			
	if(bufferId != 0)
	{
		glFinish ();
		glDeleteBuffers (1, &bufferId);
	}
	bufferId = 0;
	
	memory.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLES3DBuffer::create (Type _type, BufferUsage3D usage, uint32 sizeInBytes, uint32 strideInBytes, const void* initialData)
{
	destroy ();
	
	target = GL_ARRAY_BUFFER;
	setUsingGPUMemory (true);
	switch(_type)
	{
	case kVertexBuffer : target = GL_ARRAY_BUFFER; break;
	case kIndexBuffer : target = GL_ELEMENT_ARRAY_BUFFER; break;
	case kConstantBuffer : setUsingGPUMemory (false); break; // not available in OpenGL ES 2
	case kShaderResource : setUsingGPUMemory (false); break; // not available in OpenGL ES 2
	default:
		return false;
	}

	bufferUsage = GL_STATIC_DRAW;
	switch(usage)
	{
	case kBufferUsageDefault : bufferUsage = GL_STATIC_DRAW; break;
	case kBufferUsageDynamic : bufferUsage = GL_DYNAMIC_DRAW; break;
	case kBufferUsageImmutable : bufferUsage = GL_STATIC_DRAW; break;
	case kBufferUsageStaging : bufferUsage = GL_STATIC_DRAW; break;
	default:
		return false;
	}
	
	uint32 offset = 0;
	if(!ensureSegmentAlignment (offset, sizeInBytes, strideInBytes))
		return false;
	
	if(isUsingGPUMemory ())
	{
		glGenBuffers (1, &bufferId);
		if(bufferId == 0)
			return false;
		
		glBindBuffer (target, bufferId);
		glBufferData (target, sizeInBytes, initialData, bufferUsage);
		
		GLint size = 0;
		glGetBufferParameteriv (target, GL_BUFFER_SIZE, &size);
		if(sizeInBytes != size)
		{
			glDeleteBuffers (1, &bufferId);
			return false;
		}
	}
	
	if(initialData)
		memory = NEW Buffer (const_cast<void*> (initialData), sizeInBytes);
	else
		memory = NEW Buffer (sizeInBytes);
	
	type = _type;
	capacity = sizeInBytes;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* OpenGLES3DBuffer::map ()
{
	mapCount++;
	return memory.isValid () ? memory->getBufferAddress () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DBuffer::unmap ()
{
	mapCount--;
	if(isUsingGPUMemory () && mapCount == 0 && bufferId != 0 && memory.isValid ())
	{
		glBindBuffer (target, bufferId);
		glBufferSubData (target, 0, memory->getBufferSize (), memory->getBufferAddress ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLES3DBuffer::ensureSegmentAlignment (uint32& byteOffset, uint32& size, uint32 stride) const
{
	if(isUsingGPUMemory ())
	{
		uint32 alignment = ccl_lowest_common_multiple<uint32> (stride, 4);
		byteOffset = ccl_align_to (byteOffset, alignment);
		size = ccl_align_to (size, alignment);
	}
	return true;
}

//************************************************************************************************
// OpenGLES3DTexture2D
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OpenGLES3DTexture2D, Native3DTexture2D)

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLES3DTexture2D::OpenGLES3DTexture2D ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLES3DTexture2D::create (uint32 width, uint32 height, uint32 bytesPerRow, DataFormat3D format, TextureFlags3D flags, const void* initialData)
{
	uint32 mipLevels = 1;
	if(get_flag<TextureFlags3D> (flags, kTextureMipmapEnabled))
		mipLevels = getMipLevels (width, height);

	int wrapMode = GL_CLAMP_TO_EDGE;
	if(get_flag<TextureFlags3D> (flags, kTextureClampToBorder))
	{
		wrapMode = GL_CLAMP_TO_EDGE; // clamp to border is not supported by core OpenGL ES 2
		if(OpenGLESClient::instance ().isGLExtensionSupported ("GL_OES_texture_border_clamp"))
			wrapMode = GL_CLAMP_TO_BORDER_OES;
	}
	else if(get_flag<TextureFlags3D> (flags, kTextureRepeat))
		wrapMode = GL_REPEAT;
	else if(get_flag<TextureFlags3D> (flags, kTextureMirror))
		wrapMode = GL_MIRRORED_REPEAT;

	image.setWrapMode (wrapMode);
	image.setFormat (getOpenGLESFormatType (format));
	image.setSize (Point (width, height));
	image.setMipLevels (mipLevels);
	if(!image.create (initialData))
		return false;

	image.generateMipmaps ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OpenGLES3DTexture2D::copyFromBitmap (IBitmap* bitmap)
{
	IMultiResolutionBitmap::RepSelector selector (UnknownPtr<IMultiResolutionBitmap> (bitmap), getHighestResolutionIndex (bitmap));
	BitmapDataLocker locker (bitmap, IBitmap::kRGBAlpha, IBitmap::kLockRead);
	if(locker.result != kResultOk)
		return kResultFailed;

	if(locker.data.width != image.getSize ().x || locker.data.height != image.getSize ().y)
		return kResultInvalidArgument;

	glBindTexture (GL_TEXTURE_2D, image.getTextureID ());
	glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, image.getSize ().x, image.getSize ().y, image.getFormat (), GL_UNSIGNED_BYTE, locker.data.scan0);

	image.generateMipmaps ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DTexture2D::destroy ()
{
	image.destroy ();
}

//************************************************************************************************
// OpenGLES3DShader
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OpenGLES3DShader, Native3DGraphicsShader)

const FileType OpenGLES3DShader::kFileType ("GLSL Shader Source File", "glsl");

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLES3DShader::OpenGLES3DShader ()
: shaderId (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLES3DShader::~OpenGLES3DShader ()
{
	reset ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DShader::reset ()
{
	if(shaderId != 0)
		glDeleteShader (shaderId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLES3DShader::create (GraphicsShader3DType _type, UrlRef _path)
{
	reset ();

	path = _path;
	type = _type;
	
	return load ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLES3DShader::load ()
{
	uint32 shaderType = GL_VERTEX_SHADER;
	switch(type)
	{
		case kVertexShader : shaderType = GL_VERTEX_SHADER; break;
		case kPixelShader : shaderType = GL_FRAGMENT_SHADER; break;
	}
	
	shaderId = glCreateShader (shaderType);
	if(shaderId == 0)
		return false;
	
	AutoPtr<IMemoryStream> stream = File::loadBinaryFile (path);
	if(stream == nullptr)
		return false;
	
	GLchar* shaderCode = static_cast<GLchar*> (stream->getMemoryAddress ());
	GLint shaderLength = stream->getBytesWritten ();
	glShaderSource (shaderId, 1, &shaderCode, &shaderLength);
	
	glCompileShader (shaderId);
	
	GLint success = 0;
	glGetShaderiv (shaderId, GL_COMPILE_STATUS, &success);
	
	#if DEBUG && DEBUG_LOG
	if(success != GL_TRUE)
	{
		int bufferSize = STRING_STACK_SPACE_MAX;
		char buffer[STRING_STACK_SPACE_MAX] = "";
		glGetShaderInfoLog (shaderId, bufferSize, &bufferSize, buffer);
		CCL_PRINTF ("Failed to compile GLSL shader %s: %s", MutableCString (UrlDisplayString (path)).str (), buffer)
	}
	#endif

	return success == GL_TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::ITypeInfo* CCL_API OpenGLES3DShader::getBufferTypeInfo (int bufferIndex)
{
	if(bufferTypeInfos.isEmpty ())
		GLSLShaderReflection::getBufferTypeInfos (bufferTypeInfos, path);
	return SuperClass::getBufferTypeInfo (bufferIndex);
}

//************************************************************************************************
// OpenGLES3DPipeline
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OpenGLES3DPipeline, Native3DGraphicsPipeline)

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLES3DPipeline::OpenGLES3DPipeline ()
: topology (GL_TRIANGLE_STRIP),
  programId (0),
  enableDepthTest (true),
  enableDepthWrite (true),
  depthBias (0.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLES3DPipeline::~OpenGLES3DPipeline ()
{
	if(programId != 0)
		glDeleteProgram (programId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OpenGLES3DPipeline::setPrimitiveTopology (PrimitiveTopology3D primitiveTopology)
{
	switch(primitiveTopology)
	{
	case kPrimitiveTopologyTriangleList:
		topology = GL_TRIANGLES;
		break;

	case kPrimitiveTopologyTriangleStrip:
		topology = GL_TRIANGLE_STRIP;
		break;

	default:
		return kResultInvalidArgument;
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OpenGLES3DPipeline::setFillMode (FillMode3D mode)
{
	if(mode == kFillModeSolid)
		return kResultOk;
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OpenGLES3DPipeline::setVertexFormat (IVertexFormat3D* _format)
{
	OpenGLES3DVertexFormat* format = unknown_cast<OpenGLES3DVertexFormat> (_format);
	if(format == nullptr)
		return kResultInvalidArgument;
	vertexFormat = format;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OpenGLES3DPipeline::setVertexShader (IGraphicsShader3D* _shader)
{
	OpenGLES3DShader* shader = unknown_cast<OpenGLES3DShader> (_shader);
	if(shader == nullptr || shader->getType () != IGraphicsShader3D::kVertexShader)
		return kResultInvalidArgument;

	if(vertexShader != shader)
	{
		vertexShader = shader;
		updateProgram ();
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OpenGLES3DPipeline::setPixelShader (IGraphicsShader3D* _shader)
{
	OpenGLES3DShader* shader = unknown_cast<OpenGLES3DShader> (_shader);
	if(shader == nullptr || shader->getType () != IGraphicsShader3D::kPixelShader)
		return kResultInvalidArgument;

	if(pixelShader != shader)
	{
		pixelShader = shader;
		updateProgram ();
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OpenGLES3DPipeline::setDepthTestParameters (const DepthTestParameters3D& parameters)
{
	enableDepthTest = parameters.testEnabled;
	enableDepthWrite = parameters.writeEnabled;
	depthBias = parameters.bias;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DPipeline::applyTo (OpenGLES3DGraphicsContext& context, uint64 vertexOffset, uint32 vertexStride) const
{
	context.setTopology (topology);
	glUseProgram (programId);
	glPolygonOffset (0, depthBias);
	glDepthMask (enableDepthWrite);
	if(enableDepthTest)
		glEnable (GL_DEPTH_TEST);
	else
		glDisable (GL_DEPTH_TEST);
	if(vertexFormat.isValid ())
		vertexFormat->apply (vertexOffset, vertexStride);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DPipeline::updateProgram ()
{
	if(!pixelShader.isValid () || !vertexShader.isValid ())
		return;

	if(programId != 0)
		glDeleteProgram (programId);
	programId = glCreateProgram ();

	glAttachShader (programId, vertexShader->getShaderID ());
	glAttachShader (programId, pixelShader->getShaderID ());
	glLinkProgram (programId);

	GLint success = 0;
	glGetProgramiv (programId, GL_LINK_STATUS, &success);
	
	#if DEBUG && DEBUG_LOG
	if(success != GL_TRUE)
	{
		int bufferSize = STRING_STACK_SPACE_MAX;
		char buffer[STRING_STACK_SPACE_MAX] = "";
		glGetProgramInfoLog (programId, bufferSize, &bufferSize, buffer);
		CCL_PRINTF ("Failed to link a GLSL program: %s", buffer)
	}
	#endif
}

//************************************************************************************************
// OpenGLES3DGraphicsContext
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OpenGLES3DGraphicsContext, Native3DGraphicsDevice)

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLES3DGraphicsContext::OpenGLES3DGraphicsContext ()
: bufferStride (0),
  indexBufferFormat (kR16_UInt),
  topology (GL_TRIANGLE_STRIP)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OpenGLES3DGraphicsContext::setPipeline (IGraphicsPipeline3D* _graphicsPipeline)
{
	OpenGLES3DPipeline* graphicsPipeline = unknown_cast<OpenGLES3DPipeline> (_graphicsPipeline);
	if(graphicsPipeline == nullptr)
		return kResultInvalidArgument;

	pipeline = graphicsPipeline;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OpenGLES3DGraphicsContext::setVertexBuffer (IGraphicsBuffer3D* _buffer, uint32 stride)
{
	OpenGLES3DBuffer* buffer = unknown_cast<OpenGLES3DBuffer> (_buffer);
	if(buffer == nullptr)
		return kResultInvalidArgument;

	vertexBuffer = buffer;
	bufferStride = stride;
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OpenGLES3DGraphicsContext::setIndexBuffer (IGraphicsBuffer3D* _buffer, DataFormat3D format)
{
	OpenGLES3DBuffer* buffer = unknown_cast<OpenGLES3DBuffer> (_buffer);
	if(buffer == nullptr)
		return kResultInvalidArgument;

	indexBuffer = buffer;
	indexBufferFormat = format;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OpenGLES3DGraphicsContext::setShaderParameters (IShaderParameterSet3D* _parameters)
{
	auto* parameterSet = unknown_cast<Native3DShaderParameterSet> (_parameters);
	if(parameterSet == nullptr)
		return kResultInvalidArgument;

	shaderParameters = parameterSet;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DGraphicsContext::bindPipeline (uint64 vertexOffset, uint32 vertexStride)
{
	if(pipeline.isValid ())
		pipeline->applyTo (*this, vertexOffset, vertexStride);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DGraphicsContext::bindDescriptorSet ()
{
	if(!shaderParameters.isValid ())
		return;
	
	GLint programId = 0;
	glGetIntegerv (GL_CURRENT_PROGRAM, &programId);
	ASSERT (programId != 0)

	#if 0 && DEBUG
	GLint uniformCount = 0;
	glGetProgramiv (programId, GL_ACTIVE_UNIFORMS, &uniformCount);
	for(int i = 0; i < uniformCount; i++)
	{
		GLchar name[STRING_STACK_SPACE_MAX] = "";
		GLsizei length = 0;
		GLint size = 0;
		GLenum type = 0;
		glGetActiveUniform (programId, i, sizeof(name), &length, &size, &type, name);
		CCL_PRINTF ("active uniform \"%s\" (%d elements of type %d)\n", name, size, type)
	}
	#endif

	int uniformIndex = 0;

	auto uploadNamedUniform = [programId] (char* data, const ShaderVariable3D& variable, CStringPtr name) -> int
	{
		auto uploadStruct = [] (char* data, int location, const ShaderVariable3D& variable, auto& implementation) mutable -> int
		{
			ShaderTypeInfo3D* structType = variable.getStructType ();
			ASSERT (structType != nullptr)
			if(structType == nullptr)
				return 0;

			int result = 0;
			for(int arrayIndex = 0; arrayIndex < ccl_max<int> (1, variable.getArrayElementCount ()); arrayIndex++)
			{
				for(const ShaderVariable3D* member : iterate_as<ShaderVariable3D> (structType->getVariables ()))
					result += implementation (data + variable.getArrayElementStride () * arrayIndex, *member, location + result, implementation);
			}
			return result;
		};
		
		auto uploadUniform = [programId, uploadStruct] (char* data, const ShaderVariable3D& variable, int location, auto& implementation) mutable -> int
		{
			int count = ccl_max<int> (1, variable.getArrayElementCount ());
			switch(variable.getType ())
			{
			case ShaderVariable3D::kFloat :
				glUniform1fv (location, count, reinterpret_cast<float*> (data + variable.getOffset ()));
				return 1;
			case ShaderVariable3D::kFloat4 :
				glUniform4fv (location, count, reinterpret_cast<float*> (data + variable.getOffset ()));
				return 1;
			case ShaderVariable3D::kFloat4x4 :
				glUniformMatrix4fv (location, count, GL_FALSE, reinterpret_cast<float*> (data + variable.getOffset ()));
				return 1;
			case ShaderVariable3D::kInt :
				glUniform1iv (location, count, reinterpret_cast<int*> (data + variable.getOffset ()));
				return 1;
			case ShaderVariable3D::kStruct :
				return uploadStruct (data, location, variable, implementation);
			default :
				ASSERT (false)
			}
			return 0;
		};

		int location = glGetUniformLocation (programId, name);
		if(GLenum error = glGetError ())
			CCL_WARN ("Failed to get uniform location for %s: %x\n", name, error)
		ASSERT (location != -1)
		return uploadUniform (data, variable, location, uploadUniform);
	};

	auto uploadUniforms = [&uniformIndex, programId, uploadNamedUniform] (const Vector<Native3DShaderParameters>& shaderParameters, IGraphicsShader3D* shader)
	{
		if(shader == nullptr)
			return;
			
		for(const Native3DShaderParameters& parameters : shaderParameters)
		{
			if(parameters.segment == nullptr)
				continue;

			OpenGLES3DBuffer* buffer = unknown_cast<OpenGLES3DBuffer> (parameters.segment->getBuffer ());
			ASSERT (!buffer->isUsingGPUMemory ())
			const Buffer* parameterBuffer = buffer ? buffer->getMemory () : nullptr;
			if(parameterBuffer == nullptr)
				return;

			char* source = static_cast<char*> (parameterBuffer->getBufferAddress ()) + parameters.segment->getOffset ();
			int location = 0;

			auto* info = unknown_cast<ShaderTypeInfo3D> (shader->getBufferTypeInfo (parameters.bufferIndex));
			if(info == nullptr)
				continue;

			for(const ShaderVariable3D* variable : iterate_as<ShaderVariable3D> (info->getVariables ()))
			{
				GLchar name[STRING_STACK_SPACE_MAX] = "";
				GLsizei length = 0;
				GLint size = 0;
				GLenum type = 0;
				glGetActiveUniform (programId, uniformIndex, sizeof(name), &length, &size, &type, name);

				ASSERT ((type == GL_FLOAT && variable->getType () == ShaderVariable3D::kFloat)
					|| (type == GL_FLOAT_VEC4 && variable->getType () == ShaderVariable3D::kFloat4)
					|| (type == GL_FLOAT_MAT4 && variable->getType () == ShaderVariable3D::kFloat4x4)
					|| (type == GL_INT && variable->getType () == ShaderVariable3D::kInt)
					|| variable->getType () == ShaderVariable3D::kStruct)

				uniformIndex += uploadNamedUniform (source, *variable, name);
			}
		}
	};

	uploadUniforms (shaderParameters->getVertexShaderParameters (), pipeline->getVertexShader ());
	uploadUniforms (shaderParameters->getPixelShaderParameters (), pipeline->getPixelShader ());

	for(int i = 0; i < Native3DShaderParameterSet::kMaxTextureCount; i++)
	{
		OpenGLES3DTexture2D* texture = unknown_cast<OpenGLES3DTexture2D> (shaderParameters->getTexture (i));
		if(texture == nullptr)
			continue;

		glActiveTexture (GL_TEXTURE0 + i);
		glBindTexture (GL_TEXTURE_2D, texture->getImage ().getTextureID ());
	}
	glActiveTexture (GL_TEXTURE0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult OpenGLES3DGraphicsContext::prepareDrawing (uint32 startVertex)
{
	ASSERT (vertexBuffer.isValid ())
	if(!vertexBuffer.isValid ())
		return kResultFailed;

	glBindBuffer (GL_ARRAY_BUFFER, vertexBuffer->getBufferID ());
	if(GLenum error = glGetError ())
		CCL_WARN ("Failed to bind vertex buffer %d: %x\n", vertexBuffer->getBufferID (), error)

	bindPipeline (startVertex * bufferStride, bufferStride);
	bindDescriptorSet ();
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OpenGLES3DGraphicsContext::draw (uint32 startVertex, uint32 vertexCount)
{
	tresult result = prepareDrawing (0);
	if(result != kResultOk)
		return result;

	glDrawArrays (topology, startVertex, vertexCount);
	if(GLenum error = glGetError ())
		CCL_WARN ("Failed to draw a vertex array: %x\n", error)
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API OpenGLES3DGraphicsContext::drawIndexed (uint32 startIndex, uint32 indexCount, int32 baseVertex)
{
	tresult result = prepareDrawing (baseVertex);
	if(result != kResultOk)
		return result;

	ASSERT (indexBufferFormat == kR16_UInt || indexBufferFormat == kR8_UInt)

	glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, indexBuffer->getBufferID ());
	if(GLenum error = glGetError ())
		CCL_WARN ("Failed to bind an index buffer: %x\n", error)

	glDrawElements (topology, indexCount, getOpenGLESFormatType (indexBufferFormat), (void*)(int64(startIndex) * getOpenGLESFormatSize (indexBufferFormat)));
	if(GLenum error = glGetError ())
		CCL_WARN ("Failed to draw indexed: %x\n", error)

	return kResultOk;
}

//************************************************************************************************
// OpenGLES3DSurface
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OpenGLES3DSurface, Native3DSurface)

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLES3DSurface::OpenGLES3DSurface ()
: sampleCount (1),
  scaleFactor (1.f),
  depthBufferId (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLES3DSurface::~OpenGLES3DSurface ()
{
	destroy ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DSurface::setContent (IGraphicsContent3D* _content)
{
	SuperClass::setContent (_content);
	int _sampleCount = content ? content->getMultisampling () : 1;
	if(_sampleCount != sampleCount)
	{
		destroy ();
		applyMultisampling (_sampleCount);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DSurface::setSize (const Rect& _size)
{
	SuperClass::setSize (_size);
	destroy (); // OpenGLES and Skia objects need to be recreated. The render target will call create in the next render call.
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DSurface::applyMultisampling (int samples)
{
	sampleCount = ccl_upperPowerOf2 (samples / scaleFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLES3DSurface::create (GrRecordingContext* context, float _scaleFactor)
{
	scaleFactor = _scaleFactor;

	// emulate multisampling by rendering at a larger viewport size
	viewPortRect = PixelRect (size, scaleFactor * sampleCount);

	texture.setSize (viewPortRect.getSize ());
	if(!texture.create ())
	{
		CCL_WARN ("%s\n", "Failed to create a 3D surface texture.")
		destroy ();
		return false;
	}
	if(!texture.generateFramebuffer ())
	{
		CCL_WARN ("%s\n", "Failed to generat a 3D surface framebuffer.")
		destroy ();
		return false;
	}

	glGenRenderbuffers (1, &depthBufferId);
	glBindRenderbuffer (GL_RENDERBUFFER, depthBufferId);
	if(GLenum error = glGetError ())
		CCL_WARN ("Failed to bind a depth buffer: %x\n", error)

	glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, viewPortRect.getWidth (), viewPortRect.getHeight ());
	if(GLenum error = glGetError ())
		CCL_WARN ("Failed to create depth buffer storage: %x\n", error)

	glFramebufferRenderbuffer (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferId);
	if(GLenum error = glGetError ())
		CCL_WARN ("Failed to attach a depth buffer: %x\n", error)

	GrGLTextureInfo textureInfo = {0};
	textureInfo.fTarget = GL_TEXTURE_2D;
	textureInfo.fID = texture.getTextureID ();
	textureInfo.fFormat = GL_RGBA8_OES;

	backendTexture = GrBackendTextures::MakeGL (viewPortRect.getWidth (), viewPortRect.getHeight (), skgpu::Mipmapped::kNo, textureInfo);

	updateSkiaImage ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DSurface::updateSkiaImage ()
{
	GrDirectContext* context = OpenGLESClient::instance ().getGPUContext ();
	if(context == nullptr)
		return;

	textureImage = SkImages::BorrowTextureFrom (context, backendTexture, kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, kPremul_SkAlphaType, nullptr);
	if(textureImage == nullptr)
	{
		CCL_WARN ("%s\n", "Failed to create a Skia image from a backend texture");
		destroy ();
		return;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DSurface::destroy ()
{
	glFinish ();
	texture.destroy ();
	textureImage.reset ();
	glDeleteRenderbuffers (1, &depthBufferId);
	depthBufferId = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLES3DSurface::isValid () const
{
	return texture.getTextureID () != 0 && textureImage != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DSurface::render (OpenGLES3DGraphicsContext& context)
{
	if(!isDirty ())
		return;

	glBindFramebuffer (GL_FRAMEBUFFER, texture.getFramebufferID ());
	if(GLenum error = glGetError ())
		CCL_WARN ("Failed to bind a 3D surface framebuffer: %x\n", error)

	IGraphicsContent3D* content = getContent ();
	if(content == nullptr)
		return;

	glViewport (0, 0, viewPortRect.getWidth (), viewPortRect.getHeight ());
	glScissor (0, 0, viewPortRect.getWidth (), viewPortRect.getHeight ());

	glCullFace (GL_BACK);
	glFrontFace (GL_CCW);
	glDepthMask (GL_TRUE);
	glDepthFunc (GL_LESS);
	glDepthRangef (0.f, 1.f);
	glEnable (GL_DEPTH_TEST);
	glEnable (GL_CULL_FACE);
	glDisable (GL_SCISSOR_TEST);
	glDisable (GL_STENCIL_TEST);
	glEnable (GL_BLEND);
	glEnable (GL_POLYGON_OFFSET_FILL);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	ColorF clearColor (getClearColor ());
	#if 0 && DEBUG
	clearColor = ColorF (0.f, 0.f, 0.2f, 0.5f);
	#endif
	glClearColor (clearColor.red, clearColor.green, clearColor.blue, clearColor.alpha);
	glClearDepthf (1.f);
	glClearStencil (0);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	content->renderContent (context);
	
	updateSkiaImage ();

	setDirty (false);
}

//************************************************************************************************
// OpenGLES3DResourceManager
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OpenGLES3DResourceManager, Native3DResourceManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLES3DResourceManager::shutdown ()
{
	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DGraphicsShader* OpenGLES3DResourceManager::loadShader (UrlRef _path, GraphicsShader3DType type)
{
	AutoPtr<OpenGLES3DShader> shader = NEW OpenGLES3DShader;
	Url path (_path);
	path.setFileType (OpenGLES3DShader::kFileType);
	if(!shader->create (type, path))
		return nullptr;
	
	return shader.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DTexture2D* OpenGLES3DResourceManager::loadTexture (Bitmap* bitmap, TextureFlags3D flags)
{
	AutoPtr<OpenGLES3DTexture2D> texture = NEW OpenGLES3DTexture2D;
	if(texture->create (bitmap, flags))
	 	return texture.detach ();
	
	return nullptr;
}

//************************************************************************************************
// OpenGLES3DGraphicsFactory
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OpenGLES3DGraphicsFactory, Native3DGraphicsFactory)

//////////////////////////////////////////////////////////////////////////////////////////////////

IVertexFormat3D* CCL_API OpenGLES3DGraphicsFactory::createVertexFormat (const VertexElementDescription description[], uint32 count, const IGraphicsShader3D* shader)
{
	AutoPtr<OpenGLES3DVertexFormat> format = NEW OpenGLES3DVertexFormat;
	if(!format->create (description, count, shader))
		return nullptr;

	return format.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsBuffer3D* CCL_API OpenGLES3DGraphicsFactory::createBuffer (GraphicsBuffer3DType type, BufferUsage3D usage,
								uint32 sizeInBytes, uint32 strideInBytes, const void* initialData /* = nullptr */)
{
	AutoPtr<OpenGLES3DBuffer> buffer = NEW OpenGLES3DBuffer;
	if(!buffer->create (type, usage, sizeInBytes, strideInBytes, initialData))
		return nullptr;

	return buffer.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsTexture2D* CCL_API OpenGLES3DGraphicsFactory::createTexture (IBitmap* _bitmap, TextureFlags3D flags)
{
	auto* bitmap = unknown_cast<Bitmap> (_bitmap);
	if(bitmap == nullptr)
		return nullptr;

	if(get_flag<TextureFlags3D> (flags, kTextureImmutable))
	{
		OpenGLES3DResourceManager& manager = OpenGLES3DResourceManager::instance ();
		return return_shared (manager.getTexture (bitmap, flags));
	}

	AutoPtr<OpenGLES3DTexture2D> texture = NEW OpenGLES3DTexture2D;
	if(!texture->create (bitmap, flags))
		return nullptr;

	return texture.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsShader3D* CCL_API OpenGLES3DGraphicsFactory::createShader (GraphicsShader3DType type, UrlRef path)
{
	OpenGLES3DResourceManager& manager = OpenGLES3DResourceManager::instance ();
	return return_shared (manager.getShader (path, type));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsShader3D* CCL_API OpenGLES3DGraphicsFactory::createStockShader (GraphicsShader3DType type, StringID name)
{
	ResourceUrl url {String (name)};
	OpenGLES3DResourceManager& manager = OpenGLES3DResourceManager::instance ();
	return return_shared (manager.getShader (url, type));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsPipeline3D* CCL_API OpenGLES3DGraphicsFactory::createPipeline ()
{
	return NEW OpenGLES3DPipeline;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IShaderParameterSet3D* CCL_API OpenGLES3DGraphicsFactory::createShaderParameterSet ()
{
	return NEW Native3DShaderParameterSet;
}

//************************************************************************************************
// OpenGLES3DSupport
//************************************************************************************************

void OpenGLES3DSupport::shutdown3D ()
{
	OpenGLES3DResourceManager::instance ().shutdown ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DGraphicsFactory& OpenGLES3DSupport::get3DFactory ()
{
	return factory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Native3DSurface* OpenGLES3DSupport::create3DSurface ()
{
	return NEW OpenGLES3DSurface;
}
