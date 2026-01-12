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
// Filename    : gattcentral.linux.cpp
// Description : Bluetooth LE Gatt Central Linux
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "gattcentral.linux.h"
#include "gattshared.linux.h"

#include "ccl/base/message.h"

#include "ccl/platform/linux/interfaces/ilinuxsystem.h"
#include "ccl/platform/linux/interfaces/idbussupport.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/text/stringbuilder.h"

using namespace Core;
using namespace Core::Errors;
using namespace Core::Bluetooth;

using namespace CCL;
using namespace CCL::Bluetooth;

namespace CCL {
namespace Bluetooth {
	const CStringPtr kInterfaceDevice = "org.bluez.Device1";
	const CStringPtr kInterfaceService = "org.bluez.GattService1";
	const CStringPtr kInterfaceCharacteristics = "org.bluez.GattCharacteristic1";
	const CStringPtr kInterfaceDescriptor = "org.bluez.GattDescriptor1";
	const CStringPtr kInterfaceProperties = "org.freedesktop.DBus.Properties";
	const CStringPtr kDestinationBluez = "org.bluez";
	const CStringPtr kPropertyName = "Name";
	const CStringPtr kPropertyManufacturerData = "ManufacturerData";
	const CStringPtr kPropertyUUIDs = "UUIDs";
	const CStringPtr kPropertyConnected = "Connected";
	const CStringPtr kPropertyServicesResolved = "ServicesResolved";
	const CStringPtr kPropertyDevice = "Device";
	const CStringPtr kPropertyService = "Service";
	const CStringPtr kPropertyCharacteristic = "Characteristic";
}
}

//************************************************************************************************
// LinuxGattCentralDescriptor
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LinuxGattCentralDescriptor, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattCentralDescriptor::LinuxGattCentralDescriptor (const sdbus::ObjectPath& path, IDBusSupport& dbusSupport)
: DBusProxy (dbusSupport, kDestinationBluez, path, true)
{
	fromBluezGuid (uid, UUID ().c_str ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattCentralDescriptor::~LinuxGattCentralDescriptor ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattCentralDescriptor::readAsync ()
{
	CCL_PRINTF ("[%s]\n", __func__);

	readOperation = ReadValue ({});

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralDescriptor::onReadValueReply (const std::vector<uint8_t>& value, const sdbus::Error* error)
{
	// ASSERT Main Thread
	if(error)
	{
		PRINT_DBUS_ERROR (*error, "[onReadValueReply]")
		observers.notify (&IGattCentralDescriptorObserver::onReadCompleted, nullptr, 0, kError_Failed);
		return;
	}
	observers.notify (&IGattCentralDescriptorObserver::onReadCompleted, value.data (), int(value.size ()), kError_NoError);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattCentralDescriptor::writeAsync (const uint8 valueBuffer[], int valueSize)
{
	std::vector<uint8> vec (valueBuffer, valueBuffer + valueSize);
	writeOperation = WriteValue (vec, {});

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralDescriptor::onWriteValueReply (const sdbus::Error* error)
{
	if(error)
		PRINT_DBUS_ERROR (*error, "[onWriteValueReply]")

	observers.notify (&IGattCentralDescriptorObserver::onWriteCompleted, error ? kError_Failed : kError_NoError);
}

//************************************************************************************************
// LinuxGattCentralCharacteristic
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LinuxGattCentralCharacteristic, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattCentralCharacteristic::LinuxGattCentralCharacteristic (const sdbus::ObjectPath& path, IDBusSupport& dbusSupport)
: DBusProxy<org::bluez::GattCharacteristic1_proxy> (dbusSupport, kDestinationBluez, path, true),
  DBusProxy<sdbus::Properties_proxy> (dbusSupport, kDestinationBluez, path, true)
{
	fromBluezGuid (uid, UUID ().c_str ());
	descriptors.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattCentralCharacteristic::~LinuxGattCentralCharacteristic ()
{
	deleteDescriptors ();
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDBytes LinuxGattCentralCharacteristic::getUid () const
{
	return uid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CharacteristicProperties LinuxGattCentralCharacteristic::getProperties () const
{
	// TODO: implement
	return kNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattCentralCharacteristic::getDescriptorsAsync (const IDFilter& descriptorFilter)
{
	Vector<IGattCentralDescriptor*> filteredDescriptors;
	for(auto* descriptor : iterate_as<LinuxGattCentralDescriptor> (descriptors))
	{
		UIDRef uid = descriptor->getUid ();
		if(descriptorFilter.numIds == 0 || descriptorFilter.contains (uid))
			filteredDescriptors.add (descriptor);
	}

	observers.notify (&IGattCentralCharacteristicObserver::onGetDescriptorsCompleted, filteredDescriptors.getItems (), filteredDescriptors.count (), kError_NoError);

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattCentralCharacteristic::readAsync ()
{
	readOperation = ReadValue ({});
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralCharacteristic::onReadValueReply (const std::vector<uint8_t>& value, const sdbus::Error* error)
{
	if(error)
	{
		PRINT_DBUS_ERROR (*error, "[onReadValueReply]")
		observers.notify (&IGattCentralCharacteristicObserver::onReadCompleted, nullptr, 0, kError_Failed);
		return;
	}
	observers.notify (&IGattCentralCharacteristicObserver::onReadCompleted, value.data (), int(value.size ()), kError_NoError);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattCentralCharacteristic::subscribeAsync ()
{
	CCL_PRINTF ("[%s] uid: %s. Notifying: %d\n", __func__, UIDCString (uid).str (), Notifying ());

	subscribeOperation = StartNotify ();

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralCharacteristic::onStartNotifyReply (const sdbus::Error* error)
{
	if(error)
		PRINT_DBUS_ERROR (*error, "[onStartNotifyReply]")

	observers.notify (&IGattCentralCharacteristicObserver::onSubscribeCompleted, Notifying () ? kError_NoError : kError_Failed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattCentralCharacteristic::unsubscribeAsync ()
{
	CCL_PRINTF ("[%s]\n", __func__);

	unsubscribeOperation = StopNotify ();

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralCharacteristic::onStopNotifyReply (const sdbus::Error* error)
{
	if(error)
		PRINT_DBUS_ERROR (*error, "[onStopNotifyReply]")

	observers.notify (&IGattCentralCharacteristicObserver::onUnsubscribeCompleted, !Notifying () ? kError_NoError : kError_Failed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattCentralCharacteristic::writeAsync (const uint8 valueBuffer[], int valueSize)
{
	std::vector<uint8> vec (valueBuffer, valueBuffer + valueSize);
	writeOperation = WriteValue (vec, {});

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralCharacteristic::onWriteValueReply (const sdbus::Error* error)
{
	if(error)
		PRINT_DBUS_ERROR (*error, "[onWriteValueReply]")

	observers.notify (&IGattCentralCharacteristicObserver::onWriteCompleted, !error ? kError_NoError : kError_Failed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralCharacteristic::onPropertiesChanged (const std::string& interfaceName, const std::map<std::string, sdbus::Variant>& changed_properties, const std::vector<std::string>& invalidated_properties)
{
	for(const auto& [propertyName, propertyValue] : changed_properties)
	{
		if(propertyName == "Value" && propertyValue.peekValueType () == "ay")
		{
			observers.notify (&IGattCentralCharacteristicObserver::onNotificationReceived, (static_cast<std::vector<uint8>> (propertyValue)).data (), int((static_cast<std::vector<uint8>>(propertyValue)).size ()));
			return;
		}
		#if DEBUG_LOG
		MutableCString out = "can not interpret";
		if(propertyValue.peekValueType () == "b")
			out = bool(propertyValue) ? "true" : "false";
		CCL_PRINTF ("[PropertiesChanged] key: %s type: %s value: %s\n", propertyName.c_str (), propertyValue.peekValueType ().c_str (), out.str ());
		#endif
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralCharacteristic::deleteDescriptors ()
{
	descriptors.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralCharacteristic::addDescriptor (const sdbus::ObjectPath& path)
{
	descriptors.add (NEW LinuxGattCentralDescriptor (path, dbusSupport));
}

//************************************************************************************************
// LinuxGattCentralService
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LinuxGattCentralService, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattCentralService::LinuxGattCentralService (const sdbus::ObjectPath& path, IDBusSupport& dbusSupport)
: DBusProxy (dbusSupport, kDestinationBluez, path, true)
{
	fromBluezGuid (serviceId, UUID ().c_str ());
	characteristics.objectCleanup ();

	// TODO init included services
	// 	includedServices.add (NEW LinuxGattCentralService (...));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattCentralService::~LinuxGattCentralService ()
{
	cancelSignals ();
	deleteCharacteristics ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattCentralService::getCharacteristicsAsync (const IDFilter& characteristicFilter)
{
	CCL_PRINTF ("[%s]\n", __func__);

	Vector<IGattCentralCharacteristic*> filteredCharacteristics;
	for(auto* characteristic : iterate_as<LinuxGattCentralCharacteristic> (characteristics))
	{
		UIDRef uid = characteristic->getUid ();
		if(characteristicFilter.numIds == 0 || characteristicFilter.contains (uid))
			filteredCharacteristics.add (characteristic);
	}
	observers.notify (&IGattCentralServiceObserver::onGetCharacteristicsCompleted, filteredCharacteristics.getItems (), filteredCharacteristics.count (), kError_NoError);

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const UIDBytes& LinuxGattCentralService::getServiceId () const
{
	return serviceId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int LinuxGattCentralService::getNumIncludedServices () const
{
	// TODO
	// return Includes ().size ();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGattCentralService* LinuxGattCentralService::getIncludedService (int index) const
{
	// TODO
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralService::deleteCharacteristics ()
{
	characteristics.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattCentralCharacteristic* LinuxGattCentralService::findCharacteristicByPath (const sdbus::ObjectPath& path) const
{
	for(auto* characteristic : iterate_as<LinuxGattCentralCharacteristic> (characteristics))
		if(characteristic->getObjectPath () == path)
			return characteristic;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralService::addCharacteristic (const sdbus::ObjectPath& path)
{
	characteristics.add (NEW LinuxGattCentralCharacteristic (path, dbusSupport));
}

//************************************************************************************************
// LinuxGattCentralDevice
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LinuxGattCentralDevice, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralDevice::manufacturerDataToCStr (MutableCString& result, const std::map<uint16, sdbus::Variant>& rawData)
{
	result.empty ();
	for(auto& [vendorKey, bytes] : rawData)
		for(u_char& byte : static_cast<std::vector<u_char>> (bytes))
			result.append (reinterpret_cast<char*> (&byte), 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattCentralDevice::LinuxGattCentralDevice (LinuxGattCentral& central, const sdbus::ObjectPath& path, IDBusSupport& dbusSupport)
: DBusProxy<sdbus::Properties_proxy> (dbusSupport, kDestinationBluez, path, true),
  DBusProxy<org::bluez::Device1_proxy> (dbusSupport, kDestinationBluez, path, true),
  central (central),
  name (nullptr),
  identifier (Address ().c_str ()),
  manufacturerData (nullptr),
  connectionState (kConnectionStateDisconnected),
  deviceValidated (false)
{
	try // some properties might be not set
	{
		std::map<std::string, sdbus::Variant> properties = GetAll (kInterfaceDevice);
		if(properties.count (kPropertyManufacturerData) == 1)
			manufacturerDataToCStr (manufacturerData, properties.at (kPropertyManufacturerData));
		if(properties.count (kPropertyName) == 1)
			name = (static_cast<std::string> (properties.at (kPropertyName))).c_str ();
	}
	CATCH_DBUS_ERROR
	services.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattCentralDevice::~LinuxGattCentralDevice ()
{
	cancelSignals ();
	close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectArray& LinuxGattCentralDevice::getServices () const
{
	return services;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralDevice::onPropertiesChanged (const std::string& interfaceName, const std::map<std::string, sdbus::Variant>& changed_properties, const std::vector<std::string>& invalidated_properties)
{
	bool relevantDataChanged = false;
	for(const auto& [propName, propValue] : changed_properties)
	{
		if(propName == kPropertyUUIDs)
		{
			relevantDataChanged = true;

			#if DEBUG_LOG
			MutableCString uuidStr;
			for(const std::string& id : UUIDs ())
				uuidStr.append (id.c_str ()).append (" ");
			CCL_PRINTF ("%s changed uuids: %s\n", name.str (), uuidStr.str ());
			#endif
		}
		else if(propName == kPropertyManufacturerData)
		{
			manufacturerDataToCStr (manufacturerData, propValue);
			relevantDataChanged = true;

			CCL_PRINTF ("%s changed ManufacturerData: %s\n", name.str (), manufacturerData.str ());
		}
		else if(propName == kPropertyConnected)
			setConnected (propValue);
		else if(propName == kPropertyServicesResolved)
			setServicesResolved (propValue);
	}

	if(relevantDataChanged && central.isDeviceDataValid (this))
		central.notifyDeviceFound (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralDevice::onConnectReply (const sdbus::Error* error)
{
	if(error)
	{
		PRINT_DBUS_ERROR (*error, "[onConnectReply]")
		return;
	}
	setConnected (Connected ());
	setServicesResolved (ServicesResolved ()); // might be resolved already
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralDevice::onDisconnectReply (const sdbus::Error* error)
{
	if(error)
	{
		PRINT_DBUS_ERROR (*error, "[onDisconnectReply]")
		return;
	}
	central.notifyDeviceDisconnected (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralDevice::setServicesResolved (bool resolved)
{
	// avoid multiple notifications to observers and only notify observers that validated this device
	if(servicesResolved == resolved || !isDeviceValidated () || !connected)
		return;

	servicesResolved = resolved;
	CCL_PRINTF ("central: '%d': device '%s': changed 'servicesResolved': %s\n", central.getHashCode (10), name.str (), servicesResolved ? "true" : "false");

	if(servicesResolved)
		central.notifyGattServicesAvailable (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralDevice::setConnected (bool value)
{
	// avoid multiple notifications to observers and only notify observers that validated this device
	if(connected == value || !isDeviceValidated ())
		return;

	connected = value;
	CCL_PRINTF ("central: %d : device %s : changed 'Connected': %s\n", central.getHashCode (10), name.str (), connected ? "true" : "false");

	ConnectionState oldStatus = getConnectionState ();
	setConnectionState (connected ? kConnectionStateConnected : kConnectionStateDisconnected);
	central.notifyConnectionStatusChanged (this, getConnectionState (), oldStatus);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr LinuxGattCentralDevice::getIdentifier () const
{
	return identifier.str ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr LinuxGattCentralDevice::getName () const
{
	return name.str ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr LinuxGattCentralDevice::getManufacturerData () const
{
	return manufacturerData.str ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool LinuxGattCentralDevice::isConnected () const
{
	return connectionState == kConnectionStateConnected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattCentralDevice::setConnectionMode (ConnectionMode connectionMode)
{
	// TODO: implement
	return kError_NotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattCentralDevice::getServicesAsync ()
{
	CCL_PRINTF ("[%s]\n", __func__);

	if(!servicesResolved)
		return kError_InvalidState;

	Vector<IGattCentralService*> filteredServices;
	for(auto* service : iterate_as<LinuxGattCentralService> (services))
		filteredServices.add (service);
	observers.notify (&IGattCentralDeviceObserver::onGetServicesCompleted, filteredServices.getItems (), filteredServices.count (), kError_NoError);

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralDevice::close ()
{
	deleteServices ();

	if(getConnectionState () != kConnectionStateDisconnected)
	{
		CCL_PRINTF ("[Device::close] Device is still connected while closing. Disconnecting ...\n", 0)
		try
		{
			Disconnect ();
		}
		CATCH_DBUS_ERROR
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralDevice::addService (const sdbus::ObjectPath& path)
{
	CCL_PRINTF ("[%s] central %d expected #: %d added # %d\n", __func__, central.getHashCode (10), UUIDs ().size (), services.count () + 1);
	services.add (NEW LinuxGattCentralService (path, DBusProxy<org::bluez::Device1_proxy>::dbusSupport));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentralDevice::deleteServices ()
{
	services.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattCentralService* LinuxGattCentralDevice::findServiceByPath (const sdbus::ObjectPath& path) const
{
	for(auto* service : iterate_as<LinuxGattCentralService> (services))
		if(service->getObjectPath () == path)
			return service;
	return nullptr;
}

//************************************************************************************************
// LinuxGattCentral
//************************************************************************************************

DEFINE_CLASS_HIDDEN (LinuxGattCentral, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattCentral::LinuxGattCentral ()
: centralState (kPoweredOn)
{
	UnknownPtr<ILinuxSystem> linuxSystem (&System::GetSystem ());
	ASSERT (linuxSystem.isValid ())

	dbusSupport = linuxSystem ? linuxSystem->getDBusSupport () : nullptr;
	ASSERT (dbusSupport)
	if(dbusSupport)
	{
		adapterProxy = NEW AdapterProxy (*dbusSupport);
		objectManagerProxy = NEW ObjectManagerProxy (*dbusSupport, *this);
	}
	else
	{
		CCL_WARN ("Unable to find DBusSupport\n", 0)
	}
	devices.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattCentral::~LinuxGattCentral ()
{
	stopScanning ();
	cancelSignals ();
	devices.removeAll();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentral::closeDevice (LinuxGattCentralDevice* device)
{
	if(device != nullptr)
	{
		int index = devices.index (device);
		if(index != -1)
		{
			observers.notify (&IGattCentralObserver::onDeviceRemoved, device);
			device->close ();
			device->release ();
			devices.removeAt (index);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentral::notifyDeviceFound (LinuxGattCentralDevice* device)
{
	if(!device->isDeviceValidated ())
	{
		ASSERT (device != nullptr)
		ASSERT (device->getIdentifier () != nullptr)
		ASSERT (device->getName () != nullptr)
		ASSERT (device->getManufacturerData () != nullptr)
		ASSERT (devices.contains (device))

		device->setDeviceValidated (true);
		CCL_PRINTF ("[%s] Adding valid device: '%s' model: '%s', numIDs: %d\n", __func__, device->getName (), device->getManufacturerData (), idFilter.numIds);
		observers.notify (&IGattCentralObserver::onDeviceAdded, device);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentral::notifyConnectionStatusChanged (LinuxGattCentralDevice* device, ConnectionState newStatus, ConnectionState oldStatus)
{
	CCL_PRINTF ("[%s] %s (old state: %i): new state %i\n", __func__, device->getName (), oldStatus, newStatus);

	if(oldStatus == kConnectionStateDisconnected && newStatus == kConnectionStateConnected)
	{
		// the connection has been established automatically
		observers.notify (&IGattCentralObserver::onConnectionRestored, device);
	}
	else if(oldStatus == kConnectionStateConnected && newStatus == kConnectionStateDisconnected)
	{
		// the connection has been terminated automatically
		closeDevice (device);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentral::notifyGattServicesAvailable (LinuxGattCentralDevice* device)
{
	ASSERT (device->getConnectionState () == kConnectionStateConnected)
	observers.notify (&IGattCentralObserver::onConnectCompleted, device, kError_NoError);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxGattCentral::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "scanningStarted")
		observers.notify (&IGattCentralObserver::onScanningStarted);
	else if(msg == "scanningStopped")
		observers.notify (&IGattCentralObserver::onScanningStopped);
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GattCentralState LinuxGattCentral::getState () const
{
	// TODO
	return centralState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentral::updateCentralState (GattCentralState newState)
{
	if(centralState != newState)
	{
		centralState = newState;
		observers.notify (&IGattCentralObserver::onStateChanged, centralState);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattCentral::startScanning (const IDFilter& serviceFilter, const GattCentralScanOptions& scanOptions)
{
	CCL_PRINTF ("[LinuxGattCentral:: %s]\n", __func__)
	if(!adapterProxy)
		return kError_Failed;
	// copy filter to compare found serviceIDs within the callback
	idFilter = serviceFilter;

	// remove unconnected devices
	for(int i = devices.count () - 1; i >= 0; i--)
	{
		auto* device = static_cast<LinuxGattCentralDevice*> (devices[i]);
		if(!device->isConnected ())
			closeDevice (device);
	}

	// check existing devices first
	investigateExistingDevices ();

	// enable scanning
	try
	{
		adapterProxy->SetDiscoveryFilter ({{"Transport", "le"}});
		adapterProxy->StartDiscovery ();
	}
	catch(const sdbus::Error& e)
	{
		if(*e.getName ().c_str () != *AdapterProxy::kErrorInProgress)
		{
			PRINT_DBUS_ERROR (e, "[startScanning]")
			updateCentralState (kPermissionDenied);
			return kError_Failed;
		}
		CCL_PRINTF ("Expected error while starting scan: %s: %s\n", e.getName ().c_str (), e.getMessage ().c_str ());
	}

	// inform observers
	(NEW Message ("scanningStarted"))->post (this);
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattCentral::stopScanning ()
{
	if(!adapterProxy)
		return kError_Failed;
	try
	{
		adapterProxy->StopDiscovery ();
	}
	catch(const sdbus::Error& e)
	{
		if(*e.getMessage ().c_str () != *AdapterProxy::kErrorNotStarted) // is thrown when not running
		{
			PRINT_DBUS_ERROR (e, "[stopScanning]")
			return kError_Failed;
		}
		CCL_PRINTF ("Expected error while stopping scan: %s: %s\n", e.getName ().c_str (), e.getMessage ().c_str ());
	}
	(NEW Message ("scanningStopped"))->post (this);
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentral::investigateExistingDevices ()
{
	if(!objectManagerProxy)
		return;
	std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>> result = objectManagerProxy->GetManagedObjects ();
	for(const auto& [path, interfaces] : result)
		objectManagerProxy->onInterfacesAdded (path, interfaces);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxGattCentral::addNewDevice (const sdbus::ObjectPath& path)
{
	// ignore this ad if we already know this device
	if(findDeviceByPath (path) != nullptr)
		return false;

	devices.add (NEW LinuxGattCentralDevice (*this, path, *dbusSupport));
	if(isDeviceDataValid (static_cast<LinuxGattCentralDevice*> (devices.last ())))
		notifyDeviceFound (static_cast<LinuxGattCentralDevice*> (devices.last ()));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattCentral::connectAsync (IGattCentralDevice* device, tbool autoReconnect)
{
	// TODO: autoReconnect:
	// GattSession.MaintainConnection

	if(device == nullptr)
		return kError_InvalidArgument;

	LinuxGattCentralDevice* linuxDevice = static_cast<LinuxGattCentralDevice*> (device);
	ASSERT (linuxDevice != nullptr && devices.contains (linuxDevice))

	ASSERT (linuxDevice->getConnectionState () == kConnectionStateDisconnected)
	if(linuxDevice->getConnectionState () != kConnectionStateDisconnected)
		return kError_InvalidState;

	linuxDevice->setConnectionState (kConnectionStateConnecting);
	try
	{
		connectOperation = linuxDevice->Connect ();
	}
	catch(const sdbus::Error& e)
	{
		PRINT_DBUS_ERROR(e, "[connectAsync]")
		return kError_Failed;
	}

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode LinuxGattCentral::disconnectAsync (IGattCentralDevice* device)
{
	if(device == nullptr)
		return kError_ItemNotFound;

	CCL_PRINTF ("[%s] '%s'\n", __func__, device->getName ());

	LinuxGattCentralDevice* linuxDevice = static_cast<LinuxGattCentralDevice*> (device);
	ASSERT (linuxDevice != nullptr && devices.contains (linuxDevice))

	ASSERT (linuxDevice->getConnectionState () == kConnectionStateConnected ||
			linuxDevice->getConnectionState () == kConnectionStateConnecting)

	linuxDevice->setConnectionState (kConnectionStateDisconnecting);
	connectOperation.cancel ();
	disconnectOperation = linuxDevice->Disconnect ();

	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxGattCentral::notifyDeviceDisconnected (LinuxGattCentralDevice* device)
{
	CCL_PRINTF ("[%s] '%s'\n", __func__, device->getName ());
	device->setConnectionState (kConnectionStateDisconnected);
	observers.notify (&IGattCentralObserver::onDisconnectCompleted, device, kError_NoError);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattCentralDevice* LinuxGattCentral::findDeviceByPath (const sdbus::ObjectPath& path) const
{
	for(auto* device : iterate_as<LinuxGattCentralDevice> (devices))
	{
		if(!device->isDeviceValidated ())
			continue;
		if(device->getObjectPath () == path)
			return device;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattCentralService* LinuxGattCentral::findServiceByPath (const sdbus::ObjectPath& path) const
{
	for(auto* device : iterate_as<LinuxGattCentralDevice> (devices))
	{
		if(!device->isDeviceValidated ())
			continue;
		if(LinuxGattCentralService* service = device->findServiceByPath (path); service)
			return service;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxGattCentralCharacteristic* LinuxGattCentral::findCharacteristicByPath (const sdbus::ObjectPath& path) const
{
	for(auto* device : iterate_as<LinuxGattCentralDevice> (devices))
	{
		if(!device->isDeviceValidated ())
			continue;
		for(auto* service : iterate_as<LinuxGattCentralService> (device->getServices ()))
			if(LinuxGattCentralCharacteristic* characteristic = service->findCharacteristicByPath (path); characteristic)
				return characteristic;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxGattCentral::isDeviceDataValid (LinuxGattCentralDevice* device) const
{
	// no filter -> all valid
	if(idFilter.numIds == 0)
		return true;

	// compare uuids
	for(int i = 0; i < idFilter.numIds; i++)
		for(const std::string& uuid : device->UUIDs ())
		{
			UIDBytes uidBytes;
			fromBluezGuid (uidBytes, uuid.c_str ());
			if(uidBytes == idFilter.ids[i])
				return true;
		}

	return false;
}

//************************************************************************************************
// AdapterProxy
//************************************************************************************************

AdapterProxy::AdapterProxy (IDBusSupport& dbusSupport)
: DBusProxy (dbusSupport, kDestinationBluez, kObjectPath, true) 
{};

//************************************************************************************************
// ObjectManagerProxy
//************************************************************************************************

ObjectManagerProxy::ObjectManagerProxy (IDBusSupport& dbusSupport, LinuxGattCentral& central)
: DBusProxy (dbusSupport, kDestinationBluez, kObjectPath, true),
  central (central)
{
	deviceRegExp->construct ("^/org/bluez/hci[0-9]/dev(_[0-9A-F]{2}){6}$");
	serviceRegExp->construct ("^/org/bluez/hci\\d/dev(_[0-9A-F]{2}){6}/service[0-9a-fA-F]{4}$");
	characteristicRegExp->construct ("^/org/bluez/hci\\d/dev(_[0-9A-F]{2}){6}/service[0-9a-fA-F]{4}/char[0-9a-fA-F]{4}$");
	descriptorRegExp->construct ("^/org/bluez/hci\\d/dev(_[0-9A-F]{2}){6}/service[0-9a-fA-F]{4}/char[0-9a-fA-F]{4}/desc[0-9a-fA-F]{4}$");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectManagerProxy::onInterfacesAdded (const sdbus::ObjectPath& path, const std::map<std::string, std::map<std::string, sdbus::Variant>>& interfaces)
{
	if(deviceRegExp->isFullMatch (path.c_str ()) && interfaces.count (kInterfaceDevice) == 1)
	{
		central.addNewDevice (path);
	}
	else if(serviceRegExp->isFullMatch (path.c_str ()) && (interfaces.count (kInterfaceService) == 1) &&
			(interfaces.at (kInterfaceService).count (kPropertyDevice) == 1))
	{
		LinuxGattCentralDevice* device = central.findDeviceByPath (interfaces.at (kInterfaceService).at (kPropertyDevice));
		if(!device)
			return;
		device->addService (path);
	}
	else if(characteristicRegExp->isFullMatch (path.c_str ()) && (interfaces.count (kInterfaceCharacteristics) == 1) &&
			(interfaces.at (kInterfaceCharacteristics).count (kPropertyService) == 1))
	{
		LinuxGattCentralService* service = central.findServiceByPath (interfaces.at (kInterfaceCharacteristics).at (kPropertyService));
		if(!service)
			return;
		service->addCharacteristic (path);
	}
	else if(descriptorRegExp->isFullMatch (path.c_str ()) && (interfaces.count (kInterfaceDescriptor) == 1) &&
			(interfaces.at (kInterfaceDescriptor).count (kPropertyCharacteristic) == 1))
	{
		LinuxGattCentralCharacteristic* characteristic = central.findCharacteristicByPath (interfaces.at (kInterfaceDescriptor).at (kPropertyCharacteristic));
		if(!characteristic)
			return;
		characteristic->addDescriptor (path);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectManagerProxy::onInterfacesRemoved (const sdbus::ObjectPath& path, const std::vector<std::string>& interfaces)
{
	CCL_PRINTF ("onInterfacesRemoved: %s\n", path.c_str ());
	if(deviceRegExp->isFullMatch (path.c_str ()))
	{
		LinuxGattCentralDevice* device = central.findDeviceByPath (path);
		if(device)
			central.closeDevice (device);
	}
}
