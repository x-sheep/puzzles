//
// PresetDialog.xaml.h
// Declaration of the PresetDialog class
//

#pragma once

#include "PresetDialog.g.h"

using namespace PuzzleCommon;

namespace PuzzleModern
{
	namespace Phone
	{
		public delegate void CustomClickedEventHandler();
		public delegate void PresetClickedEventHandler(Platform::Object ^sender, Preset ^preset);

		[Windows::Foundation::Metadata::WebHostHidden]
		public ref class PresetDialog sealed
		{
		public:
			PresetDialog(PresetList ^list);

			event CustomClickedEventHandler ^CustomClicked;
			event PresetClickedEventHandler ^PresetClicked;

		private:
			void ContentDialog_PrimaryButtonClick(Windows::UI::Xaml::Controls::ContentDialog^ sender, Windows::UI::Xaml::Controls::ContentDialogButtonClickEventArgs^ args);
			void ContentDialog_SecondaryButtonClick(Windows::UI::Xaml::Controls::ContentDialog^ sender, Windows::UI::Xaml::Controls::ContentDialogButtonClickEventArgs^ args);

			void PresetListView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs e);

			PresetList ^_list;
			void TextBlock_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		};
	}
}