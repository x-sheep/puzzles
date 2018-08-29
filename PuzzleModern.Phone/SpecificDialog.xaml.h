//
// SpecificDialog.xaml.h
// Declaration of the SpecificDialog class
//

#pragma once

#include "SpecificDialog.g.h"

namespace PuzzleModern
{
	public delegate void GameIDSpecifiedEventHandler(Platform::Object ^sender, Platform::String ^id, bool isRandomSeed);

	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class SpecificDialog sealed
	{
	public:
		SpecificDialog(Platform::String ^title, Platform::String ^gameId, bool isRandomSeed);
		event GameIDSpecifiedEventHandler ^GameIDSpecified;
	private:
		void ContentDialog_PrimaryButtonClick(Windows::UI::Xaml::Controls::ContentDialog^ sender, Windows::UI::Xaml::Controls::ContentDialogButtonClickEventArgs^ args);
		void ContentDialog_SecondaryButtonClick(Windows::UI::Xaml::Controls::ContentDialog^ sender, Windows::UI::Xaml::Controls::ContentDialogButtonClickEventArgs^ args);

		Platform::String ^_gameId;
		bool _isRandomSeed;
	};
}
