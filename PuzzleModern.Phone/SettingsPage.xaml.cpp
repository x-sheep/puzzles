//
// SettingsPage.xaml.cpp
// Implementation of the SettingsPage class
//

#include "pch.h"
#include "SettingsPage.xaml.h"

using namespace PuzzleModern::Phone;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Basic Page item template is documented at http://go.microsoft.com/fwlink/?LinkID=390556

SettingsPage::SettingsPage()
{
	InitializeComponent();
	SetValue(_defaultViewModelProperty, ref new Platform::Collections::Map<String^, Object^>(std::less<String^>()));
	auto navigationHelper = ref new Common::NavigationHelper(this);
	SetValue(_navigationHelperProperty, navigationHelper);
	navigationHelper->LoadState += ref new Common::LoadStateEventHandler(this, &SettingsPage::LoadState);
	navigationHelper->SaveState += ref new Common::SaveStateEventHandler(this, &SettingsPage::SaveState);

	auto settings = ApplicationData::Current->RoamingSettings;

	_loaded = false;
	if (settings->Values->HasKey("cfg_colorblind"))
		ColorblindSwitch->IsOn = safe_cast<bool>(settings->Values->Lookup("cfg_colorblind"));
	if (settings->Values->HasKey("cfg_newgameprompt"))
		NewGameSwitch->IsOn = safe_cast<bool>(settings->Values->Lookup("cfg_newgameprompt"));
	if (settings->Values->HasKey("cfg_gridview"))
		GridViewSwitch->IsOn = safe_cast<bool>(settings->Values->Lookup("cfg_gridview"));
	if (settings->Values->HasKey("env_MAP_VIVID_COLOURS") && safe_cast<bool>(settings->Values->Lookup("env_MAP_VIVID_COLOURS")))
	{
		MapPaletteBox->SelectedIndex = 1;
		VividPalettePreview->Visibility = Windows::UI::Xaml::Visibility::Visible;
		DefaultPalettePreview->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	}
	if (settings->Values->HasKey("env_FIXED_PENCIL_MARKS") && safe_cast<bool>(settings->Values->Lookup("env_FIXED_PENCIL_MARKS")))
	{
		FixedPencilMarksBox->SelectedIndex = 1;
		FixedPencilPreview->Visibility = Windows::UI::Xaml::Visibility::Visible;
		SequentialPencilPreview->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	}

	_loaded = true;
}

DependencyProperty^ SettingsPage::_defaultViewModelProperty =
DependencyProperty::Register("DefaultViewModel",
TypeName(IObservableMap<String^, Object^>::typeid), TypeName(SettingsPage::typeid), nullptr);

/// <summary>
/// Used as a trivial view model.
/// </summary>
IObservableMap<String^, Object^>^ SettingsPage::DefaultViewModel::get()
{
	return safe_cast<IObservableMap<String^, Object^>^>(GetValue(_defaultViewModelProperty));
}

DependencyProperty^ SettingsPage::_navigationHelperProperty =
DependencyProperty::Register("NavigationHelper",
TypeName(Common::NavigationHelper::typeid), TypeName(SettingsPage::typeid), nullptr);

/// <summary>
/// Gets an implementation of <see cref="NavigationHelper"/> designed to be
/// used as a trivial view model.
/// </summary>
Common::NavigationHelper^ SettingsPage::NavigationHelper::get()
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

void SettingsPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedTo(e);
}

void SettingsPage::OnNavigatedFrom(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedFrom(e);
}

#pragma endregion

/// <summary>
/// Populates the page with content passed during navigation. Any saved state is also
/// provided when recreating a page from a prior session.
/// </summary>
/// <param name="sender">
/// The source of the event; typically <see cref="NavigationHelper"/>
/// </param>
/// <param name="e">Event data that provides both the navigation parameter passed to
/// <see cref="Frame::Navigate(Type, Object)"/> when this page was initially requested and
/// a dictionary of state preserved by this page during an earlier
/// session. The state will be null the first time a page is visited.</param>
void SettingsPage::LoadState(Object^ sender, Common::LoadStateEventArgs^ e)
{
	(void) sender;	// Unused parameter
	(void) e;	// Unused parameter
}

/// <summary>
/// Preserves state associated with this page in case the application is suspended or the
/// page is discarded from the navigation cache.  Values must conform to the serialization
/// requirements of <see cref="SuspensionManager::SessionState"/>.
/// </summary>
/// <param name="sender">The source of the event; typically <see cref="NavigationHelper"/></param>
/// <param name="e">Event data that provides an empty dictionary to be populated with
/// serializable state.</param>
void SettingsPage::SaveState(Object^ sender, Common::SaveStateEventArgs^ e){
	(void) sender;	// Unused parameter
	(void) e; // Unused parameter
}


void SettingsPage::ColorblindSwitch_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (!_loaded) return;

	ApplicationData::Current->RoamingSettings->Values->Insert("cfg_colorblind", ColorblindSwitch->IsOn);
}


void SettingsPage::NewGameSwitch_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (!_loaded) return;

	ApplicationData::Current->RoamingSettings->Values->Insert("cfg_newgameprompt", NewGameSwitch->IsOn);
}


void SettingsPage::GridViewSwitch_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (!_loaded) return;

	ApplicationData::Current->RoamingSettings->Values->Insert("cfg_gridview", GridViewSwitch->IsOn);
}


void SettingsPage::MapPaletteBox_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
	if (!_loaded) return;

	if (MapPaletteBox->SelectedIndex == 1)
	{
		VividPalettePreview->Visibility = Windows::UI::Xaml::Visibility::Visible;
		DefaultPalettePreview->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	}
	else
	{
		VividPalettePreview->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		DefaultPalettePreview->Visibility = Windows::UI::Xaml::Visibility::Visible;
	}

	ApplicationData::Current->RoamingSettings->Values->Insert("env_MAP_VIVID_COLOURS", MapPaletteBox->SelectedIndex == 1);
}

void SettingsPage::FixedPencilMarksBox_SelectionChanged(Platform::Object^ sender, SelectionChangedEventArgs^ e)
{
	if (!_loaded) return;

	if (FixedPencilMarksBox->SelectedIndex == 1)
	{
		FixedPencilPreview->Visibility = Windows::UI::Xaml::Visibility::Visible;
		SequentialPencilPreview->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	}
	else
	{
		FixedPencilPreview->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		SequentialPencilPreview->Visibility = Windows::UI::Xaml::Visibility::Visible;
	}

	ApplicationData::Current->RoamingSettings->Values->Insert("env_FIXED_PENCIL_MARKS", FixedPencilMarksBox->SelectedIndex == 1);
}
