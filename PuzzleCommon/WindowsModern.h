#pragma once
#include "pch.h"

extern "C" {
#include "../../puzzles.h"
}

#include "PuzzleData.h"
#include "IPuzzleCanvas.h"
#include <ppltasks.h>

namespace PuzzleModern
{
	struct write_save_context
	{
		char *buf;
		int pos;
		int len;
	};

	class WindowsModern sealed {
	public:
		WindowsModern();
		bool CreateForGame(Platform::String ^name, IPuzzleCanvas ^icanvas, IPuzzleStatusBar ^ibar, IPuzzleTimer ^itimer);
		~WindowsModern();

		static PuzzleList^ GetPuzzleList();
		Puzzle^ GetCurrentPuzzle();
		void SetPreset(int i);
		void NewGame(Windows::Foundation::IAsyncAction^ workItem);

		PresetList^ GetPresetList(bool includeCustom, int maximumDepth);
		Platform::String ^GetCurrentPresetName(PresetList ^list);
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
		concurrency::task<bool> LoadGameFromStorage(Platform::String ^name);
		concurrency::task<Platform::String^> LoadGameFromTemporary();
		concurrency::task<Platform::String^> LoadGameFromFile(Windows::Storage::StorageFile ^file);
		Platform::String ^LoadGameFromString(Platform::String ^saved);
		static concurrency::task<GameLaunchParameters^> LoadAndIdentify(Windows::Storage::StorageFile ^file);
		concurrency::task<bool> SaveGameToStorage(Platform::String ^name);
		concurrency::task<bool> SaveGameToFile(Windows::Storage::StorageFile ^file);
		Platform::String ^SaveGameToString();

		Platform::String ^Solve();
		bool CanPrint();
		bool CanSolve();
		void RestartGame();
		bool CanUndo();
		void Undo();
		bool JustPerformedUndo();
		bool CanRedo();
		void Redo();
		int GameWon();
		bool HasStatusbar();
		void ReloadColors();
		bool IsRightButtonDisabled();

		VirtualButtonCollection^ GetVirtualButtonBar();
		void SendClick(int x, int y, ButtonType type, ButtonState state);
		void SendKey(Windows::System::VirtualKey key, bool shift, bool control);
		void SetInputScale(float scale);

		static const Windows::System::VirtualKey ButtonMarkOn = (Windows::System::VirtualKey)BUTTON_MARK_ON;
		static const Windows::System::VirtualKey ButtonMarkOff = (Windows::System::VirtualKey)BUTTON_MARK_OFF;

	private:
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
