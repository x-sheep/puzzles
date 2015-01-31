//
// HelpFlyout.xaml.h
// Declaration of the HelpFlyout class
//

#pragma once

#include "HelpFlyout.g.h"

namespace PuzzleModern
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class HelpFlyout sealed
	{
	public:
		HelpFlyout(Platform::String ^page);
	private:
		void HelpView_NavigationStarting(Windows::UI::Xaml::Controls::WebView^ sender, Windows::UI::Xaml::Controls::WebViewNavigationStartingEventArgs^ args);
		void SettingsFlyout_SizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);
	};
}
