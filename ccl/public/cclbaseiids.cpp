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
// Filename    : ccl/public/cclbaseiids.cpp
// Description : IID symbols
//
//************************************************************************************************

#define INIT_IID

#include "ccl/public/text/cstring.h"

// Base
#include "ccl/public/base/ibuffer.h"
#include "ccl/public/base/iactivatable.h"
#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/base/iasyncoperation.h"
#include "ccl/public/base/iconverter.h"
#include "ccl/public/base/idatatransformer.h"
#include "ccl/public/base/iformatter.h"
#include "ccl/public/base/iobjectnode.h"
#include "ccl/public/base/iobject.h"
#include "ccl/public/base/iobserver.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/base/irecognizer.h"
#include "ccl/public/base/istream.h"
#include "ccl/public/base/itrigger.h"
#include "ccl/public/base/itypelib.h"
#include "ccl/public/base/iextensible.h"
#include "ccl/public/base/iunittest.h"

// Text
#include "ccl/public/text/istring.h"
#include "ccl/public/text/istringdict.h"
#include "ccl/public/text/ihtmlwriter.h"
#include "ccl/public/text/iattributehandler.h"
#include "ccl/public/text/iregexp.h"
#include "ccl/public/text/itranslationtable.h"
#include "ccl/public/text/ixmlparser.h"
#include "ccl/public/text/ixmlwriter.h"
#include "ccl/public/text/itextbuilder.h"
#include "ccl/public/text/itextstreamer.h"
#include "ccl/public/text/language.h"

// Storage
#include "ccl/public/storage/filetype.h"
#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/storage/iconfiguration.h"
#include "ccl/public/storage/ifileresource.h"
#include "ccl/public/storage/ipersistattributes.h"
#include "ccl/public/storage/istorage.h"
#include "ccl/public/storage/iurl.h"
#include "ccl/public/storage/ixmltree.h"

// System
#include "ccl/public/system/iallocator.h"
#include "ccl/public/system/ianalyticsmanager.h"
#include "ccl/public/system/iatomtable.h"
#include "ccl/public/system/iconsole.h"
#include "ccl/public/system/ierrorhandler.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/ifilesystemsecuritystore.h"
#include "ccl/public/system/iinterprocess.h"
#include "ccl/public/system/ilocaleinfo.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/ilockable.h"
#include "ccl/public/system/ilogger.h"
#include "ccl/public/system/imediathreading.h"
#include "ccl/public/system/imultiworker.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/system/iperformance.h"
#include "ccl/public/system/iprotocolhandler.h"
#include "ccl/public/system/isearcher.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ithreading.h"
#include "ccl/public/system/ithreadpool.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/system/istatistics.h"
#include "ccl/public/system/ipersistentexpression.h"
#include "ccl/public/system/ipersistentstore.h"
#include "ccl/public/system/ifilemanager.h"
#include "ccl/public/system/isafetymanager.h"
#include "ccl/public/system/idiagnosticstore.h"
#include "ccl/public/system/idiagnosticdataprovider.h"
#include "ccl/public/system/icryptor.h"
#include "ccl/public/system/ifileitem.h"
#include "ccl/public/system/ifilesystem.h"
#include "ccl/public/system/ikeyprovider.h"

// Devices
#include "ccl/public/devices/ibluetoothstatics.h"
#include "ccl/public/devices/ideviceenumerator.h"
#include "ccl/public/devices/iusbhidstatics.h"

// Plug-ins
#include "ccl/public/plugins/iclassfactory.h"
#include "ccl/public/plugins/icoderesource.h"
#include "ccl/public/plugins/icomponent.h"
#include "ccl/public/plugins/icoreplugin.h"
#include "ccl/public/plugins/idatabase.h"
#include "ccl/public/plugins/idebugservice.h"
#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/plugins/ipluginmanager.h"
#include "ccl/public/plugins/ipluginscanner.h"
#include "ccl/public/plugins/iscriptcodeloader.h"
#include "ccl/public/plugins/iscriptengine.h"
#include "ccl/public/plugins/iscriptingmanager.h"
#include "ccl/public/plugins/iservicemanager.h"
#include "ccl/public/plugins/itypelibregistry.h"

// Security
#include "ccl/public/security/iasn1contenthandler.h"
#include "ccl/public/security/iauthorizationmanager.h"
#include "ccl/public/security/iauthorizationpolicy.h"
#include "ccl/public/security/icredentialmanager.h"
#include "ccl/public/security/icryptointeger.h"
#include "ccl/public/security/icryptokeystore.h"
#include "ccl/public/security/icryptoservice.h"
