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
// Filename    : ccl/platform/shared/skia/skiaglue.h
// Description : Skia Helper
//
//************************************************************************************************

#ifndef _ccl_skia_glue_h
#define _ccl_skia_glue_h

#include "ccl/public/base/ccldefpush.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"

#include "skia/include/core/SkBitmap.h"
#include "skia/include/core/SkFont.h"
#include "skia/include/core/SkFontStyle.h"
#include "skia/include/core/SkFontMgr.h"
#include "skia/include/core/SkTypeface.h"
#include "skia/include/core/SkPaint.h"
#include "skia/include/core/SkPath.h"
#include "skia/include/core/SkCanvas.h"
#include "skia/include/core/SkColor.h"
#include "skia/include/core/SkSurface.h"
#include "skia/include/core/SkImageGenerator.h"
#include "skia/include/core/SkTextBlob.h"
#include "skia/include/core/SkSamplingOptions.h"
#include "skia/include/core/SkStream.h"
#include "skia/include/codec/SkEncodedImageFormat.h"
#include "skia/include/codec/SkCodec.h"
#include "skia/include/encode/SkJpegEncoder.h"
#include "skia/include/encode/SkPngEncoder.h"

#include "skia/include/docs/SkPDFDocument.h"

#include "skia/include/gpu/ganesh/GrDirectContext.h"
#include "skia/include/gpu/ganesh/SkSurfaceGanesh.h"
#include "skia/include/gpu/ganesh/GrBackendSemaphore.h"
#include "skia/include/gpu/ganesh/GrBackendSurface.h"
#include "skia/include/gpu/ganesh/SkImageGanesh.h"

#if SK_METAL
#include "skia/include/gpu/ganesh/mtl/GrMtlBackendContext.h"
#include "skia/include/gpu/ganesh/mtl/GrMtlBackendSemaphore.h"
#include "skia/include/gpu/ganesh/mtl/GrMtlBackendSurface.h"
#include "skia/include/gpu/ganesh/mtl/GrMtlDirectContext.h"
#include "skia/include/gpu/ganesh/mtl/GrMtlTypes.h"
#include "skia/include/gpu/ganesh/mtl/SkSurfaceMetal.h"
#endif

#if SK_VULKAN
#include "skia/include/gpu/ganesh/vk/GrVkTypes.h"
#include "skia/include/gpu/ganesh/vk/GrVkDirectContext.h"
#include "skia/include/gpu/ganesh/vk/GrVkBackendSurface.h"
#include "skia/include/gpu/ganesh/vk/GrVkBackendSemaphore.h"
#include "skia/include/gpu/vk/VulkanMutableTextureState.h"
#include "skia/include/gpu/vk/VulkanBackendContext.h"
#include "skia/include/gpu/vk/VulkanExtensions.h"
#endif

#if SK_GL
#include "skia/include/gpu/ganesh/gl/GrGLTypes.h"
#include "skia/include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "skia/include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "skia/include/gpu/ganesh/gl/GrGLInterface.h"
#include "skia/include/gpu/ganesh/gl/egl/GrGLMakeEGLInterface.h"
#endif

#if SK_FONTCONFIG
#include "skia/include/ports/SkFontMgr_fontconfig.h"
#endif

#if CCL_PLATFORM_MAC || CCL_PLATFORM_IOS
#include "skia/include/ports/SkFontMgr_mac_ct.h"
#endif

#include "skia/include/effects/SkGradientShader.h"

#include "skia/modules/skshaper/include/SkShaper.h"
#if CCL_PLATFORM_MAC || CCL_PLATFORM_IOS
#include "skia/modules/skshaper/include/SkShaper_coretext.h"
#endif
#include "skia/modules/skparagraph/include/ParagraphBuilder.h"

#pragma GCC diagnostic pop

#include "ccl/public/base/ccldefpop.h"

#endif // _ccl_skia_glue_h
