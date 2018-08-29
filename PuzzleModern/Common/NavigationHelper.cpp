﻿//
// NavigationHelper.cpp
// Implementation of the NavigationHelper and associated classes
//

#include "pch.h"
#include "NavigationHelper.h"
#include "RelayCommand.h"
#include "SuspensionManager.h"

using namespace PuzzleModern::Common;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::System;
using namespace Windows::UI::Core;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Navigation;

/// <summary>
/// Initializes a new instance of the <see cref="LoadStateEventArgs"/> class.
/// </summary>
/// <param name="navigationParameter">
/// The parameter value passed to <see cref="Frame->Navigate(Type, Object)"/> 
/// when this page was initially requested.
/// </param>
/// <param name="pageState">
/// A dictionary of state preserved by this page during an earlier
/// session.  This will be null the first time a page is visited.
/// </param>
LoadStateEventArgs::LoadStateEventArgs(Object^ navigationParameter, IMap<String^, Object^>^ pageState)
{
	_navigationParameter = navigationParameter;
	_pageState = pageState;
}

/// <summary>
/// Gets the <see cref="NavigationParameter"/> property of <see cref"LoadStateEventArgs"/> class.
/// </summary>
Object^ LoadStateEventArgs::NavigationParameter::get()
{
	return _navigationParameter;
}

/// <summary>
/// Gets the <see cref="PageState"/> property of <see cref"LoadStateEventArgs"/> class.
/// </summary>
IMap<String^, Object^>^ LoadStateEventArgs::PageState::get()
{
	return _pageState;
}

/// <summary>
/// Initializes a new instance of the <see cref="SaveStateEventArgs"/> class.
/// </summary>
/// <param name="pageState">An empty dictionary to be populated with serializable state.</param>
SaveStateEventArgs::SaveStateEventArgs(IMap<String^, Object^>^ pageState)
{
	_pageState = pageState;
}

/// <summary>
/// Gets the <see cref="PageState"/> property of <see cref"SaveStateEventArgs"/> class.
/// </summary>
IMap<String^, Object^>^ SaveStateEventArgs::PageState::get()
{
	return _pageState;
}

/// <summary>
/// Initializes a new instance of the <see cref="NavigationHelper"/> class.
/// </summary>
/// <param name="page">A reference to the current page used for navigation.  
/// This reference allows for frame manipulation and to ensure that keyboard 
/// navigation requests only occur when the page is occupying the entire window.</param>
NavigationHelper::NavigationHelper(Page^ page, RelayCommand^ goBack, RelayCommand^ goForward) :
	_page(page),
	_goBackCommand(goBack),
	_goForwardCommand(goForward)
	{
		// When this page is part of the visual tree make two changes:
		// 1) Map application view state to visual state for the page
		// 2) Handle keyboard and mouse navigation requests
		_loadedEventToken = page->Loaded += ref new RoutedEventHandler(this, &NavigationHelper::OnLoaded);

		//// Undo the same changes when the page is no longer visible
		_unloadedEventToken = page->Unloaded += ref new RoutedEventHandler(this, &NavigationHelper::OnUnloaded);
	}

	NavigationHelper::~NavigationHelper()
	{
		delete _goBackCommand;
		delete _goForwardCommand;

		_page = nullptr;
	}

	/// <summary>
	/// Invoked when the page is part of the visual tree
	/// </summary>
	/// <param name="sender">Instance that triggered the event.</param>
	/// <param name="e">Event data describing the conditions that led to the event.</param>
	void NavigationHelper::OnLoaded(Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
	{
		// Keyboard and mouse navigation only apply when occupying the entire window
		if (_page->ActualHeight == Window::Current->Bounds.Height &&
			_page->ActualWidth == Window::Current->Bounds.Width)
		{
			// Listen to the window directly so focus isn't required
			_acceleratorKeyEventToken = Window::Current->CoreWindow->Dispatcher->AcceleratorKeyActivated +=
				ref new TypedEventHandler<CoreDispatcher^, AcceleratorKeyEventArgs^>(this,
				&NavigationHelper::CoreDispatcher_AcceleratorKeyActivated);

			_navigationShortcutsRegistered = true;
		}
	}

	/// <summary>
	/// Invoked when the page is removed from visual tree
	/// </summary>
	/// <param name="sender">Instance that triggered the event.</param>
	/// <param name="e">Event data describing the conditions that led to the event.</param>
	void NavigationHelper::OnUnloaded(Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
	{
		if (_navigationShortcutsRegistered)
		{
			Window::Current->CoreWindow->Dispatcher->AcceleratorKeyActivated -= _acceleratorKeyEventToken;
			_navigationShortcutsRegistered = false;
		}

		// Remove handler and release the reference to page
		_page->Loaded -= _loadedEventToken;
		_page->Unloaded -= _unloadedEventToken;
	}

#pragma region Navigation support

	/// <summary>
	/// Method used by the <see cref="GoBackCommand"/> property
	/// to determine if the <see cref="Frame"/> can go back.
	/// </summary>
	/// <returns>
	/// true if the <see cref="Frame"/> has at least one entry 
	/// in the back navigation history.
	/// </returns>
	bool NavigationHelper::CanGoBack()
	{
		auto frame = _page->Frame;
		return (frame != nullptr && frame->CanGoBack);
	}

	/// <summary>
	/// Method used by the <see cref="GoBackCommand"/> property
	/// to invoke the <see cref="Windows::UI::Xaml::Controls::Frame::GoBack"/> method.
	/// </summary>
	void NavigationHelper::GoBack()
	{
		auto frame = _page->Frame;
		if (frame != nullptr && frame->CanGoBack)
		{
			frame->GoBack();
		}
	}

	/// <summary>
	/// Method used by the <see cref="GoForwardCommand"/> property
	/// to determine if the <see cref="Frame"/> can go forward.
	/// </summary>
	/// <returns>
	/// true if the <see cref="Frame"/> has at least one entry 
	/// in the forward navigation history.
	/// </returns>
	bool NavigationHelper::CanGoForward()
	{
		auto frame = _page->Frame;
		return (frame != nullptr && frame->CanGoForward);
	}

	/// <summary>
	/// Method used by the <see cref="GoForwardCommand"/> property
	/// to invoke the <see cref="Windows::UI::Xaml::Controls::Frame::GoBack"/> method.
	/// </summary>
	void NavigationHelper::GoForward()
	{
		auto frame = _page->Frame;
		if (frame != nullptr && frame->CanGoForward)
		{
			frame->GoForward();
		}
	}

	/// <summary>
	/// Gets the <see cref="GoBackCommand"/> property of <see cref"NavigationHelper"/> class.
	/// </summary>
	RelayCommand^ NavigationHelper::GoBackCommand::get()
	{
		if (_goBackCommand == nullptr)
		{
			_goBackCommand = ref new RelayCommand(
				[this](Object^) -> bool
			{
				return CanGoBack();
			},
				[this](Object^) -> void
				{
					GoBack();
				}
				);
		}
		return _goBackCommand;
	}

	/// <summary>
	/// Gets the <see cref="GoForwardCommand"/> property of <see cref"NavigationHelper"/> class.
	/// </summary>
	RelayCommand^ NavigationHelper::GoForwardCommand::get()
	{
		if (_goForwardCommand == nullptr)
		{
			_goForwardCommand = ref new RelayCommand(
				[this](Object^) -> bool
			{
				return CanGoForward();
			},
				[this](Object^) -> void
				{
					GoForward();
				}
				);
		}
		return _goForwardCommand;
	}

	/// <summary>
	/// Invoked on every keystroke, including system keys such as Alt key combinations, when
	/// this page is active and occupies the entire window.  Used to detect keyboard navigation
	/// between pages even when the page itself doesn't have focus.
	/// </summary>
	/// <param name="sender">Instance that triggered the event.</param>
	/// <param name="e">Event data describing the conditions that led to the event.</param>
	void NavigationHelper::CoreDispatcher_AcceleratorKeyActivated(CoreDispatcher^ sender,
		AcceleratorKeyEventArgs^ e)
		{
			sender; // Unused parameter
			auto virtualKey = e->VirtualKey;

			// Only investigate further when Left, Right, or the dedicated Previous or Next keys
			// are pressed
			if ((e->EventType == CoreAcceleratorKeyEventType::SystemKeyDown ||
				e->EventType == CoreAcceleratorKeyEventType::KeyDown) &&
				(virtualKey == VirtualKey::Left || virtualKey == VirtualKey::Right ||
				virtualKey == VirtualKey::GoBack || virtualKey == VirtualKey::GoForward))
			{
				auto coreWindow = Window::Current->CoreWindow;
				auto downState = Windows::UI::Core::CoreVirtualKeyStates::Down;
				bool menuKey = (coreWindow->GetKeyState(VirtualKey::Menu) & downState) == downState;
				bool controlKey = (coreWindow->GetKeyState(VirtualKey::Control) & downState) == downState;
				bool shiftKey = (coreWindow->GetKeyState(VirtualKey::Shift) & downState) == downState;
				bool noModifiers = !menuKey && !controlKey && !shiftKey;
				bool onlyAlt = menuKey && !controlKey && !shiftKey;

				if ((virtualKey == VirtualKey::GoBack && noModifiers) ||
					(virtualKey == VirtualKey::Left && onlyAlt))
				{
					// When the previous key or Alt+Left are pressed navigate back
					e->Handled = true;
					GoBackCommand->Execute(this);
				}
				else if ((virtualKey == VirtualKey::GoForward && noModifiers) ||
					(virtualKey == VirtualKey::Right && onlyAlt))
				{
					// When the next key or Alt+Right are pressed navigate forward
					e->Handled = true;
					GoForwardCommand->Execute(this);
				}
			}
		}

#pragma endregion

#pragma region Process lifetime management

		/// <summary>
		/// Invoked when this page is about to be displayed in a Frame.
		/// </summary>
		/// <param name="e">Event data that describes how this page was reached.  The Parameter
		/// property provides the group to be displayed.</param>
		void NavigationHelper::OnNavigatedTo(NavigationEventArgs^ e)
		{
			auto frameState = SuspensionManager::SessionStateForFrame(_page->Frame);
			_pageKey = "Page-" + _page->Frame->BackStackDepth;

			if (e->NavigationMode == NavigationMode::New)
			{
				// Clear existing state for forward navigation when adding a new page to the
				// navigation stack
				auto nextPageKey = _pageKey;
				int nextPageIndex = _page->Frame->BackStackDepth;
				while (frameState->HasKey(nextPageKey))
				{
					frameState->Remove(nextPageKey);
					nextPageIndex++;
					nextPageKey = "Page-" + nextPageIndex;
				}

				// Pass the navigation parameter to the new page
				LoadState(this, ref new LoadStateEventArgs(e->Parameter, nullptr));
			}
			else
			{
				// Pass the navigation parameter and preserved page state to the page, using
				// the same strategy for loading suspended state and recreating pages discarded
				// from cache
				LoadState(this, ref new LoadStateEventArgs(e->Parameter, safe_cast<IMap<String^, Object^>^>(frameState->Lookup(_pageKey))));
			}
		}

		/// <summary>
		/// Invoked when this page will no longer be displayed in a Frame.
		/// </summary>
		/// <param name="e">Event data that describes how this page was reached.  The Parameter
		/// property provides the group to be displayed.</param>
		void NavigationHelper::OnNavigatedFrom(NavigationEventArgs^ e)
		{
			auto frameState = SuspensionManager::SessionStateForFrame(_page->Frame);
			auto pageState = ref new Map<String^, Object^>();
			SaveState(this, ref new SaveStateEventArgs(pageState));
			frameState->Insert(_pageKey, pageState);
		}
#pragma endregion
