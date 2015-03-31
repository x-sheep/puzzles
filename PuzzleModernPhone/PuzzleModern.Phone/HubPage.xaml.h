//
// HubPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "HubPage.g.h"
#include "PuzzleData.h"

namespace PuzzleModern
{
	namespace Phone
	{
		public ref class HubPage sealed
		{
		public:
			HubPage();

			/// <summary>
			/// Gets the <see cref="NavigationHelper"/> associated with this <see cref="Page"/>.
			/// </summary>
			property Common::NavigationHelper ^NavigationHelper
			{
				Common::NavigationHelper^ get();
			}

			/// <summary>
			/// Gets the view model for this <see cref="Page"/>.
			/// This can be changed to a strongly typed view model.
			/// </summary>
			property Windows::Foundation::Collections::IObservableMap<Platform::String^, Platform::Object^>^ DefaultViewModel
			{
				Windows::Foundation::Collections::IObservableMap<Platform::String^, Platform::Object^>^  get();
			}

		protected:
			virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
			virtual void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

		private:
			void NavigationHelper_LoadState(Platform::Object^ sender, Common::LoadStateEventArgs^ e);
			void NavigationHelper_SaveState(Platform::Object^ sender, Common::SaveStateEventArgs^ e);

			static Windows::UI::Xaml::DependencyProperty^ _defaultViewModelProperty;
			static Windows::UI::Xaml::DependencyProperty^ _navigationHelperProperty;
			void AboutButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void ListViewItem_Holding(Platform::Object^ sender, Windows::UI::Xaml::Input::HoldingRoutedEventArgs^ e);
			void ListViewItem_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void HelpButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void SettingsButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void SwitchViewButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void SwitchView();

			Control ^AllPuzzlesGridView, ^AllPuzzlesListView;
			void AllPuzzlesGridView_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void AllPuzzlesListView_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void LoadGameButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		};
	}
}