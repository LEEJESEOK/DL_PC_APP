#pragma once

#include "Scenario2_Connect.g.h"
#include "MainPage.h"

namespace winrt::PC_APP::implementation
{
    struct Scenario2_Connect : Scenario2_ConnectT<Scenario2_Connect>
    {
        Scenario2_Connect();

		void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const& e);
		fire_and_forget OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs const&);

		fire_and_forget ConnectButton_Click();
		fire_and_forget ServiceList_SelectionChanged();
		fire_and_forget CharacteristicList_SelectionChanged();
		fire_and_forget CharacteristicReadButton_Click();
		fire_and_forget CharacteristicWriteButton_Click();
		fire_and_forget CharacteristicWriteButtonInt_Click();
		fire_and_forget ValueChangedSubscribeToggle_Click();

		void LockButton_Click();
		void UnlockButton_Click();
		void TestButton_Click();

	private:
		bool isConnect = false;

		PC_APP::MainPage rootPage{ MainPage::Current() };
		Windows::Devices::Bluetooth::BluetoothLEDevice bluetoothLeDevice{ nullptr };
		Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic selectedCharacteristic{ nullptr };

		// Only one registered characteristic at a time.
		Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic registeredCharacteristic{ nullptr };
		Windows::Devices::Bluetooth::GenericAttributeProfile::GattPresentationFormat presentationFormat{ nullptr };

		event_token notificationsToken;

		Windows::Foundation::IAsyncOperation<bool> ClearBluetoothLEDeviceAsync();
		void AddValueChangedHandler();
		void RemoveValueChangedHandler();
		void EnableCharacteristicPanels(Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristicProperties properties);
		Windows::Foundation::IAsyncOperation<bool> WriteBufferToSelectedCharacteristicAsync(Windows::Storage::Streams::IBuffer buffer);
		hstring FormatValueByPresentation(Windows::Storage::Streams::IBuffer const& buffer, Windows::Devices::Bluetooth::GenericAttributeProfile::GattPresentationFormat const& format);
		fire_and_forget Characteristic_ValueChanged(Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic const&, Windows::Devices::Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs args);
    };
}

namespace winrt::PC_APP::factory_implementation
{
    struct Scenario2_Connect : Scenario2_ConnectT<Scenario2_Connect, implementation::Scenario2_Connect>
    {
    };
}
