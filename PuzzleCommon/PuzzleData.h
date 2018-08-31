#pragma once
#include "pch.h"

namespace PuzzleCommon
{
	[Windows::UI::Xaml::Data::Bindable]
	[Windows::Foundation::Metadata::WebHostHidden]
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
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class PuzzleList sealed
	{
	public:
		PuzzleList();

		property Windows::Foundation::Collections::IObservableVector<Puzzle^>^ Items
		{
			Windows::Foundation::Collections::IObservableVector<Puzzle^>^ get()
			{
				return _items;
			}
		}

		property Windows::Foundation::Collections::IObservableVector<Puzzle^>^ Favourites
		{
			Windows::Foundation::Collections::IObservableVector<Puzzle^>^ get()
			{
				return _favourites;
			}
		}

		void AddPuzzle(Puzzle^ p);

		void AddFavourite(Puzzle^ p);
		void RemoveFavourite(Puzzle^ p);
		bool IsFavourite(Puzzle ^p);

	private:
		Windows::Foundation::Collections::IObservableVector<Puzzle^> ^_items, ^_favourites;
	};

	ref class PresetList;

	[Windows::UI::Xaml::Data::Bindable]
	[Windows::Foundation::Metadata::WebHostHidden]
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

		property PresetList ^SubMenu
		{
			PresetList ^get() { return _subMenu; }
			void set(PresetList ^val) { _subMenu = val; }
		}

		property bool Checked
		{
			bool get() { return _checked; }
			void set(bool val) { _checked = val; }
		}

	private:
		Platform::String^ _name;
		int _index;
		bool _checked;
		PresetList ^_subMenu;
	};

	[Windows::UI::Xaml::Data::Bindable]
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class PresetList sealed
	{
	public:
		PresetList();

		property Windows::Foundation::Collections::IObservableVector<Preset^>^ Items
		{
			Windows::Foundation::Collections::IObservableVector<Preset^>^ get()
			{
				return _items;
			}
		}

		property bool Custom
		{
			bool get() { return _custom; }
			void set(bool value) { _custom = value; }
		}

		void AddPreset(Preset^ p);

	private:
		bool _custom;
		Windows::Foundation::Collections::IObservableVector<Preset^>^ _items;
	};

	public enum class ButtonState { DOWN, DRAG, UP, TAP };
	public enum class ButtonType { LEFT, MIDDLE, RIGHT };

	public enum class ConfigField { INTEGER, TEXT, BOOLEAN, ENUM, FLOAT };

	[Windows::Foundation::Metadata::WebHostHidden]
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

	[Windows::UI::Xaml::Data::Bindable]
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class VirtualButton sealed
	{
	public:
		VirtualButton();

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

		static VirtualButton ^ToggleButton(Platform::String ^name, Windows::UI::Xaml::Controls::Symbol icon)
		{
			auto button = ref new VirtualButton();
			button->Name = name;
			button->Icon = ref new Windows::UI::Xaml::Controls::SymbolIcon(icon);
			return button;
		}

	private:
		Platform::String ^_label;
		Windows::System::VirtualKey _key;
		Windows::UI::Xaml::Controls::IconElement ^_icon;
	};

	[Windows::UI::Xaml::Data::Bindable]
	[Windows::Foundation::Metadata::WebHostHidden]
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

		property Windows::System::VirtualKey ColorBlindKey
		{
			Windows::System::VirtualKey get() { return _colorBlind; }
			void set(Windows::System::VirtualKey val) { _colorBlind = val; }
		}

		property VirtualButton^ ToolButton
		{
			VirtualButton^ get() { return _toolButton; }
			void set(VirtualButton ^val) { _toolButton = val; }
		}

		property VirtualButton^ ToggleButton
		{
			VirtualButton^ get() { return _toggleButton; }
			void set(VirtualButton ^val) { _toggleButton = val; }
		}

		property bool HasInputButtons
		{
			bool get() {
				return _buttons && _buttons->Size > 0;
			}
		}

		property ButtonType LeftAction
		{
			ButtonType get() { return _leftAction; }
			void set(ButtonType val) { _leftAction = val; }
		}
		property ButtonType MiddleAction
		{
			ButtonType get() { return _middleAction; }
			void set(ButtonType val) { _middleAction = val; }
		}
		property ButtonType RightAction
		{
			ButtonType get() { return _rightAction; }
			void set(ButtonType val) { _rightAction = val; }
		}
		property ButtonType TouchAction
		{
			ButtonType get() { return _touchAction; }
			void set(ButtonType val) { _touchAction = val; }
		}
		property ButtonType HoldAction
		{
			ButtonType get() { return _holdAction; }
			void set(ButtonType val) { _holdAction = val; }
		}
		property bool SwitchMiddle
		{
			bool get() { return _switchMiddle; }
			void set(bool val) { _switchMiddle = val; }
		}

	private:
		Windows::Foundation::Collections::IVector<VirtualButton^>^ _buttons;
		VirtualButton ^_toggleButton, ^_toolButton;
		Windows::System::VirtualKey _colorBlind;
		bool _switchMiddle;
		ButtonType _leftAction, _middleAction, _rightAction, _touchAction, _holdAction;
	};

	[Windows::Foundation::Metadata::WebHostHidden]
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