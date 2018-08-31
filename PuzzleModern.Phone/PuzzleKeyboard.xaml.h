//
// PuzzleKeyboard.xaml.h
// Declaration of the PuzzleKeyboard class
//

#pragma once

#include "PuzzleKeyboard.g.h"

using namespace PuzzleCommon;

namespace PuzzleModern
{
	namespace Phone
	{
		public delegate void ButtonBarChangedEvent(Platform::Object ^sender);
		public delegate void ButtonBarPressedEvent(Platform::Object ^sender, VirtualButton ^button);

		[Windows::Foundation::Metadata::WebHostHidden]
		public ref class PuzzleKeyboard sealed
		{
		public:
			PuzzleKeyboard();
			event ButtonBarChangedEvent ^ButtonBarChanged;
			event ButtonBarPressedEvent ^ButtonPressed;
			property VirtualButtonCollection ^Buttons
			{
				VirtualButtonCollection ^get() { return _buttons; }
				void set(VirtualButtonCollection ^value)
				{
					_buttons = value;
					ButtonBarChanged(this);
				}
			}
		private:
			VirtualButtonCollection ^_buttons;
			void UserControl_ButtonBarChanged(Platform::Object ^sender);
			void Rectangle_OnPointerMoved(Platform::Object ^sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^e);

			Windows::UI::Xaml::Media::Brush ^_background, ^_foreground, ^_text, ^_selected;

			bool _holding;
			int _total;
			VirtualButton ^_heldButton;
			void MainGrid_PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
			void MainGrid_PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
			void Rectangle_OnPointerExited(Platform::Object ^sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^e);
			void MainGrid_PointerExited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
		};
	}
}