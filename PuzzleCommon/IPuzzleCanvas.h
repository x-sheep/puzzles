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

	public interface class IPuzzleStatusBar
	{
		void UpdateStatusBar(Platform::String ^status);
	};

	public interface class IPuzzleTimer
	{
		void StartTimer();
		void EndTimer();
	};
}