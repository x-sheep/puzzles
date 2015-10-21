//
// PuzzleKeyboard.xaml.cpp
// Implementation of the PuzzleKeyboard class
//

#include "pch.h"
#include "PuzzleKeyboard.xaml.h"
#include "..\..\PuzzleCommon\PuzzleData.h"

using namespace PuzzleModern;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Shapes;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

PuzzleKeyboard::PuzzleKeyboard()
{
	_holding = false;
	_heldButton = nullptr;
	_buttons = nullptr;
	InitializeComponent();
	ButtonBarChanged += ref new PuzzleModern::ButtonBarChangedEvent(this, &PuzzleModern::PuzzleKeyboard::UserControl_ButtonBarChanged);
}


void PuzzleKeyboard::UserControl_ButtonBarChanged(Platform::Object ^sender)
{
	int i, x, total;

	MainGrid->Children->Clear();
	MainGrid->ColumnDefinitions->Clear();

	if (!_buttons)
		return;

	total = 0;
	for each (auto b in _buttons->Buttons)
	{
		if (b->Type == VirtualButtonType::INPUT)
			total++;
	}

	for (i = 0; i < total; i++)
	{ 
		MainGrid->ColumnDefinitions->Append(ref new ColumnDefinition());
	}

	if (Application::Current->RequestedTheme == ApplicationTheme::Light)
	{
		_background = ref new SolidColorBrush(Windows::UI::Colors::White);
		_foreground = ref new SolidColorBrush(Windows::UI::Colors::LightGray);
		_text = ref new SolidColorBrush(Windows::UI::Colors::Black);
	}
	else
	{
		_background = safe_cast<Brush^>(Application::Current->Resources->Lookup("ContentDialogBackgroundThemeBrush"));
		_foreground = ref new SolidColorBrush(Windows::UI::Colors::Black);
		_text = ref new SolidColorBrush(Windows::UI::Colors::White);
	}

	_selected = safe_cast<Brush^>(Application::Current->Resources->Lookup("PhoneAccentBrush"));
	HighlightedBorder->Background = _selected;

	x = 0;
	for each (auto b in _buttons->Buttons)
	{
		if (b->Type != VirtualButtonType::INPUT)
			continue;

		auto rect = ref new Rectangle();
		rect->Fill = _background;
		rect->Stroke = _foreground;
		rect->StrokeThickness = 3;
		rect->Tag = b;
		rect->IsHitTestVisible = true;
		rect->PointerPressed += ref new PointerEventHandler(this, &PuzzleModern::PuzzleKeyboard::Rectangle_OnPointerMoved);
		rect->PointerMoved += ref new PointerEventHandler(this, &PuzzleModern::PuzzleKeyboard::Rectangle_OnPointerMoved);
		rect->PointerExited += ref new PointerEventHandler(this, &PuzzleModern::PuzzleKeyboard::Rectangle_OnPointerExited);
		MainGrid->Children->Append(rect);

		auto text = ref new TextBlock();
		text->Text = b->Name;
		text->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Center;
		text->VerticalAlignment = Windows::UI::Xaml::VerticalAlignment::Center;
		text->FontSize = 20;
		text->IsHitTestVisible = false;
		text->Foreground = _text;

		MainGrid->Children->Append(text);
		MainGrid->SetColumn(rect, x);
		MainGrid->SetColumn(text, x);

		x++;
	}
}


void PuzzleKeyboard::Rectangle_OnPointerMoved(Platform::Object ^sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^e)
{
	auto rect = safe_cast<Rectangle^>(sender);
	auto button = safe_cast<VirtualButton^>(rect->Tag);

	if (_heldButton == button)
		return;
	_heldButton = button;
	rect->Fill = _selected;
	HighlightedText->Text = button->Name;
	HighlightedBorder->Visibility = Windows::UI::Xaml::Visibility::Visible;
	HighlightedText->UpdateLayout();

	auto buttonX = MainGrid->TransformToVisual(rect)->TransformPoint(Point(0, 0)).X;
	auto center = rect->ActualWidth - HighlightedBorder->ActualWidth;
	auto x = -buttonX + (center / 2);
	x = min(x, MainGrid->ActualWidth);
	HighlightedBorder->Margin = Thickness(max(0, x), -50, 0, 0);
}

void PuzzleKeyboard::Rectangle_OnPointerExited(Platform::Object ^sender, PointerRoutedEventArgs ^e)
{
	auto rect = safe_cast<Rectangle^>(sender);
	auto button = safe_cast<VirtualButton^>(rect->Tag);
	rect->Fill = _background;
}

void PuzzleKeyboard::MainGrid_PointerPressed(Platform::Object^ sender, PointerRoutedEventArgs^ e)
{
	_holding = true;
}

void PuzzleKeyboard::MainGrid_PointerReleased(Platform::Object^ sender, PointerRoutedEventArgs^ e)
{
	_holding = false;
	HighlightedBorder->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	if (!_heldButton)
		return;

	ButtonPressed(this, _heldButton);

	_heldButton = nullptr;
}


void PuzzleKeyboard::MainGrid_PointerExited(Platform::Object^ sender, PointerRoutedEventArgs^ e)
{
	HighlightedBorder->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	_heldButton = nullptr;
}
