//
// SpecificDialog.xaml.cpp
// Implementation of the SpecificDialog class
//

#include "pch.h"
#include "SpecificDialog.xaml.h"

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

SpecificDialog::SpecificDialog(Platform::String ^title, Platform::String ^gameId, bool isRandomSeed)
{
	InitializeComponent();
	Title = title;
	_isRandomSeed = isRandomSeed;

	IDField->Text = gameId;
	if (isRandomSeed)
		IDField->Header = "Random seed";
}

void SpecificDialog::ContentDialog_PrimaryButtonClick(ContentDialog^ sender, ContentDialogButtonClickEventArgs^ args)
{
	Hide();
	GameIDSpecified(this, IDField->Text, _isRandomSeed);
}

void SpecificDialog::ContentDialog_SecondaryButtonClick(ContentDialog^ sender, ContentDialogButtonClickEventArgs^ args)
{
}
