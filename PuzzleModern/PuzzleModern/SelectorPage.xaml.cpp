﻿//
// ItemsPage1.xaml.cpp
// Implementation of the SelectorPage class
//

#include "pch.h"
#include "SelectorPage.xaml.h"
#include "GamePage.xaml.h"
#include "..\..\PuzzleCommon\WindowsModern.h"
#include "..\..\PuzzleCommon\PuzzleData.h"
#include "HelpFlyout.xaml.h"

using namespace PuzzleModern;

using namespace Platform;
using namespace Platform::Collections;
using namespace concurrency;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::UI::Popups;
using namespace Windows::UI::StartScreen;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Items Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234233

static const Windows::UI::Xaml::Visibility Visible = Windows::UI::Xaml::Visibility::Visible;
static const Windows::UI::Xaml::Visibility Collapsed = Windows::UI::Xaml::Visibility::Collapsed;

SelectorPage::SelectorPage()
{
	InitializeComponent();
	SetValue(_defaultViewModelProperty, ref new Map<String^, Object^>(std::less<String^>()));
	auto navigationHelper = ref new Common::NavigationHelper(this);
	SetValue(_navigationHelperProperty, navigationHelper);
	navigationHelper->LoadState += ref new Common::LoadStateEventHandler(this, &SelectorPage::LoadState);

	this->Loaded += ref new Windows::UI::Xaml::RoutedEventHandler(this, &SelectorPage::OnLoaded);
	this->Unloaded += ref new Windows::UI::Xaml::RoutedEventHandler(this, &SelectorPage::OnUnloaded);

	_puzzles = WindowsModern::GetPuzzleList();
	DefaultViewModel->Insert("Items", _puzzles->Items);
	DefaultViewModel->Insert("Favourites", _puzzles->Favourites);

	DefaultViewModel->Insert("PinVisible", Collapsed);
	DefaultViewModel->Insert("UnpinVisible", Collapsed);
	DefaultViewModel->Insert("FavVisible", Collapsed);
	DefaultViewModel->Insert("UnfavVisible", Collapsed);

	auto roamingSettings = ApplicationData::Current->RoamingSettings->Values;
	bool zoomedOut = roamingSettings->HasKey("feature_zoom_out") && safe_cast<bool>(roamingSettings->Lookup("feature_zoom_out"));
	DefaultViewModel->Insert("ZoomInVisible", zoomedOut ? Visible : Collapsed);
	DefaultViewModel->Insert("ZoomOutVisible", !zoomedOut ? Visible : Collapsed);

	auto localSettings = ApplicationData::Current->LocalSettings->Values;
	bool hideFavouritesFooter = localSettings->HasKey("feature_favourites") && safe_cast<bool>(localSettings->Lookup("feature_favourites"));
	DefaultViewModel->Insert("FavFooterVisible", hideFavouritesFooter ? Collapsed : Visible);
}

void SelectorPage::OnLoaded(Platform::Object ^sender, RoutedEventArgs ^e)
{
	_shareEventToken = Windows::ApplicationModel::DataTransfer::DataTransferManager::GetForCurrentView()->DataRequested += 
		ref new Windows::Foundation::TypedEventHandler<Windows::ApplicationModel::DataTransfer::DataTransferManager ^, 
		Windows::ApplicationModel::DataTransfer::DataRequestedEventArgs ^>
		(this, &SelectorPage::OnDataRequested);

	_commandsRequestedEventRegistrationToken = SettingsPane::GetForCurrentView()->CommandsRequested +=
		ref new TypedEventHandler<SettingsPane^, SettingsPaneCommandsRequestedEventArgs^>(this, &SelectorPage::OnCommandsRequested);

	DefaultViewModel->Insert("FavouritesSectionVisibility", _puzzles->Favourites->Size > 0 ? Visible : Collapsed);
}

void SelectorPage::OnUnloaded(Platform::Object ^sender, RoutedEventArgs ^e)
{
	Windows::ApplicationModel::DataTransfer::DataTransferManager::GetForCurrentView()->DataRequested -= _shareEventToken;
	SettingsPane::GetForCurrentView()->CommandsRequested -= _commandsRequestedEventRegistrationToken;
}

DependencyProperty^ SelectorPage::_defaultViewModelProperty =
	DependencyProperty::Register("DefaultViewModel",
		TypeName(IObservableMap<String^,Object^>::typeid), TypeName(SelectorPage::typeid), nullptr);

/// <summary>
/// used as a trivial view model.
/// </summary>
IObservableMap<String^, Object^>^ SelectorPage::DefaultViewModel::get()
{
	return safe_cast<IObservableMap<String^, Object^>^>(GetValue(_defaultViewModelProperty));
}

DependencyProperty^ SelectorPage::_navigationHelperProperty =
	DependencyProperty::Register("NavigationHelper",
		TypeName(Common::NavigationHelper::typeid), TypeName(SelectorPage::typeid), nullptr);

/// <summary>
/// Gets an implementation of <see cref="NavigationHelper"/> designed to be
/// used as a trivial view model.
/// </summary>
Common::NavigationHelper^ SelectorPage::NavigationHelper::get()
{
	return safe_cast<Common::NavigationHelper^>(GetValue(_navigationHelperProperty));
}

#pragma region Navigation support

/// The methods provided in this section are simply used to allow
/// NavigationHelper to respond to the page's navigation methods.
/// 
/// Page specific logic should be placed in event handlers for the  
/// <see cref="NavigationHelper::LoadState"/>
/// and <see cref="NavigationHelper::SaveState"/>.
/// The navigation parameter is available in the LoadState method 
/// in addition to page state preserved during an earlier session.

void SelectorPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedTo(e);
}

void SelectorPage::OnNavigatedFrom(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedFrom(e);
}

#pragma endregion

/// <summary>
/// Populates the page with content passed during navigation.  Any saved state is also
/// provided when recreating a page from a prior session.
/// </summary>
/// <param name="navigationParameter">The parameter value passed to
/// <see cref="Frame::Navigate(Type, Object)"/> when this page was initially requested.
/// </param>
/// <param name="pageState">A map of state preserved by this page during an earlier
/// session.  This will be null the first time a page is visited.</param>
void SelectorPage::LoadState(Platform::Object^ sender, Common::LoadStateEventArgs^ e)
{
	(void) sender;	// Unused parameter
	(void) e;	// Unused parameter
}


void SelectorPage::itemGridView_ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e)
{
	(void)sender;	// Unused parameter

	// Navigate to the appropriate destination page, configuring the new page
	// by passing required information as a navigation parameter
	auto item = safe_cast<Puzzle^>(e->ClickedItem);

	App::Current->ActivatePuzzle(ref new GameLaunchParameters(item->Name));
}


void SelectorPage::ButtonHelp_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	(ref new HelpFlyout(nullptr))->ShowIndependent();
}

Rect SelectorPage::GetElementRect(FrameworkElement^ element)
{
	Windows::UI::Xaml::Media::GeneralTransform^ buttonTransform = element->TransformToVisual(nullptr);
	const Point pointOrig(0, 0);
	const Point pointTransformed = buttonTransform->TransformPoint(pointOrig);
	const Rect rect(pointTransformed.X,
		pointTransformed.Y,
		safe_cast<float>(element->ActualWidth),
		safe_cast<float>(element->ActualHeight));

	return rect;
}

void SelectorPage::ButtonPin_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Puzzle ^item;
	if (itemGridView->SelectedItem)
		item = safe_cast<Puzzle^>(itemGridView->SelectedItem);
	else if (favouritesGridView->SelectedItem)
		item = safe_cast<Puzzle^>(favouritesGridView->SelectedItem);
	else
		return;

	String ^id = item->HelpName;
	String ^args = item->Name;
	Uri ^imageUri = item->ImageUri;
	Uri ^iconUri = item->IconUri;
		
	auto tile = ref new SecondaryTile(id, item->Name, args, imageUri, TileSize::Default);
	tile->VisualElements->Square70x70Logo = iconUri;
	tile->VisualElements->Square30x30Logo = iconUri;
	tile->VisualElements->BackgroundColor = Windows::UI::Colors::LightGray;

	create_task(tile->RequestCreateForSelectionAsync(GetElementRect(ButtonPin), Windows::UI::Popups::Placement::Right)).then([this](bool isCreated)
	{
		if (isCreated)
		{
			itemGridView->SelectedIndex = -1;
			favouritesGridView->SelectedIndex = -1;
			BottomAppBar->IsOpen = false; 
		}
	});
}


void SelectorPage::ButtonUnpin_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Puzzle ^item;
	if (itemGridView->SelectedItem)
		item = safe_cast<Puzzle^>(itemGridView->SelectedItem);
	else if (favouritesGridView->SelectedItem)
		item = safe_cast<Puzzle^>(favouritesGridView->SelectedItem);
	else
		return;

	String ^id = item->HelpName;

	if (!SecondaryTile::Exists(id))
	{
		BottomAppBar->IsOpen = false;
		return;
	}

	auto tile = ref new SecondaryTile(id);

	create_task(tile->RequestDeleteForSelectionAsync(GetElementRect(ButtonUnpin), Windows::UI::Popups::Placement::Right)).then([this](bool isDeleted)
	{
		if (isDeleted)
		{
			itemGridView->SelectedIndex = -1;
			favouritesGridView->SelectedIndex = -1;
			BottomAppBar->IsOpen = false;
		}
	});
}


void SelectorPage::ButtonFav_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Puzzle ^item;
	if (itemGridView->SelectedItem)
		item = safe_cast<Puzzle^>(itemGridView->SelectedItem);
	else if (favouritesGridView->SelectedItem)
		item = safe_cast<Puzzle^>(favouritesGridView->SelectedItem);
	else
		return;

	ApplicationData::Current->LocalSettings->Values->Insert("feature_favourites", true); 
	ApplicationData::Current->RoamingSettings->Values->Insert("fav_" + item->Name, true);
	_puzzles->AddFavourite(item);

	DefaultViewModel->Insert("FavVisible", Collapsed);
	DefaultViewModel->Insert("UnfavVisible", Visible);
	DefaultViewModel->Insert("FavFooterVisible", Collapsed);
	DefaultViewModel->Insert("FavouritesSectionVisibility", Visible);
}


void SelectorPage::ButtonUnfav_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Puzzle ^item;
	if (itemGridView->SelectedItem)
		item = safe_cast<Puzzle^>(itemGridView->SelectedItem);
	else if (favouritesGridView->SelectedItem)
		item = safe_cast<Puzzle^>(favouritesGridView->SelectedItem);
	else
		return;

	ApplicationData::Current->LocalSettings->Values->Insert("feature_favourites", true);
	ApplicationData::Current->RoamingSettings->Values->Insert("fav_" + item->Name, false);
	_puzzles->RemoveFavourite(item);

	DefaultViewModel->Insert("FavVisible", Visible);
	DefaultViewModel->Insert("UnfavVisible", Collapsed);
	DefaultViewModel->Insert("FavFooterVisible", Collapsed);
	DefaultViewModel->Insert("FavouritesSectionVisibility", _puzzles->Favourites->Size > 0 ? Visible : Collapsed);
}


void SelectorPage::itemGridView_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
	if (_suppressEvent || !itemGridView || !favouritesGridView)
		return;

	if (itemGridView->SelectedIndex == -1 && favouritesGridView->SelectedIndex == -1)
	{
		DefaultViewModel->Insert("PinVisible", Collapsed);
		DefaultViewModel->Insert("UnpinVisible", Collapsed);
		DefaultViewModel->Insert("FavVisible", Collapsed);
		DefaultViewModel->Insert("UnfavVisible", Collapsed);
		BottomAppBar->IsOpen = false;
	}
	
	_suppressEvent = true;
	auto gridView = safe_cast<GridView^>(sender);
	Puzzle^ item = nullptr;
	if (gridView == itemGridView)
		favouritesGridView->SelectedIndex = -1;
	else if (gridView == favouritesGridView)
		itemGridView->SelectedIndex = -1;
	_suppressEvent = false;

	if(itemGridView->SelectedItem)
		item = safe_cast<Puzzle^>(itemGridView->SelectedItem);
	else if (favouritesGridView->SelectedItem)
		item = safe_cast<Puzzle^>(favouritesGridView->SelectedItem);

	if(item)
	{
		if (SecondaryTile::Exists(item->HelpName))
		{
			DefaultViewModel->Insert("PinVisible", Collapsed);
			DefaultViewModel->Insert("UnpinVisible", Visible);
		}
		else
		{
			DefaultViewModel->Insert("PinVisible", Visible);
			DefaultViewModel->Insert("UnpinVisible", Collapsed);
		}

		if (_puzzles->IsFavourite(item))
		{
			DefaultViewModel->Insert("FavVisible", Collapsed);
			DefaultViewModel->Insert("UnfavVisible", Visible);
		}
		else
		{
			DefaultViewModel->Insert("FavVisible", Visible);
			DefaultViewModel->Insert("UnfavVisible", Collapsed);
		}

		BottomAppBar->IsOpen = true;
	}
}


#define ZOOMED_WIDTH 700
void SelectorPage::pageRoot_SizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
{
	auto settings = ApplicationData::Current->RoamingSettings;
	bool zoomedOut = settings->Values->HasKey("feature_zoom_out") && safe_cast<bool>(settings->Values->Lookup("feature_zoom_out"));
	if (e->NewSize.Width < ZOOMED_WIDTH)
	{
		DefaultViewModel->Insert("ZoomOutVisible", Collapsed);
		DefaultViewModel->Insert("ZoomInVisible", Collapsed);
	}
	else
	{
		DefaultViewModel->Insert("ZoomInVisible", zoomedOut ? Visible : Collapsed);
		DefaultViewModel->Insert("ZoomOutVisible", !zoomedOut ? Visible : Collapsed);
	}

	if (itemGridView)
	{
		if ((e->NewSize.Width < ZOOMED_WIDTH || zoomedOut) && itemGridView->ItemTemplate != SmallPuzzleTemplate)
		{
			itemGridView->ItemTemplate = SmallPuzzleTemplate;
		}
		else if (e->NewSize.Width >= ZOOMED_WIDTH && !zoomedOut && itemGridView->ItemTemplate != NormalPuzzleTemplate)
		{
			itemGridView->ItemTemplate = NormalPuzzleTemplate;
		}
	}
}


void SelectorPage::ButtonOpen_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	FileOpenPicker^ openPicker = ref new FileOpenPicker();
	openPicker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;
	openPicker->FileTypeFilter->Append(".puzzle");
	openPicker->FileTypeFilter->Append(".sav");
	openPicker->CommitButtonText = "Load game";

	create_task(openPicker->PickSingleFileAsync()).then([this](StorageFile^ file)
	{
		if (file == nullptr)
			return;

		create_task(WindowsModern::LoadAndIdentify(file)).then([this, file](GameLaunchParameters ^result)
		{
			auto msg = result->Error;

			if (msg)
			{
				(ref new Windows::UI::Popups::MessageDialog(App::AddPeriods(msg), "Could not load game"))->ShowAsync();
			}
			else
			{
				App::Current->ActivatePuzzle(result);
			}
		});
	});
}

void SelectorPage::OnDataRequested(Windows::ApplicationModel::DataTransfer::DataTransferManager ^sender, Windows::ApplicationModel::DataTransfer::DataRequestedEventArgs ^args)
{
	args->Request->FailWithDisplayText("To share a saved game, select a puzzle first.");
}

void SelectorPage::OnCommandsRequested(SettingsPane^ settingsPane, SettingsPaneCommandsRequestedEventArgs^ e)
{
	auto handler = ref new Windows::UI::Popups::UICommandInvokedHandler(this, &SelectorPage::OnSettingsCommand);

	SettingsCommand^ helpCommand = ref new SettingsCommand("helpindex", "Help", handler);
	e->Request->ApplicationCommands->Append(helpCommand);
}

void SelectorPage::OnSettingsCommand(IUICommand^ command)
{
	(ref new HelpFlyout(nullptr))->Show();
}

void SelectorPage::itemGridView_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	itemGridView = safe_cast<GridView^>(sender);
	itemGridView->SelectedItem = nullptr;

	auto settings = ApplicationData::Current->RoamingSettings;
	bool zoomedOut = settings->Values->HasKey("feature_zoom_out") && safe_cast<bool>(settings->Values->Lookup("feature_zoom_out"));
	itemGridView->ItemTemplate = zoomedOut ? SmallPuzzleTemplate : NormalPuzzleTemplate;
}

void SelectorPage::favouritesGridView_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	favouritesGridView = safe_cast<GridView^>(sender);
	favouritesGridView->SelectedItem = nullptr;
}


void SelectorPage::ButtonZoomOut_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	ApplicationData::Current->RoamingSettings->Values->Insert("feature_zoom_out", true);
	itemGridView->ItemTemplate = SmallPuzzleTemplate;
	DefaultViewModel->Insert("ZoomInVisible", Visible);
	DefaultViewModel->Insert("ZoomOutVisible", Collapsed);
}


void SelectorPage::ButtonZoomIn_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	ApplicationData::Current->RoamingSettings->Values->Insert("feature_zoom_out", false);
	itemGridView->ItemTemplate = NormalPuzzleTemplate;
	DefaultViewModel->Insert("ZoomInVisible", Collapsed);
	DefaultViewModel->Insert("ZoomOutVisible", Visible);
}
