//
// PulseEffect.xaml.h
// Declaration of the PulseEffect class
//

#pragma once

#include "PulseEffect.g.h"

namespace PuzzleModern
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class PulseEffect sealed
	{
	public:
		PulseEffect();
		event Windows::Foundation::EventHandler<Platform::Object ^> ^Completed;
		void Start();
	private:
		void ExpandAndDisappearStoryboard_Completed(Platform::Object^ sender, Platform::Object^ e);
	};
}
