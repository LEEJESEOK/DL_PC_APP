#pragma once
#include "App.xaml.g.h"

namespace winrt::PC_APP::implementation
{
    struct App : AppT<App>
    {
        App();
		Windows::UI::Xaml::Controls::Frame CreateRootFrame();

        void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs const&);
		void OnActivated(Windows::ApplicationModel::Activation::IActivatedEventArgs const&);
		void OnFileActivated(Windows::ApplicationModel::Activation::FileActivatedEventArgs const&);
        void OnNavigationFailed(IInspectable const&, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs const&);
    };
}
