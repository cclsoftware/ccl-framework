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
// Filename    : gattperipheral.win.cpp
// Description : Bluetooth LE Gatt Peripheral
//
//************************************************************************************************

#include "gattperipheral.win.h"
#include "gattshared.win.h"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>

#include "ccl/public/base/debug.h"

using namespace Core;
using namespace Core::Errors;

using namespace CCL;
using namespace CCL::Bluetooth;

//using namespace winrt;
//using namespace winrt::Windows::Devices::Bluetooth;
//using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;

using WinRTGattLocalDescriptor = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattLocalDescriptor;
using WinRTGattLocalCharacteristic = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattLocalCharacteristic;
using WinRTGattReadRequestedEventArgs = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattReadRequestedEventArgs;
using WinRTGattWriteRequestedEventArgs = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattWriteRequestedEventArgs;
using WinRTGattReadRequest = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattReadRequest;
using WinRTGattWriteRequest = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattWriteRequest;
using WinRTGattWriteOption = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattWriteOption;
using WinRTGattProtocolError = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattProtocolError;
using WinRTGattClientNotificationResult = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattClientNotificationResult;
using WinRTGattLocalDescriptorParameters = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattLocalDescriptorParameters;
using WinRTGattLocalCharacteristicParameters = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattLocalCharacteristicParameters;
using WinRTGattLocalDescriptorResult = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattLocalDescriptorResult;
using WinRTBluetoothError = winrt::Windows::Devices::Bluetooth::BluetoothError;
using WinRTGattCharacteristicProperties = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristicProperties;
using WinRTGattLocalCharacteristicResult = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattLocalCharacteristicResult;
using WinRTGattServiceProviderAdvertisingParameters = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattServiceProviderAdvertisingParameters;
using WinRTGattServiceProvider = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattServiceProvider;
using WinRTGattServiceProviderResult = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattServiceProviderResult;

template <class T> using IAsyncOperation = winrt::Windows::Foundation::IAsyncOperation<T>;
using AsyncStatus = winrt::Windows::Foundation::AsyncStatus;
template <class T> using IVectorView = winrt::Windows::Foundation::Collections::IVectorView<T>;

//************************************************************************************************
// WindowsGattPeripheralDescriptor
//************************************************************************************************

WindowsGattPeripheralDescriptor::WindowsGattPeripheralDescriptor ()
{
	readToken = descriptor.ReadRequested ([&] (WinRTGattLocalDescriptor const& sender, WinRTGattReadRequestedEventArgs args)
	{
		auto readDeferral = args.GetDeferral ();
		auto asyncop = args.GetRequestAsync();
		asyncop.Completed ([&, readDeferral] (IAsyncOperation<WinRTGattReadRequest> op, const AsyncStatus status)
		{
			if(op.GetResults () != nullptr)
			{
				auto valueBuffer = NEW uint8[kAttributeCapacity];
				int valueSize;
				observers.notify (&IGattPeripheralDescriptorObserver::onRead, valueBuffer, valueSize);
				winrt::Windows::Storage::Streams::Buffer buffer (valueSize);
				memcpy (buffer.data (), valueBuffer, valueSize);
				delete valueBuffer;
				op.GetResults ().RespondWithValue (buffer);
			}
			readDeferral.Complete ();
		});
	});

	writeToken = descriptor.WriteRequested ([&] (WinRTGattLocalDescriptor const& sender, WinRTGattWriteRequestedEventArgs args)
	{
		auto writeDeferral = args.GetDeferral ();

		auto asyncop = args.GetRequestAsync();
		asyncop.Completed ([&, writeDeferral] (IAsyncOperation<WinRTGattWriteRequest> op, const AsyncStatus status)
		{
			if(op.GetResults () != nullptr)
			{
				uint8 valueBuffer[kAttributeCapacity];
				int valueSize = op.GetResults ().Value (). Length ();
				memcpy (valueBuffer, op.GetResults ().Value ().data (), op.GetResults ().Value (). Length ());
				observers.notify (&IGattPeripheralDescriptorObserver::onWrite, valueBuffer, valueSize);
			}
			writeDeferral.Complete();
		});
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsGattPeripheralDescriptor::~WindowsGattPeripheralDescriptor ()
{
	descriptor.ReadRequested (readToken);
	descriptor.WriteRequested (writeToken);
}

//************************************************************************************************
// WindowsGattPeripheralCharacteristic
//************************************************************************************************

WindowsGattPeripheralCharacteristic::WindowsGattPeripheralCharacteristic ()
: characteristic (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsGattPeripheralCharacteristic::WindowsGattPeripheralCharacteristic (WinRTGattLocalCharacteristic& characteristicArg)
: characteristic (characteristicArg)
{
	readToken = characteristic.ReadRequested ([&] (const WinRTGattLocalCharacteristic& sender, WinRTGattReadRequestedEventArgs args)
	{
		auto readDeferral = args.GetDeferral ();
		auto asyncop = args.GetRequestAsync();
		asyncop.Completed ([&, readDeferral] (IAsyncOperation<WinRTGattReadRequest> op, const AsyncStatus status)
		{
			auto results = op.GetResults ();
			if(results != nullptr)
			{
				auto valueBuffer = NEW uint8[kAttributeCapacity];
				int valueSize = 0;
				observers.notify (&IGattPeripheralCharacteristicObserver::onRead, valueBuffer, valueSize);
				if(valueSize > 0)
				{
					winrt::Windows::Storage::Streams::Buffer buffer (valueSize);
					memcpy (buffer.data (), valueBuffer, valueSize);
					buffer.Length (valueSize);
					op.GetResults ().RespondWithValue (buffer);
				}
				else
				{
					op.GetResults ().RespondWithProtocolError (WinRTGattProtocolError::UnlikelyError ());
				}
				delete valueBuffer;
			}
			readDeferral.Complete ();
		});
	});

	writeToken = characteristic.WriteRequested ([&] (const WinRTGattLocalCharacteristic& sender, WinRTGattWriteRequestedEventArgs args)
	{
		auto writeDeferral = args.GetDeferral ();
		auto asyncop = args.GetRequestAsync();
		asyncop.Completed ([&, writeDeferral] (IAsyncOperation<WinRTGattWriteRequest> op, const AsyncStatus status)
		{
			auto results = op.GetResults ();
			if(results != nullptr)
			{
				uint8 valueBuffer[kAttributeCapacity];
				int valueSize = results.Value ().Length ();
				memcpy (valueBuffer, results.Value ().data (), results.Value (). Length ());
				observers.notify (&IGattPeripheralCharacteristicObserver::onWrite, valueBuffer, valueSize);
				if(results.Option () == WinRTGattWriteOption::WriteWithResponse)
				{
					if(valueSize == results.Value ().Length ())
					{
						results.Respond ();
					}
					else
					{
						results.RespondWithProtocolError (WinRTGattProtocolError::UnlikelyError ());
					}
				}
			}
			writeDeferral.Complete();
		});
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsGattPeripheralCharacteristic::~WindowsGattPeripheralCharacteristic ()
{
	characteristic.ReadRequested (readToken);
	characteristic.WriteRequested (writeToken);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattPeripheralCharacteristic::notify (const uint8 valueBuffer[], int valueSize)
{
	winrt::Windows::Storage::Streams::Buffer buffer (valueSize);
	memcpy (buffer.data (), valueBuffer, valueSize);
	buffer.Length (valueSize);

	auto asyncop = characteristic.NotifyValueAsync (buffer);
	asyncop.Completed ([&] (IAsyncOperation<IVectorView<WinRTGattClientNotificationResult>> op, const AsyncStatus status)
	{
		observers.notify (&IGattPeripheralCharacteristicObserver::onNotify);
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattPeripheralCharacteristic::createDescriptorAsync (Core::UIDRef uuid, const uint8 valueBuffer[], int valueSize)
{
	auto guid = toWinrtGuid (uuid);
	winrt::Windows::Storage::Streams::Buffer buffer (valueSize);
	memcpy (buffer.data (), valueBuffer, valueSize);
	WinRTGattLocalDescriptorParameters parameters;

	parameters.StaticValue (buffer);

	auto asyncop = characteristic.CreateDescriptorAsync (guid, parameters);
	asyncop.Completed ([&] (IAsyncOperation<WinRTGattLocalDescriptorResult> op, const AsyncStatus status)
	{
		if(op.GetResults ().Error() == WinRTBluetoothError::Success)
		{
			auto rv = NEW WindowsGattPeripheralDescriptor;
			rv->descriptor = op.GetResults ().Descriptor ();
			observers.notify (&IGattPeripheralCharacteristicObserver::onDescriptorCreated, rv);
		}
		else
		{
			CCL_PRINTF ( "Could not create clientCounter characteristic: %s\n", op.GetResults ().Error ());
			observers.notify (&IGattPeripheralCharacteristicObserver::onDescriptorCreated, nullptr);
		}
	});
	return kError_NoError;
}

//************************************************************************************************
// WindowsGattPeripheralService
//************************************************************************************************

ErrorCode WindowsGattPeripheralService::createCharacteristicAsync (const CharacteristicInfo& characteristicInfo)
{
	auto guid = toWinrtGuid (characteristicInfo.uuid);
	WinRTGattLocalCharacteristicParameters parameters;
	parameters.CharacteristicProperties (static_cast<WinRTGattCharacteristicProperties> (characteristicInfo.properties));
	parameters.UserDescription (winrt::to_hstring (characteristicInfo.description));

	auto asyncop = serviceProvider.Service ().CreateCharacteristicAsync (guid, parameters);
	asyncop.Completed ([&] (IAsyncOperation<WinRTGattLocalCharacteristicResult> op, const AsyncStatus status)
	{
		if(op.GetResults ().Error() == WinRTBluetoothError::Success)
		{
			characteristics.add (NEW WindowsGattPeripheralCharacteristic (op.GetResults ().Characteristic ()));
			observers.notify (&IGattPeripheralServiceObserver::onCharacteristicCreated, characteristics.last ());
		}
		else
		{
			CCL_PRINTF ( "Could not create clientCounter characteristic: %s\n", op.GetResults ().Error ());
			observers.notify (&IGattPeripheralServiceObserver::onCharacteristicCreated, nullptr);
		}
	});
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint16 WindowsGattPeripheralService::getStartHandle () const
{
	/* Can't find implementation in Winrt*/
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint16 WindowsGattPeripheralService::getStopHandle () const
{
	/* Can't find implementation in Winrt*/
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattPeripheralService::addInclude (IGattPeripheralService* service)
{
	/* Can't find implementation in Winrt*/
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool WindowsGattPeripheralService::startAdvertising ()
{
	WinRTGattServiceProviderAdvertisingParameters advParameters;
	advParameters.IsConnectable (true);
	advParameters.IsDiscoverable (true);
	serviceProvider.StartAdvertising (advParameters);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool WindowsGattPeripheralService::stopAdvertising ()
{
	serviceProvider.StopAdvertising ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattPeripheralService::close ()
{
	for(auto& characteristic : characteristics)
	{
		delete characteristic;
	}
	stopAdvertising ();
}

//************************************************************************************************
// WindowsGattPeripheral
//************************************************************************************************

WindowsGattPeripheral::WindowsGattPeripheral ()
: users (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattPeripheral::startup ()
{
	using BluetoothAdapter = winrt::Windows::Devices::Bluetooth::BluetoothAdapter;
	if(users++ == 0)
	{
		auto asyncop = BluetoothAdapter::GetDefaultAsync ();
		asyncop.Completed ([&] (IAsyncOperation<BluetoothAdapter> op, const AsyncStatus status)
		{
			if(!op.GetResults ().IsPeripheralRoleSupported ())
			{
				CCL_PRINTF ("Bluetooth Peripheral not supported\n");
				observers.notify (&IGattPeripheralObserver::onPeripheralChanged, GattPeripheralStatusEnum::kPeripheralUnsupported);
				return;
			}
			if(!op.GetResults ().IsLowEnergySupported ())
			{
				CCL_PRINTF ("Bluetooth LE not supported\n");
				observers.notify (&IGattPeripheralObserver::onPeripheralChanged, GattPeripheralStatusEnum::kLEUnsupported);
			}
			observers.notify (&IGattPeripheralObserver::onPeripheralChanged, GattPeripheralStatusEnum::kReady);
		});
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode WindowsGattPeripheral::createServiceAsync (Core::UIDRef uuid)
{
	auto asyncop = WinRTGattServiceProvider::CreateAsync (toWinrtGuid (uuid));
	asyncop.Completed ([&] (IAsyncOperation<WinRTGattServiceProviderResult> op, const AsyncStatus status)
	{
		if (op.GetResults ().Error() == WinRTBluetoothError::Success)
		{
			auto newService = NEW WindowsGattPeripheralService ();
			newService->serviceProvider = op.GetResults ().ServiceProvider ();
			services.add (newService);
			observers.notify (&IGattPeripheralObserver::onServiceCreated, services.last (), Core::Errors::ErrorCodes::kError_NoError);
		}
		else
		{
			CCL_PRINTF ("Could not create service provider\n");
			observers.notify (&IGattPeripheralObserver::onServiceCreated, nullptr, Core::Errors::ErrorCodes::kError_Failed);
		}
	});
	return kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsGattPeripheral::shutdown ()
{
	if(--users == 0)
	{
		for(auto& service : services)
		{
			service->close ();
			delete service;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGattPeripheralService* WindowsGattPeripheral::getService (int index) const
{
	if(services.isValidIndex (index))
	{
		return services[index];
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WindowsGattPeripheral::getNumServices () const
{
	return services.count ();
}
