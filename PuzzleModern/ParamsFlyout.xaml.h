//
// ParamsFlyout.xaml.h
// Declaration of the ParamsFlyout class
//

#pragma once

#include "ParamsFlyout.g.h"

using namespace PuzzleCommon;

namespace PuzzleModern
{
	public delegate void NewConfigurationEventHandler(ParamsFlyout ^sender, Windows::Foundation::Collections::IVector<ConfigItem^>^ newConfig);

	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class ParamsFlyout sealed
	{
	public:
		ParamsFlyout(Windows::Foundation::Collections::IVector<ConfigItem^>^ items);
		void ShowErrorMessage(Platform::String ^error);
		event NewConfigurationEventHandler ^NewConfiguration;
	private:
		Windows::Foundation::Collections::IVector<ConfigItem^>^ _configItems;
		void ConfirmButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
