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
// Filename    : ccl/gui/system/clipboard.h
// Description : Clipboard
//
//************************************************************************************************

#ifndef _ccl_clipboard_h
#define _ccl_clipboard_h

#include "ccl/base/singleton.h"

#include "ccl/public/gui/framework/iclipboard.h"

namespace CCL {

//************************************************************************************************
// Clipboard
//************************************************************************************************

class Clipboard: public Object,
				 public IClipboard,
				 public ExternalSingleton<Clipboard>
{
public:
	Clipboard ();
	~Clipboard ();

	DECLARE_CLASS (Clipboard, Object)
	DECLARE_METHOD_NAMES (FileSelector)

	// IClipboard
	tbool CCL_API isEmpty () const override;
	IUnknown* CCL_API getContent () const override;
	tbool CCL_API setContent (IUnknown* object) override;
	tbool CCL_API empty () override;
	tbool CCL_API setText (StringRef text) override;
	tbool CCL_API getText (String& text) const override;
	void CCL_API registerFilter (IConvertFilter* filter) override;
	void CCL_API unregisterFilter (IConvertFilter* filter) override;

	bool checkNativeContent ();

	static bool toText (String& text, IUnknown* object);
	static IUnknown* fromText (StringRef text);

	CLASS_INTERFACE (IClipboard, Object)

protected:
	IUnknown* content;

	// Object
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

	// to be implemented by derived platform classes:
	virtual bool setNativeText (StringRef text);
	virtual bool getNativeText (String& text) const;
	virtual bool hasNativeContentChanged (); ///< tells if the native clipboard has changed since the last call (implementation must reset internal observation state then)
};

} // namespace CCL

#endif // _ccl_clipboard_h
