//
// Direct2DPuzzleCanvas.xaml.h
// Declaration of the Direct2DPuzzleCanvas class
//

#pragma once

#include "Direct2DPuzzleCanvas.g.h"
#include "PuzzleImageSource.h"


namespace PuzzleCommon
{
	public delegate void ForceRedrawDelegate();
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class Direct2DPuzzleCanvas sealed : IPuzzleCanvas
	{
	public:
		Direct2DPuzzleCanvas();
		void CreateSource(int w, int h);

		event ForceRedrawDelegate^ NeedsRedraw;

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

		virtual void BlitterNew(int id, int w, int h);
		virtual void BlitterFree(int id);
		virtual void BlitterSave(int id, int x, int y, int w, int h);
		virtual void BlitterLoad(int id, int x, int y, int w, int h);

		void Pulsate(int x, int y);
	private:
		PuzzleImageSource ^img;
		Windows::Foundation::Collections::IVector<uint32> ^colors;
		Windows::UI::Color firstColor;
		void OnSizeChanged(Platform::Object ^sender, Windows::UI::Xaml::SizeChangedEventArgs ^e);

		bool isDrawing;
		void OnPulsateCompleted(Platform::Object ^sender, Platform::Object ^args);
	};
}
