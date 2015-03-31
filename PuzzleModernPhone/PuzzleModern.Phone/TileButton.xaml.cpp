//
// TileButton.xaml.cpp
// Implementation of the TileButton class
//

#include "pch.h"
#include "TileButton.xaml.h"

using namespace PuzzleModern;

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

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

TileButton::TileButton()
{
	InitializeComponent();
}

void TileButton::OnPointerPressed(PointerRoutedEventArgs^ e)
{
	CapturePointer(e->Pointer);
	VisualStateManager::GoToState(this, "Pressed", true);
}

void TileButton::OnPointerReleased(PointerRoutedEventArgs^ e)
{
	VisualStateManager::GoToState(this, "Normal", true);
	ReleasePointerCapture(e->Pointer);
	auto pos = e->GetCurrentPoint(TiltContainer)->Position;
	if (pos.X >= 0 && pos.Y >= 0 && pos.X < TiltContainer->Width && pos.Y < TiltContainer->Height)
		Click(this, e);
}

DependencyProperty^ TileButton::_LabelProperty = DependencyProperty::Register("Label", Platform::String::typeid, TileButton::typeid, nullptr);
DependencyProperty^ TileButton::_IconProperty = DependencyProperty::Register("Icon", UIElement::typeid, TileButton::typeid, nullptr);

void PuzzleModern::TileButton::TiltContainer_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
}
