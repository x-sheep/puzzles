//
// GeneralSettingsFlyout.xaml.cpp
// Implementation of the GeneralSettingsFlyout class
//

#include "pch.h"
#include "GeneralSettingsFlyout.xaml.h"

using namespace PuzzleModern;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Popups;

// The Settings Flyout item template is documented at http://go.microsoft.com/fwlink/?LinkId=273769

GeneralSettingsFlyout::GeneralSettingsFlyout()
{
	InitializeComponent();
	auto settings = ApplicationData::Current->RoamingSettings;

	_loaded = false;
	if (settings->Values->HasKey("cfg_colorblind"))
		ColorblindSwitch->IsOn = safe_cast<bool>(settings->Values->Lookup("cfg_colorblind"));
	//if (settings->Values->HasKey("cfg_lockappbar"))
	//	BottomAppbarSwitch->IsOn = safe_cast<bool>(settings->Values->Lookup("cfg_lockappbar"));
	if (settings->Values->HasKey("cfg_newgameprompt"))
		NewGameSwitch->IsOn = safe_cast<bool>(settings->Values->Lookup("cfg_newgameprompt"));

	_loaded = true;
}


void GeneralSettingsFlyout::ColorblindSwitch_Toggled(Platform::Object^ sender, RoutedEventArgs^ e)
{
	if (!_loaded) return;

	ApplicationData::Current->RoamingSettings->Values->Insert("cfg_colorblind", ColorblindSwitch->IsOn);
	App::Current->NotifySettingChanged(this, "cfg_colorblind", ColorblindSwitch->IsOn);
}


void GeneralSettingsFlyout::BottomAppbarSwitch_Toggled(Platform::Object^ sender, RoutedEventArgs^ e)
{
	if (!_loaded) return;
}


void PuzzleModern::GeneralSettingsFlyout::NewGameSwitch_Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (!_loaded) return;

	ApplicationData::Current->RoamingSettings->Values->Insert("cfg_newgameprompt", NewGameSwitch->IsOn);
	App::Current->NotifySettingChanged(this, "cfg_newgameprompt", NewGameSwitch->IsOn);
}
