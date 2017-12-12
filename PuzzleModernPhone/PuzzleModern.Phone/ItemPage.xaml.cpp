//
// ItemPage.xaml.cpp
// Implementation of the ItemPage class.
//

#include "pch.h"
#include "..\..\PuzzleCommon\PuzzleData.h"
#include "ItemPage.xaml.h"
#include "HelpPage.xaml.h"
#include "ParamsDialog.xaml.h"
#include "PresetDialog.xaml.h"
#include "SpecificDialog.xaml.h"
#include "PulseEffect.xaml.h"

using namespace PuzzleModern::Phone;
using namespace PuzzleModern::Phone::Common;

using namespace concurrency;
using namespace Platform;
using namespace Windows::ApplicationModel::DataTransfer;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::System;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;

// The Hub Application template is documented at http://go.microsoft.com/fwlink/?LinkID=391641

ItemPage::ItemPage()
{
	InitializeComponent();

	NavigationCacheMode = Navigation::NavigationCacheMode::Disabled;

	auto navigationHelper = ref new Common::NavigationHelper(this);
	navigationHelper->LoadState += ref new LoadStateEventHandler(this, &ItemPage::NavigationHelper_LoadState);
	navigationHelper->SaveState += ref new SaveStateEventHandler(this, &ItemPage::NavigationHelper_SaveState);

	_forceRedrawEventToken = DrawCanvas->NeedsRedraw += ref new ForceRedrawDelegate(this, &ItemPage::ForceRedraw);
	
	SetValue(_defaultViewModelProperty, ref new Platform::Collections::Map<String^, Object^>(std::less<String^>()));
	SetValue(_navigationHelperProperty, navigationHelper);

	this->Loaded += ref new Windows::UI::Xaml::RoutedEventHandler(this, &PuzzleModern::Phone::ItemPage::OnLoaded);
	this->Unloaded += ref new Windows::UI::Xaml::RoutedEventHandler(this, &PuzzleModern::Phone::ItemPage::OnUnloaded);
}

DependencyProperty^ ItemPage::_defaultViewModelProperty = DependencyProperty::Register(
	"DefaultViewModel",
	IObservableMap<String^, Object^>::typeid,
	ItemPage::typeid,
	nullptr);

IObservableMap<String^, Object^>^ ItemPage::DefaultViewModel::get()
{
	return safe_cast<IObservableMap<String^, Object^>^>(GetValue(_defaultViewModelProperty));
}

DependencyProperty^ ItemPage::_navigationHelperProperty = DependencyProperty::Register(
	"NavigationHelper",
	NavigationHelper::typeid,
	ItemPage::typeid,
	nullptr);

NavigationHelper^ ItemPage::NavigationHelper::get()
{
	return safe_cast<Common::NavigationHelper^>(GetValue(_navigationHelperProperty));
}

void ItemPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedTo(e);
}

void ItemPage::OnNavigatedFrom(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedFrom(e);
}

/// <summary>
/// Populates the page with content passed during navigation. Any saved state is also
/// provided when recreating a page from a prior session.
/// </summary>
/// <param name="sender">
/// The source of the event; typically <see cref="NavigationHelper"/>.
/// </param>
/// <param name="e">Event data that provides both the navigation parameter passed to
/// <see cref="Frame->Navigate(Type, Object)"/> when this page was initially requested and
/// a dictionary of state preserved by this page during an earlier
/// session. The state will be null the first time a page is visited.</param>
void ItemPage::NavigationHelper_LoadState(Object^ sender, LoadStateEventArgs^ e)
{
	(void) sender;	// Unused parameter
	(void) e;		// Unused parameter

	_puzzleName = safe_cast<Platform::String^>(e->NavigationParameter);
	fe = std::make_shared<WindowsModern>(WindowsModern());
	_hasGame = fe->CreateForGame(_puzzleName, DrawCanvas, this, this);

	Puzzle^ selectedPuzzle = fe->GetCurrentPuzzle();
	if (!fe->CanSolve())
		SolveButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	
	DefaultViewModel->Insert("PuzzleName", selectedPuzzle->Name);
	_puzzleNameUpper = App::ToUpper(selectedPuzzle->Name);
	DefaultViewModel->Insert("PuzzleNameUpper", _puzzleNameUpper);
	DefaultViewModel->Insert("PageBackground", DrawCanvas->GetFirstColor());
	DefaultViewModel->Insert("PageWidth", 400);

	if(!fe->HasStatusbar())
		PuzzleStatusBar->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

	_generatingGame = true;
	_isLoaded = false;

	if (e->PageState)
		BeginActivatePuzzle(nullptr);
}

void ItemPage::BeginActivatePuzzle(PuzzleModern::GameLaunchParameters ^p)
{
	if (_isLoaded && _generatingGame)
		return;

	_isLoaded = true;
	_generatingGame = false;

	if (p && p->SaveFile)
		BeginLoadGame(p->SaveFile, true);
	else if (_hasGame)
		BeginResumeGame();
	else
		BeginNewGame();
}

/// <summary>
/// Preserves state associated with this page in case the application is suspended or the
/// page is discarded from the navigation cache. Values must conform to the serialization
/// requirements of <see cref="SuspensionManager.SessionState"/>.
/// </summary>
/// <param name="sender">
/// The source of the event; typically <see cref="NavigationHelper"/>.
/// </param>
/// <param name="e">
/// Event data that provides an empty dictionary to be populated with serializable state.
/// </param>
void ItemPage::NavigationHelper_SaveState(Object^ sender, SaveStateEventArgs^ e)
{
	if (!_generatingGame)
		fe->SaveGameToStorage(_puzzleName);
	else if (generatingWorkItem)
		generatingWorkItem->Cancel();
}

void ItemPage::BeginLoadGame(Windows::Storage::StorageFile ^file, bool firstLaunch)
{
	OutputDebugString(L"Loading game...\n");
	if (_isLoaded && _generatingGame)
		return;

	BusyLabel->Text = "Loading game";
	OnGenerationStart();

	create_task(fe->LoadGameFromFile(file)).then([=](Platform::String ^err)
	{
		Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([=]{
			_generatingGame = false;
			_isLoaded = true;
			if (err)
			{
				(ref new Windows::UI::Popups::MessageDialog(err, "Could not load game"))->ShowAsync();
				if (firstLaunch)
				{
					if (_hasGame)
						BeginResumeGame();
					else
						BeginNewGame();
				}
			}
			OnGenerationEnd();
		}));
	});
}

void ItemPage::BeginResumeGame()
{
	if (_isLoaded && _generatingGame)
		return;

	BusyLabel->Text = "Loading game";
	OnGenerationStart();

	create_task(fe->LoadGameFromStorage(_puzzleName)).then([this](task<bool> task)
	{
		bool loaded;
		try
		{
			loaded = task.get();
		}
		catch (Exception^)
		{
			loaded = false;
		}
		Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
			ref new Windows::UI::Core::DispatchedHandler([this, loaded]
		{
			_generatingGame = false;
			_isLoaded = true;
			if (loaded)
			{
				_wonGame = fe->GameWon() == 1;
				OnGenerationEnd();
			}
			else
				BeginNewGame();
		}));
	});
}

void ItemPage::BeginNewGame()
{
	if (_isLoaded && _generatingGame)
		return;

	BusyLabel->Text = "Creating puzzle";
	OnGenerationStart();

	auto workItemDelegate = [this](IAsyncAction^ workItem)
	{
		generatingWorkItem = workItem;
		fe->NewGame(workItem);
		generatingWorkItem = nullptr;

		bool success = workItem->Status != AsyncStatus::Canceled;

		Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this, success]
		{
			_isLoaded = true;
			if (success)
				OnGenerationEnd();
		}));
	};

	auto workItemHandler = ref new Windows::System::Threading::WorkItemHandler(workItemDelegate);
	auto workItem = Windows::System::Threading::ThreadPool::RunAsync(workItemHandler, Windows::System::Threading::WorkItemPriority::Normal);
}

void ItemPage::BusyOverlayDisappearingAnimation_Completed(Platform::Object^ sender, Platform::Object^ e)
{
	BusyOverlay->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}


void ItemPage::DoubleAnimation_Completed(Platform::Object^ sender, Platform::Object^ e)
{
	_finishedOverlayAnimation = true;
}

void ItemPage::OnGenerationStart()
{
	_generatingGame = true;
	BusyOverlay->Visibility = Windows::UI::Xaml::Visibility::Visible;
	BusyLabel->Visibility = Windows::UI::Xaml::Visibility::Visible;
	BusyIndicator->IsIndeterminate = true;
	BusyIndicator->Visibility = Windows::UI::Xaml::Visibility::Visible;
	_finishedOverlayAnimation = false;
	BusyOverlayDisappearingStoryboard->Stop();
	BusyOverlayAppearingStoryboard->Begin();
}

void ItemPage::OnGenerationEnd()
{
	_generatingGame = false;
	_controls = fe->GetVirtualButtonBar();

	if (_controls->HasInputButtons)
		VirtualButtonBar->Buttons = _controls;
	else
		VirtualButtonBar->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

	bool isSwitched = LeftRightButton->IsChecked->Value;
	_touchAction = isSwitched ? _controls->HoldAction: _controls->TouchAction;
	_holdAction = isSwitched ? _controls->TouchAction: _controls->HoldAction;

	if (_controls->ToolButton)
	{
		toolKey = _controls->ToolButton;
		ToolButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
		ToolButton->Label = App::ToLower(toolKey->Name);
		ToolButton->Icon = toolKey->Icon;
	}
	else if (_controls->ToggleButton)
	{
		LeftRightButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
		LeftRightButton->Label = App::ToLower(_controls->ToggleButton->Name);
		LeftRightButton->Icon = _controls->ToggleButton->Icon;
	}

	_wonGame = fe->GameWon() == 1;
	UpdateUndoButtons();
	ResizeGame();
	if (!_finishedOverlayAnimation)
	{
		BusyOverlayAppearingStoryboard->Stop();
		BusyOverlay->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	}
	else
	{
		BusyOverlayDisappearingStoryboard->Begin();
	}
	BusyIndicator->IsIndeterminate = false;
	BusyIndicator->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	BusyLabel->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	
	auto presets = fe->GetPresetList(false, 1);
	TypeButton->Label = "type: " + fe->GetCurrentPresetName(presets);

	if (_controls->ColorBlindKey != VirtualKey::None)
	{
		auto settings = ApplicationData::Current->RoamingSettings;
		if (settings->Values->HasKey("cfg_colorblind") && safe_cast<bool>(settings->Values->Lookup("cfg_colorblind")))
			fe->SendKey(_controls->ColorBlindKey, false, false);
	}
}

void ItemPage::ResizeGame()
{
	if (!fe || _generatingGame)
		return;

	int iw = (int)GameBorder->ActualWidth;
	int ih = (int)GameBorder->ActualHeight;

	float scale = Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi / 96;

	int ow = 0;
	int oh = 0;

	fe->GetMaxSize(iw*scale, ih*scale, &ow, &oh);

	DrawCanvas->Width = ow / scale;
	DrawCanvas->Height = oh / scale;

	if (ow > 20 && oh > 20)
		fe->Redraw(DrawCanvas, this, this, true);
	fe->SetInputScale(scale);
	DefaultViewModel->Insert("PageWidth", this->ActualWidth);
}

void ItemPage::GameBorder_SizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
{
	if (_generatingGame)
		return;
	ResizeGame();
}

void ItemPage::DrawCanvas_PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	auto ptrPt = e->GetCurrentPoint(DrawCanvas);
	if (e->Pointer->PointerDeviceType != Windows::Devices::Input::PointerDeviceType::Touch || fe->IsRightButtonDisabled())
	{
		int x = ptrPt->Position.X, y = ptrPt->Position.Y;

		if (ptrPt->Properties->IsLeftButtonPressed)
		{
			fe->SendClick(x, y, _touchAction, ButtonState::DOWN);
			_touchPressed = true;
			e->Handled = true;
		}
		if (ptrPt->Properties->IsRightButtonPressed)
		{
			fe->SendClick(x, y, _holdAction, ButtonState::DOWN);
			_holdPressed = true;
			e->Handled = true;
		}
	}

	/*
	* When using touch, we use gestures to differentiate between left and right clicks.
	* Tapping and dragging count as left button, while holding counts as right button.
	* If this is a right button, we can not yet process the Pressed event.
	*/
	else
	{
		_initialPoint = ptrPt->Position;
		_initialPressed = true;
		e->Handled = true;
		auto uiDelegate = [this]()
		{
			if (!_generatingGame && _initialPressed)
			{
				float x = _initialPoint.X, y = _initialPoint.Y;
				_initialPressed = false;
				fe->SendClick(x, y,_holdAction, ButtonState::DOWN);
				_holdPressed = true;
				DrawCanvas->Pulsate(x, y);
			}
		};
		auto handle = ref new Windows::UI::Core::DispatchedHandler(uiDelegate);
		auto timerDelegate = [this, handle](Windows::System::Threading::ThreadPoolTimer^ timer)
		{
			Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::High, handle);
		};
		Windows::Foundation::TimeSpan period = TimeSpan();
		period.Duration = 600 * 10000; // 0.6 seconds
		if (RightClickTimer)
			RightClickTimer->Cancel();
		RightClickTimer = Windows::System::Threading::ThreadPoolTimer::CreateTimer(ref new Windows::System::Threading::TimerElapsedHandler(timerDelegate), period);
	}
}


void ItemPage::pageRoot_PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	auto ptrPt = e->GetCurrentPoint(DrawCanvas);
	int x = ptrPt->Position.X, y = ptrPt->Position.Y;

	if (_touchPressed)
	{
		fe->SendClick(x, y, _touchAction, ButtonState::UP);
		_touchPressed = false;
		e->Handled = true;
	}
	if (_holdPressed)
	{
		fe->SendClick(x, y, _holdAction, ButtonState::UP);
		_holdPressed = false;
		e->Handled = true;
	}
	if (_initialPressed)
	{
		fe->SendClick(_initialPoint.X, _initialPoint.Y, _touchAction, ButtonState::DOWN);
		fe->SendClick(x, y, _touchAction, ButtonState::UP);
		_initialPressed = false;
		e->Handled = true;
	}

	if (RightClickTimer)
	{
		RightClickTimer->Cancel();
		RightClickTimer = nullptr;
	}

	UpdateUndoButtons();
}

#define LEFT_DISTANCE 10

void ItemPage::pageRoot_PointerMoved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	auto ptrPt = e->GetCurrentPoint(DrawCanvas);
	int x = ptrPt->Position.X, y = ptrPt->Position.Y;

	/* When moving outside a certain region, confirm this as a left drag */
	if (_initialPressed)
	{
		float xdiff = fabs(x - _initialPoint.X), ydiff = fabs(y - _initialPoint.Y);
		if ((xdiff*xdiff) + (ydiff*ydiff) > LEFT_DISTANCE*LEFT_DISTANCE)
		{
			_initialPressed = false;
			fe->SendClick(_initialPoint.X, _initialPoint.Y, _touchAction, ButtonState::DOWN);
			_touchPressed = true;
		}
	}
	if (_touchPressed)
	{
		fe->SendClick(x, y, _touchAction, ButtonState::DRAG);
		e->Handled = true;
	}
	if (_holdPressed)
	{
		fe->SendClick(x, y, _holdAction, ButtonState::DRAG);
		e->Handled = true;
	}
}

void ItemPage::UpdateStatusBar(Platform::String ^status)
{
	PuzzleStatusBar->Text = status;
}

void ItemPage::ForceRedraw()
{
	if (_generatingGame)
		return;

	fe->Redraw(DrawCanvas, this, this, true);
}

void ItemPage::UpdateUndoButtons()
{
	UndoButton->IsEnabled = fe->CanUndo();
	RedoButton->IsEnabled = fe->CanRedo();

	int win = fe->GameWon();

	if (win == +1 && !_wonGame)
	{
		_wonGame = true;
		if (fe->JustPerformedUndo())
			return;

		auto settings = ApplicationData::Current->RoamingSettings;
		if (settings->Values->HasKey("cfg_newgameprompt") && !safe_cast<bool>(settings->Values->Lookup("cfg_newgameprompt")))
			return;

		auto timer = ref new DispatcherTimer();
		auto time = TimeSpan();
		time.Duration = 700 * 10000;
		timer->Interval = time;
		timer->Tick += ref new EventHandler<Object^>([this, timer](Object ^sender, Object ^data)
		{
			timer->Stop();

			CompletePopup->IsOpen = true;
		});
		timer->Start();
	}

	if (fe->JustPerformedRedo())
		_wonGame = false;
}

void ItemPage::UndoButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	fe->Undo();
	UpdateUndoButtons();
}


void ItemPage::RedoButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	fe->Redo();
	UpdateUndoButtons();
}

void ItemPage::RestartButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	fe->RestartGame();
	UpdateUndoButtons();
}

void ItemPage::NewButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	CompletePopup->IsOpen = false;

	fe->Redraw(DrawCanvas, this, this, true);
	BeginNewGame();
}

void ItemPage::TypeButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	CompletePopup->IsOpen = false;

	if (fe->IsCustomGame())
		OpenCustomDialog();
	else
		OpenPresetsDialog();
}

void ItemPage::LoadSaveButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;
	
	ExportPopup->IsOpen = true;
}

void ItemPage::GameID_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	ExportPopup->IsOpen = false;
	auto dialog = ref new SpecificDialog(_puzzleNameUpper, fe->GetGameDesc(), false);
	dialog->GameIDSpecified += ref new PuzzleModern::GameIDSpecifiedEventHandler(this, &PuzzleModern::Phone::ItemPage::OnGameIDSpecified);
	dialog->ShowAsync();
}

void ItemPage::RandomSeed_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	ExportPopup->IsOpen = false;
	auto dialog = ref new SpecificDialog(_puzzleNameUpper, fe->GetRandomSeed(), true);
	dialog->GameIDSpecified += ref new PuzzleModern::GameIDSpecifiedEventHandler(this, &PuzzleModern::Phone::ItemPage::OnGameIDSpecified);
	dialog->ShowAsync();
}

void ItemPage::OnGameIDSpecified(Platform::Object ^sender, Platform::String ^id, bool isRandomSeed)
{
	auto error = fe->SetGameID(id);

	if (error)
	{
		auto messageDialog = ref new Windows::UI::Popups::MessageDialog(error, "Invalid ID");
		create_task(messageDialog->ShowAsync()).then([this, id, isRandomSeed](Windows::UI::Popups::IUICommand^ command)
		{
			SpecificDialog ^dialog;
			dialog = ref new SpecificDialog(_puzzleNameUpper, id, isRandomSeed);

			dialog->GameIDSpecified += ref new PuzzleModern::GameIDSpecifiedEventHandler(this, &PuzzleModern::Phone::ItemPage::OnGameIDSpecified);
			dialog->ShowAsync();
		});
	}
	else
	{
		BeginNewGame();
	}
}

void ItemPage::OpenPresetsDialog()
{
	if (_generatingGame)
		return;

	auto presets = fe->GetPresetList(false, 1);
	auto dialog = ref new PresetDialog(presets);
	dialog->PresetClicked += ref new PuzzleModern::Phone::PresetClickedEventHandler(this, &PuzzleModern::Phone::ItemPage::OnNewPreset);
	dialog->CustomClicked += ref new PuzzleModern::Phone::CustomClickedEventHandler(this, &PuzzleModern::Phone::ItemPage::OpenCustomDialog);
	dialog->ShowAsync();
}

void ItemPage::OpenCustomDialog()
{
	if (_generatingGame)
		return;

	auto dialog = ref new ParamsDialog(fe->GetConfiguration());
	dialog->NewConfiguration += ref new PuzzleModern::Phone::NewConfigurationEventHandler(this, &PuzzleModern::Phone::ItemPage::OnNewConfiguration);
	dialog->PresetsClicked += ref new PuzzleModern::Phone::PresetsClickedEventHandler(this, &PuzzleModern::Phone::ItemPage::OpenPresetsDialog);
	dialog->ShowAsync();
}

void ItemPage::OnNewPreset(Platform::Object ^sender, PuzzleModern::Preset ^preset)
{
	if (_generatingGame)
		return;
	fe->SetPreset(preset->Index);
	BeginNewGame();
}

void ItemPage::OnNewConfiguration(ParamsDialog ^sender, Windows::Foundation::Collections::IVector<PuzzleModern::ConfigItem ^> ^newConfig)
{
	if (_generatingGame)
		return;
	auto error = fe->SetConfiguration(newConfig);
	if (error)
	{
		auto messageDialog = ref new Windows::UI::Popups::MessageDialog(error, "Invalid parameters");
		create_task(messageDialog->ShowAsync()).then([this, sender](Windows::UI::Popups::IUICommand^ command)
		{
			sender->ShowAsync();
		});
	}
	else
	{
		BeginNewGame();
		sender->Hide();
	}
}

void ItemPage::SolveButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	String ^msg = fe->Solve();
	if (msg)
	{
		// Style choice: Popup messages should end with a period.
		if (msg->End()[-1] != L'.')
			msg += L".";

		(ref new Windows::UI::Popups::MessageDialog(msg, "Cannot solve"))->ShowAsync();
	}
	_wonGame = true;
	UpdateUndoButtons();
}


void ItemPage::StartTimer()
{
	if (PeriodicTimer != nullptr)
		PeriodicTimer->Cancel();

	auto uiDelegate = [this]()
	{
		auto newtime = GetTickCount64();
		auto delta = newtime - LastTime;
		if (!delta) return;

		LastTime = newtime;
		Timer(delta / 1000.0f);
	};
	auto handle = ref new Windows::UI::Core::DispatchedHandler(uiDelegate);
	auto timerDelegate = [this, handle](Windows::System::Threading::ThreadPoolTimer^ timer)
	{
		Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::High, handle);
	};
	Windows::Foundation::TimeSpan period = TimeSpan();
	period.Duration = 5 * 10000; // 5 milliseconds

	LastTime = GetTickCount64();
	PeriodicTimer = Windows::System::Threading::ThreadPoolTimer::CreatePeriodicTimer(ref new Windows::System::Threading::TimerElapsedHandler(timerDelegate), period);
}

void ItemPage::EndTimer()
{
	if (PeriodicTimer != nullptr)
	{
		PeriodicTimer->Cancel();
		PeriodicTimer = nullptr;
	}
}

void ItemPage::Timer(float seconds)
{
	if (_generatingGame)
		return;

	fe->UpdateTimer(DrawCanvas, this, this, seconds);
}

void ItemPage::HelpButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Frame->Navigate(HelpPage::typeid, fe->GetCurrentPuzzle()->HelpName);
}

void ItemPage::VirtualButtonBar_ButtonPressed(Platform::Object^ sender, PuzzleModern::VirtualButton^ button)
{
	if (_generatingGame)
		return;

	fe->SendKey(button->Key, false, false);

	UpdateUndoButtons();
}

void ItemPage::ToolButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	fe->SendKey(toolKey->Key, false, false);

	UpdateUndoButtons();
}

void ItemPage::LeftRightButton_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	_touchAction = _controls->HoldAction;
	_holdAction = _controls->TouchAction;

	if (!_generatingGame)
		fe->SendKey(WindowsModern::ButtonMarkOn, false, false);
}

void ItemPage::LeftRightButton_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	_touchAction = _controls->TouchAction;
	_holdAction = _controls->HoldAction;

	if (!_generatingGame)
		fe->SendKey(WindowsModern::ButtonMarkOff, false, false);
}

void ItemPage::LoadGame_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
	FileOpenPicker^ openPicker = ref new FileOpenPicker();
	openPicker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;
	openPicker->FileTypeFilter->Append(".puzzle");
	openPicker->FileTypeFilter->Append(".sav");
	openPicker->CommitButtonText = "Load game";

	openPicker->PickSingleFileAndContinue();
}


void ItemPage::SaveGame_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
	create_task([=]{
		return fe->SaveGameToStorage(_puzzleName);
	}).then([=](bool saved){
		if (!saved) return;
		
		FileSavePicker^ savePicker = ref new FileSavePicker();
		savePicker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;

		auto extensions = ref new Platform::Collections::Vector<String^>();
		extensions->Append(".puzzle");
		extensions->Append(".sav");
		savePicker->FileTypeChoices->Insert("Saved game", extensions);
		savePicker->CommitButtonText = "Save game";
		savePicker->SuggestedFileName = _puzzleName;

		savePicker->ContinuationData->Insert("PuzzleFile", _puzzleName + ".puzzle");

		savePicker->PickSaveFileAndContinue();
	}, task_continuation_context::use_current());
}


void ItemPage::ShareGame_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
	DataTransferManager::ShowShareUI();
}


void ItemPage::OnLoaded(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^e)
{
	_shareEventToken = DataTransferManager::GetForCurrentView()->DataRequested +=
		ref new Windows::Foundation::TypedEventHandler<DataTransferManager ^, DataRequestedEventArgs ^>
		(this, &ItemPage::OnDataRequested);
}


void ItemPage::OnUnloaded(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^e)
{
	DataTransferManager::GetForCurrentView()->DataRequested -= _shareEventToken;
}

void ItemPage::OnDataRequested(DataTransferManager ^sender, DataRequestedEventArgs ^args)
{
	auto data = args->Request->Data;
	data->Properties->Title = "Saved Game for " + _puzzleName;
	data->Properties->Description = fe->CanUndo() ? "Share your progress in " + _puzzleName : "Share this " + _puzzleName + " puzzle";
	data->Properties->FileTypes->Clear();
	data->Properties->FileTypes->Append(".puzzle");
	data->SetDataProvider(StandardDataFormats::StorageItems, ref new DataProviderHandler(this, &ItemPage::OnDeferredDataRequestedHandler));
}

void ItemPage::OnDeferredDataRequestedHandler(DataProviderRequest^ request)
{
	auto deferral = request->GetDeferral();

	create_task([this](){
		return ApplicationData::Current->TemporaryFolder->CreateFileAsync(_puzzleName + ".puzzle", CreationCollisionOption::ReplaceExisting);
	}).then([this, request, deferral](StorageFile ^opened){

		create_task([=](){
			return fe->SaveGameToFile(opened);
		}).then([=](bool result){
			auto files = ref new Platform::Collections::Vector<StorageFile^>();
			files->Append(opened);
			request->SetData(files);

			deferral->Complete();
		});
	});
}

void ItemPage::MenuButton_Click(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
	CompletePopup->IsOpen = false;
	if (NavigationHelper->CanGoBack())
		NavigationHelper->GoBack();
	else
	{
		App::Current->ActivatePuzzle(nullptr, false, true);
	}
}
