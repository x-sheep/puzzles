//
// App.xaml.cpp
// Implementation of the App class.
//

#include "pch.h"
#include "HubPage.xaml.h"
#include "ItemPage.xaml.h"
#include "..\PuzzleCommon\PuzzleData.h"

using namespace PuzzleModern::Phone;
using namespace PuzzleModern::Phone::Common;

using namespace concurrency;
using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml::Media::Animation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;

// The Hub Application template is documented at http://go.microsoft.com/fwlink/?LinkID=391641

/// <summary>
/// Initializes the singleton application object. This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
	InitializeComponent();
	Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);
}

void App::ActivatePuzzle(PuzzleModern::GameLaunchParameters ^launch, bool wasTerminated, bool clearStack)
{
	auto rootFrame = dynamic_cast<Frame^>(Window::Current->Content);
	ItemPage^ page;

	if (rootFrame == nullptr)
	{
		// Create a Frame to act as the navigation context and associate it with
		// a SuspensionManager key
		rootFrame = ref new Frame();

		// Associate the frame with a SuspensionManager key.
		SuspensionManager::RegisterFrame(rootFrame, "AppFrame");

		rootFrame->CacheSize = 1;

		auto prerequisite = task<void>([](){});
		if (wasTerminated)
		{
			// Restore the saved session state only when appropriate, scheduling the
			// final launch steps after the restore is complete
			prerequisite = SuspensionManager::RestoreAsync();
		}

		prerequisite.then([=](task<void> prerequisite)
		{
			try
			{
				prerequisite.get();
			}
			catch (Platform::Exception^)
			{
				// Something went wrong restoring state.
				// Assume there is no state and continue.
				__debugbreak();
			}

			// Place the frame in the current Window
			Window::Current->Content = rootFrame;

			ActivatePuzzle(launch, false, clearStack);

		}, task_continuation_context::use_current());
		return;
	}

	if (rootFrame->Content == nullptr)
	{
		if (launch != nullptr)
			rootFrame->Navigate(TypeName(ItemPage::typeid), launch->Name);
		else
			rootFrame->Navigate(TypeName(HubPage::typeid), nullptr);
		if (clearStack)
			rootFrame->BackStack->Clear();
	}
	else if (launch != nullptr)
	{
		String ^currentName = nullptr;
		if (rootFrame->CurrentSourcePageType.Name == TypeName(ItemPage::typeid).Name)
		{
			page = safe_cast<ItemPage^>(rootFrame->Content);
			currentName = safe_cast<String^>(page->DefaultViewModel->Lookup("PuzzleName"));
		}
		if (launch->Name != currentName)
		{
			/* Navigate back to the selector page */
			while (rootFrame->CanGoBack)
				rootFrame->GoBack();

			rootFrame->Navigate(TypeName(ItemPage::typeid), launch->Name);
		}
	}
	else if (!rootFrame->CanGoBack)
	{
		// TODO don't do this when resuming
		/* Open a selector page */
		rootFrame->Navigate(TypeName(HubPage::typeid), nullptr);
		rootFrame->BackStack->Clear();
	}

	Window::Current->Activate();
	page = dynamic_cast<ItemPage^>(rootFrame->Content);
	if (page)
	{
		page->BeginActivatePuzzle(launch);
	}
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

			// Clear backstack and previous content
			Window::Current->Content = nullptr;

			if (msg)
			{
				ActivatePuzzle(nullptr, false, true);
				(ref new Windows::UI::Popups::MessageDialog(msg, "Could not load game"))->ShowAsync();
			}
			else
			{
				ActivatePuzzle(result, false, true);
			}
		}));
	});
}

void App::OnActivated(Windows::ApplicationModel::Activation::IActivatedEventArgs^ e)
{
	Application::OnActivated(e);

	if (e->Kind == ActivationKind::PickFileContinuation)
	{
		auto fileargs = dynamic_cast<FileOpenPickerContinuationEventArgs^>(e);
		if (fileargs)
		{
			auto item = fileargs->Files->First();

			StorageFile ^file = nullptr;
			if (item->HasCurrent && item->Current->IsOfType(StorageItemTypes::File))
				file = dynamic_cast<StorageFile^>(item->Current);

			if (file)
			{
				create_task(WindowsModern::LoadAndIdentify(file)).then([this](GameLaunchParameters ^result)
				{
					Window::Current->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([=]{
						auto msg = result->Error;

						if (msg)
						{
							ActivatePuzzle(nullptr, false, false);
							(ref new Windows::UI::Popups::MessageDialog(msg, "Could not load game"))->ShowAsync();
						}
						else
						{
							ActivatePuzzle(result, false, false);
						}
					}));
				});
			}
			return;
		}
	}

	if (e->Kind == ActivationKind::PickSaveFileContinuation)
	{
		auto fileargs = dynamic_cast<FileSavePickerContinuationEventArgs^>(e);
		auto oldFileName = dynamic_cast<String^>(fileargs->ContinuationData->Lookup("PuzzleFile"));
		auto newFile = fileargs->File;
		
		auto indicator = Windows::UI::ViewManagement::StatusBar::GetForCurrentView()->ProgressIndicator;

		if (newFile)
		{
			indicator->Text = "Saving game...";
			indicator->ShowAsync();
			concurrency::create_task([=]{
				return ApplicationData::Current->LocalFolder->GetFileAsync(oldFileName);
			}).then([=](StorageFile ^oldFile){
				return oldFile->CopyAndReplaceAsync(newFile);
			}).then([=](concurrency::task<void> prevTask){
				bool success = true;
				try
				{
					prevTask.get();
				}
				catch (Platform::Exception^)
				{
					success = false;
				}

				// TODO success/failure notification
				// TODO actually resume the game, not start a new one
				indicator->HideAsync();
			});
		}
		return;
	}

	ActivatePuzzle(nullptr, false, false);
}

/// <summary>
/// Invoked when the application is launched normally by the end user.  Other entry points
/// will be used when the application is launched to open a specific file, to display
/// search results, and so forth.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched(LaunchActivatedEventArgs^ e)
{

#if _DEBUG
	// Show graphics profiling information while debugging.
	if (IsDebuggerPresent())
	{
		// Display the current frame rate counters
		DebugSettings->EnableFrameRateCounter = true;
	}
#endif

	GameLaunchParameters ^launch = nullptr;
	if (e && e->Arguments != nullptr)
	{
		String ^puzzleName = e->Arguments;
		launch = ref new GameLaunchParameters(puzzleName);
	}

	ActivatePuzzle(launch, e->PreviousExecutionState == ApplicationExecutionState::Terminated, false);
}

/// <summary>
/// Restores the content transitions after the app has launched.
/// </summary>
void App::RootFrame_FirstNavigated(Object^ sender, NavigationEventArgs^ e)
{
	auto rootFrame = safe_cast<Frame^>(sender);

	TransitionCollection^ newTransitions;
	if (_transitions == nullptr)
	{
		newTransitions = ref new TransitionCollection();
		newTransitions->Append(ref new NavigationThemeTransition());
	}
	else
	{
		newTransitions = _transitions;
	}

	rootFrame->ContentTransitions = newTransitions;
	rootFrame->Navigated -= _firstNavigatedToken;
}

/// <summary>
/// Invoked when application execution is being suspended. Application state is saved
/// without knowing whether the application will be terminated or resumed with the contents
/// of memory still intact.
/// </summary>
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ e)
{
	(void) sender;	// Unused parameter
	(void) e;		// Unused parameter

	auto deferral = e->SuspendingOperation->GetDeferral();
	SuspensionManager::SaveAsync().then([=]()
	{
		deferral->Complete();
	});
}

Platform::String ^App::ToUpper(Platform::String ^input)
{
	int nameLength = input->Length() + 1;
	wchar_t *upper = snewn(nameLength, wchar_t);
	auto nameData = input->Data();
	for (int i = 0; i < nameLength; i++)
	{
		upper[i] = towupper(nameData[i]);
	}
	auto ret = ref new Platform::String(upper);
	sfree(upper);
	return ret;
}

Platform::String ^App::ToLower(Platform::String ^input)
{
	int nameLength = input->Length() + 1;
	wchar_t *lower = snewn(nameLength, wchar_t);
	auto nameData = input->Data();
	for (int i = 0; i < nameLength; i++)
	{
		lower[i] = towlower(nameData[i]);
	}
	auto ret = ref new Platform::String(lower);
	sfree(lower);
	return ret;
}
