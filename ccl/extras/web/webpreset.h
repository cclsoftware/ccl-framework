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
// Filename    : ccl/extras/web/webpreset.h
// Description : Web Preset
//
//************************************************************************************************

#ifndef _ccl_webpreset_h
#define _ccl_webpreset_h

#include "ccl/app/presets/preset.h"

#include "ccl/base/singleton.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/base/itrigger.h"

#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/gui/idatatarget.h"
#include "ccl/public/gui/iparameter.h"

#include "ccl/public/system/ifileitem.h"

#include "ccl/public/network/web/itransfermanager.h"

namespace CCL {

interface IFileDescriptor;
class PresetPartList;

//************************************************************************************************
// WebPreset
/** Placeholder for a Preset that needs to be downloaded before it can be used. */
//************************************************************************************************

class WebPreset: public Preset
{
public:
	static void registerConvertFilter ();

	DECLARE_CLASS_ABSTRACT (WebPreset, Preset)

	WebPreset (IFileDescriptor* descriptor);
	~WebPreset ();

	UrlRef getDestPath () const;
	int getTransferState () const;
	int updateTransferState ();

	IDataTarget* getDataTarget () const;
	void setDataTarget (IDataTarget* target);

	IParameter* getProgress () const;
	void setProgress (IParameter* param);

	PROPERTY_SHARED_AUTO (IFileDescriptor, fileDescriptor, FileDescriptor)
	PROPERTY_SHARED_AUTO (Web::ITransfer, transfer, Transfer)

	// Preset
	IAttributeList* CCL_API getMetaInfo () const override;
	tbool CCL_API getUrl (IUrl& url) const override;
	tbool CCL_API store (IUnknown* target) override;
	tbool CCL_API restore (IUnknown* target) const override;

private:
	IAttributeList* metaInfo;
	mutable Url destPath;
	ObservedPtr<IDataTarget> dataTarget;
	ObservedPtr<IParameter> progress;
};

//************************************************************************************************
// WebPresetCollection
//************************************************************************************************

class WebPresetCollection: public WebPreset,
						   public IPresetCollection
{
public:
	DECLARE_CLASS_ABSTRACT (WebPresetCollection, WebPreset)

	WebPresetCollection (IFileDescriptor* descriptor);
	~WebPresetCollection ();

	PresetPartList& getParts () const;

	// IPresetCollection
	int CCL_API countPresets () override;
	IPreset* CCL_API openPreset (int index) override;
	IPreset* CCL_API openPreset (const IStringDictionary& parameters) override;
	IPreset* CCL_API createPreset (IAttributeList& metaInfo) override;
	IStream* CCL_API openStream (StringRef path, int mode) override {return nullptr;}

	CLASS_INTERFACE (IPresetCollection, WebPreset)

private:
	mutable PresetPartList* parts;
};

//************************************************************************************************
// PresetTransferHandler
//************************************************************************************************

class PresetTransferHandler: public CCL::Object,
							 public CCL::IdleClient,
							 public CCL::Singleton<PresetTransferHandler>
{
public:
	DECLARE_CLASS (PresetTransferHandler, Object)

	PresetTransferHandler ();
	~PresetTransferHandler ();

	class Finalizer;

	tbool startTransfer (WebPreset* webPreset);
	void removeTransfer (WebPreset* webPreset);
	void onTransferFinished (WebPreset* webPreset);

	CLASS_INTERFACE (ITimerTask, Object)

protected:
	ObjectList transferPresets;
	ObjectList finishedPresets;
	ObjectList tempFolders;
	int64 lastUpdateTime;
	bool restoring;

	void restoreAll ();
	void restorePreset (WebPreset* webPreset);

	// IdleClient
	void onIdleTimer () override;
};

//************************************************************************************************
// PresetTransferHandler::Finalizer
//************************************************************************************************

class PresetTransferHandler::Finalizer: public CCL::Object,
										public CCL::ITriggerAction
{
public:
	Finalizer (WebPreset* webPreset);

	PROPERTY_SHARED_AUTO (WebPreset, webPreset, WebPreset)

	// ITriggerAction
	void CCL_API execute (CCL::IObject* target) override;

	CLASS_INTERFACE (ITriggerAction, Object)
};

} // namespace CCL

#endif // _ccl_webpreset_h
