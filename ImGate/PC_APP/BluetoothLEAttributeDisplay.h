#pragma once

namespace winrt::PC_APP::DisplayHelpers
{
    hstring GetServiceName(Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService const& service);
    hstring GetCharacteristicName(Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic const& characteristic);
}
