#include "pch.h"
#include <winrt/PC_APP.h>
#include "MainPage.h"
#include "SampleConfiguration.h"

using namespace winrt;
using namespace PC_APP;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;

hstring implementation::MainPage::FEATURE_NAME()
{
    return L"Doorlock Test";
}

IVector<Scenario> implementation::MainPage::scenariosInner = winrt::single_threaded_observable_vector<Scenario>(
{
    Scenario{ L"Client: Discover servers", xaml_typename<PC_APP::Scenario1_Discovery>() },
    Scenario{ L"Client: Connect to a server", xaml_typename<PC_APP::Scenario2_Connect>() },
});

hstring SampleState::SelectedBleDeviceId;
hstring SampleState::SelectedBleDeviceName{ L"No device selected" };

Rect winrt::PC_APP::GetElementRect(FrameworkElement const& element)
{
    auto transform = element.TransformToVisual(nullptr);
    Point point = transform.TransformPoint({});
    return { point, { static_cast<float>(element.ActualWidth()), static_cast<float>(element.ActualHeight()) } };
}