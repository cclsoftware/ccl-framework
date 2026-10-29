//************************************************************************************************
//
// Audio Service
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : audiosourcebase.h
// Description : Audio Source Base Classes
//
//************************************************************************************************

#ifndef _audiosourcebase_h
#define _audiosourcebase_h

#include "ccl/base/object.h"

#include "ccl/public/plugins/pluginst.h"

#include "ccl/public/audio/iaudiosource.h"

namespace CCL {

//************************************************************************************************
// AudioSourceBase
//************************************************************************************************

class AudioSourceBase: public Object,
					   public PluginInstance
{
public:
	DECLARE_CLASS (AudioSourceBase, Object)

	CLASS_INTERFACE (IPluginInstance, Object)
};

//************************************************************************************************
// AudioSinkBase
//************************************************************************************************

class AudioSinkBase: public Object,
					 public PluginInstance
{
public:
	DECLARE_CLASS (AudioSinkBase, Object)

	CLASS_INTERFACE (IPluginInstance, Object)
};

} // namespace CCL

#endif // _audiosourcebase_h