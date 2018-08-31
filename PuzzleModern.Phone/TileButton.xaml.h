//
// TileButton.xaml.h
// Declaration of the TileButton class
//

#pragma once

#include "TileButton.g.h"

using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Markup;

namespace PuzzleModern
{
	namespace Phone
	{
		[ContentProperty(Name = "Icon")]
		[Windows::Foundation::Metadata::WebHostHidden]
		public ref class TileButton sealed
		{
		private:
			static DependencyProperty^ _LabelProperty, ^_IconProperty;

		protected:
			virtual void OnPointerPressed(Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e) override;
			virtual void OnPointerReleased(Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e) override;

		public:
			TileButton();

			event RoutedEventHandler ^Click;

			static property DependencyProperty^ IconProperty
			{
				DependencyProperty^ get() { return _IconProperty; }
			}

			static property DependencyProperty^ LabelProperty
			{
				DependencyProperty^ get() { return _LabelProperty; }
			}

			property UIElement ^Icon
			{
				UIElement^ get() { return (UIElement^)GetValue(IconProperty); }
				void set(UIElement^ value) { SetValue(IconProperty, value); }
			}

			property String^ Label
			{
				String^ get() { return (String^)GetValue(LabelProperty); }
				void set(String^ value) { SetValue(LabelProperty, value); }
			}
		private:
			void TiltContainer_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
		};
	}
}