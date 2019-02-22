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

	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class WindowsModern sealed {
	public:
		WindowsModern(Platform::String ^name, IPuzzleCanvas ^icanvas, IPuzzleStatusBar ^ibar, IPuzzleTimer ^itimer);

		static PuzzleList^ GetPuzzleList();
		Puzzle^ GetCurrentPuzzle();
		void SetPreset(int i);
		IAsyncOperation<bool> ^NewGame();

		PresetList^ GetPresetList(bool includeCustom, int maximumDepth);
		bool IsCustomGame();
		Windows::Foundation::Collections::IVector<ConfigItem^>^ GetConfiguration();
		Platform::String^ SetConfiguration(Windows::Foundation::Collections::IVector<ConfigItem^>^ input);

		Platform::String^ GetRandomSeed();
		Platform::String^ GetGameDesc();
		Platform::String^ SetGameID(Platform::String ^newId);

		void GetMaxSize(int iw, int ih, int *ow, int *oh);
		Windows::Foundation::Size PrintGame(IPuzzleCanvas ^icanvas, bool addSolution);
		void Redraw(IPuzzleCanvas ^icanvas, IPuzzleStatusBar ^ibar, IPuzzleTimer ^timer, bool force);
		void UpdateTimer(IPuzzleCanvas ^icanvas, IPuzzleStatusBar ^ibar, IPuzzleTimer ^timer, float delta);
		IAsyncOperation<bool> ^LoadGameFromStorage(Platform::String ^name);
		IAsyncOperation<Platform::String^> ^LoadGameFromTemporary();
		IAsyncOperation<Platform::String^> ^LoadGameFromFile(Windows::Storage::StorageFile ^file);
		Platform::String ^LoadGameFromString(Platform::String ^saved);
		static IAsyncOperation<GameLaunchParameters^> ^LoadAndIdentify(Windows::Storage::StorageFile ^file);
		IAsyncOperation<bool> ^SaveGameToStorage(Platform::String ^name);
		IAsyncOperation<bool> ^SaveGameToFile(Windows::Storage::StorageFile ^file);
		Platform::String ^SaveGameToString();

		Platform::String ^Solve();
		bool CanPrint();
		bool CanSolve();
		void RestartGame();
		bool CanUndo();
		void Undo();
		bool JustPerformedUndo();
		bool JustPerformedRedo();
		bool CanRedo();
		void Redo();
		int GameWon();
		bool HasStatusbar();
		void ReloadColors();
		bool IsRightButtonDisabled();

		VirtualButtonCollection^ GetVirtualButtonBar();
		void SendClick(int x, int y, ButtonType type, ButtonState state);
		void SendKey(VirtualKey key, bool shift, bool control);
		void SetInputScale(float scale);

		static property VirtualKey ButtonMarkOn { VirtualKey get() { return (VirtualKey)BUTTON_MARK_ON; }}
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
		static Puzzle^ FromConstGame(const game *g);
		static char *ToChars(Platform::String ^input);
	
		void Destroy();
	};
}
