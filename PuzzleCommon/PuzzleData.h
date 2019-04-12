#pragma once
#include "pch.h"

namespace PuzzleCommon
{
	/// <summary>
	/// Represents an option in the puzzle selection screen.
	/// </summary>
	[Windows::UI::Xaml::Data::Bindable]
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class Puzzle sealed
	{
	public:
		Puzzle();

		/// <summary>
		/// The display name of the puzzle.
		/// </summary>
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

		/// <summary>
		/// The stub name of the puzzle. Used for internal files.
		/// </summary>
		property Platform::String^ HelpName
		{
			Platform::String^ get() { return _name; }
			void set(Platform::String^ val) { _name = val; }
		}

		/// <summary>
		/// A description of the puzzle.
		/// </summary>
		property Platform::String^ Synopsis
		{
			Platform::String^ get() { return _synopsis; }
			void set(Platform::String^ val) { _synopsis = val; }
		}


		/// <summary>
		/// A full-size preview image.
		/// </summary>
		/// <seealso cref="ImageUri"/>
		property Windows::UI::Xaml::Media::ImageSource^ Image
		{
			Windows::UI::Xaml::Media::ImageSource^ get()
			{
				return ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(this->ImageUri);
			}
		}

		/// <summary>
		/// A small icon-size preview image.
		/// </summary>
		/// <seealso cref="IconUri"/>
		property Windows::UI::Xaml::Media::ImageSource^ Icon
		{
			Windows::UI::Xaml::Media::ImageSource^ get()
			{
				return ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(this->IconUri);
			}
		}

		/// <summary>
		/// Uri to a full-size preview image.
		/// </summary>
		property Windows::Foundation::Uri^ ImageUri
		{
			Windows::Foundation::Uri^ get()
			{
				return ref new Windows::Foundation::Uri("ms-appx:///Icons/" + _name + "-large.png");
			}
		}

		/// <summary>
		/// Uri to a small icon-size preview image.
		/// </summary>
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
	
	/// <summary>
	/// The view model for the puzzle selection screen.
	/// </summary>
	[Windows::UI::Xaml::Data::Bindable]
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class PuzzleList sealed
	{
	public:
		PuzzleList();

		/// <summary>
		/// A list of all puzzles.
		/// </summary>
		property Windows::Foundation::Collections::IObservableVector<Puzzle^>^ Items
		{
			Windows::Foundation::Collections::IObservableVector<Puzzle^>^ get()
			{
				return _items;
			}
		}

		/// <summary>
		/// A list of puzzles marked as favourite by the user.
		/// </summary>
		property Windows::Foundation::Collections::IObservableVector<Puzzle^>^ Favourites
		{
			Windows::Foundation::Collections::IObservableVector<Puzzle^>^ get()
			{
				return _favourites;
			}
		}

		/// <summary>
		/// Add a puzzle to the full list of puzzles.
		/// </summary>
		/// <param name="p">The puzzle to add.</param>
		void AddPuzzle(Puzzle^ p);

		/// <summary>
		/// Add a puzzle to the list of favourites.
		/// </summary>
		/// <param name="p">The puzzle to add.</param>
		void AddFavourite(Puzzle^ p);
		/// <summary>
		/// Remove a puzzle from the list of favourites.
		/// </summary>
		/// <param name="p">The puzzle to remove.</param>
		void RemoveFavourite(Puzzle^ p);
		/// <summary>
		/// Check if a puzzle is currently in the list of favourites.
		/// </summary>
		/// <param name="p">The puzzle to remove.</param>
		/// <returns>True if the puzzle is in the list of favourites, otherwise False.</returns>
		bool IsFavourite(Puzzle ^p);

	private:
		Windows::Foundation::Collections::IObservableVector<Puzzle^> ^_items, ^_favourites;
	};

	// Forward declaration for Presets containing PresetLists
	ref class PresetList;

	/// <summary>
	/// Represents a node in the preset tree. Contains either puzzle parameters or a sub-menu.
	/// </summary>
	[Windows::UI::Xaml::Data::Bindable]
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class Preset sealed
	{
	public:
		Preset();
	
		/// <summary>
		/// The identifier.
		/// </summary>
		property int Index
		{
			int get() { return _index; }
			void set(int val) { _index = val; }
		}

		/// <summary>
		/// The display name.
		/// </summary>
		property Platform::String^ Name
		{
			Platform::String^ get() { return _name; }
			void set(Platform::String^ val) {
				_name = val; 
			}
		}

		/// <summary>
		/// An optional list of sub-presets. If null, this node is a leaf which can be selected as a preset.
		/// </summary>
		property PresetList ^SubMenu
		{
			PresetList ^get() { return _subMenu; }
			void set(PresetList ^val) { _subMenu = val; }
		}

		/// <summary>
		/// If this node is a leaf, this property is True if this is the active preset. Otherwise, this property is True if one of its children (at any depth) is the active preset.
		/// </summary>
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

	/// <summary>
	/// Represents a tree of presets.
	/// </summary>
	[Windows::UI::Xaml::Data::Bindable]
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class PresetList sealed
	{
	public:
		PresetList();

		/// <summary>
		/// A list of direct descendants from this tree node.
		/// </summary>
		property Windows::Foundation::Collections::IObservableVector<Preset^>^ Items
		{
			Windows::Foundation::Collections::IObservableVector<Preset^>^ get()
			{
				return _items;
			}
		}

		/// <summary>
		/// If True, the puzzle supports custom configurations. This property is only set on the root of the tree.
		/// </summary>
		property bool Custom
		{
			bool get() { return _custom; }
			void set(bool value) { _custom = value; }
		}

		/// <summary>
		/// Find the name of the current preset, or the string "Custom" if it is not part of the preset list.
		/// </summary>
		/// <returns>The name of the preset.</returns>
		Platform::String ^GetCurrentPresetName();

		/// <summary>
		/// Add a preset to this list.
		/// </summary>
		/// <param name="p">The preset to add.</param>
		void AddPreset(Preset^ p);
		/// <summary>
		/// Update the <see cref="Preset::Checked"/> property on all children. The preset with the given identifier is set to True, along with its ancestors. All other nodes are set to False.
		/// </summary>
		/// <param name="id">The identifier of the preset to check.</param>
		/// <returns>True if a preset with this identifier was found in this tree.</returns>
		bool CheckPresetItem(int id);

	private:
		bool _custom;
		Windows::Foundation::Collections::IObservableVector<Preset^>^ _items;
	};

	public enum class ButtonState { DOWN, DRAG, UP, TAP };
	public enum class ButtonType { LEFT, MIDDLE, RIGHT };

	public enum class ConfigField { INTEGER, TEXT, BOOLEAN, ENUM, FLOAT };

	/// <summary>
	/// Represents one input on the puzzle configuration form.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class ConfigItem sealed
	{
	public:
		ConfigItem();

		/// <summary>
		/// The type of input.
		/// </summary>
		property ConfigField Field
		{
			ConfigField get(){ return _field; }
			void set(ConfigField value){ _field = value; }
		}

		/// <summary>
		/// The description.
		/// </summary>
		property Platform::String ^Label
		{
			Platform::String ^get(){ return _label; }
			void set(Platform::String ^value){ _label = value; }
		}

		/// <summary>
		/// The value for <see cref="ConfigField::INTEGER"/>, <see cref="ConfigField::TEXT"/> and <see cref="ConfigField::FLOAT"/> types.
		/// </summary>
		property Platform::String ^StringValue
		{
			Platform::String ^get(){ return _sval; }
			void set(Platform::String ^value){ _sval = value; }
		}

		/// <summary>
		/// For <see cref="ConfigField::ENUM"/> type, this is the selected value. For <see cref="ConfigField::BOOLEAN"/> type, 0 is False and 1 is True.
		/// </summary>
		property int IntValue
		{
			int get(){ return _ival; }
			void set(int value){ _ival = value; }
		}

		/// <summary>
		/// For <see cref="ConfigField::ENUM"/> type, this is the list of possible values.
		/// </summary>
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

	/// <summary>
	/// Represents a single button on a virtual keyboard or toolbar.
	/// </summary>
	[Windows::UI::Xaml::Data::Bindable]
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class VirtualButton sealed
	{
	public:
		VirtualButton();

		/// <summary>
		/// The label of the button.
		/// </summary>
		property Platform::String ^Name
		{
			Platform::String ^get(){ return _label; }
			void set(Platform::String ^value){ _label = value; }
		}

		/// <summary>
		/// The key code to send.
		/// </summary>
		property Windows::System::VirtualKey Key
		{
			Windows::System::VirtualKey get(){ return _key; }
			void set(Windows::System::VirtualKey value){ _key = value; }
		}

		/// <summary>
		/// The optional icon to display.
		/// </summary>
		property Windows::UI::Xaml::Controls::IconElement ^Icon
		{
			Windows::UI::Xaml::Controls::IconElement ^get(){ return _icon; }
			void set(Windows::UI::Xaml::Controls::IconElement ^value){ _icon = value; }
		}

		/// <summary>
		/// Shorthand constructor for toggle buttons.
		/// </summary>
		/// <param name="name">The label of the button.</param>
		/// <param name="icon">A predefined symbol to use as the icon.</param>
		/// <returns>A new instance of VirtualButton.</returns>
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

	/// <summary>
	/// Contains all input details for a specific game.
	/// </summary>
	[Windows::UI::Xaml::Data::Bindable]
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class VirtualButtonCollection sealed
	{
	public:
		VirtualButtonCollection();

		/// <summary>
		/// A list of buttons to display on a virtual keyboard.
		/// </summary>
		property Windows::Foundation::Collections::IVector<VirtualButton^>^ Buttons
		{
			Windows::Foundation::Collections::IVector<VirtualButton^>^ get()
			{
				return _buttons;
			}
		}

		/// <summary>
		/// If set, this keycode is used to toggle colorblind mode.
		/// </summary>
		property Windows::System::VirtualKey ColorBlindKey
		{
			Windows::System::VirtualKey get() { return _colorBlind; }
			void set(Windows::System::VirtualKey val) { _colorBlind = val; }
		}

		/// <summary>
		/// If set, this button should be displayed on the toolbar.
		/// </summary>
		property VirtualButton^ ToolButton
		{
			VirtualButton^ get() { return _toolButton; }
			void set(VirtualButton ^val) { _toolButton = val; }
		}

		/// <summary>
		/// If set, this button can be used to switch the left/right mouse button actions.
		/// </summary>
		property VirtualButton^ ToggleButton
		{
			VirtualButton^ get() { return _toggleButton; }
			void set(VirtualButton ^val) { _toggleButton = val; }
		}

		/// <summary>
		/// If true, a virtual keyboard should be displayed.
		/// </summary>
		property bool HasInputButtons
		{
			bool get() {
				return _buttons && _buttons->Size > 0;
			}
		}

		/// <summary>
		/// The mouse event to send when pressing the left mouse button.
		/// </summary>
		property ButtonType LeftAction
		{
			ButtonType get() { return _leftAction; }
			void set(ButtonType val) { _leftAction = val; }
		}
		/// <summary>
		/// The mouse event to send when pressing the middle mouse button.
		/// </summary>
		property ButtonType MiddleAction
		{
			ButtonType get() { return _middleAction; }
			void set(ButtonType val) { _middleAction = val; }
		}
		/// <summary>
		/// The mouse event to send when pressing the right mouse button.
		/// </summary>
		property ButtonType RightAction
		{
			ButtonType get() { return _rightAction; }
			void set(ButtonType val) { _rightAction = val; }
		}
		/// <summary>
		/// The mouse event to send when performing a tap on a touchscreen.
		/// </summary>
		property ButtonType TouchAction
		{
			ButtonType get() { return _touchAction; }
			void set(ButtonType val) { _touchAction = val; }
		}
		/// <summary>
		/// The mouse event to send when pressing down on a touchscreen.
		/// </summary>
		property ButtonType HoldAction
		{
			ButtonType get() { return _holdAction; }
			void set(ButtonType val) { _holdAction = val; }
		}
		/// <summary>
		/// If true, the <see cref="ToggleButton"/> should switch the left/middle mouse button actions instead.
		/// </summary>
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

	/// <summary>
	/// Contains a number of parameters for activating the gameplay page.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class GameLaunchParameters sealed
	{
	public:
		GameLaunchParameters(){};
		GameLaunchParameters(Platform::String ^name) : _name(name){};

		/// <summary>
		/// The name of the puzzle to launch.
		/// </summary>
		property Platform::String ^Name
		{
			Platform::String ^get(){ return _name; }
			void set(Platform::String ^value){ _name = value; }
		}
		
		/// <summary>
		/// An optional file to load after activating.
		/// </summary>
		property Windows::Storage::StorageFile ^SaveFile
		{
			Windows::Storage::StorageFile ^get(){ return _saveFile; }
			void set(Windows::Storage::StorageFile ^value){ _saveFile = value; }
		}

		/// <summary>
		/// An optional game ID to load after activating.
		/// </summary>
		property Platform::String ^GameID
		{
			Platform::String ^get(){ return _gameID; }
			void set(Platform::String ^value){ _gameID = value; }
		}
		/// <summary>
		/// If not null, an error occurred when parsing the launch parameters. Display this message to the user.
		/// </summary>
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