//
// HelpFlyout.xaml.cpp
// Implementation of the HelpFlyout class
//

#include "pch.h"
#include "HelpFlyout.xaml.h"

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

PuzzleModern::HelpFlyout::HelpFlyout(Platform::String ^page)
{
	if (!page)
		page = "index";

	InitializeComponent();

	String^ url = "ms-appx-web:///" + page + ".html";
	HelpView->Navigate(ref new Uri(url));

	Width = Window::Current->Bounds.Width < 646 ? 346 : 646;
}


void HelpFlyout::HelpView_NavigationStarting(Windows::UI::Xaml::Controls::WebView^ sender, Windows::UI::Xaml::Controls::WebViewNavigationStartingEventArgs^ args)
{
	if (args->Uri->SchemeName == "http" || args->Uri->SchemeName == "https")
	{
		args->Cancel = true;
		Windows::System::Launcher::LaunchUriAsync(args->Uri);
	}
}


void HelpFlyout::SettingsFlyout_SizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
{
	HelpView->Width = e->NewSize.Width;
	HelpView->Height = e->NewSize.Height - 80;
}
