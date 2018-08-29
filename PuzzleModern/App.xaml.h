//
// App.xaml.h
// Declaration of the App class.
//

#pragma once

#include "App.g.h"
#include "..\PuzzleCommon\PuzzleData.h"
using namespace Windows::UI::ApplicationSettings;

namespace PuzzleModern
{
	public delegate void SettingsChangedEventHandler(Platform::Object ^sender, Platform::String ^key, Platform::Object ^value);

	/// <summary>
	/// Provides application-specific behavior to supplement the default Application class.
	/// </summary>
	ref class App sealed
	{
	public:
		static Platform::String ^AddPeriods(Platform::String ^input);
		static property App ^Current
		{
			App ^get()
			{
				return safe_cast<App^>(Application::Current);
			}
		}

		event SettingsChangedEventHandler ^SettingChanged;
		void NotifySettingChanged(Platform::Object ^sender, Platform::String ^key, Platform::Object ^value);
		void ActivatePuzzle(PuzzleModern::GameLaunchParameters ^launch);

	protected:
		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e) override;
		virtual void OnWindowCreated(Windows::UI::Xaml::WindowCreatedEventArgs^ e) override;
		virtual void OnFileActivated(Windows::ApplicationModel::Activation::FileActivatedEventArgs^ e) override;

	internal:
		App();

	private:

		void OnSettingsCommand(Windows::UI::Popups::IUICommand^ command);
		void OnCommandsRequested(SettingsPane^ settingsPane, SettingsPaneCommandsRequestedEventArgs^ e);
		void OnNavigationFailed(Platform::Object ^sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs ^e);
	};
}
