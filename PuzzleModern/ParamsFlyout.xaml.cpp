//
// ParamsFlyout.xaml.cpp
// Implementation of the ParamsFlyout class
//

#include "pch.h"
#include "ParamsFlyout.xaml.h"

using namespace PuzzleModern;
using namespace PuzzleCommon;

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

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

ParamsFlyout::ParamsFlyout(Windows::Foundation::Collections::IVector<ConfigItem^>^ items)
{
	InitializeComponent();
	_configItems = nullptr;
	if (items)
	{
		int i = 0;
		TextBox ^txt;
		CheckBox ^toggle;
		ComboBox ^box;
		ComboBoxItem ^choice;
		Slider ^slider;

		_configItems = items;
		for each (ConfigItem^ var in items)
		{
			switch (var->Field)
			{
			case ConfigField::BOOLEAN:
				toggle = ref new CheckBox();
				toggle->Name = "ConfigItem" + i.ToString();
				toggle->IsChecked = var->IntValue != 0;
				toggle->Content = var->Label;
				toggle->ContentTemplate = LabelTemplate;
				toggle->Margin = Thickness(0, 0, 0, 10);

				ItemsPanel->Children->Append(toggle);

				break;
			case ConfigField::ENUM:
				box = ref new ComboBox();
				box->Name = "ConfigItem" + i.ToString();
				box->HeaderTemplate = LabelTemplate;
				for each (String ^s in var->StringValues)
				{
					choice = ref new ComboBoxItem();
					choice->Content = s;
					box->Items->Append(choice);
				}

				box->SelectedIndex = var->IntValue;
				box->Header = var->Label;
				box->Margin = Thickness(0, 0, 0, 10);
				ItemsPanel->Children->Append(box);

				break;
			case ConfigField::TEXT:
			case ConfigField::INTEGER:
				txt = ref new TextBox();
				txt->Name = "ConfigItem" + i.ToString();
				txt->HeaderTemplate = LabelTemplate;
				txt->Header = var->Label;
				txt->Text = var->StringValue;

				// Use the numeric keyboard.
				if (var->Field == ConfigField::INTEGER)
				{
					auto scope = ref new InputScope();
					scope->Names->Append(ref new InputScopeName(InputScopeNameValue::Number));
					txt->InputScope = scope;
				}
				
				txt->Margin = Thickness(0, 0, 0, 10);

				ItemsPanel->Children->Append(txt);
				break;
			case ConfigField::FLOAT:
				slider = ref new Slider();
				slider->Name = "ConfigItem" + i.ToString();
				slider->Value = _wtof(var->StringValue->Data());
				slider->Minimum = 0;
				slider->Maximum = 1;
				slider->StepFrequency = 0.05;
				slider->TickFrequency = 0.25;
				slider->TickPlacement = Windows::UI::Xaml::Controls::Primitives::TickPlacement::BottomRight;
				slider->Header = var->Label;
				slider->HeaderTemplate = LabelTemplate;

				ItemsPanel->Children->Append(slider);
				break;
			}
			i++;
		}
	}
	else
	{
		NoConfigLabel->Visibility = Windows::UI::Xaml::Visibility::Visible;
		ConfirmButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	}
}


void ParamsFlyout::ConfirmButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (!_configItems)
	{
		ErrorLabel->Text = "No configuration available";
		return;
	}

	int i = 0;
	TextBox ^txt;
	CheckBox ^toggle;
	ComboBox ^box;
	Slider ^slider;
	for each (ConfigItem^ var in _configItems)
	{
		auto itemObj = FindName("ConfigItem" + i.ToString());
		switch (var->Field)
		{
		case ConfigField::BOOLEAN:
			toggle = safe_cast<CheckBox^>(itemObj);
			var->IntValue = toggle->IsChecked->Value ? 1 : 0;
			break;
		case ConfigField::ENUM:
			box = safe_cast<ComboBox^>(itemObj);
			var->IntValue = box->SelectedIndex;
			break;
		case ConfigField::TEXT:
		case ConfigField::INTEGER:
			txt = safe_cast<TextBox^>(itemObj);
			var->StringValue = txt->Text;
			break;
		case ConfigField::FLOAT:
			slider = safe_cast<Slider^>(itemObj);
			var->StringValue = slider->Value.ToString();
			break;
		}
		i++;
	}

	NewConfiguration(this, _configItems);
}

void ParamsFlyout::ShowErrorMessage(Platform::String ^error)
{
	ErrorLabel->Visibility = Windows::UI::Xaml::Visibility::Visible;
	ErrorLabel->Text = error;
	ErrorAppearingStoryboard->Begin();
}