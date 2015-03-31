//
// ItemPage.xaml.h
// Declaration of the ItemPage class.
//

#pragma once

#include "ItemPage.g.h"
#include "Direct2DPuzzleCanvas.xaml.h"
#include "ParamsDialog.xaml.h"
#include "PuzzleKeyboard.xaml.h"
#include "SpecificDialog.xaml.h"
#include "PuzzleData.h"
#include "WindowsModern.h"

namespace PuzzleModern
{
	namespace Phone
	{
		/// <summary>
		/// A page that displays details for a single item within a group.
		/// </summary>
		public ref class ItemPage sealed : IPuzzleStatusBar, IPuzzleTimer
		{
		public:
			ItemPage();

			/// <summary>
			/// Gets the <see cref="NavigationHelper"/> associated with this <see cref="Page"/>.
			/// </summary>
			property Common::NavigationHelper^ NavigationHelper
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

			virtual void UpdateStatusBar(Platform::String ^status);
			virtual void StartTimer();
			virtual void EndTimer();
			
			void BeginLoadGame(Windows::Storage::StorageFile ^file) { BeginLoadGame(file, false); }
			void BeginActivatePuzzle(PuzzleModern::GameLaunchParameters ^p);

		protected:
			virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
			virtual void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

		private:
			void NavigationHelper_LoadState(Platform::Object^ sender, Common::LoadStateEventArgs^ e);
			void NavigationHelper_SaveState(Platform::Object^ sender, Common::SaveStateEventArgs^ e);

			Platform::String ^_puzzleName, ^_puzzleNameUpper;
			std::shared_ptr<PuzzleModern::WindowsModern> fe;
			
			unsigned long long LastTime;
			Windows::System::Threading::ThreadPoolTimer ^PeriodicTimer;

			void GameBorder_SizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);

			PresetList ^presets;
			VirtualButton ^toolKey;

			void OpenPresetsDialog();
			void OpenCustomDialog();

			Windows::Foundation::EventRegistrationToken _forceRedrawEventToken;
			Windows::Foundation::EventRegistrationToken _shareEventToken;

			void BusyOverlayDisappearingAnimation_Completed(Platform::Object^ sender, Platform::Object^ e);
			void DoubleAnimation_Completed(Platform::Object^ sender, Platform::Object^ e);

			void AddOverlay();
			void RemoveOverlay();
			void BeginLoadGame(Windows::Storage::StorageFile ^file, bool firstLaunch);
			void BeginResumeGame();
			void BeginNewGame();
			void ResizeGame();
			void ForceRedraw();
			void UpdateUndoButtons();
			void Timer(float seconds);

			void UndoButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void RedoButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void RestartButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void TypeButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void SolveButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void NewButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

			void GameID_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void RandomSeed_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

			void DrawCanvas_PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
			void DrawCanvas_PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
			void DrawCanvas_PointerMoved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);

			bool _leftPressed, _rightPressed, _initialPressed, _generatingGame, _finishedOverlayAnimation, _ctrlPressed, _shiftPressed, _wonGame, _hasGame, _isLoaded;
			Windows::Foundation::Point _initialPoint;
			Windows::System::Threading::ThreadPoolTimer ^RightClickTimer;
			
			Windows::Foundation::IAsyncAction^ generatingWorkItem;

			ButtonType _leftAction, _rightAction;

			static Windows::UI::Xaml::DependencyProperty^ _defaultViewModelProperty;
			static Windows::UI::Xaml::DependencyProperty^ _navigationHelperProperty;
			void OnNewConfiguration(ParamsDialog ^sender, Windows::Foundation::Collections::IVector<PuzzleModern::ConfigItem ^> ^newConfig);
			void OnNewPreset(Platform::Object ^sender, PuzzleModern::Preset ^preset);
			void OnGameIDSpecified(Platform::Object ^sender, Platform::String ^id, bool isRandomSeed);
			void LoadSaveButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void HelpButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void VirtualButtonBar_ButtonPressed(Platform::Object^ sender, PuzzleModern::VirtualButton^ button);
			void ToolButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void LeftRightButton_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void LeftRightButton_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void LoadGame_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void SaveGame_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void ShareGame_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
			void OnDataRequested(Windows::ApplicationModel::DataTransfer::DataTransferManager ^sender, Windows::ApplicationModel::DataTransfer::DataRequestedEventArgs ^args);
			void OnDeferredDataRequestedHandler(Windows::ApplicationModel::DataTransfer::DataProviderRequest^ request);
			void OnLoaded(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^e);
			void OnUnloaded(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^e);
			void MenuButton_Click(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
};
	}
}