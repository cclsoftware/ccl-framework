//************************************************************************************************
//
// Bluetooth Support
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
// Filename    : gattcentral.linux.h
// Description : Bluetooth LE Gatt Central Linux
//
//************************************************************************************************

#ifndef _gattcentral_linux_h
#define _gattcentral_linux_h

#include "ccl/platform/linux/interfaces/idbussupport.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/plugins/icoreplugin.h"
#include "ccl/public/text/iregexp.h"
#include "ccl/public/text/cstring.h"

#include "core/public/coreobserver.h"
#include "core/public/devices/coregattcentral.h"

#include "services/bluetooth/meta/generated/cpp/bluetooth-constants-generated.h"

#include "org-bluez-adapter1-client.h"
#include "org-bluez-device1-client.h"
#include "org-bluez-service1-client.h"
#include "org-bluez-characteristics1-client.h"
#include "org-bluez-descriptor1-client.h"
#include <sdbus-c++/StandardInterfaces.h>

namespace CCL {
namespace Bluetooth {

using namespace Core::Bluetooth;

class LinuxGattCentral;
class LinuxGattCentralDevice;

//************************************************************************************************
// LinuxGattCentralDescriptor
//************************************************************************************************

class LinuxGattCentralDescriptor: public Object,
								  public IGattCentralDescriptor,
								  public DBusProxy<org::bluez::GattDescriptor1_proxy>
{
public:
	DECLARE_CLASS_ABSTRACT (LinuxGattCentralDescriptor, Object)

	LinuxGattCentralDescriptor (const sdbus::ObjectPath& path, IDBusSupport& dbusSupport);
	~LinuxGattCentralDescriptor ();

	PROPERTY_OBJECT (UIDBytes, uid, Uid)

	// IGattCentralDescription
	Core::ErrorCode readAsync () override;
	Core::ErrorCode writeAsync (const uint8 valueBuffer[], int valueSize) override;

	DEFINE_OBSERVER (IGattCentralDescriptorObserver)

private:
	sdbus::PendingAsyncCall readOperation;
	sdbus::PendingAsyncCall writeOperation;

	// GattDescriptor1_proxy
	void onReadValueReply (const std::vector<uint8_t>& value, const sdbus::Error* error) override;
	void onWriteValueReply (const sdbus::Error* error) override;
};

//************************************************************************************************
// LinuxGattCentralCharacteristic
//************************************************************************************************

class LinuxGattCentralCharacteristic: public Object,
									  public IGattCentralCharacteristic,
									  public DBusProxy<org::bluez::GattCharacteristic1_proxy>,
									  public DBusProxy<sdbus::Properties_proxy>
{
public:
	DECLARE_CLASS_ABSTRACT (LinuxGattCentralCharacteristic, Object)

	LinuxGattCentralCharacteristic (const sdbus::ObjectPath& path, IDBusSupport& dbusSupport);
	~LinuxGattCentralCharacteristic ();

	void addDescriptor (const sdbus::ObjectPath& path);

	// IGattCentralCharacteristic
	UIDBytes getUid () const override;
	CharacteristicProperties getProperties () const override;
	Core::ErrorCode getDescriptorsAsync (const IDFilter& descriptorFilter) override;
	Core::ErrorCode readAsync () override;
	Core::ErrorCode writeAsync (const uint8 valueBuffer[], int valueSize) override;
	Core::ErrorCode subscribeAsync () override;
	Core::ErrorCode unsubscribeAsync () override;

	// GattCharacteristic1_proxy
	using DBusProxy<org::bluez::GattCharacteristic1_proxy>::getObjectPath;
	using DBusProxy<org::bluez::GattCharacteristic1_proxy>::dbusSupport;

	DEFINE_OBSERVER (IGattCentralCharacteristicObserver)

private:
	UIDBytes uid;
	ObjectArray descriptors;

	sdbus::PendingAsyncCall readOperation;
	sdbus::PendingAsyncCall writeOperation;
	sdbus::PendingAsyncCall subscribeOperation;
	sdbus::PendingAsyncCall unsubscribeOperation;

	void deleteDescriptors ();

	// GattCharacteristic1_proxy
	void onReadValueReply (const std::vector<uint8_t>& value, const sdbus::Error* error) override;
	void onWriteValueReply (const sdbus::Error* error) override;
	void onStartNotifyReply (const sdbus::Error* error) override;
	void onStopNotifyReply (const sdbus::Error* error) override;
	void onPropertiesChanged (const std::string& interfaceName, const std::map<std::string, sdbus::Variant>& changed_properties, const std::vector<std::string>& invalidated_properties) override;
};

//************************************************************************************************
// LinuxGattCentralService
//************************************************************************************************

class LinuxGattCentralService: public Object,
							   public IGattCentralService,
							   public DBusProxy<org::bluez::GattService1_proxy>
{
public:
	DECLARE_CLASS_ABSTRACT (LinuxGattCentralService, Object)

	LinuxGattCentralService (const sdbus::ObjectPath& path, IDBusSupport& dbusSupport);
	~LinuxGattCentralService ();

	void addCharacteristic (const sdbus::ObjectPath& path);
	LinuxGattCentralCharacteristic* findCharacteristicByPath (const sdbus::ObjectPath& path) const;

	// IGattCentralService
	const UIDBytes& getServiceId () const override;
	int getNumIncludedServices () const override;
	IGattCentralService* getIncludedService (int index) const override;
	Core::ErrorCode getCharacteristicsAsync (const IDFilter& characteristicFilter) override;

	DEFINE_OBSERVER (IGattCentralServiceObserver)

private:
	UIDBytes serviceId;
	ObjectArray characteristics;

	void deleteCharacteristics ();
};

//************************************************************************************************
// LinuxGattCentralDevice
//************************************************************************************************

class LinuxGattCentralDevice: public Object,
							  public IGattCentralDevice,
							  public DBusProxy<sdbus::Properties_proxy>,
							  public DBusProxy<org::bluez::Device1_proxy>
{
public:
	DECLARE_CLASS_ABSTRACT (LinuxGattCentralDevice, Object)

	LinuxGattCentralDevice (LinuxGattCentral& central, const sdbus::ObjectPath& path, IDBusSupport& dbusSupport);
	~LinuxGattCentralDevice () override;

	PROPERTY_VARIABLE (ConnectionState, connectionState, ConnectionState)
	PROPERTY_BOOL (deviceValidated, DeviceValidated)

	void close ();
	void addService (const sdbus::ObjectPath& path);
	LinuxGattCentralService* findServiceByPath (const sdbus::ObjectPath& path) const;
	const ObjectArray& getServices () const;

	// Device1_proxy
	using DBusProxy<org::bluez::Device1_proxy>::getObjectPath;

	// IGattCentralDevice
	CStringPtr getIdentifier () const override;
	CStringPtr getName () const override;
	CStringPtr getManufacturerData () const override;
	tbool isConnected () const override;
	Core::ErrorCode setConnectionMode (ConnectionMode connectionMode) override;
	Core::ErrorCode getServicesAsync () override;

	DEFINE_OBSERVER (IGattCentralDeviceObserver)

private:
	LinuxGattCentral& central;

	MutableCString name;
	MutableCString identifier;
	MutableCString manufacturerData;
	bool servicesResolved = false;
	bool connected = false;
	ObjectArray services;

	static void manufacturerDataToCStr (MutableCString& result, const std::map<uint16, sdbus::Variant>& rawData);

	void setServicesResolved (bool resolved);
	void setConnected (bool state);
	void deleteServices ();

	// Properties_proxy
	void onPropertiesChanged (const std::string& interfaceName, const std::map<std::string, sdbus::Variant>& changed_properties, const std::vector<std::string>& invalidated_properties) override;

	// Device1_proxy
	void onConnectReply (const sdbus::Error* error) override;
	void onDisconnectReply (const sdbus::Error* error) override;
};

//************************************************************************************************
// ObjectManagerProxy
//************************************************************************************************

class ObjectManagerProxy: public Object,
						  public DBusProxy<sdbus::ObjectManager_proxy>
{
public:
	ObjectManagerProxy (IDBusSupport& dbusSupport, LinuxGattCentral& central);

	static constexpr CStringPtr kObjectPath = "/";

	void onInterfacesAdded (const sdbus::ObjectPath& path, const std::map<std::string, std::map<std::string, sdbus::Variant>>& interfaces) override;
	void onInterfacesRemoved (const sdbus::ObjectPath& path, const std::vector<std::string>& interfaces) override;

private:
	LinuxGattCentral& central;

	AutoPtr<IRegularExpression> deviceRegExp = System::CreateRegularExpression ();
	AutoPtr<IRegularExpression> serviceRegExp = System::CreateRegularExpression ();
	AutoPtr<IRegularExpression> characteristicRegExp = System::CreateRegularExpression ();
	AutoPtr<IRegularExpression> descriptorRegExp = System::CreateRegularExpression ();
};

//************************************************************************************************
// AdapterProxy
//************************************************************************************************

class AdapterProxy: public Object,
					public DBusProxy<org::bluez::Adapter1_proxy>
{
public:
	AdapterProxy (IDBusSupport& dbusSupport);

	static constexpr CStringPtr kObjectPath = "/org/bluez/hci0";

	// expected error messages
	static constexpr CStringPtr kErrorInProgress = "org.bluez.Error.InProgress";
	static constexpr CStringPtr kErrorNotStarted = "No discovery started";
};

//************************************************************************************************
// LinuxGattCentral
//************************************************************************************************

class LinuxGattCentral: public CorePropertyHandler<IGattCentral, Object, IObject>
{
public:
	DECLARE_CLASS_ABSTRACT (LinuxGattCentral, Object)

	LinuxGattCentral ();
	~LinuxGattCentral ();

	PROPERTY_POINTER (IDBusSupport, dbusSupport, DBusSupport)

	void closeDevice (LinuxGattCentralDevice* device);
	bool addNewDevice (const sdbus::ObjectPath& path);
	bool isDeviceDataValid (LinuxGattCentralDevice* device) const;

	void notifyDeviceFound (LinuxGattCentralDevice* device);
	void notifyConnectionStatusChanged (LinuxGattCentralDevice* device, ConnectionState newStatus, ConnectionState oldStatus);
	void notifyGattServicesAvailable (LinuxGattCentralDevice* device);
	void notifyDeviceDisconnected (LinuxGattCentralDevice* device);

	LinuxGattCentralDevice* findDeviceByPath (const sdbus::ObjectPath& path) const;
	LinuxGattCentralService* findServiceByPath (const sdbus::ObjectPath& path) const;
	LinuxGattCentralCharacteristic* findCharacteristicByPath (const sdbus::ObjectPath& path) const;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IGattCentral
	GattCentralState getState () const override;
	Core::ErrorCode startScanning (const IDFilter& serviceFilter, const GattCentralScanOptions& scanOptions = {}) override;
	Core::ErrorCode stopScanning () override;
	Core::ErrorCode connectAsync (IGattCentralDevice* device, tbool autoReconnect) override;
	Core::ErrorCode disconnectAsync (IGattCentralDevice* device) override;

	DEFINE_OBSERVER (IGattCentralObserver)

private:
	GattCentralState centralState;
	ObjectArray devices;
	AutoPtr<AdapterProxy> adapterProxy;
	AutoPtr<ObjectManagerProxy> objectManagerProxy;
	IDFilter idFilter;

	sdbus::PendingAsyncCall connectOperation;
	sdbus::PendingAsyncCall disconnectOperation;

	void investigateExistingDevices ();
	void updateCentralState (GattCentralState newState);
};

} // namespace Bluetooth
} // namespace CCL

#endif // _gattcentral_linux_h
