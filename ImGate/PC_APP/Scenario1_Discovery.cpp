#include "pch.h"
#include "Scenario1_Discovery.h"
#if __has_include("Scenario1_Discovery.g.cpp")
#include "Scenario1_Discovery.g.cpp"
#endif
#include "SampleConfiguration.h"
#include "BluetoothLEDeviceDisplay.h"
#include "BluetoothLEAttributeDisplay.h"

#include "winrt/Windows.System.Threading.h"

using namespace winrt;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Globalization;
using namespace Windows::Security::Cryptography;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;

using namespace Windows::System::Threading;

namespace
{
	const hstring testDeviceName = L"Nordic_test";

	// Utility function to convert a string to an int32_t and detect bad input
	bool TryParseInt(const wchar_t* str, int32_t& result)
	{
		wchar_t* end;
		errno = 0;
		result = std::wcstol(str, &end, 0);

		if (str == end)
		{
			// Not parseable.
			return false;
		}

		if (errno == ERANGE || result < INT_MIN || INT_MAX < result)
		{
			// Out of range.
			return false;
		}

		if (*end != L'\0')
		{
			// Extra unparseable characters at the end.
			return false;
		}

		return true;
	}

	template<typename T>
	T Read(DataReader const& reader);

	template<>
	uint32_t Read<uint32_t>(DataReader const& reader)
	{
		return reader.ReadUInt32();
	}

	template<>
	int32_t Read<int32_t>(DataReader const& reader)
	{
		return reader.ReadInt32();
	}

	template<>
	uint8_t Read<uint8_t>(DataReader const& reader)
	{
		return reader.ReadByte();
	}

	template<typename T>
	bool TryExtract(IBuffer const& buffer, T& result)
	{
		if (buffer.Length() >= sizeof(T))
		{
			DataReader reader = DataReader::FromBuffer(buffer);
			result = Read<T>(reader);
			return true;
		}
		return false;
	}
}

namespace winrt
{
	hstring to_hstringGattCommunicationStatus(GattCommunicationStatus status)
	{
		switch (status)
		{
		case GattCommunicationStatus::Success: return L"Success";
		case GattCommunicationStatus::Unreachable: return L"Unreachable";
		case GattCommunicationStatus::ProtocolError: return L"ProtocolError";
		case GattCommunicationStatus::AccessDenied: return L"AccessDenied";
		}
		return to_hstring(static_cast<int>(status));
	}
}

namespace winrt::PC_APP::implementation
{
#pragma region UI Code
	Scenario1_Discovery::Scenario1_Discovery()
	{
		InitializeComponent();
	}

	fire_and_forget Scenario1_Discovery::OnNavigatedFrom(NavigationEventArgs const&)
	{
		auto lifetime = get_strong();

		StopBleDeviceWatcher();

		// Save the selected device's ID for use in other scenarios.
		auto bleDeviceDisplay = ResultsListView().SelectedItem().as<PC_APP::BluetoothLEDeviceDisplay>();
		if (bleDeviceDisplay != nullptr)
		{
			SampleState::SelectedBleDeviceId = bleDeviceDisplay.Id();
			SampleState::SelectedBleDeviceName = bleDeviceDisplay.Name();
		}

		if (!co_await ClearBluetoothLEDeviceAsync())
		{
			rootPage.NotifyUser(L"Error: Unable to reset app state", NotifyType::ErrorMessage);
		}
	}

	void Scenario1_Discovery::ActionButton_Click()
	{
		if (deviceWatcher == nullptr)
		{
			ConnectButton_Click();
		}
		else
		{
			DisconnectButton_Click();
		}
	}

	void Scenario1_Discovery::ConnectButton_Click()
	{
		ActionButton().Content(box_value(L"Stop"));

		StartBleDeviceWatcher();
	}

	void Scenario1_Discovery::DisconnectButton_Click()
	{
		StopBleDeviceWatcher();
		if (isConnect) {
			Disconnect();
			isConnect = false;
		}
		ActionButton().Content(box_value(L"Start"));
	}
#pragma endregion

#pragma region Device discovery
	/// <summary>
	/// Starts a device watcher that looks for all nearby Bluetooth devices (paired or unpaired). 
	/// Attaches event handlers to populate the device collection.
	/// </summary>
	void Scenario1_Discovery::StartBleDeviceWatcher()
	{
		// Additional properties we would like about the device.
		// Property strings are documented here https://msdn.microsoft.com/en-us/library/windows/desktop/ff521659(v=vs.85).aspx
		auto requestedProperties = single_threaded_vector<hstring>({ L"System.Devices.Aep.DeviceAddress", L"System.Devices.Aep.IsConnected", L"System.Devices.Aep.Bluetooth.Le.IsConnectable" });

		// BT_Code: Example showing paired and non-paired in a single query.
		hstring aqsAllBluetoothLEDevices = L"(System.Devices.Aep.ProtocolId:=\"{bb7bb05e-5972-42b5-94fc-76eaa7084d49}\")";

		deviceWatcher =
			Windows::Devices::Enumeration::DeviceInformation::CreateWatcher(
				aqsAllBluetoothLEDevices,
				requestedProperties,
				DeviceInformationKind::AssociationEndpoint);

		// Register event handlers before starting the watcher.
		deviceWatcherAddedToken = deviceWatcher.Added({ get_weak(), &Scenario1_Discovery::DeviceWatcher_Added });
		deviceWatcherUpdatedToken = deviceWatcher.Updated({ get_weak(), &Scenario1_Discovery::DeviceWatcher_Updated });
		deviceWatcherRemovedToken = deviceWatcher.Removed({ get_weak(), &Scenario1_Discovery::DeviceWatcher_Removed });
		deviceWatcherEnumerationCompletedToken = deviceWatcher.EnumerationCompleted({ get_weak(), &Scenario1_Discovery::DeviceWatcher_EnumerationCompleted });
		deviceWatcherStoppedToken = deviceWatcher.Stopped({ get_weak(), &Scenario1_Discovery::DeviceWatcher_Stopped });

		// Start over with an empty collection.
		m_knownDevices.Clear();

		//hstring tempString = logtext().Text() + L"\n";
		//hstring message = tempString + L"scan";
		//logtext().Text(message);

		// Start the watcher. Active enumeration is limited to approximately 30 seconds.
		// This limits power usage and reduces interference with other Bluetooth activities.
		// To monitor for the presence of Bluetooth LE devices for an extended period,
		// use the BluetoothLEAdvertisementWatcher runtime class. See the BluetoothAdvertisement
		// sample for an example.
		deviceWatcher.Start();
	}

	/// <summary>
	/// Stops watching for all nearby Bluetooth devices.
	/// </summary>
	void Scenario1_Discovery::StopBleDeviceWatcher()
	{
		if (deviceWatcher != nullptr)
		{
			// Unregister the event handlers.
			deviceWatcher.Added(deviceWatcherAddedToken);
			deviceWatcher.Updated(deviceWatcherUpdatedToken);
			deviceWatcher.Removed(deviceWatcherRemovedToken);
			deviceWatcher.EnumerationCompleted(deviceWatcherEnumerationCompletedToken);
			deviceWatcher.Stopped(deviceWatcherStoppedToken);

			// Stop the watcher.
			deviceWatcher.Stop();
			deviceWatcher = nullptr;
		}
	}

	std::tuple<PC_APP::BluetoothLEDeviceDisplay, uint32_t> Scenario1_Discovery::FindBluetoothLEDeviceDisplay(hstring const& id)
	{
		uint32_t size = m_knownDevices.Size();
		for (uint32_t index = 0; index < size; index++)
		{
			auto bleDeviceDisplay = m_knownDevices.GetAt(index).as<PC_APP::BluetoothLEDeviceDisplay>();
			if (bleDeviceDisplay.Id() == id)
			{
				return { bleDeviceDisplay, index };
			}
		}
		return { nullptr, 0 - 1U };
	}

	std::vector<Windows::Devices::Enumeration::DeviceInformation>::iterator Scenario1_Discovery::FindUnknownDevices(hstring const& id)
	{
		return std::find_if(UnknownDevices.begin(), UnknownDevices.end(), [&](auto&& bleDeviceInfo)
			{
				return bleDeviceInfo.Id() == id;
			});
	}

	fire_and_forget Scenario1_Discovery::DeviceWatcher_Added(DeviceWatcher sender, DeviceInformation deviceInfo)
	{
		// We must update the collection on the UI thread because the collection is databound to a UI element.
		auto lifetime = get_strong();
		co_await resume_foreground(Dispatcher());

		// Protect against race condition if the task runs after the app stopped the deviceWatcher.
		if (sender == deviceWatcher)
		{
			// Make sure device isn't already present in the list.
			if (std::get<0>(FindBluetoothLEDeviceDisplay(deviceInfo.Id())) == nullptr)
			{
				if (!deviceInfo.Name().empty())
				{
					// If device has a friendly name display it immediately.
					m_knownDevices.Append(make<BluetoothLEDeviceDisplay>(deviceInfo));
					if (deviceInfo.Name() == testDeviceName)
					{
						SampleState::SelectedBleDeviceId = deviceInfo.Id();
						SampleState::SelectedBleDeviceName = deviceInfo.Name();
						//rootPage.NotifyUser(L"Find Test device : " + SampleState::SelectedBleDeviceName + L"/" + SampleState::SelectedBleDeviceId, NotifyType::StatusMessage);

						co_await ConnectTestDevice();

						TestAction();
					}
				}
				else
				{
					// Add it to a list in case the name gets updated later. 
					UnknownDevices.push_back(deviceInfo);
				}
			}
		}
	}

	fire_and_forget Scenario1_Discovery::DeviceWatcher_Updated(DeviceWatcher sender, DeviceInformationUpdate deviceInfoUpdate)
	{
		// We must update the collection on the UI thread because the collection is databound to a UI element.
		auto lifetime = get_strong();
		co_await resume_foreground(Dispatcher());

		// Protect against race condition if the task runs after the app stopped the deviceWatcher.
		if (sender == deviceWatcher)
		{
			PC_APP::BluetoothLEDeviceDisplay bleDeviceDisplay = std::get<0>(FindBluetoothLEDeviceDisplay(deviceInfoUpdate.Id()));
			if (bleDeviceDisplay != nullptr)
			{
				// Device is already being displayed - update UX.
				bleDeviceDisplay.Update(deviceInfoUpdate);
				co_return;
			}

			auto deviceInfo = FindUnknownDevices(deviceInfoUpdate.Id());
			if (deviceInfo != UnknownDevices.end())
			{
				deviceInfo->Update(deviceInfoUpdate);
				// If device has been updated with a friendly name it's no longer unknown.
				if (!deviceInfo->Name().empty())
				{
					m_knownDevices.Append(make<BluetoothLEDeviceDisplay>(*deviceInfo));
					UnknownDevices.erase(deviceInfo);
				}
			}
		}
	}

	fire_and_forget Scenario1_Discovery::DeviceWatcher_Removed(DeviceWatcher sender, DeviceInformationUpdate deviceInfoUpdate)
	{
		// We must update the collection on the UI thread because the collection is databound to a UI element.
		auto lifetime = get_strong();
		co_await resume_foreground(Dispatcher());

		// Protect against race condition if the task runs after the app stopped the deviceWatcher.
		if (sender == deviceWatcher)
		{
			// Find the corresponding DeviceInformation in the collection and remove it.
			auto [bleDeviceDisplay, index] = FindBluetoothLEDeviceDisplay(deviceInfoUpdate.Id());
			if (bleDeviceDisplay != nullptr)
			{
				m_knownDevices.RemoveAt(index);
			}

			auto deviceInfo = FindUnknownDevices(deviceInfoUpdate.Id());
			if (deviceInfo != UnknownDevices.end())
			{
				UnknownDevices.erase(deviceInfo);
			}
		}
	}

	fire_and_forget Scenario1_Discovery::DeviceWatcher_EnumerationCompleted(DeviceWatcher sender, IInspectable const&)
	{
		// Access this->deviceWatcher on the UI thread to avoid race conditions.
		auto lifetime = get_strong();
		co_await resume_foreground(Dispatcher());

		// Protect against race condition if the task runs after the app stopped the deviceWatcher.
		if (sender == deviceWatcher)
		{
			rootPage.NotifyUser(to_hstring(m_knownDevices.Size()) + L" devices found. Enumeration completed.",
				NotifyType::StatusMessage);

		}
	}

	fire_and_forget Scenario1_Discovery::DeviceWatcher_Stopped(DeviceWatcher sender, IInspectable const&)
	{
		// Access this->deviceWatcher on the UI thread to avoid race conditions.
		auto lifetime = get_strong();
		co_await resume_foreground(Dispatcher());

		// Protect against race condition if the task runs after the app stopped the deviceWatcher.
		if (sender == deviceWatcher)
		{
			rootPage.NotifyUser(L"No longer watching for devices.",
				sender.Status() == DeviceWatcherStatus::Aborted ? NotifyType::ErrorMessage : NotifyType::StatusMessage);
		}
	}
#pragma endregion

#pragma region Enumerating Services
	IAsyncOperation<bool> Scenario1_Discovery::ClearBluetoothLEDeviceAsync()
	{
		auto lifetime = get_strong();

		if (notificationsToken)
		{
			// Need to clear the CCCD from the remote device so we stop receiving notifications
			GattCommunicationStatus result = co_await registeredCharacteristic.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue::None);
			if (result != GattCommunicationStatus::Success)
			{
				co_return false;
			}
			else
			{
				nordicUARTNotify.ValueChanged(std::exchange(notificationsToken, {}));
			}
		}

		if (bluetoothLeDevice != nullptr)
		{
			bluetoothLeDevice.Close();
			bluetoothLeDevice = nullptr;
		}

		co_return true;
	}

	IAsyncAction Scenario1_Discovery::ConnectTestDevice()
	{
		auto lifetime = get_strong();

		StopBleDeviceWatcher();
		RemoveValueChangedHandler();

		//connect
		if (!co_await ClearBluetoothLEDeviceAsync())
		{
			rootPage.NotifyUser(L"Error: Unable to reset state, try again.", NotifyType::ErrorMessage);
			co_return;
		}

		try
		{
			// BT_Code: BluetoothLEDevice.FromIdAsync must be called from a UI thread because it may prompt for consent.
			bluetoothLeDevice = co_await BluetoothLEDevice::FromIdAsync(SampleState::SelectedBleDeviceId);

			if (bluetoothLeDevice == nullptr)
			{
				rootPage.NotifyUser(L"Failed to connect to device.", NotifyType::ErrorMessage);
			}
		}
		catch (hresult_error& ex)
		{
			if (ex.to_abi() == HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_AVAILABLE))
			{
				rootPage.NotifyUser(L"Bluetooth radio is not on.", NotifyType::ErrorMessage);
			}
			else
			{
				throw;
			}
		}

		if (bluetoothLeDevice != nullptr)
		{
			isConnect = true;

			//Service Result
			// Note: BluetoothLEDevice.GattServices property will return an empty list for unpaired devices. For all uses we recommend using the GetGattServicesAsync method.
			// BT_Code: GetGattServicesAsync returns a list of all the supported services of the device (even if it's not paired to the system).
			// If the services supported by the device are expected to change during BT usage, subscribe to the GattServicesChanged event.
			GattDeviceServicesResult result = co_await bluetoothLeDevice.GetGattServicesAsync(BluetoothCacheMode::Uncached);
			if (result.Status() == GattCommunicationStatus::Success)
			{
				IVectorView<GattDeviceService> services = result.Services();
				for (auto&& service : services)
				{
					IVectorView<GattCharacteristic> characteristics{ nullptr };
					try
					{
						// Ensure we have access to the device.
						auto accessStatus = co_await service.RequestAccessAsync();
						if (accessStatus == DeviceAccessStatus::Allowed)
						{
							// BT_Code: Get all the child characteristics of a service. Use the cache mode to specify uncached characterstics only 
							// and the new Async functions to get the characteristics of unpaired devices as well. 
							GattCharacteristicsResult result = co_await service.GetCharacteristicsAsync(BluetoothCacheMode::Uncached);
							if (result.Status() == GattCommunicationStatus::Success)
							{
								characteristics = result.Characteristics();
							}
							else
							{
								rootPage.NotifyUser(L"Error accessing service.", NotifyType::ErrorMessage);
							}
						}
						else
						{
							// Not granted access
							rootPage.NotifyUser(L"Error accessing service.", NotifyType::ErrorMessage);

						}
					}
					catch (hresult_error& ex)
					{
						rootPage.NotifyUser(L"Restricted service. Can't read characteristics: " + ex.message(), NotifyType::ErrorMessage);
					}
					if (characteristics)
					{
						for (GattCharacteristic&& c : characteristics)
						{
							if (DisplayHelpers::GetCharacteristicName(c) == L"RX Characteristic")
							{
								//rootPage.NotifyUser(L"Found " + DisplayHelpers::GetCharacteristicName(c) + L" characteristic", NotifyType::StatusMessage);
								nordicUARTWrite = c;
							}
							if (DisplayHelpers::GetCharacteristicName(c) == L"TX Characteristic")
							{
								//rootPage.NotifyUser(L"Found " + DisplayHelpers::GetCharacteristicName(c) + L" characteristic", NotifyType::StatusMessage);
								nordicUARTNotify = c;
							}
						}
					}
				}
			}
			else
			{
				rootPage.NotifyUser(L"Device unreachable", NotifyType::ErrorMessage);
			}
			//End of Service Result

			GattClientCharacteristicConfigurationDescriptorValue cccdValue = GattClientCharacteristicConfigurationDescriptorValue::None;
			if ((nordicUARTNotify.CharacteristicProperties() & GattCharacteristicProperties::Indicate) != GattCharacteristicProperties::None)
			{
				cccdValue = GattClientCharacteristicConfigurationDescriptorValue::Indicate;
			}

			else if ((nordicUARTNotify.CharacteristicProperties() & GattCharacteristicProperties::Notify) != GattCharacteristicProperties::None)
			{
				cccdValue = GattClientCharacteristicConfigurationDescriptorValue::Notify;
			}
			try
			{
				// BT_Code: Must write the CCCD in order for server to send indications.
				// We receive them in the ValueChanged event handler.
				GattCommunicationStatus status = co_await nordicUARTNotify.WriteClientCharacteristicConfigurationDescriptorAsync(cccdValue);

				if (status == GattCommunicationStatus::Success)
				{
					AddValueChangedHandler();
					rootPage.NotifyUser(L"Successfully subscribed for value changes", NotifyType::StatusMessage);
				}
				else
				{
					rootPage.NotifyUser(L"Error registering for value changes: Status = " + to_hstringGattCommunicationStatus(status), NotifyType::ErrorMessage);
				}
			}
			catch (hresult_access_denied& ex)
			{
				// This usually happens when a device reports that it support indicate, but it actually doesn't.
				rootPage.NotifyUser(ex.message(), NotifyType::ErrorMessage);
			}
		}
	}
#pragma endregion

#pragma region ValueChangeHandler
	void Scenario1_Discovery::AddValueChangedHandler()
	{
		if (!notificationsToken)
		{
			registeredCharacteristic = nordicUARTNotify;
			notificationsToken = registeredCharacteristic.ValueChanged({ get_weak(), &Scenario1_Discovery::Characteristic_ValueChanged });
		}
	}
	void Scenario1_Discovery::RemoveValueChangedHandler()
	{
		if (notificationsToken)
		{
			registeredCharacteristic.ValueChanged(std::exchange(notificationsToken, {}));
			registeredCharacteristic = nullptr;
		}
	}

	fire_and_forget Scenario1_Discovery::Characteristic_ValueChanged(GattCharacteristic const&, GattValueChangedEventArgs args)
	{
		auto lifetime = get_strong();

		// BT_Code: An Indicate or Notify reported that the value has changed.
		// Display the new value with a timestamp.
		hstring newValue = FormatValueByPresentation(args.CharacteristicValue(), presentationFormat);
		std::time_t now = clock::to_time_t(clock::now());
		char buffer[26];
		ctime_s(buffer, ARRAYSIZE(buffer), &now);
		hstring message = L"Value at " + to_hstring(buffer) + L": " + newValue;
		co_await resume_foreground(Dispatcher());
		CharacteristicLatestValue().Text(message);
	}
#pragma endregion

	IAsyncOperation<bool> Scenario1_Discovery::WriteBufferToNordicUARTAsync(IBuffer buffer)
	{
		auto lifetime = get_strong();

		try
		{
			// BT_Code: Writes the value from the buffer to the characteristic.
			GattWriteResult result = co_await nordicUARTWrite.WriteValueWithResultAsync(buffer);

			if (result.Status() == GattCommunicationStatus::Success)
			{
				//rootPage.NotifyUser(L"Successfully wrote value to device", NotifyType::StatusMessage);
				co_return true;
			}
			else
			{
				rootPage.NotifyUser(L"Write failed: Status = " + to_hstringGattCommunicationStatus(result.Status()), NotifyType::ErrorMessage);
				co_return false;
			}
		}
		catch (hresult_error& ex)
		{
			if (ex.code() == E_BLUETOOTH_ATT_INVALID_PDU)
			{
				rootPage.NotifyUser(ex.message(), NotifyType::ErrorMessage);
				co_return false;
			}
			if (ex.code() == E_BLUETOOTH_ATT_WRITE_NOT_PERMITTED || ex.code() == E_ACCESSDENIED)
			{
				// This usually happens when a device reports that it support writing, but it actually doesn't.
				rootPage.NotifyUser(ex.message(), NotifyType::ErrorMessage);
				co_return false;
			}
			throw;
		}
	}

	hstring Scenario1_Discovery::FormatValueByPresentation(IBuffer const& buffer, GenericAttributeProfile::GattPresentationFormat const& format)
	{
		// BT_Code: For the purpose of this sample, this function converts only UInt32 and
		// UTF-8 buffers to readable text. It can be extended to support other formats if your app needs them.
		if (format != nullptr)
		{
			if (format.FormatType() == GattPresentationFormatTypes::UInt32())
			{
				uint32_t value;
				if (TryExtract(buffer, value))
				{
					return to_hstring(value);
				}
				return L"(error: Invalid UInt32)";

			}
			else if (format.FormatType() == GattPresentationFormatTypes::Utf8())
			{
				try
				{
					return CryptographicBuffer::ConvertBinaryToString(BinaryStringEncoding::Utf8, buffer);
				}
				catch (hresult_invalid_argument&)
				{
					return L"(error: Invalid UTF-8 string)";
				}
			}
			else
			{
				// Add support for other format types as needed.
				return L"Unsupported format: " + CryptographicBuffer::EncodeToHexString(buffer);
			}
		}
		else if (buffer != nullptr)
		{
			// We don't know what format to use. Let's try some well-known profiles, or default back to UTF-8.
			if (nordicUARTNotify != nullptr)
			{
				return L"Receive message : " + CryptographicBuffer::ConvertBinaryToString(BinaryStringEncoding::Utf8, buffer);
			}
			else
			{
				try
				{
					return L"Unknown format: " + CryptographicBuffer::ConvertBinaryToString(BinaryStringEncoding::Utf8, buffer);

				}
				catch (hresult_invalid_argument&)
				{
					return L"Unknown format";
				}
			}
		}
		else
		{
			return L"Empty data received";
		}
	}

	void Scenario1_Discovery::Timer(const long timeSpan)
	{
		TimeSpan period(timeSpan * 10000);

		bool completed = false;

		ThreadPoolTimer DelayTimer = ThreadPoolTimer::CreateTimer(
			TimerElapsedHandler([&](ThreadPoolTimer source)
				{
					Lock();

					Dispatcher().RunAsync(CoreDispatcherPriority::High,
						DispatchedHandler([&]()
							{
							}));

					completed = true;

				}),
			period,
					TimerDestroyedHandler([&](ThreadPoolTimer source)
						{
							Disconnect();
							isConnect = false;

							Dispatcher().RunAsync(CoreDispatcherPriority::High,
								DispatchedHandler([&]()
									{
										if (completed)
										{
											ActionButton().Content(box_value(L"Start"));
										}
										else
										{
										}
									}));
						}));
	}



	fire_and_forget Scenario1_Discovery::Lock()
	{
		auto lifetime = get_strong();

		IBuffer writeBuffer = CryptographicBuffer::ConvertStringToBinary(L"0", BinaryStringEncoding::Utf8);
		co_await WriteBufferToNordicUARTAsync(writeBuffer);
	}

	fire_and_forget Scenario1_Discovery::Unlock()
	{
		auto lifetime = get_strong();

		IBuffer writeBuffer = CryptographicBuffer::ConvertStringToBinary(L"1", BinaryStringEncoding::Utf8);
		co_await WriteBufferToNordicUARTAsync(writeBuffer);
	}

	fire_and_forget Scenario1_Discovery::Disconnect()
	{
		auto lifetime = get_strong();

		IBuffer writeBuffer = CryptographicBuffer::ConvertStringToBinary(L"9", BinaryStringEncoding::Utf8);
		co_await WriteBufferToNordicUARTAsync(writeBuffer);

		ClearBluetoothLEDeviceAsync();
	}

	void Scenario1_Discovery::TestAction()
	{
		Unlock();
		////TODO 결과 확인

		Timer(3000);
		////TODO 결과 확인

	}
}
