//
// App.xaml.cpp
// Implementation of the App class.
//

#include "pch.h"
#include "SelectorPage.xaml.h"
#include "GamePage.xaml.h"
#include "AboutFlyout.xaml.h"
#include "GeneralSettingsFlyout.xaml.h"
#include "WindowsModern.h"

using namespace PuzzleModern;

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::UI::ApplicationSettings;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
	InitializeComponent();
}

void App::OnWindowCreated(Windows::UI::Xaml::WindowCreatedEventArgs^ e)
{
	// Add the About option in Settings
	SettingsPane::GetForCurrentView()->CommandsRequested += 
		ref new TypedEventHandler<SettingsPane^, SettingsPaneCommandsRequestedEventArgs^>(this, &App::OnCommandsRequested);
}

void App::ActivatePuzzle(PuzzleModern::GameLaunchParameters ^launch)
{
	Frame ^rootFrame;
	try
	{
		rootFrame = dynamic_cast<Frame^>(Window::Current->Content);
	}
	catch (...)
	{
		rootFrame = nullptr;
	}

	// Do not repeat app initialization when the Window already has content,
	// just ensure that the window is active
	if (rootFrame == nullptr)
	{
		// Create a Frame to act as the navigation context and associate it with
		// a SuspensionManager key
		rootFrame = ref new Frame();

		// Set the default language
		rootFrame->Language = Windows::Globalization::ApplicationLanguages::Languages->GetAt(0);

		rootFrame->NavigationFailed += ref new Windows::UI::Xaml::Navigation::NavigationFailedEventHandler(this, &App::OnNavigationFailed);

		if (rootFrame->Content == nullptr)
		{
			/* Always put the Selector page first on the navigation stack */
			rootFrame->Navigate(TypeName(SelectorPage::typeid), nullptr);
			if (launch)
				rootFrame->Navigate(TypeName(GamePage::typeid), launch->Stringify());
		}

		// Place the frame in the current Window
		Window::Current->Content = rootFrame;
		// Ensure the current window is active
		Window::Current->Activate();
	}
	else
	{
		if (rootFrame->Content == nullptr)
		{
			/* Always put the Selector page first on the navigation stack */
			rootFrame->Navigate(TypeName(SelectorPage::typeid), nullptr);
			if (launch)
				rootFrame->Navigate(TypeName(GamePage::typeid), launch->Stringify());
		}
		else if (launch)
		{
			String ^puzzleName = launch->Name;
			String ^currentName = nullptr;
			if (rootFrame->CurrentSourcePageType.Name == TypeName(GamePage::typeid).Name)
			{
				auto page = safe_cast<GamePage^>(rootFrame->Content);
				currentName = safe_cast<String^>(page->DefaultViewModel->Lookup("PuzzleName"));

				if (puzzleName == currentName && launch->LoadTempFile)
					page->BeginLoadTemp(false, false);
			}
			if (puzzleName != currentName)
			{
				/* Navigate back to the selector page */
				while (rootFrame->CanGoBack)
					rootFrame->GoBack();

				rootFrame->Navigate(TypeName(GamePage::typeid), launch->Stringify());
			}
		}
		// Ensure the current window is active
		Window::Current->Activate();
	}
}

/// <summary>
/// Invoked when the application is launched normally by the end user.	Other entry points
/// will be used such as when the application is launched to open a specific file.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e)
{
#if _DEBUG
		// Show graphics profiling information while debugging.
		if (IsDebuggerPresent())
		{
			// Don't display the current frame rate counters
			 DebugSettings->EnableFrameRateCounter = false;
		}
#endif

	if (e->Arguments != nullptr)
	{
		GameLaunchParameters ^launch = nullptr;
		String ^puzzleName = e->Arguments;
		launch = ref new GameLaunchParameters(puzzleName);
		ActivatePuzzle(launch);
	}
	// Open the selector page
	else
		ActivatePuzzle(nullptr);
}

void App::OnFileActivated(Windows::ApplicationModel::Activation::FileActivatedEventArgs^ e)
{
	auto item = e->Files->First();

	StorageFile ^file = nullptr;
	if (item->HasCurrent && item->Current->IsOfType(StorageItemTypes::File))
		file = dynamic_cast<StorageFile^>(item->Current);

	create_task(WindowsModern::LoadAndIdentify(file)).then([this](GameLaunchParameters ^result)
	{
		Window::Current->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([=]{
			auto msg = result->Error;

			if (msg)
			{
				ActivatePuzzle(nullptr);
				(ref new Windows::UI::Popups::MessageDialog(App::AddPeriods(msg), "Could not load game"))->ShowAsync();
			}
			else
			{
				ActivatePuzzle(result);
			}
		}));
	});
}

void App::OnSettingsCommand(Windows::UI::Popups::IUICommand^ command)
{
	SettingsFlyout ^flyout = nullptr;
	auto id = command->Id->ToString();

	if (id == "about")
		flyout = ref new PuzzleModern::AboutFlyout();
	else if (id == "settings")
		flyout = ref new PuzzleModern::GeneralSettingsFlyout();

	if (flyout)
		flyout->Show();
}

void App::OnCommandsRequested(SettingsPane^ settingsPane, SettingsPaneCommandsRequestedEventArgs^ e)
{
	auto handler = ref new Windows::UI::Popups::UICommandInvokedHandler(this, &App::OnSettingsCommand);

	e->Request->ApplicationCommands->Append(ref new SettingsCommand("about", "Credits", handler));
	e->Request->ApplicationCommands->Append(ref new SettingsCommand("settings", "Settings", handler));
}

Platform::String ^App::AddPeriods(Platform::String ^input)
{
	if (input->End()[-1] != L'.')
		input += L".";
	return input;
}

void App::NotifySettingChanged(Platform::Object ^sender, Platform::String ^key, Platform::Object ^value)
{
	SettingChanged(sender, key, value);
}

/// <summary>
/// Invoked when Navigation to a certain page fails
/// </summary>
/// <param name="sender">The Frame which failed navigation</param>
/// <param name="e">Details about the navigation failure</param>
void App::OnNavigationFailed(Platform::Object ^sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs ^e)
{
	throw ref new FailureException("Failed to load Page " + e->SourcePageType.Name);
}