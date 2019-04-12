#pragma once
#include "pch.h"

using namespace Windows::Foundation::Collections;

namespace PuzzleCommon
{
	public enum class GameFontType
	{
		VariableWidth,
		FixedWidth
	};

	public enum class GameFontVAlign { VerticalBase, VerticalCentre };
	public enum class GameFontHAlign { HorizontalLeft, HorizontalCenter, HorizontalRight };

	/// <summary>
	/// Interface for the drawing API. Each implementation of this interface represents a single drawing surface.
	/// </summary>
	public interface class IPuzzleCanvas
	{
		void AddColor(float r, float g, float b);
		void RemoveColors();
		void SetPrintMode(bool printing);
		bool StartDraw();
		void EndDraw();
		void UpdateArea(int x, int y, int w, int h);

		void StartClip(int x, int y, int w, int h);
		void EndClip();
		void DrawRect(int x, int y, int w, int h, int color);
		void DrawCircle(int x, int y, int r, int fill, int outline);
		void DrawLine(int x1, int y1, int x2, int y2, int color);
		void DrawText(double x, double y, GameFontType fonttype, GameFontHAlign halign, GameFontVAlign valign, int size, int color, Platform::String ^text);
		void DrawPolygon(IVector<Windows::Foundation::Point> ^points, int fill, int outline);
		void ClearAll();

		void SetLineWidth(float width);
		void SetLineDotted(bool dotted);

		void BlitterNew(int id, int w, int h);
		void BlitterFree(int id);
		void BlitterSave(int id, int x, int y, int w, int h);
		void BlitterLoad(int id, int x, int y, int w, int h);
	};

	/// <summary>
	/// Callbacks for updating a status bar in the UI.
	/// </summary>
	public interface class IPuzzleStatusBar
	{
		/// <summary>
		/// Replace the status bar text.
		/// </summary>
		/// <param name="status">The new status.</param>
		void UpdateStatusBar(Platform::String ^status);
	};

	/// <summary>
	/// Callbacks for starting and stopping a refresh timer.
	/// </summary>
	public interface class IPuzzleTimer
	{
		/// <summary>
		/// Start sending tick events to the midend.
		/// </summary>
		void StartTimer();

		/// <summary>
		/// Stop sending tick events.
		/// </summary>
		void EndTimer();
	};
}