//
// ParamsDialog.xaml.h
// Declaration of the ParamsDialog class
//

#pragma once

#include "ParamsDialog.g.h"
#include "..\PuzzleCommon\PuzzleData.h"

namespace PuzzleModern
{
	namespace Phone
	{
		public delegate void NewConfigurationEventHandler(ParamsDialog ^sender, Windows::Foundation::Collections::IVector<ConfigItem^>^ newConfig);
		public delegate void PresetsClickedEventHandler();

		[Windows::Foundation::Metadata::WebHostHidden]
		public ref class ParamsDialog sealed
		{
		public:
			ParamsDialog(Windows::Foundation::Collections::IVector<ConfigItem^>^ items);
			event NewConfigurationEventHandler ^NewConfiguration;
			event PresetsClickedEventHandler ^PresetsClicked;
		private:
			void ContentDialog_PrimaryButtonClick(Windows::UI::Xaml::Controls::ContentDialog^ sender, Windows::UI::Xaml::Controls::ContentDialogButtonClickEventArgs^ args);
			void ContentDialog_SecondaryButtonClick(Windows::UI::Xaml::Controls::ContentDialog^ sender, Windows::UI::Xaml::Controls::ContentDialogButtonClickEventArgs^ args);
			Windows::Foundation::Collections::IVector<ConfigItem^>^ _configItems;
			void OnShowing(Windows::UI::ViewManagement::InputPane ^sender, Windows::UI::ViewManagement::InputPaneVisibilityEventArgs ^args);
			void OnHiding(Windows::UI::ViewManagement::InputPane ^sender, Windows::UI::ViewManagement::InputPaneVisibilityEventArgs ^args);
			void LayoutScrollViewer_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

			Windows::UI::Xaml::Controls::ScrollViewer ^LayoutScrollViewer;
			void ContentDialog_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void ContentDialog_Unloaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

			Windows::Foundation::EventRegistrationToken _showingEventToken;
			Windows::Foundation::EventRegistrationToken _hidingEventToken;
		};
	}
}