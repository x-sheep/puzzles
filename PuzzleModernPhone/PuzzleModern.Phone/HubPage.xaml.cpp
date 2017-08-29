//
// HubPage.xaml.cpp
// Implementation of the HubPage class.
//

#include "pch.h"
#include "HubPage.xaml.h"
#include "ItemPage.xaml.h"
#include "AboutPage.xaml.h"
#include "HelpPage.xaml.h"
#include "SettingsPage.xaml.h"
#include "..\..\PuzzleCommon\WindowsModern.h"
#include "..\..\PuzzleCommon\PuzzleData.h"

using namespace PuzzleModern;
using namespace PuzzleModern::Phone;
using namespace PuzzleModern::Phone::Common;

using namespace concurrency;
using namespace Platform;
using namespace Windows::ApplicationModel::Resources;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::UI::Popups;
using namespace Windows::UI::StartScreen;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;

// The Hub Application template is documented at http://go.microsoft.com/fwlink/?LinkId=391641

HubPage::HubPage()
{
	InitializeComponent();
	
	DisplayInformation::AutoRotationPreferences = DisplayOrientations::Portrait;
	NavigationCacheMode = Navigation::NavigationCacheMode::Required;

	auto navigationHelper = ref new Common::NavigationHelper(this);
	navigationHelper->LoadState += ref new LoadStateEventHandler(this, &HubPage::NavigationHelper_LoadState);
	navigationHelper->SaveState += ref new SaveStateEventHandler(this, &HubPage::NavigationHelper_SaveState);

	SetValue(_defaultViewModelProperty, ref new Platform::Collections::Map<String^, Object^>(std::less<String^>()));
	SetValue(_navigationHelperProperty, navigationHelper);

	_puzzles = WindowsModern::GetPuzzleList();
	DefaultViewModel->Insert("Items", _puzzles->Items);
	DefaultViewModel->Insert("Favourites", _puzzles->Favourites);

	if (_puzzles->Favourites->Size != 0 && ApplicationData::Current->LocalSettings->Values->HasKey("feature_favourites"))
		FavouritesFooter->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

DependencyProperty^ HubPage::_defaultViewModelProperty = DependencyProperty::Register(
	"DefaultViewModel",
	IObservableMap<String^, Object^>::typeid, 
	HubPage::typeid, 
	nullptr);

IObservableMap<String^, Object^>^ HubPage::DefaultViewModel::get()
{
	return safe_cast<IObservableMap<String^, Object^>^>(GetValue(_defaultViewModelProperty));	
}

DependencyProperty^ HubPage::_navigationHelperProperty = DependencyProperty::Register(
	"NavigationHelper",
	NavigationHelper::typeid,
	HubPage::typeid,
	nullptr);

NavigationHelper^ HubPage::NavigationHelper::get()
{
	return safe_cast<Common::NavigationHelper^>(GetValue(_navigationHelperProperty));
}

void HubPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedTo(e);
	SwitchView();
	
	auto settings = ApplicationData::Current->LocalSettings->Values;
	if (settings->HasKey("feature_hubpage"))
		Hub->SelectedIndex = safe_cast<int>(settings->Lookup("feature_hubpage"));
}

void HubPage::OnNavigatedFrom(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedFrom(e);
	ApplicationData::Current->LocalSettings->Values->Insert("feature_hubpage", Hub->SelectedIndex);
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
void HubPage::NavigationHelper_LoadState(Object^ sender, LoadStateEventArgs^ e)
{
	(void)sender;	// Unused parameter
	(void)e;		// Unused parameter
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
void HubPage::NavigationHelper_SaveState(Object^ sender, SaveStateEventArgs^ e)
{
}

void HubPage::AboutButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Frame->Navigate(TypeName(AboutPage::typeid));
}

void HubPage::ListViewItem_Holding(Platform::Object^ sender, Windows::UI::Xaml::Input::HoldingRoutedEventArgs^ e)
{
	if (e->HoldingState != Windows::UI::Input::HoldingState::Started)
		return;
	e->Handled = true;

	auto element = safe_cast<ContentControl^>(sender);
	auto item = safe_cast<Puzzle^>(element->DataContext);
	auto menu = ref new Windows::UI::Popups::PopupMenu();

	auto settings = ApplicationData::Current->RoamingSettings->Values;
	bool isFav = _puzzles->IsFavourite(item);
	auto favCommand = ref new UICommand(!isFav ? "Add to favourites" : "Remove from favourites");
	menu->Commands->Append(favCommand);

	bool pinExists = SecondaryTile::Exists(item->HelpName);
	auto pinCommand = ref new UICommand(!pinExists ? "Pin to Start" : "Unpin from Start");
	menu->Commands->Append(pinCommand);
	
	GeneralTransform^ buttonTransform = element->TransformToVisual(nullptr);
	const Point pointOrig(0, 0);
	const Point pointTransformed = buttonTransform->TransformPoint(pointOrig);
	const Rect rect(pointTransformed.X, pointTransformed.Y, safe_cast<float>(element->ActualWidth), safe_cast<float>(element->ActualHeight));

	create_task(menu->ShowForSelectionAsync(rect)).then([=](IUICommand^ command)
	{
		if (command == favCommand)
		{
			ApplicationData::Current->LocalSettings->Values->Insert("feature_favourites", true);
			settings->Insert("fav_" + item->Name, !isFav);
			if (isFav)
				_puzzles->RemoveFavourite(item);
			else
			{
				_puzzles->AddFavourite(item);
				this->Hub->SelectedIndex = 1;
			}

			if(_puzzles->Favourites->Size == 0)
				FavouritesFooter->Visibility = Windows::UI::Xaml::Visibility::Visible;
			else
				FavouritesFooter->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		}
		else if (command == pinCommand)
		{
			String ^id = item->HelpName;
			if (!pinExists)
			{
				String ^args = item->Name;
				Uri ^imageUri = item->ImageUri;
				Uri ^iconUri = item->IconUri;

				auto tile = ref new SecondaryTile(id, item->Name, args, imageUri, TileSize::Default);
				tile->RequestCreateAsync();
			}
			else
			{
				auto tile = ref new SecondaryTile(id);
				tile->RequestDeleteAsync();
			}
		}
	});
}


void HubPage::ListViewItem_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
	e->Handled = true;
	auto element = safe_cast<ContentControl^>(sender);
	auto item = safe_cast<Puzzle^>(element->DataContext);
	Frame->Navigate(TypeName(ItemPage::typeid), item->Name);
	safe_cast<ItemPage^>(Frame->Content)->BeginActivatePuzzle(nullptr);
}


void HubPage::HelpButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Frame->Navigate(TypeName(HelpPage::typeid), nullptr);
}


void HubPage::SettingsButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	Frame->Navigate(TypeName(SettingsPage::typeid), nullptr);
}

void HubPage::SwitchView()
{
	if (!AllPuzzlesGridView || !AllPuzzlesListView)
		return;

	auto settings = ApplicationData::Current->RoamingSettings;
	bool toGrid = settings->Values->HasKey("cfg_gridview") ? safe_cast<bool>(settings->Values->Lookup("cfg_gridview")) : false;

	if (toGrid)
	{
		AllPuzzlesGridView->Visibility = Windows::UI::Xaml::Visibility::Visible;
		AllPuzzlesListView->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	}
	else
	{
		AllPuzzlesGridView->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		AllPuzzlesListView->Visibility = Windows::UI::Xaml::Visibility::Visible;
	}
}

void HubPage::LoadGameButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	FileOpenPicker^ openPicker = ref new FileOpenPicker();
	openPicker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;
	openPicker->FileTypeFilter->Append(".puzzle");
	openPicker->FileTypeFilter->Append(".sav");
	openPicker->CommitButtonText = "Load game";

	openPicker->PickSingleFileAndContinue();
}
