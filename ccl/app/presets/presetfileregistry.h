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
// Filename    : ccl/app/presets/presetfileregistry.h
// Description : Preset File Registry
//
//************************************************************************************************

#ifndef _ccl_presetfileregistry_h
#define _ccl_presetfileregistry_h

#include "ccl/app/component.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/app/ipreset.h"
#include "ccl/public/system/ilockable.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

class FileType;
class FileTypeFilter;
class UrlWithTitle;

//************************************************************************************************
// PresetFileRegistry
//************************************************************************************************

class PresetFileRegistry: public Component,
						  public ComponentSingleton<PresetFileRegistry>,
						  public IPresetFileRegistry,
						  public Threading::ILockProvider
{
public:
	DECLARE_CLASS (PresetFileRegistry, Component)
	PresetFileRegistry ();
	~PresetFileRegistry ();
	
	// IPresetFileRegistry
	void CCL_API addHandler (IPresetFileHandler* handler, tbool isDefault = false) override; ///< takes ownership	
	int CCL_API countHandlers () const override;
	IPresetFileHandler* CCL_API getHandler (int index) const override;
	IPresetFileHandler* CCL_API getHandlerForTarget (IUnknown* target) const override;
	IPresetFileHandler* CCL_API getHandlerForFile (UrlRef url) const override;
	IPresetFileHandler* CCL_API getHandlerForFileType (const FileType& fileType) const override;
	IPresetFileHandler* CCL_API getHandlerForMimeType (StringID mimeType) const override;
	IPresetFileHandler* CCL_API getDefaultHandler () const override;
	void CCL_API collectFileTypes (IFileTypeFilter& fileTypes, IUnknown* target = nullptr, int requiredHandlerFlags = 0) const override;
	void CCL_API setSubFolderPrefix (UrlRef location, StringRef subFolder) override;
	StringRef CCL_API getSubFolderPrefix (UrlRef url) const override;

	// ILockProvider
	Threading::ILockable* CCL_API getLock () const override;

	// Component
	tresult CCL_API initialize (IUnknown* context) override;
	tresult CCL_API terminate () override;
	void CCL_API notify (ISubject* s, MessageRef msg) override;

	CLASS_INTERFACE2 (IPresetFileRegistry, ILockProvider, Component)

	static void registerFileHandler ();
	static void unregisterFileHandler ();

	class FileHandler;
protected:
	Threading::ILockable* handlerLock;
	Vector<IPresetFileHandler*> handlers;
	IPresetFileHandler* defaultHandler;
	ObjectArray subFolderPrefixes;

	void releaseHandler (IPresetFileHandler* handler);
	UrlWithTitle* findSubFolderPrefixLocation (UrlRef url) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_presetfileregistry_h
