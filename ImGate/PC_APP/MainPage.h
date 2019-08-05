#pragma once

#include "MainPage.g.h"

namespace winrt::PC_APP::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

		static hstring FEATURE_NAME();
		static Windows::Foundation::Collections::IVector<Scenario> scenarios() { return scenariosInner; }
		static PC_APP::MainPage Current() { return current; }

		void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs const& e);

		void NotifyUser(hstring const& strMessage, NotifyType type);
		void Navigate(Windows::UI::Xaml::Interop::TypeName const& typeName, Windows::Foundation::IInspectable const& parameter);

		// These methods are public so they can be called by binding.
		void ScenarioControl_SelectionChanged(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
		void Footer_Click(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& e);
		void Button_Click(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& e);
	private:
		static PC_APP::MainPage current;
		static winrt::Windows::Foundation::Collections::IVector<Scenario> scenariosInner;

		bool navigating = false;
		void NavigateTo(Windows::UI::Xaml::Interop::TypeName const& typeName, Windows::Foundation::IInspectable const& parameter);
		void UpdateStatus(hstring const& strMessage, NotifyType type);
    };
}

namespace winrt::PC_APP::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
