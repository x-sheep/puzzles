//
// GamePage.xaml.h
// Declaration of the GamePage class
//

#pragma once

#include "GamePage.g.h"
#include "ParamsFlyout.g.h"
#include "Common\NavigationHelper.h"
#include "..\..\PuzzleCommon\PuzzleData.h"
#include "..\..\PuzzleCommon\WindowsModern.h"

namespace PuzzleModern
{
	/// <summary>
	/// A page that displays details for a single item within a group while allowing gestures to
	/// flip through other items belonging to the same group.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class GamePage sealed : IPuzzleStatusBar, IPuzzleTimer
	{
	public:
		GamePage();

		/// <summary>
		/// This can be changed to a strongly typed view model.
		/// </summary>
		property Windows::Foundation::Collections::IObservableMap<Platform::String^, Platform::Object^>^ DefaultViewModel
		{
			Windows::Foundation::Collections::IObservableMap<Platform::String^, Platform::Object^>^  get();
		}

		/// <summary>
		/// NavigationHelper is used on each page to aid in navigation and 
		/// process lifetime management
		/// </summary>
		property Common::NavigationHelper^ NavigationHelper
		{
			Common::NavigationHelper^ get();
		}

		void Timer(float seconds);
		virtual void UpdateStatusBar(Platform::String ^status);

		void ForceRedraw();
		void BeginLoadGame(Windows::Storage::StorageFile ^file, bool makeNew);
		void BeginActivatePuzzle(PuzzleModern::GameLaunchParameters ^p);

		virtual void StartTimer();
		virtual void EndTimer();

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
		virtual void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

	private:
		PuzzleModern::Puzzle ^currentPuzzle;
		Platform::String ^_puzzleName;
		WindowsModern* fe;
		unsigned long long LastTime;
		Windows::System::Threading::ThreadPoolTimer ^PeriodicTimer;

		Windows::Foundation::EventRegistrationToken _forceRedrawEventToken;
		Windows::Foundation::EventRegistrationToken _acceleratorKeyEventToken;
		Windows::Foundation::EventRegistrationToken _visibilityChangedEventToken;
		Windows::Foundation::EventRegistrationToken _commandsRequestedEventRegistrationToken;
		Windows::Foundation::EventRegistrationToken _suspendingEventToken;
		Windows::Foundation::EventRegistrationToken _resumingEventToken;
		Windows::Foundation::EventRegistrationToken _settingChangedEventToken;
		Windows::Foundation::EventRegistrationToken _shareEventToken;
		Windows::Foundation::EventRegistrationToken _printEventToken;

		VirtualButtonCollection ^_controls;

		void LoadState(Platform::Object^ sender, Common::LoadStateEventArgs^ e);

		void UpdateUndoButtons();
		void ResizeGame();
		void ResizeWindow(int newWidth, int newHeight);
		void OpenParamsFlyout(bool independent);
		void OpenHelpFlyout(bool independent);
		void BeginNewGame();
		void BeginResumeGame();
		void OnGenerationStart();
		void OnGenerationEnd();
		void HighlightPreset(int index);

		static Windows::UI::Xaml::DependencyProperty^ _defaultViewModelProperty;
		static Windows::UI::Xaml::DependencyProperty^ _navigationHelperProperty;
		void SpecificGameID_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void SpecificRandomSeed_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonNew_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonRestart_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonSolve_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonHelp_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void GameBorder_SizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);
		void pageRoot_SizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);
		void OnLoaded(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^e);
		void OnUnloaded(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^e);
		void OnSettingsCommand(Windows::UI::Popups::IUICommand^ command);
		void OnCommandsRequested(Windows::UI::ApplicationSettings::SettingsPane^ settingsPane, Windows::UI::ApplicationSettings::SettingsPaneCommandsRequestedEventArgs^ e);
		void OnAcceleratorKeyActivated(Windows::UI::Core::CoreDispatcher ^sender, Windows::UI::Core::AcceleratorKeyEventArgs ^args);
		void DrawCanvas_PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
		void pageRoot_PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
		void pageRoot_PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
		void pageRoot_PointerMoved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);

		bool _leftPressed, _middlePressed, _rightPressed, _initialPressed, _touchPressed, _holdPressed, _isFlyoutOpen, _generatingGame;
		bool _finishedOverlayAnimation, _ctrlPressed, _shiftPressed, _wonGame, _hasGame, _isLoaded;
		bool _undoHotkey, _redoHotkey;
		Windows::Foundation::Point _initialPoint;
		Windows::System::Threading::ThreadPoolTimer ^RightClickTimer;

		Windows::Foundation::IAsyncAction^ generatingWorkItem;

		ButtonType _leftAction, _middleAction, _rightAction, _touchAction, _holdAction;

		void DrawCanvas_RightTapped(Platform::Object^ sender, Windows::UI::Xaml::Input::RightTappedRoutedEventArgs^ e);
		void ButtonUndo_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonRedo_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonBar_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnNewConfiguration(PuzzleModern::ParamsFlyout ^sender, Windows::Foundation::Collections::IVector<PuzzleModern::ConfigItem ^> ^newConfig);
		void OnParamsFlyoutUnloaded(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^e);
		void PresetGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
		void BusyOverlayDisappearingAnimation_Completed(Platform::Object^ sender, Platform::Object^ e);
		void DoubleAnimation_Completed(Platform::Object^ sender, Platform::Object^ e);
		void SaveState(Platform::Object ^sender, PuzzleModern::Common::SaveStateEventArgs ^e);
		void OnVisibilityChanged(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::VisibilityChangedEventArgs ^args);
		void PromptPopup_Closed(Platform::Object^ sender, Platform::Object^ e);
		void PromptPopupButtonOK_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void PromptPopupButtonCancel_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonMenu_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void PresetGridView_LayoutUpdated(Platform::Object^ sender, Platform::Object^ e);
		void OnSuspending(Platform::Object ^sender, Windows::ApplicationModel::SuspendingEventArgs ^e);
		void OnResuming(Platform::Object ^sender, Platform::Object ^args);
		void SpecificSaveGame_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void SpecificLoadGame_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnDataRequested(Windows::ApplicationModel::DataTransfer::DataTransferManager ^sender, Windows::ApplicationModel::DataTransfer::DataRequestedEventArgs ^args);
		void OnDeferredDataRequestedHandler(Windows::ApplicationModel::DataTransfer::DataProviderRequest^ request);
		void OnPrintTaskRequested(Windows::Graphics::Printing::PrintManager ^sender, Windows::Graphics::Printing::PrintTaskRequestedEventArgs ^args);

#ifdef WIN10_PRINTING
		Windows::UI::Xaml::Printing::PrintDocument ^_printDoc;
		Windows::Graphics::Printing::PrintTaskOptions ^_printOptions;
		Windows::Graphics::Printing::IPrintDocumentSource ^_printSource;
		Windows::UI::Xaml::UIElement ^CreatePrintPage(bool isSolution);
		void OnAddPages(Platform::Object ^sender, Windows::UI::Xaml::Printing::AddPagesEventArgs ^e);
		void OnPaginate(Platform::Object ^sender, Windows::UI::Xaml::Printing::PaginateEventArgs ^e);
		void OnGetPreviewPage(Platform::Object ^sender, Windows::UI::Xaml::Printing::GetPreviewPageEventArgs ^e);
		void OnPrintTaskOptionChanged(Windows::Graphics::Printing::OptionDetails::PrintTaskOptionDetails ^sender, Windows::Graphics::Printing::OptionDetails::PrintTaskOptionChangedEventArgs ^args);
#endif

		void BusyCancelButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnSettingChanged(Platform::Object ^sender, Platform::String ^key, Platform::Object ^value);
		void ButtonLeftRight_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonLeftRight_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void LabelLeftRight_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
		void SpecificShareGame_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonSettings_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void BottomAppBar_Closed(Platform::Object^ sender, Platform::Object^ e);
};
}
