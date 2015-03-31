#pragma once
#include "pch.h"

namespace PuzzleModern
{
	[Windows::UI::Xaml::Data::Bindable]
	public ref class Puzzle sealed
	{
	public:
		Puzzle();

		property Platform::String^ Name
		{
			Platform::String^ get() { return _title; }
			void set(Platform::String^ val) { _title = val; }
		}

		property int Index
		{
			int get() { return _index; }
			void set(int val) { _index = val; }
		}

		property Platform::String^ HelpName
		{
			Platform::String^ get() { return _name; }
			void set(Platform::String^ val) { _name = val; }
		}

		property Platform::String^ Synopsis
		{
			Platform::String^ get() { return _synopsis; }
			void set(Platform::String^ val) { _synopsis = val; }
		}

		property Windows::UI::Xaml::Media::ImageSource^ Image
		{
			Windows::UI::Xaml::Media::ImageSource^ get()
			{
				return ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(this->ImageUri);
			}
		}

		property Windows::UI::Xaml::Media::ImageSource^ Icon
		{
			Windows::UI::Xaml::Media::ImageSource^ get()
			{
				return ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(this->IconUri);
			}
		}

		property Windows::Foundation::Uri^ ImageUri
		{
			Windows::Foundation::Uri^ get()
			{
				return ref new Windows::Foundation::Uri("ms-appx:///Icons/" + _name + "-large.png");
			}
		}

		property Windows::Foundation::Uri^ IconUri
		{
			Windows::Foundation::Uri^ get()
			{
				return ref new Windows::Foundation::Uri("ms-appx:///Icons/" + _name + "-small.png");
			}
		}

	private:
		Platform::String^ _name;
		Platform::String^ _title;
		Platform::String^ _synopsis;
		int _index;
	};
	
	[Windows::UI::Xaml::Data::Bindable]
	public ref class PuzzleList sealed
	{
	public:
		PuzzleList();

		property Windows::Foundation::Collections::IObservableVector<Platform::Object^>^ Items
		{
			Windows::Foundation::Collections::IObservableVector<Platform::Object^>^ get()
			{
				return _items;
			}
		}

		void AddPuzzle(Puzzle^ p);

	private:
		Windows::Foundation::Collections::IObservableVector<Platform::Object^>^ _items;
	};

	[Windows::UI::Xaml::Data::Bindable]
	public ref class Preset sealed
	{
	public:
		Preset();
	
		property int Index
		{
			int get() { return _index; }
			void set(int val) { _index = val; }
		}

		property Platform::String^ Name
		{
			Platform::String^ get() { return _name; }
			void set(Platform::String^ val) {
				_name = val; 
			}
		}

	private:
		Platform::String^ _name;
		int _index;
	};

	[Windows::UI::Xaml::Data::Bindable]
	public ref class PresetList sealed
	{
	public:
		PresetList();

		property Windows::Foundation::Collections::IObservableVector<Platform::Object^>^ Items
		{
			Windows::Foundation::Collections::IObservableVector<Platform::Object^>^ get()
			{
				return _items;
			}
		}

		property int Current
		{
			int get() { return _current; }
			void set(int value) { _current = value; }
		}

		property bool Custom
		{
			bool get() { return _custom; }
			void set(bool value) { _custom = value; }
		}

		void AddPreset(Preset^ p);

	private:
		int _current;
		bool _custom;
		Windows::Foundation::Collections::IObservableVector<Platform::Object^>^ _items;
	};

	public enum class ButtonState { DOWN, DRAG, UP, TAP };
	public enum class ButtonType { LEFT, RIGHT };

	public enum class ConfigField { INTEGER, TEXT, BOOLEAN, ENUM, FLOAT };

	public ref class ConfigItem sealed
	{
	public:
		ConfigItem();

		property ConfigField Field
		{
			ConfigField get(){ return _field; }
			void set(ConfigField value){ _field = value; }
		}

		property Platform::String ^Label
		{
			Platform::String ^get(){ return _label; }
			void set(Platform::String ^value){ _label = value; }
		}

		property Platform::String ^StringValue
		{
			Platform::String ^get(){ return _sval; }
			void set(Platform::String ^value){ _sval = value; }
		}

		property int IntValue
		{
			int get(){ return _ival; }
			void set(int value){ _ival = value; }
		}

		property Windows::Foundation::Collections::IVector<Platform::String^> ^StringValues
		{
			Windows::Foundation::Collections::IVector<Platform::String^> ^get(){ return _strings; }
			void set(Windows::Foundation::Collections::IVector<Platform::String^> ^value){ _strings = value; }
		}

	private:
		ConfigField _field;
		Platform::String ^_label;
		Platform::String ^_sval;
		int _ival;
		Windows::Foundation::Collections::IVector<Platform::String^> ^_strings;
	};

	public enum class VirtualButtonType
	{
		INPUT, TOOL, COLORBLIND, TOGGLE_MOUSE
	};

	[Windows::UI::Xaml::Data::Bindable]
	public ref class VirtualButton sealed
	{
	public:
		VirtualButton();
		static VirtualButton ^FromNumber(int c);

		property Platform::String ^Name
		{
			Platform::String ^get(){ return _label; }
			void set(Platform::String ^value){ _label = value; }
		}

		property Windows::System::VirtualKey Key
		{
			Windows::System::VirtualKey get(){ return _key; }
			void set(Windows::System::VirtualKey value){ _key = value; }
		}

		property Windows::UI::Xaml::Controls::IconElement ^Icon
		{
			Windows::UI::Xaml::Controls::IconElement ^get(){ return _icon; }
			void set(Windows::UI::Xaml::Controls::IconElement ^value){ _icon = value; }
		}

		property VirtualButtonType Type
		{
			VirtualButtonType get(){ return _type; }
			void set(VirtualButtonType value){ _type = value; }
		}

		static VirtualButton ^Backspace()
		{
			auto button = ref new VirtualButton();
			button->Name = L"\u232b";
			button->Key = Windows::System::VirtualKey::Back;
			return button;
		}

		static VirtualButton ^ToggleButton(Platform::String ^name, Windows::UI::Xaml::Controls::Symbol icon)
		{
			auto button = ref new VirtualButton();
			button->Name = name;
			button->Type = VirtualButtonType::TOGGLE_MOUSE;
			button->Icon = ref new Windows::UI::Xaml::Controls::SymbolIcon(icon);
			return button;
		}

		static VirtualButton ^Pencil()
		{
			return ToggleButton("Mark", Windows::UI::Xaml::Controls::Symbol::Edit);
		}

	private:
		Platform::String ^_label;
		VirtualButtonType _type;
		Windows::System::VirtualKey _key;
		Windows::UI::Xaml::Controls::IconElement ^_icon;
	};

	[Windows::UI::Xaml::Data::Bindable]
	public ref class VirtualButtonCollection sealed
	{
	public:
		VirtualButtonCollection();

		property Windows::Foundation::Collections::IVector<VirtualButton^>^ Buttons
		{
			Windows::Foundation::Collections::IVector<VirtualButton^>^ get()
			{
				return _buttons;
			}
		}

		property VirtualButton^ ColorBlindKey
		{
			VirtualButton^ get()
			{
				return _colorBlind;
			}
			void set(VirtualButton ^val)
			{
				_colorBlind = val;
			}
		}

		property VirtualButton^ ToolButton
		{
			VirtualButton^ get()
			{
				return _toolButton;
			}
			void set(VirtualButton ^val)
			{
				_toolButton = val;
			}
		}

		property bool HasInputButtons
		{
			bool get() {
				if (!_buttons)
					return false;

				for each(auto b in _buttons)
				{
					if (b->Type == VirtualButtonType::INPUT)
						return true;
				}
				return false;
			}
		}

	private:
		Windows::Foundation::Collections::IVector<VirtualButton^>^ _buttons;
		VirtualButton ^_colorBlind, ^_toolButton;
	};

	public ref class GameLaunchParameters sealed
	{
	public:
		GameLaunchParameters(){};
		GameLaunchParameters(Platform::String ^name) : _name(name){};

		property Platform::String ^Name
		{
			Platform::String ^get(){ return _name; }
			void set(Platform::String ^value){ _name = value; }
		}
		
		property Windows::Storage::StorageFile ^SaveFile
		{
			Windows::Storage::StorageFile ^get(){ return _saveFile; }
			void set(Windows::Storage::StorageFile ^value){ _saveFile = value; }
		}

		property Platform::String ^GameID
		{
			Platform::String ^get(){ return _gameID; }
			void set(Platform::String ^value){ _gameID = value; }
		}
		property Platform::String ^Error
		{
			Platform::String ^get(){ return _error; }
			void set(Platform::String ^value){ _error = value; }
		}
	private:
		Platform::String ^_name;
		Windows::Storage::StorageFile ^_saveFile;
		Platform::String ^_gameID;
		Platform::String ^_error;
	};
}