//
// GeneralSettingsFlyout.xaml.h
// Declaration of the GeneralSettingsFlyout class
//

#pragma once

#include "GeneralSettingsFlyout.g.h"

namespace PuzzleModern
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class GeneralSettingsFlyout sealed
	{
	public:
		GeneralSettingsFlyout();
	private:
		void ColorblindSwitch_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void BottomAppbarSwitch_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		bool _loaded;
		void NewGameSwitch_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void MapPaletteBox_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e);
	};
}
