#pragma once
#include "pch.h"

extern "C" {
#include "../puzzles.h"
}

#include "IPuzzleCanvas.h"
#include <ppltasks.h>

using namespace Windows::System;
using namespace Windows::Foundation;

namespace PuzzleCommon
{
	struct write_save_context
	{
		char *buf;
		int pos;
		int len;
	};

	/// <summary>
	/// The primary entry point for the original source code. One instance represents a single frontend.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class WindowsModern sealed {
	public:
		/// <summary>
		/// Create a frontend for a puzzle.
		/// </summary>
		/// <param name="name">The name of the puzzle to load.</param>
		/// <param name="icanvas">The canvas to redraw.</param>
		/// <param name="ibar">A status bar callback.</param>
		/// <param name="timer">The puzzle timer callback.</param>
		/// <exception cref="Platform::InvalidArgumentException">No puzzle with the given name was found.</exception>
		WindowsModern(Platform::String ^name, IPuzzleCanvas ^icanvas, IPuzzleStatusBar ^ibar, IPuzzleTimer ^itimer);

		/// <summary>
		/// Get the full list of puzzles.
		/// </summary>
		static PuzzleList^ GetPuzzleList();
		Puzzle^ GetCurrentPuzzle();
		/// <summary>
		/// Set the puzzle parameters to a given preset.
		/// </summary>
		/// <param name="i">The identifier of the preset to load.</param>
		void SetPreset(int i);
		/// <summary>
		/// Generate a new game with the given parameters, or load in the Game ID.
		/// </summary>
		/// <returns>True if generation was succesful, False if the generation failed or was aborted.</returns>
		IAsyncOperation<bool> ^NewGame();

		/// <summary>
		/// Get the tree of possible presets for the current puzzle.
		/// </summary>
		/// <param name="includeCustom">If True, the option for Custom parameters is added as a node in the tree.</param>
		/// <param name="maximumDepth">When the depth of the tree is higher than this value, the bottom of the tree is flattened to the given depth. Enter 0 to skip flattening the tree.</param>
		/// <returns>The preset tree.</returns>
		PresetList^ GetPresetList(bool includeCustom, int maximumDepth);
		/// <summary>
		/// If true, the current preset is a custom game.
		/// </summary>
		bool IsCustomGame();
		/// <summary>
		/// Get the current configuration form.
		/// </summary>
		/// <returns>A new instance, with the currently active values set.</returns>
		Windows::Foundation::Collections::IVector<ConfigItem^>^ GetConfiguration();
		/// <summary>
		/// Set the current configuration. Call <see cref="NewGame"/> to activate the configuration.
		/// </summary>
		/// <returns>When not null, a validation error has occured. Display this message to the user.</returns>
		Platform::String^ SetConfiguration(Windows::Foundation::Collections::IVector<ConfigItem^>^ input);

		/// <summary>
		/// Get the Random Seed used to generate this game.
		/// </summary>
		Platform::String^ GetRandomSeed();
		/// <summary>
		/// Get the description of this game.
		/// </summary>
		Platform::String^ GetGameDesc();
		/// <summary>
		/// Set the game ID or random seed. Call <see cref="NewGame"/> to load the game.
		/// </summary>
		/// <returns>When not null, a validation error has occured. Display this message to the user.</returns>
		Platform::String^ SetGameID(Platform::String ^newId);

		/// <summary>
		/// Find the closest canvas size that fits in the given bounds.
		/// </summary>
		/// <param name="iw">The input maximum width.</param>
		/// <param name="ih">The input maximum height.</param>
		/// <param name="ow">The output width.</param>
		/// <param name="oh">The output height.</param>
		void GetMaxSize(int iw, int ih, int *ow, int *oh);
		Windows::Foundation::Size PrintGame(IPuzzleCanvas ^icanvas, bool addSolution);
		/// <summary>
		/// Update the drawing.
		/// </summary>
		/// <param name="icanvas">The canvas to redraw.</param>
		/// <param name="ibar">A status bar callback.</param>
		/// <param name="timer">The puzzle timer callback.</param>
		/// <param name="force">If True, always redraw the entire canvas, instead of only the changes.</param>
		void Redraw(IPuzzleCanvas ^icanvas, IPuzzleStatusBar ^ibar, IPuzzleTimer ^timer, bool force);

		/// <summary>
		/// Process a tick event and redraw.
		/// </summary>
		/// <param name="icanvas">The canvas to redraw.</param>
		/// <param name="ibar">A status bar callback.</param>
		/// <param name="timer">The puzzle timer callback.</param>
		/// <param name="delta">The time in seconds since the last redraw.</param>
		void UpdateTimer(IPuzzleCanvas ^icanvas, IPuzzleStatusBar ^ibar, IPuzzleTimer ^timer, float delta);

		/// <summary>
		/// Load a saved game from the app local storage.
		/// </summary>
		/// <param name="name">The name of the file.</param>
		/// <returns>True if loading was succesful, otherwise False.</returns>
		IAsyncOperation<bool> ^LoadGameFromStorage(Platform::String ^name);
		IAsyncOperation<Platform::String^> ^LoadGameFromTemporary();
		/// <summary>
		/// Load a saved game from a file.
		/// </summary>
		/// <param name="file">The file to open.</param>
		/// <returns>When not null, an error has occured. Display this message to the user.</returns>
		IAsyncOperation<Platform::String^> ^LoadGameFromFile(Windows::Storage::StorageFile ^file);
		/// <summary>
		/// Load a saved game from a string.
		/// </summary>
		/// <param name="saved">The serialized game.</param>
		/// <returns>When not null, an error has occured. Display this message to the user.</returns>
		Platform::String ^LoadGameFromString(Platform::String ^saved);
		/// <summary>
		/// Identify the game belonging to a save file.
		/// </summary>
		/// <param name="file">The file to open.</param>
		/// <returns>The launch parameters for this save file.</returns>
		static IAsyncOperation<GameLaunchParameters^> ^LoadAndIdentify(Windows::Storage::StorageFile ^file);
		/// <summary>
		/// Save the current game to the app local storage.
		/// </summary>
		/// <param name="name">The name of the file.</param>
		/// <returns>True if saving was succesful, otherwise False.</returns>
		IAsyncOperation<bool> ^SaveGameToStorage(Platform::String ^name);
		/// <summary>
		/// Save the current game to a file.
		/// </summary>
		/// <param name="file">The file to write to.</param>
		/// <returns>True if saving was succesful, otherwise False.</returns>
		IAsyncOperation<bool> ^SaveGameToFile(Windows::Storage::StorageFile ^file);
		/// <summary>
		/// Serialize the current game to a string.
		/// </summary>
		/// <returns>The serialization.</returns>
		Platform::String ^SaveGameToString();

		/// <summary>
		/// Solve the current game.
		/// </summary>
		/// <returns>When not null, an error has occured. Display this message to the user.</returns>
		Platform::String ^Solve();
		bool CanPrint();
		/// <summary>
		/// True if solving is available for this puzzle type.
		/// </summary>
		bool CanSolve();
		/// <summary>
		/// Restart the current game.
		/// </summary>
		void RestartGame();
		bool CanUndo();
		/// <summary>
		/// Undo one move.
		/// </summary>
		void Undo();
		bool CanRedo();
		/// <summary>
		/// Redo one move.
		/// </summary>
		void Redo();
		/// <summary>
		/// True if a status bar should be displayed.
		/// </summary>
		bool HasStatusbar();
		/// <summary>
		/// Add all colors to the canvas.
		/// </summary>
		void ReloadColors();
		/// <summary>
		/// True if no hold actions should occur, and touches should be immediately processed as a left-mouse click.
		/// </summary>
		bool IsRightButtonDisabled();

		/// <summary>
		/// Get the controls for the current puzzle configuration.
		/// </summary>
		/// <returns>A new instance of VirtualButtonCollection.</returns>
		VirtualButtonCollection^ GetVirtualButtonBar();
		/// <summary>
		/// Send a click event.
		/// </summary>
		/// <param name="x">The x coordinate, in density-independent pixels.</param>
		/// <param name="y">The y coordinate, in density-independent pixels.</param>
		/// <param name="type">The mouse button type.</param>
		/// <param name="state">The current state of the button press.</param>
		void SendClick(int x, int y, ButtonType type, ButtonState state);
		/// <summary>
		/// Send a keyboard event.
		/// </summary>
		/// <param name="key">The keycode.</param>
		/// <param name="shift">True if the Shift button is held down.</param>
		/// <param name="control">True if the Ctrl button is held down.</param>
		void SendKey(VirtualKey key, bool shift, bool control);
		/// <summary>
		/// Depends on screen density. Required for translating input coordinates, and ensuring a minimum size for certain UI elements.
		/// </summary>
		void SetInputScale(float scale);

		/// <summary>
		/// Event that fires when a game has just been completed.
		/// </summary>
		event EventHandler<Platform::Object^> ^GameCompleted;

		/// <summary>
		/// The keycode for enabling the pencil marks cursor.
		/// </summary>
		static property VirtualKey ButtonMarkOn { VirtualKey get() { return (VirtualKey)BUTTON_MARK_ON; }}
		/// <summary>
		/// The keycode for disabling the pencil marks cursor.
		/// </summary>
		static property VirtualKey ButtonMarkOff { VirtualKey get() { return (VirtualKey)BUTTON_MARK_OFF; }}

	private:
		~WindowsModern();

		const game *ourgame;
		midend *me;
		frontend *fe;
		IPuzzleCanvas ^canvas;
		IPuzzleStatusBar ^statusbar;
		IPuzzleTimer ^timer;
		PresetList^ presets;
		std::atomic_bool _generating;
		bool _wonGame;
		static Puzzle^ FromConstGame(const game *g);
		static char *ToChars(Platform::String ^input);
	
		void Destroy();
		void CheckGameCompletion();
	};
}
