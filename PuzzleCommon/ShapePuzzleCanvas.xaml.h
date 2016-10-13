//
// GameCanvas.xaml.h
// Declaration of the GameCanvas class
//

#pragma once

#include <collection.h>
#include "ShapePuzzleCanvas.g.h"
#include "IPuzzleCanvas.h"

using namespace Windows::Foundation::Collections;

namespace PuzzleModern
{
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class ShapePuzzleCanvas sealed : IPuzzleCanvas
	{
	public:
		ShapePuzzleCanvas();

		Windows::UI::Xaml::Media::Brush^ GetFirstColor();
		
		virtual void AddColor(float r, float g, float b);
		virtual void RemoveColors();
		virtual void SetPrintMode(bool printing);

		virtual bool StartDraw();
		virtual void EndDraw();
		virtual void UpdateArea(int x, int y, int w, int h);

		virtual void StartClip(int x, int y, int w, int h);
		virtual void EndClip();
		virtual void DrawRect(int x, int y, int w, int h, int color);
		virtual void DrawCircle(int x, int y, int r, int fill, int outline);
		virtual void DrawLine(int x1, int y1, int x2, int y2, int color);
		virtual void DrawText(double x, double y, GameFontType fonttype, GameFontHAlign halign, GameFontVAlign valign, int size, int color, Platform::String ^text);
		virtual void DrawPolygon(IVector<Windows::Foundation::Point> ^points, int fill, int outline);
		virtual void ClearAll();

		virtual void SetLineWidth(float width);
		virtual void SetLineDotted(bool dotted);

		virtual int BlitterNew(int w, int h);
		virtual void BlitterFree(int id);
		virtual void BlitterSave(int id, int x, int y, int w, int h);
		virtual void BlitterLoad(int id, int x, int y, int w, int h);

	private:
		bool _printMode;
		bool _lineDotted;
		float _lineWidth;
		Windows::UI::Xaml::Media::Brush^ GetColor(int index);
		Windows::UI::Xaml::Controls::Canvas ^currentDrawCanvas;
		Windows::UI::Xaml::Controls::Canvas ^activeCanvas;
		Windows::Foundation::Collections::IVector<Windows::UI::Xaml::Media::Brush^> ^brushes;
	};
}
