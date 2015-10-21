//
// PulseEffect.xaml.cpp
// Implementation of the PulseEffect class
//

#include "pch.h"
#include "PulseEffect.xaml.h"

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

PulseEffect::PulseEffect()
{
	InitializeComponent();

}

void PulseEffect::Start()
{
	ExpandAndDisappearStoryboard->Begin();
}

void PulseEffect::ExpandAndDisappearStoryboard_Completed(Platform::Object^ sender, Platform::Object^ e)
{
	Completed(this, nullptr);
}
