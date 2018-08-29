//
// PresetDialog.xaml.cpp
// Implementation of the PresetDialog class
//

#include "pch.h"
#include "..\PuzzleCommon\PuzzleData.h"
#include "PresetDialog.xaml.h"

using namespace PuzzleModern;
using namespace PuzzleModern::Phone;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Content Dialog item template is documented at http://go.microsoft.com/fwlink/?LinkID=390556

PresetDialog::PresetDialog(PresetList ^list)
{
	_list = list;
	InitializeComponent();
	PresetListView->ItemsSource = list->Items;
}

void PresetDialog::PresetListView_ItemClick(Platform::Object^ sender, ItemClickEventArgs e)
{
	Hide();
	PresetClicked(this, safe_cast<Preset^>(e.ClickedItem));
}

void PresetDialog::ContentDialog_PrimaryButtonClick(Windows::UI::Xaml::Controls::ContentDialog^ sender, Windows::UI::Xaml::Controls::ContentDialogButtonClickEventArgs^ args)
{
}

void PresetDialog::ContentDialog_SecondaryButtonClick(Windows::UI::Xaml::Controls::ContentDialog^ sender, Windows::UI::Xaml::Controls::ContentDialogButtonClickEventArgs^ args)
{
	Hide();
	CustomClicked();
}


void PuzzleModern::Phone::PresetDialog::TextBlock_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto tb = safe_cast<TextBlock^>(sender);
	auto preset = safe_cast<Preset^>(tb->DataContext);
	
	if (preset->Checked)
	{
		tb->Foreground = safe_cast<Brush^>(Application::Current->Resources->Lookup("PhoneAccentBrush"));
	}
}
