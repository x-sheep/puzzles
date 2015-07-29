//
// AboutFlyout.xaml.cpp
// Implementation of the AboutFlyout class
//

#include "pch.h"
#include "AboutFlyout.xaml.h"

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
using namespace Windows::UI::Popups;

// The Settings Flyout item template is documented at http://go.microsoft.com/fwlink/?LinkId=273769

PuzzleModern::AboutFlyout::AboutFlyout()
{
	InitializeComponent();
	Width = Window::Current->Bounds.Width < 646 ? 346 : 646;
}
