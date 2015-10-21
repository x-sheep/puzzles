//
// ParamsDialog.xaml.cpp
// Implementation of the ParamsDialog class
//

#include "pch.h"
#include "ParamsDialog.xaml.h"
#include "..\..\PuzzleCommon\PuzzleData.h"

using namespace PuzzleModern;
using namespace PuzzleModern::Phone;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Content Dialog item template is documented at http://go.microsoft.com/fwlink/?LinkID=390556

ParamsDialog::ParamsDialog(IVector<ConfigItem^>^ items)
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

				ItemsPanel->Children->Append(toggle);

				break;
			case ConfigField::ENUM:
				box = ref new ComboBox();
				box->Name = "ConfigItem" + i.ToString();
				for each (String ^s in var->StringValues)
				{
					choice = ref new ComboBoxItem();
					choice->Content = s;
					box->Items->Append(choice);
				}

				box->SelectedIndex = var->IntValue;
				box->Header = var->Label;
				ItemsPanel->Children->Append(box);

				break;
			case ConfigField::TEXT:
			case ConfigField::INTEGER:
				txt = ref new TextBox();
				txt->Name = "ConfigItem" + i.ToString();
				txt->Header = var->Label;
				txt->Text = var->StringValue;
				txt->Tag = i;

				// Use the numeric keyboard.
				if (var->Field != ConfigField::TEXT)
				{
					auto scope = ref new InputScope();
					scope->Names->Append(ref new InputScopeName(InputScopeNameValue::Number));
					txt->InputScope = scope;
				}
				txt->IsTextPredictionEnabled = false;

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
				slider->Header = var->Label;
				slider->Background = ref new SolidColorBrush(
					Application::Current->RequestedTheme == ApplicationTheme::Light ? 
					Windows::UI::Colors::White : Windows::UI::Colors::Black);

				ItemsPanel->Children->Append(slider);
				break;
			}
			i++;
		}

		if (i < 3)
			KeyboardArea->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
		else
			KeyboardArea->Visibility = Windows::UI::Xaml::Visibility::Visible;
	}
}

void ParamsDialog::ContentDialog_PrimaryButtonClick(ContentDialog^ sender, ContentDialogButtonClickEventArgs^ args)
{
	if (!_configItems)
		return;

	args->Cancel = true;

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

void ParamsDialog::ContentDialog_SecondaryButtonClick(ContentDialog^ sender, ContentDialogButtonClickEventArgs^ args)
{
	Hide();
	PresetsClicked();
}


void ParamsDialog::OnShowing(InputPane ^sender, InputPaneVisibilityEventArgs ^args)
{
	auto rect = args->OccludedRect;
	KeyboardArea->Width = rect.Width;
	KeyboardArea->Height = rect.Height;
	auto focusObj = FocusManager::GetFocusedElement();

	if (focusObj)
	{
		FrameworkElement^ focused;
		focused = safe_cast<FrameworkElement^>(focusObj);
		int tag = safe_cast<int>(focused->Tag);

		LayoutScrollViewer->ChangeView(nullptr, tag * focused->ActualHeight, nullptr);
	}
}


void ParamsDialog::OnHiding(InputPane ^sender, InputPaneVisibilityEventArgs ^args)
{
	KeyboardArea->Height = 0;
}


void ParamsDialog::LayoutScrollViewer_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	LayoutScrollViewer = safe_cast<ScrollViewer^>(sender);
}


void ParamsDialog::ContentDialog_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto inputPane = InputPane::GetForCurrentView();
	_showingEventToken = inputPane->Showing += ref new TypedEventHandler<InputPane ^, InputPaneVisibilityEventArgs ^>(this, &ParamsDialog::OnShowing);
	_hidingEventToken = inputPane->Hiding += ref new TypedEventHandler<InputPane ^, InputPaneVisibilityEventArgs ^>(this, &ParamsDialog::OnHiding);
}


void ParamsDialog::ContentDialog_Unloaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto inputPane = InputPane::GetForCurrentView();
	inputPane->Showing -= _showingEventToken;
	inputPane->Hiding -= _hidingEventToken;
}
