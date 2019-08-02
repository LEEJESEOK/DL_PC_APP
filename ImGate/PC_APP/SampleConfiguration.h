#pragma once 
#include "pch.h"

namespace winrt::PC_APP
{
    struct SampleState
    {
        static hstring SelectedBleDeviceId;
        static hstring SelectedBleDeviceName;
    };

    Windows::Foundation::Rect GetElementRect(Windows::UI::Xaml::FrameworkElement const& element);
}
