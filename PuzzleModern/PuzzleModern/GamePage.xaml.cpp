﻿//
// GamePage.xaml.cpp
// Implementation of the GamePage class
//

#include "pch.h"
#include "GamePage.xaml.h"
#include "HelpFlyout.xaml.h"
#include "ParamsFlyout.xaml.h"
#include "..\..\PuzzleCommon\ShapePuzzleCanvas.xaml.h"
#include "..\..\PuzzleCommon\Direct2DPuzzleCanvas.xaml.h"

using namespace PuzzleModern;

using namespace Platform;
using namespace Platform::Collections;
using namespace concurrency;
using namespace Windows::ApplicationModel::DataTransfer;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
#ifdef WIN10_PRINTING
using namespace Windows::Graphics::Printing;
using namespace Windows::Graphics::Printing::OptionDetails;
#endif
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::UI::ApplicationSettings;
using namespace Windows::UI::Popups;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Xaml::Printing;

GamePage::GamePage()
{
	InitializeComponent();

	auto navigationHelper = ref new Common::NavigationHelper(this);
	SetValue(_navigationHelperProperty, navigationHelper);
	SetValue(_defaultViewModelProperty, ref new Map<String^, Object^>(std::less<String^>()));

	navigationHelper->LoadState += ref new Common::LoadStateEventHandler(this, &GamePage::LoadState);
	navigationHelper->SaveState += ref new Common::SaveStateEventHandler(this, &GamePage::SaveState);

	this->Loaded += ref new Windows::UI::Xaml::RoutedEventHandler(this, &GamePage::OnLoaded);
	this->Unloaded += ref new Windows::UI::Xaml::RoutedEventHandler(this, &GamePage::OnUnloaded);

	_forceRedrawEventToken = DrawCanvas->NeedsRedraw += ref new ForceRedrawDelegate(this, &GamePage::ForceRedraw);
	_suspendingEventToken = Application::Current->Suspending += ref new SuspendingEventHandler(this, &GamePage::OnSuspending);
	_resumingEventToken = Application::Current->Resuming += ref new EventHandler<Platform::Object ^>(this, &GamePage::OnResuming);
	_settingChangedEventToken = App::Current->SettingChanged += ref new SettingsChangedEventHandler(this, &GamePage::OnSettingChanged);
}

DependencyProperty^ GamePage::_defaultViewModelProperty =
	DependencyProperty::Register("DefaultViewModel",
		TypeName(IObservableMap<String^,Object^>::typeid), TypeName(GamePage::typeid), nullptr);

IObservableMap<String^, Object^>^ GamePage::DefaultViewModel::get()
{
	return safe_cast<IObservableMap<String^, Object^>^>(GetValue(_defaultViewModelProperty));
}

#pragma region Navigation support

DependencyProperty^ GamePage::_navigationHelperProperty =
	DependencyProperty::Register("NavigationHelper",
		TypeName(Common::NavigationHelper::typeid), TypeName(GamePage::typeid), nullptr);

/// <summary>
/// Gets an implementation of <see cref="NavigationHelper"/> designed to be
/// used as a trivial view model.
/// </summary>
Common::NavigationHelper^ GamePage::NavigationHelper::get()
{
	return safe_cast<Common::NavigationHelper^>(GetValue(_navigationHelperProperty));
}

/// The methods provided in this section are simply used to allow
/// NavigationHelper to respond to the page's navigation methods.
/// 
/// Page specific logic should be placed in event handlers for the  
/// <see cref="NavigationHelper::LoadState"/>
/// and <see cref="NavigationHelper::SaveState"/>.
/// The navigation parameter is available in the LoadState method 
/// in addition to page state preserved during an earlier session.

void GamePage::OnNavigatedTo(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedTo(e);
}

void GamePage::OnNavigatedFrom(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedFrom(e);
}

#pragma endregion

void GamePage::ResizeWindow(int newWidth, int newHeight)
{
	if (newWidth < 768)
	{
		DefaultViewModel->Insert("LargePageElementVisible", Windows::UI::Xaml::Visibility::Collapsed);
		PromptPopupBorder->Padding = Thickness(25);
	}
	else
	{
		DefaultViewModel->Insert("LargePageElementVisible", Windows::UI::Xaml::Visibility::Visible);
		PromptPopupBorder->Padding = Thickness(100, 25, 100, 25);
	}
	
	if (newWidth < 640 && VirtualButtonBar->Items->Size > 3)
		DefaultViewModel->Insert("LargePageUndoMargin", Thickness(0, -70, 0, 0));
	else
		DefaultViewModel->Insert("LargePageUndoMargin", Thickness(0, 0, 0, 0));
	PromptPopupBorder->Width = newWidth;
	PromptPopup->VerticalOffset = (newHeight-200) / 2;
}

/// <summary>
/// Populates the page with content passed during navigation.  Any saved state is also
/// provided when recreating a page from a prior session.
/// </summary>
/// <param name="navigationParameter">The parameter value passed to
/// <see cref="Frame::Navigate(Type, Object)"/> when this page was initially requested.
/// </param>
/// <param name="pageState">A map of state preserved by this page during an earlier
/// session.  This will be null the first time a page is visited.</param>
void GamePage::LoadState(Object^ sender, Common::LoadStateEventArgs^ e)
{
	_puzzleName = safe_cast<String^>(e->NavigationParameter);
	fe = std::make_shared<WindowsModern>(WindowsModern());
	_hasGame = fe->CreateForGame(_puzzleName, DrawCanvas, this, this);

	currentPuzzle = fe->GetCurrentPuzzle();
	DefaultViewModel->Insert("PuzzleName", currentPuzzle->Name);
	DefaultViewModel->Insert("PageBackground", DrawCanvas->GetFirstColor());

	auto settings = ApplicationData::Current->RoamingSettings;
	if (!settings->Values->HasKey("cfg_show_appbar") || safe_cast<bool>(settings->Values->Lookup("cfg_show_appbar")))
	{
		BottomAppBar->IsSticky = true;
		BottomAppBar->IsOpen = true;
		MainGrid->Margin = Thickness(0, 0, 0, 90);
	}

	ResizeWindow(this->ActualWidth, this->ActualHeight);

	if (!fe->CanSolve())
		ButtonSolve->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

	PresetList ^presets = fe->GetPresetList(true, 2);

	PresetGridView->DataContext = presets->Items;

	_generatingGame = true;
	_isLoaded = false;

	if (e->PageState)
		BeginActivatePuzzle(nullptr);
}

void GamePage::BeginActivatePuzzle(PuzzleModern::GameLaunchParameters ^p)
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

void GamePage::BeginLoadGame(Windows::Storage::StorageFile ^file, bool makeNew)
{
	if (_isLoaded && _generatingGame)
		return;

	BusyLabel->Text = "Loading game";
	OnGenerationStart();

	create_task(fe->LoadGameFromFile(file)).then([=](Platform::String ^err)
	{
		Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([=]{
			_generatingGame = false;
			_isLoaded = true; 
			if (!err)
				OnGenerationEnd();
			else
			{
				(ref new Windows::UI::Popups::MessageDialog(App::AddPeriods(err), "Could not load game"))->ShowAsync();
				if (makeNew)
					BeginNewGame();
				else
					OnGenerationEnd();
			}
		}));
	});
}

void GamePage::BeginResumeGame()
{
	if (_isLoaded && _generatingGame)
		return;

	BusyLabel->Text = "Loading game";
	OnGenerationStart();
	BusyCancelButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

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
			if (loaded)
			{
				_isLoaded = true;
				OnGenerationEnd();
			}
			else
				BeginNewGame();
		}));
	});
}

void GamePage::BeginNewGame()
{
	if (_isLoaded && _generatingGame)
		return;

	BusyLabel->Text = "Creating puzzle";
	OnGenerationStart();
	if (fe->IsCustomGame())
		BusyCancelButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
	else
		BusyCancelButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

	auto workItemDelegate = [this](IAsyncAction^ workItem)
	{
		generatingWorkItem = workItem;
		fe->NewGame(workItem);
		generatingWorkItem = nullptr;

		bool success = workItem->Status != AsyncStatus::Canceled;

		Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([=]{
			_isLoaded = true; 
			if (success)
				OnGenerationEnd();
			else
				NavigationHelper->GoBack();
		}));
	};

	auto workItemHandler = ref new Windows::System::Threading::WorkItemHandler(workItemDelegate);
	auto workItem = Windows::System::Threading::ThreadPool::RunAsync(workItemHandler, Windows::System::Threading::WorkItemPriority::Normal);
}


void GamePage::UpdateUndoButtons()
{
	if (_generatingGame)
		return;

	ButtonUndo->IsEnabled = fe->CanUndo();
	ButtonRedo->IsEnabled = fe->CanRedo();

	int win = fe->GameWon();

	if (win == +1 && !_wonGame)
	{
		_wonGame = true;
		
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

			auto messageDialog = ref new MessageDialog("You have completed the puzzle!" "\n\n\n", "Congratulations");
			
			messageDialog->Commands->Append(ref new UICommand("New Game", nullptr, PropertyValue::CreateInt32(0)));
			messageDialog->Commands->Append(ref new UICommand("Menu", nullptr, PropertyValue::CreateInt32(1)));
			messageDialog->Commands->Append(ref new UICommand("Close", nullptr, PropertyValue::CreateInt32(2)));

			messageDialog->DefaultCommandIndex = 0;
			messageDialog->CancelCommandIndex = 2;

			create_task(messageDialog->ShowAsync()).then([this](IUICommand^ command)
			{
				if (!command->Id) return;

				int id = safe_cast<int>(command->Id);

				switch (id)
				{
				case 0:
					BeginNewGame();
					break;
				case 1:
					NavigationHelper->GoBack();
					break;
				}
			});

		});
		timer->Start();
	}
}

void GamePage::HighlightCurrentPreset()
{
	HighlightPreset(-2);
}

void GamePage::HighlightPreset(int index)
{
	if (index == -1)
		index = PresetGridView->Items->Size - 1;

	for (int i = 0; i < PresetGridView->Items->Size; i++)
	{
		auto containerObj = PresetGridView->ContainerFromIndex(i);
		if (!containerObj) continue;

		auto preset = safe_cast<Preset^>(PresetGridView->Items->GetAt(i));
		auto container = safe_cast<GridViewItem^>(containerObj);

		bool checked = i == index || (index == -2 && preset->Checked);
		container->Background = checked ? ref new SolidColorBrush(Windows::UI::Colors::Green) : nullptr;
	}
}

void GamePage::BusyOverlayDisappearingAnimation_Completed(Platform::Object^ sender, Platform::Object^ e)
{
	BusyOverlay->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

void GamePage::DoubleAnimation_Completed(Platform::Object^ sender, Platform::Object^ e)
{
	_finishedOverlayAnimation = true;
}

void GamePage::OnGenerationStart()
{
	_generatingGame = true;
	BusyOverlay->Visibility = Windows::UI::Xaml::Visibility::Visible;
	BusyLabel->Visibility = Windows::UI::Xaml::Visibility::Visible;
	BusyIndicator->IsActive = true;
	_finishedOverlayAnimation = false;
	BusyOverlayDisappearingStoryboard->Stop();
	BusyOverlayAppearingStoryboard->Begin();
}

void GamePage::OnGenerationEnd()
{
	_controls = fe->GetVirtualButtonBar();

	VirtualButtonBar->DataContext = _controls;

	if (_controls->ToggleButton)
	{
		LabelLeftRight->Text = _controls->ToggleButton->Name;
		ButtonLeftRight->Icon = _controls->ToggleButton->Icon;
		LeftRightGrid->Visibility = Windows::UI::Xaml::Visibility::Visible;
	}
	else
		LeftRightGrid->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

	bool isSwitched = ButtonLeftRight->IsChecked->Value;
	if (_controls->SwitchMiddle)
	{
		_leftAction = isSwitched ? _controls->MiddleAction : _controls->LeftAction;
		_middleAction = isSwitched ? _controls->LeftAction : _controls->MiddleAction;
		_rightAction = _controls->RightAction;
	}
	else
	{
		_leftAction = isSwitched ? _controls->RightAction : _controls->LeftAction;
		_rightAction = isSwitched ? _controls->LeftAction : _controls->RightAction;
		_middleAction = _controls->MiddleAction;
	}
	_touchAction = isSwitched ? _controls->HoldAction : _controls->TouchAction;
	_holdAction = isSwitched ? _controls->TouchAction : _controls->HoldAction;

	_wonGame = fe->GameWon() == 1;
	UpdateUndoButtons();
	fe->GetPresetList(true, 2);
	HighlightCurrentPreset();
	ResizeWindow(this->ActualWidth, this->ActualHeight);
	ResizeGame();

	_generatingGame = false;
	if (!_finishedOverlayAnimation)
	{
		BusyOverlayAppearingStoryboard->Stop();
		BusyOverlay->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	}
	else
	{
		BusyOverlayDisappearingStoryboard->Begin();
	}
	BusyIndicator->IsActive = false;
	BusyLabel->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	BusyCancelButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

	if (_controls->ColorBlindKey != VirtualKey::None)
	{
		auto settings = ApplicationData::Current->RoamingSettings;
		if (settings->Values->HasKey("cfg_colorblind") && safe_cast<bool>(settings->Values->Lookup("cfg_colorblind")))
			fe->SendKey(_controls->ColorBlindKey, false, false);
	}

	_undoHotkey = _redoHotkey = true;
	for each (auto b in _controls->Buttons)
	{
		if (b->Key == VirtualKey::U)
			_undoHotkey = false;
		if (b->Key == VirtualKey::R)
			_redoHotkey = false;
	}
}

void GamePage::SpecificGameID_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	_isFlyoutOpen = true;
	BottomAppBar->IsOpen = false;
	TopAppBar->IsOpen = false;

	PromptPopupText->Text = fe->GetGameDesc();
	PromptPopupHeader->Text = "Game ID";
	PromptPopup->IsOpen = true;
	PromptPopupBackground->Visibility = Windows::UI::Xaml::Visibility::Visible;
}

void GamePage::SpecificRandomSeed_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	_isFlyoutOpen = true;
	BottomAppBar->IsOpen = false;
	TopAppBar->IsOpen = false;
	
	PromptPopupText->Text = fe->GetRandomSeed();
	PromptPopupHeader->Text = "Random seed";
	PromptPopup->IsOpen = true;
	PromptPopupBackground->Visibility = Windows::UI::Xaml::Visibility::Visible;
}

void GamePage::PromptPopup_Closed(Platform::Object^ sender, Platform::Object^ e)
{
	_isFlyoutOpen = false;
	PromptPopupBackground->Visibility = Windows::UI::Xaml::Visibility::Collapsed; 
	PromptPopupErrorLabel->Text = "";
	BottomAppBar->IsOpen = BottomAppBar->IsSticky;
}


void GamePage::ButtonNew_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;
	fe->Redraw(DrawCanvas, this, this, true);
	BeginNewGame();
	BottomAppBar->IsOpen = BottomAppBar->IsSticky;
	TopAppBar->IsOpen = false;
}

void GamePage::ButtonRestart_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;
	BottomAppBar->IsOpen = BottomAppBar->IsSticky;
	TopAppBar->IsOpen = false;

	fe->RestartGame();
	UpdateUndoButtons();
}


void GamePage::ButtonSolve_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	BottomAppBar->IsOpen = BottomAppBar->IsSticky;
	TopAppBar->IsOpen = false;

	String ^msg = fe->Solve();
	if (msg)
		(ref new Windows::UI::Popups::MessageDialog(App::AddPeriods(msg), "Cannot solve"))->ShowAsync();
	else
		_wonGame = true;
	UpdateUndoButtons();
}

void GamePage::OpenHelpFlyout(bool independent)
{
	auto flyout = ref new HelpFlyout(currentPuzzle->HelpName);
	if (independent)
		flyout->ShowIndependent();
	else
		flyout->Show();
}

void GamePage::ButtonHelp_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	BottomAppBar->IsOpen = BottomAppBar->IsSticky;
	TopAppBar->IsOpen = false;

	OpenHelpFlyout(true);
}

void GamePage::ButtonUndo_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	fe->Undo();
	UpdateUndoButtons();
}


void GamePage::ButtonRedo_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	fe->Redo();
	UpdateUndoButtons();
}

void GamePage::OpenParamsFlyout(bool independent)
{
	if (_generatingGame)
		return;

	BottomAppBar->IsOpen = BottomAppBar->IsSticky;
	auto cfgs = fe->GetConfiguration();

	auto flyout = ref new ParamsFlyout(cfgs);

	flyout->NewConfiguration += ref new PuzzleModern::NewConfigurationEventHandler(this, &PuzzleModern::GamePage::OnNewConfiguration);
	flyout->Unloaded += ref new Windows::UI::Xaml::RoutedEventHandler(this, &PuzzleModern::GamePage::OnParamsFlyoutUnloaded);

	if (independent)
		flyout->ShowIndependent();
	else
		flyout->Show();
	_isFlyoutOpen = true;
	HighlightPreset(-1);
}

void GamePage::PresetGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e)
{
	if (_generatingGame || _isFlyoutOpen)
		return;

	// This event also fires when the selection was not changed by the user.
	if (!BottomAppBar->IsOpen && !TopAppBar->IsOpen)
		return;
	
	Preset^ p = safe_cast<Preset^>(e->ClickedItem);

	if (!p) return;

	if (p->Index == -1)
	{
		OpenParamsFlyout(true);
	}
	else if (p->SubMenu)
	{
		auto menu = ref new MenuFlyout();
		for each(auto sub in p->SubMenu->Items)
		{
			auto item = ref new MenuFlyoutItem();
			
			item->FontFamily = ref new Windows::UI::Xaml::Media::FontFamily("Segoe UI Symbol");
			wchar_t mark = sub->Checked ? 0xe0a3 : 0xe070;
			item->Text = ref new Platform::String(&mark, 1) + "         " + sub->Name;

			item->Tag = sub->Index;
			item->Click += ref new Windows::UI::Xaml::RoutedEventHandler(this, &PuzzleModern::GamePage::PresetMenuFlyout_Click);
			menu->Items->Append(item);
		}
		menu->Placement = FlyoutPlacementMode::Bottom;

		auto container = safe_cast<GridViewItem^>(PresetGridView->ContainerFromItem(p));
		menu->Closed += ref new Windows::Foundation::EventHandler<Platform::Object ^>(this, &PuzzleModern::GamePage::PresetMenuFlyout_OnClosed);
		menu->ShowAt(container);
		_isFlyoutOpen = true;
		HighlightPreset(PresetGridView->IndexFromContainer(container));
	}
	else
	{
		fe->SetPreset(p->Index);
		BeginNewGame();
	}
}

void GamePage::PresetMenuFlyout_Click(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^e)
{
	if (_generatingGame)
		return;

	auto item = safe_cast<MenuFlyoutItem^>(sender);
	auto box = safe_cast<Platform::IBox<int>^>(item->Tag);

	fe->SetPreset(box->Value);
	BeginNewGame();
}

void GamePage::PresetMenuFlyout_OnClosed(Platform::Object ^sender, Platform::Object ^args)
{
	_isFlyoutOpen = false;
}

void GamePage::ResizeGame()
{
	if (!fe || !Window::Current->CoreWindow->Visible)
		return;

	int iw = (int)GameBorder->ActualWidth;
	int ih = (int)GameBorder->ActualHeight;

	int ow = 0;
	int oh = 0;

	float scale = Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi / 96;

	fe->GetMaxSize(ceil(iw*scale), ceil(ih*scale), &ow, &oh);

	DrawCanvas->Width = ow / scale;
	DrawCanvas->Height = oh / scale;

	if (ow > 20 && oh > 20)
		fe->Redraw(DrawCanvas, this, this, true);

	fe->SetInputScale(scale);
}

void GamePage::StartTimer()
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
void GamePage::EndTimer()
{
	if (PeriodicTimer != nullptr)
	{
		PeriodicTimer->Cancel();
		PeriodicTimer = nullptr;
	}
}

void GamePage::Timer(float seconds)
{
	if (_generatingGame)
		return;
	
	fe->UpdateTimer(DrawCanvas, this, this, seconds);
}

void GamePage::UpdateStatusBar(Platform::String ^status)
{
	StatusBar->Text = status;
}

void GamePage::ForceRedraw()
{
	if (_generatingGame)
		return;
	
	fe->Redraw(DrawCanvas, this, this, true);
}

void GamePage::GameBorder_SizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
{
	if (_generatingGame)
		return;
	ResizeGame();
}

void GamePage::pageRoot_SizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
{
	ResizeWindow(e->NewSize.Width, e->NewSize.Height);
}


void GamePage::OnLoaded(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^e)
{
	_acceleratorKeyEventToken = Window::Current->CoreWindow->Dispatcher->AcceleratorKeyActivated += 
		ref new Windows::Foundation::TypedEventHandler
		<Windows::UI::Core::CoreDispatcher ^, Windows::UI::Core::AcceleratorKeyEventArgs ^>
		(this, &GamePage::OnAcceleratorKeyActivated);

	_commandsRequestedEventRegistrationToken = SettingsPane::GetForCurrentView()->CommandsRequested += 
		ref new TypedEventHandler<SettingsPane^, SettingsPaneCommandsRequestedEventArgs^>(this, &GamePage::OnCommandsRequested);

	_visibilityChangedEventToken = Window::Current->CoreWindow->VisibilityChanged +=
		ref new Windows::Foundation::TypedEventHandler
		<Windows::UI::Core::CoreWindow ^, Windows::UI::Core::VisibilityChangedEventArgs ^>
		(this, &GamePage::OnVisibilityChanged);

	_shareEventToken = DataTransferManager::GetForCurrentView()->DataRequested += 
		ref new Windows::Foundation::TypedEventHandler<DataTransferManager ^, DataRequestedEventArgs ^>
		(this, &GamePage::OnDataRequested);

	// Printing is disabled until I figure out a way to make sure only one PrintTaskRequested
	// event handler is active at a time. It also needs a function for generating puzzles in bulk,
	// while allowing the print handlers to access any random page at a time (as opposed to the sequential
	// printing currently present in the main version)
#ifdef WIN10_PRINTING
	if (fe->CanPrint())
	{
		_printEventToken = PrintManager::GetForCurrentView()->PrintTaskRequested +=
			ref new Windows::Foundation::TypedEventHandler<PrintManager ^, PrintTaskRequestedEventArgs ^>
			(this, &GamePage::OnPrintTaskRequested);

		_printDoc = ref new PrintDocument();
		_printDoc->AddPages += ref new AddPagesEventHandler(this, &GamePage::OnAddPages);
		_printDoc->Paginate += ref new PaginateEventHandler(this, &GamePage::OnPaginate);
		_printDoc->GetPreviewPage += ref new GetPreviewPageEventHandler(this, &GamePage::OnGetPreviewPage);
		_printSource = _printDoc->DocumentSource;
	}
#endif
}

void GamePage::OnParamsFlyoutUnloaded(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^e)
{
	if (!_generatingGame)
		HighlightCurrentPreset();
	_isFlyoutOpen = false;
}

void GamePage::OnUnloaded(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^e)
{
	Window::Current->CoreWindow->Dispatcher->AcceleratorKeyActivated -= _acceleratorKeyEventToken;
	SettingsPane::GetForCurrentView()->CommandsRequested -= _commandsRequestedEventRegistrationToken;
	Window::Current->CoreWindow->VisibilityChanged -= _visibilityChangedEventToken;
	DataTransferManager::GetForCurrentView()->DataRequested -= _shareEventToken;
#ifdef WIN10_PRINTING
	PrintManager::GetForCurrentView()->PrintTaskRequested -= _printEventToken;
#endif
	EndTimer();
	DrawCanvas->NeedsRedraw -= _forceRedrawEventToken;
	Application::Current->Suspending -= _suspendingEventToken;
	Application::Current->Resuming -= _resumingEventToken;
	App::Current->SettingChanged -= _settingChangedEventToken;
}

void GamePage::OnSettingsCommand(IUICommand^ command)
{
	auto id = command->Id->ToString();

	if (id == "customparams")
		OpenParamsFlyout(false);
	if (id == "puzzlehelp")
		OpenHelpFlyout(false);
}

void GamePage::OnCommandsRequested(SettingsPane^ settingsPane, SettingsPaneCommandsRequestedEventArgs^ e)
{
	auto handler = ref new Windows::UI::Popups::UICommandInvokedHandler(this, &GamePage::OnSettingsCommand);

	SettingsCommand^ helpCommand = ref new SettingsCommand("puzzlehelp", "Help - " + DefaultViewModel->Lookup("PuzzleName"), handler);
	SettingsCommand^ paramsCommand = ref new SettingsCommand("customparams", DefaultViewModel->Lookup("PuzzleName") + " parameters", handler);
	e->Request->ApplicationCommands->Append(helpCommand);
	e->Request->ApplicationCommands->Append(paramsCommand);
}

void GamePage::OnAcceleratorKeyActivated(Windows::UI::Core::CoreDispatcher ^sender, Windows::UI::Core::AcceleratorKeyEventArgs ^e)
{
	auto k = e->VirtualKey;
	
	if (k == VirtualKey::Shift)
	{
		_shiftPressed = e->EventType == Windows::UI::Core::CoreAcceleratorKeyEventType::KeyDown;
		e->Handled = true;
	}
	if (k == VirtualKey::Control)
	{
		_ctrlPressed = e->EventType == Windows::UI::Core::CoreAcceleratorKeyEventType::KeyDown;
		e->Handled = true;
	}
	if (k == VirtualKey::Escape && e->EventType == Windows::UI::Core::CoreAcceleratorKeyEventType::KeyDown)
	{
		if (PromptPopup->IsOpen)
			PromptPopup->IsOpen = false;
		else if (_isFlyoutOpen)
			return;
		else if (TopAppBar->IsOpen)
		{
			BottomAppBar->IsOpen = BottomAppBar->IsSticky;
			TopAppBar->IsOpen = false;
		}
		else
			NavigationHelper->GoBack();
		
		e->Handled = true;
	}

	if (_generatingGame || _isFlyoutOpen)
		return;
	
	if (k == VirtualKey::Home)
		k = VirtualKey::NumberPad7;
	else if (k == VirtualKey::PageUp)
		k = VirtualKey::NumberPad9;
	else if (k == VirtualKey::End)
		k = VirtualKey::NumberPad1;
	else if (k == VirtualKey::PageDown)
		k = VirtualKey::NumberPad3;
	else if (k == VirtualKey::Delete)
		k = VirtualKey::Back;

	if (e->EventType == Windows::UI::Core::CoreAcceleratorKeyEventType::KeyDown)
	{
		if ((_ctrlPressed && k == VirtualKey::Z) || (_undoHotkey && k == VirtualKey::U))
		{
			fe->Undo();
			e->Handled = true;
			UpdateUndoButtons();
		}
		if ((_ctrlPressed && k == VirtualKey::Y) || (_redoHotkey && k == VirtualKey::R))
		{
			fe->Redo();
			e->Handled = true;
			UpdateUndoButtons();
		}
		if (_ctrlPressed && k == VirtualKey::N)
		{
			BeginNewGame();
			e->Handled = true;
		}
	}
	if (!e->Handled && ( (k >= VirtualKey::Number0 && k <= VirtualKey::Number9) /* Number keys */
		|| (k >= VirtualKey::NumberPad0 && k <= VirtualKey::NumberPad9) /* Number pad */
		|| k == VirtualKey::Enter || k == VirtualKey::Space /* Button one and two */
		|| (k >= VirtualKey::Left && k <= VirtualKey::Down) /* Directional keys */
		|| (k >= VirtualKey::A && k <= VirtualKey::Z) /* Letters */
		|| k == VirtualKey::Delete || k == VirtualKey::Back /* Delete */
		|| k == VirtualKey::Escape
		) )
	{
		/* Only send key down events. */
		if (e->EventType == Windows::UI::Core::CoreAcceleratorKeyEventType::KeyDown &&
			(!_controls || k != _controls->ColorBlindKey))
			fe->SendKey(k, _shiftPressed, _ctrlPressed);
		e->Handled = true;
		UpdateUndoButtons();
	}
}


void GamePage::DrawCanvas_PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	if (_generatingGame)
		return;
	
	auto ptrPt = e->GetCurrentPoint(DrawCanvas);
	int x = ptrPt->Position.X, y = ptrPt->Position.Y;
	/* 
	 * When using touch, we use gestures to differentiate between left and right clicks.
	 * Tapping and dragging count as left button, while holding counts as right button.
	 * If this is a right button, we can not yet process the Pressed event.
	 */
	if (e->Pointer->PointerDeviceType != Windows::Devices::Input::PointerDeviceType::Touch || fe->IsRightButtonDisabled())
	{
		if (ptrPt->Properties->IsLeftButtonPressed)
		{
			fe->SendClick(x, y, _leftAction, ButtonState::DOWN);
			_leftPressed = true;
			e->Handled = true;
		}
		if (ptrPt->Properties->IsMiddleButtonPressed)
		{
			fe->SendClick(x, y, _middleAction, ButtonState::DOWN);
			_middlePressed = true;
			e->Handled = true;
		}
		if (ptrPt->Properties->IsRightButtonPressed)
		{
			fe->SendClick(x, y, _rightAction, ButtonState::DOWN);
			_rightPressed = true;
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
				fe->SendClick(x, y, _holdAction, ButtonState::DOWN);
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

void GamePage::pageRoot_PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	auto ptrPt = e->GetCurrentPoint(nullptr);
	if (ptrPt->Properties->IsXButton1Pressed)
	{
		e->Handled = true;
		fe->Undo();
	}
	else if (ptrPt->Properties->IsXButton2Pressed)
	{
		e->Handled = true;
		fe->Redo();
	}
}

void GamePage::pageRoot_PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	auto ptrPt = e->GetCurrentPoint(DrawCanvas);
	int x = ptrPt->Position.X, y = ptrPt->Position.Y;

	if (_leftPressed)
	{
		fe->SendClick(x, y, _leftAction, ButtonState::UP);
		e->Handled = true;
	}
	if (_middlePressed)
	{
		fe->SendClick(x, y, _middleAction, ButtonState::UP);
		e->Handled = true;
	}
	if (_rightPressed)
	{
		fe->SendClick(x, y, _rightAction, ButtonState::UP);
		e->Handled = true;
	}
	if (_touchPressed)
	{
		fe->SendClick(x, y, _touchAction, ButtonState::UP);
		e->Handled = true;
	}
	if (_holdPressed)
	{
		fe->SendClick(x, y, _holdAction, ButtonState::UP);
		e->Handled = true;
	}
	if (_initialPressed)
	{
		fe->SendClick(_initialPoint.X, _initialPoint.Y, _touchAction, ButtonState::DOWN);
		fe->SendClick(x, y, _touchAction, ButtonState::UP);
		e->Handled = true;
	}

	_leftPressed = _middlePressed = _rightPressed = false;
	_touchPressed = _holdPressed = _initialPressed = false;

	if (RightClickTimer)
	{
		RightClickTimer->Cancel();
		RightClickTimer = nullptr;
	}
	UpdateUndoButtons();
}

#define LEFT_DISTANCE 10

void GamePage::pageRoot_PointerMoved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
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
	if (_leftPressed)
	{
		fe->SendClick(x, y, _leftAction, ButtonState::DRAG);
		e->Handled = true;
	}
	if (_middlePressed)
	{
		fe->SendClick(x, y, _middleAction, ButtonState::DRAG);
		e->Handled = true;
	}
	if (_rightPressed)
	{
		fe->SendClick(x, y, _rightAction, ButtonState::DRAG);
		e->Handled = true;
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

void GamePage::DrawCanvas_RightTapped(Platform::Object^ sender, Windows::UI::Xaml::Input::RightTappedRoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	/* 
	 * This is used to suppress the appbar appearing when right-clicking inside the game canvas.
	 * Actual right taps are processed by the PointerPressed and PointerReleased events.
	 */
	e->Handled = true;
}


void GamePage::ButtonBar_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	auto button = safe_cast<FrameworkElement^>(sender);
	auto p = safe_cast<VirtualButton^>(button->DataContext);
	
	fe->SendKey(p->Key, false, false);

	UpdateUndoButtons();
}


void GamePage::OnNewConfiguration(ParamsFlyout ^sender, Windows::Foundation::Collections::IVector<ConfigItem ^> ^newConfig)
{
	auto error = fe->SetConfiguration(newConfig);
	if (error)
	{
		sender->ShowErrorMessage(error);
	}
	else
	{
		BeginNewGame();
		sender->Hide();
		BottomAppBar->IsOpen = BottomAppBar->IsSticky;
		TopAppBar->IsOpen = false;
		_isFlyoutOpen = false;
	}
}

void GamePage::SaveState(Platform::Object ^sender, PuzzleModern::Common::SaveStateEventArgs ^e)
{
	OutputDebugString(L"Saving state...\n");
	if (!_generatingGame)
		fe->SaveGameToStorage(_puzzleName);
	else if (generatingWorkItem)
		generatingWorkItem->Cancel();
}


void GamePage::OnVisibilityChanged(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::VisibilityChangedEventArgs ^args)
{
	if (_generatingGame || !_isLoaded)
		return;

	if (!args->Visible)
		fe->SaveGameToStorage(_puzzleName);
	else
		ForceRedraw();
}


void GamePage::PromptPopupButtonOK_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	// The Random Seed and Game Description dialogs use the same handling
	auto error = fe->SetGameID(PromptPopupText->Text);
	
	if (error)
	{
		PromptPopupErrorLabel->Text = error;
		ErrorAppearingStoryboard->Begin();
	}
	else
	{
		PromptPopup->IsOpen = false;
		HighlightCurrentPreset();
		BeginNewGame();
	}
}


void GamePage::PromptPopupButtonCancel_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	PromptPopup->IsOpen = false;
}


void GamePage::ButtonMenu_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	BottomAppBar->IsOpen = true;
	TopAppBar->IsOpen = true;
}


void GamePage::PresetGridView_LayoutUpdated(Platform::Object^ sender, Platform::Object^ e)
{
	if (_generatingGame)
		return;

	if(fe && !_isFlyoutOpen)
		HighlightCurrentPreset();
}


void GamePage::OnSuspending(Platform::Object ^sender, Windows::ApplicationModel::SuspendingEventArgs ^e)
{
	EndTimer();
}


void GamePage::OnResuming(Platform::Object ^sender, Platform::Object ^args)
{
	StartTimer();
}


void GamePage::SpecificSaveGame_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	FileSavePicker^ savePicker = ref new FileSavePicker();
	savePicker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;

	auto extensions = ref new Platform::Collections::Vector<String^>();
	extensions->Append(".puzzle");
	extensions->Append(".sav");
	savePicker->FileTypeChoices->Insert("Saved game", extensions);
	savePicker->CommitButtonText = "Save game";
	savePicker->SuggestedFileName = _puzzleName;

	create_task(savePicker->PickSaveFileAsync()).then([this](StorageFile^ file)
	{
		if (file == nullptr)
			return;
		create_task(fe->SaveGameToFile(file)).then([this, file](bool success)
		{
			if (!success)
				(ref new Windows::UI::Popups::MessageDialog("The game could not be saved."))->ShowAsync();
		});
	});
}


void GamePage::SpecificLoadGame_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_generatingGame)
		return;

	FileOpenPicker^ openPicker = ref new FileOpenPicker();
	openPicker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;
	openPicker->FileTypeFilter->Append(".puzzle");
	openPicker->CommitButtonText = "Load game";
	
	create_task(openPicker->PickSingleFileAsync()).then([this](StorageFile^ file)
	{
		if (file == nullptr)
			return;

		BusyLabel->Text = "Loading game";
		OnGenerationStart();
		create_task(fe->LoadGameFromFile(file)).then([this, file](Platform::String ^msg)
		{
			if (!msg)
				_finishedOverlayAnimation = true;
			OnGenerationEnd();
			if (msg)
				(ref new Windows::UI::Popups::MessageDialog(App::AddPeriods(msg), "Could not load game"))->ShowAsync();
		});
	});
}


void GamePage::SpecificShareGame_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	DataTransferManager::ShowShareUI();
}


void GamePage::OnDataRequested(DataTransferManager ^sender, DataRequestedEventArgs ^args)
{
	if (_generatingGame)
	{
		args->Request->FailWithDisplayText("The app is currently processing a puzzle.\n\nPlease try again when the operation has finished.");
		return;
	}

	auto data = args->Request->Data;
	data->Properties->Title = "Saved Game for " + _puzzleName;
	data->Properties->Description = fe->CanUndo() ? "Share your progress in " + _puzzleName : "Share this " + _puzzleName + " puzzle";
	data->Properties->FileTypes->Clear();
	data->Properties->FileTypes->Append(".puzzle");
	data->SetDataProvider(StandardDataFormats::StorageItems, ref new DataProviderHandler(this, &GamePage::OnDeferredDataRequestedHandler));
}

void GamePage::OnDeferredDataRequestedHandler(DataProviderRequest^ request)
{
	auto deferral = request->GetDeferral();

	create_task([this](){
		return ApplicationData::Current->TemporaryFolder->CreateFileAsync(_puzzleName + ".puzzle", CreationCollisionOption::ReplaceExisting);
	}).then([this, request, deferral](StorageFile ^opened){

		create_task([=](){
			return fe->SaveGameToFile(opened);
		}).then([=](bool result){
			auto files = ref new Vector<StorageFile^>();
			files->Append(opened);
			request->SetData(files);
			
			deferral->Complete();
		});
	});
}

#ifdef WIN10_PRINTING
void GamePage::OnPrintTaskRequested(PrintManager ^sender, PrintTaskRequestedEventArgs ^e)
{
	if (_generatingGame)
		return;

	PrintTask ^task = e->Request->CreatePrintTask(_puzzleName + " puzzle", ref new PrintTaskSourceRequestedHandler(
		[=](PrintTaskSourceRequestedArgs^ args)
	{
		args->SetSource(_printSource);
	}));

	auto printDetailedOptions = PrintTaskOptionDetails::GetFromPrintTaskOptions(task->Options);
	printDetailedOptions->OptionChanged += ref new Windows::Foundation::TypedEventHandler<PrintTaskOptionDetails ^, PrintTaskOptionChangedEventArgs ^>(this, &GamePage::OnPrintTaskOptionChanged);

	auto options = task->Options->DisplayedOptions;
	options->Clear();
		
	auto solutionsOption = printDetailedOptions->CreateItemListOption(L"PuzzleSolution", L"Puzzle solution");
	solutionsOption->AddItem(L"solOff", "Don't include solution");
	solutionsOption->AddItem(L"solOn", "Include solution");

	auto scaleOption = printDetailedOptions->CreateItemListOption(L"PuzzleScale", L"Scale");
	scaleOption->AddItem(L"scale0.5", "50%");
	scaleOption->AddItem(L"scale0.6", "60%");
	scaleOption->AddItem(L"scale0.7", "70%");
	scaleOption->AddItem(L"scale0.8", "80%");
	scaleOption->AddItem(L"scale0.9", "90%");
	scaleOption->AddItem(L"scale1", "100%");
	scaleOption->TrySetValue(L"scale1");

	options->Append(StandardPrintTaskOptions::Copies);
	options->Append(StandardPrintTaskOptions::Orientation);
	options->Append(StandardPrintTaskOptions::MediaSize);
	options->Append(L"PuzzleScale");
	options->Append(L"PuzzleSolution");
}

void GamePage::OnPaginate(Platform::Object ^sender, PaginateEventArgs ^e)
{
	auto printDoc = safe_cast<PrintDocument^>(sender);
	_printOptions = e->PrintTaskOptions;

	auto printDetailedOptions = PrintTaskOptionDetails::GetFromPrintTaskOptions(_printOptions);
	bool includeSolutions = printDetailedOptions->Options->Lookup(L"PuzzleSolution")->Value->ToString() == L"solOn";

	if (includeSolutions)
		printDoc->SetPreviewPageCount(2, PreviewPageCountType::Intermediate);
	else
		printDoc->SetPreviewPageCount(1, PreviewPageCountType::Intermediate);
}

void GamePage::OnAddPages(Platform::Object ^sender, AddPagesEventArgs ^e)
{
	auto printDoc = safe_cast<PrintDocument^>(sender);

	auto page = CreatePrintPage(false);
	printDoc->AddPage(page);

	auto printDetailedOptions = PrintTaskOptionDetails::GetFromPrintTaskOptions(_printOptions);
	bool includeSolutions = printDetailedOptions->Options->Lookup(L"PuzzleSolution")->Value->ToString() == L"solOn";

	if (includeSolutions)
	{
		page = CreatePrintPage(true);
		printDoc->AddPage(page);
	}

	printDoc->AddPagesComplete();
}

UIElement ^GamePage::CreatePrintPage(bool isSolution)
{
	ShapePuzzleCanvas ^canvas = ref new ShapePuzzleCanvas();

	auto printDetailedOptions = PrintTaskOptionDetails::GetFromPrintTaskOptions(_printOptions);
	auto strScale = printDetailedOptions->Options->Lookup(L"PuzzleScale")->Value->ToString();

	auto size = fe->PrintGame(canvas, isSolution);

	auto desc = _printOptions->GetPageDescription(0);
	auto targetRect = desc.ImageableRect;
	double maxScale = (std::min)(targetRect.Height / size.Height, targetRect.Width / size.Width);
	double scale = maxScale;

	if (strScale == L"scale0.9")
		scale *= 0.9;
	else if (strScale == L"scale0.8")
		scale *= 0.8;
	else if (strScale == L"scale0.7")
		scale *= 0.7;
	else if (strScale == L"scale0.6")
		scale *= 0.6;
	else if (strScale == L"scale0.5")
		scale *= 0.5;

	canvas->Margin = Thickness(targetRect.Left, targetRect.Top, 0, 0);

	auto transform = ref new ScaleTransform();
	transform->ScaleX = scale;
	transform->ScaleY = scale;
	canvas->RenderTransform = transform;

	return canvas;
}

void GamePage::OnGetPreviewPage(Platform::Object ^sender, GetPreviewPageEventArgs ^e)
{
	auto printDoc = safe_cast<PrintDocument^>(sender);

	auto page = CreatePrintPage(e->PageNumber == 2);

	printDoc->SetPreviewPage(e->PageNumber, page);
}

void GamePage::OnPrintTaskOptionChanged(PrintTaskOptionDetails ^sender, PrintTaskOptionChangedEventArgs ^args)
{
	bool invalidate = false;
	if (args->OptionId->ToString() == L"PuzzleSolution")
		invalidate = true;
	if (args->OptionId->ToString() == L"PuzzleScale")
		invalidate = true;

	if (invalidate)
	{
		Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, 
			ref new Windows::UI::Core::DispatchedHandler([this]()
		{
			_printDoc->InvalidatePreview();
		}));
	}
}
#endif

void GamePage::BusyCancelButton_Click(Platform::Object^ sender, RoutedEventArgs^ e)
{
	if (generatingWorkItem)
	{
		generatingWorkItem->Cancel();
		BusyCancelButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		BusyLabel->Text = "Canceling";
	}
}


void GamePage::OnSettingChanged(Platform::Object ^sender, Platform::String ^key, Platform::Object ^value)
{
	if (_controls && key == "cfg_colorblind" && _controls->ColorBlindKey != VirtualKey::None && !_generatingGame)
		fe->SendKey(_controls->ColorBlindKey, false, false);
	if (key=="cfg_show_appbar")
	{
		bool show = safe_cast<bool>(value);
		BottomAppBar->IsSticky = show;
		BottomAppBar->IsOpen = show;
		MainGrid->Margin = Thickness(0, 0, 0, show ? 90 : 0);
	}
	if (key == "env_MAP_VIVID_COLOURS" && _puzzleName == "Map" && _isLoaded)
	{
		DrawCanvas->RemoveColors();
		fe->ReloadColors();
		ForceRedraw();
	}
	if (key == "env_FIXED_PENCIL_MARKS" && _isLoaded)
		ForceRedraw();
}


void GamePage::ButtonLeftRight_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (_controls->SwitchMiddle)
	{
		_leftAction = _controls->MiddleAction;
		_middleAction = _controls->LeftAction;
	}
	else
	{
		_leftAction = _controls->RightAction;
		_rightAction = _controls->LeftAction;
	}
	_touchAction = _controls->HoldAction;
	_holdAction = _controls->TouchAction;

	if (!_generatingGame)
		fe->SendKey(VirtualKey::XButton1, false, false);
}


void GamePage::ButtonLeftRight_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	_leftAction = _controls->LeftAction;
	_middleAction = _controls->MiddleAction;
	_rightAction = _controls->RightAction;
	_touchAction = _controls->TouchAction;
	_holdAction = _controls->HoldAction;

	if (!_generatingGame)
		fe->SendKey(VirtualKey::XButton2, false, false);
}


void GamePage::LabelLeftRight_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
	ButtonLeftRight->IsChecked = (_leftAction == _controls->LeftAction);
}


void GamePage::ButtonSettings_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	SettingsPane::Show();
}


void GamePage::BottomAppBar_Closed(Platform::Object^ sender, Platform::Object^ e)
{
	if (BottomAppBar->IsSticky && !_isFlyoutOpen)
		BottomAppBar->IsOpen = true;
}
