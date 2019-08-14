#pragma once

#include "Scenario1_Discovery.g.h"
#include "MainPage.h"

#include "winrt/Windows.System.Threading.h"

namespace winrt::PC_APP::implementation
{
    struct Scenario1_Discovery : Scenario1_DiscoveryT<Scenario1_Discovery>
    {
        Scenario1_Discovery();

		//fire_and_forget OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const& e);

		Windows::Foundation::Collections::IObservableVector<Windows::Foundation::IInspectable> KnownDevices()
		{
			return m_knownDevices;
		}

		void ActionButton_Click();

		bool Not(bool value) { return !value; }

	private:
		PC_APP::MainPage rootPage{ MainPage::Current() };

		//scenario1
		Windows::Foundation::Collections::IObservableVector<Windows::Foundation::IInspectable> m_knownDevices = single_threaded_observable_vector<Windows::Foundation::IInspectable>();
		std::vector<Windows::Devices::Enumeration::DeviceInformation> UnknownDevices;
		Windows::Devices::Enumeration::DeviceWatcher deviceWatcher{ nullptr };
		event_token deviceWatcherAddedToken;
		event_token deviceWatcherUpdatedToken;
		event_token deviceWatcherRemovedToken;
		event_token deviceWatcherEnumerationCompletedToken;
		event_token deviceWatcherStoppedToken;

		//scenario2
		event_token notificationsToken;
		Windows::Devices::Bluetooth::BluetoothLEDevice bluetoothLeDevice{ nullptr };
		Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic nordicUARTWrite{ nullptr };
		Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic nordicUARTNotify{ nullptr };
		Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic registeredCharacteristic{ nullptr };
		Windows::Devices::Bluetooth::GenericAttributeProfile::GattPresentationFormat presentationFormat{ nullptr };

		Windows::System::Threading::ThreadPoolTimer messageTimeoutTimer = NULL;
		int retryCnt;

		bool isTest = false;
		std::clock_t actionStartTime = 0, actionEndTime = 0;
		int valueCnt = 0, lockCnt = 0, unlockCnt = 0, connectCnt = 0, disconnectCnt = 0;

		//scenario1 - enumeration
		void StartBleDeviceWatcher();
		void StopBleDeviceWatcher();
		std::tuple<PC_APP::BluetoothLEDeviceDisplay, uint32_t> FindBluetoothLEDeviceDisplay(hstring const& id);
		std::vector<Windows::Devices::Enumeration::DeviceInformation>::iterator FindUnknownDevices(hstring const& id);

		fire_and_forget DeviceWatcher_Added(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Devices::Enumeration::DeviceInformation deviceInfo);
		fire_and_forget DeviceWatcher_Updated(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Devices::Enumeration::DeviceInformationUpdate deviceInfoUpdate);
		fire_and_forget DeviceWatcher_Removed(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Devices::Enumeration::DeviceInformationUpdate deviceInfoUpdate);
		fire_and_forget DeviceWatcher_EnumerationCompleted(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Foundation::IInspectable const&);
		fire_and_forget DeviceWatcher_Stopped(Windows::Devices::Enumeration::DeviceWatcher sender, Windows::Foundation::IInspectable const&);

		//scenario2 - connection
		Windows::Foundation::IAsyncOperation<bool> ClearBluetoothLEDeviceAsync();
		Windows::Foundation::IAsyncAction ConnectTestDevice();
		void AddValueChangedHandler();
		void RemoveValueChangedHandler();
		fire_and_forget Characteristic_ValueChanged(Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic const&, Windows::Devices::Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs args);
		Windows::Foundation::IAsyncOperation<bool> WriteBufferToNordicUARTAsync(Windows::Storage::Streams::IBuffer buffer);
		hstring FormatValueByPresentation(Windows::Storage::Streams::IBuffer const& buffer, Windows::Devices::Bluetooth::GenericAttributeProfile::GattPresentationFormat const& format);


		void Lock();
		void Unlock();
		fire_and_forget Invert();
		void SendConnectMessage();
		void SendDisconnectMessage();
		void SendTestMessage();
		void StartTimeoutTimer();
		void TestAction();
		void RestartTestAction();

		void LogWriter(hstring str, std::clock_t elapsedTime, int cnt);
		void LogWriter(hstring str);
	};
}

namespace winrt::PC_APP::factory_implementation
{
    struct Scenario1_Discovery : Scenario1_DiscoveryT<Scenario1_Discovery, implementation::Scenario1_Discovery>
    {
    };
}
