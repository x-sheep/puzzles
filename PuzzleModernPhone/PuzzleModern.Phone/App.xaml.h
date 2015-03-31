//
// App.xaml.h
// Declaration of the App class.
//

#pragma once

#include "App.g.h"
#include "PuzzleData.h"

namespace PuzzleModern
{
	namespace Phone
	{
		/// <summary>
		/// Provides application-specific behavior to supplement the default Application class.
		/// </summary>
		ref class App sealed
		{
		public:
			App();

			static property App ^Current
			{
				App ^get()
				{
					return safe_cast<App^>(Application::Current);
				}
			}

			void ActivatePuzzle(PuzzleModern::GameLaunchParameters ^launch, bool wasTerminated, bool clearStack);

			static Platform::String ^ToLower(Platform::String ^input);
			static Platform::String ^ToUpper(Platform::String ^input);

		protected:
			virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e) override;
			virtual void OnActivated(Windows::ApplicationModel::Activation::IActivatedEventArgs^ e) override;
			virtual void OnFileActivated(Windows::ApplicationModel::Activation::FileActivatedEventArgs^ e) override;

		private:
			Windows::UI::Xaml::Media::Animation::TransitionCollection^ _transitions;
			Windows::Foundation::EventRegistrationToken _firstNavigatedToken;


			void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
			void RootFrame_FirstNavigated(Platform::Object^ sender, Windows::UI::Xaml::Navigation::NavigationEventArgs^ e);
		};
	}
}