//
// ShapePuzzleCanvas.xaml.cpp
// Implementation of the ShapePuzzleCanvas class
//

#include "pch.h"
#include "ShapePuzzleCanvas.xaml.h"

using namespace PuzzleCommon;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Xaml::Shapes;

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

ShapePuzzleCanvas::ShapePuzzleCanvas()
{
	InitializeComponent();
	brushes = ref new Vector<Brush^>();
	currentDrawCanvas = activeCanvas = nullptr;
}

Windows::UI::Xaml::Media::Brush^ ShapePuzzleCanvas::GetColor(int index)
{
	return brushes->GetAt(index);
}

void ShapePuzzleCanvas::RemoveColors()
{
	brushes->Clear();
}

void ShapePuzzleCanvas::AddColor(float r, float g, float b)
{
	Windows::UI::Color color = Windows::UI::Color();
	color.R = (unsigned char)(255 * r);
	color.G = (unsigned char)(255 * g);
	color.B = (unsigned char)(255 * b);
	color.A = 255;
	Brush ^brush = ref new SolidColorBrush(color);
	brushes->Append(brush);
}

void ShapePuzzleCanvas::SetPrintMode(bool printing)
{
	_printMode = printing;
}

Windows::UI::Xaml::Media::Brush^ ShapePuzzleCanvas::GetFirstColor()
{
	if (brushes->Size > 0)
		return brushes->GetAt(0);
	return nullptr;
}

void ShapePuzzleCanvas::ClearAll()
{
	currentDrawCanvas = activeCanvas = nullptr;
	RootCanvas->Children->Clear();
}

void ShapePuzzleCanvas::StartClip(int x, int y, int w, int h)
{
	if (w <= 0 || h <= 0)
		return;

	Canvas ^clipCanvas = ref new Canvas();
	clipCanvas->Width = RootCanvas->ActualWidth;
	clipCanvas->Height = RootCanvas->ActualHeight;
	auto clip = ref new RectangleGeometry();
	clip->Rect = Rect((float)x, (float)y, (float)w, (float)h);
	clipCanvas->Clip = clip;

	currentDrawCanvas->Children->Append(clipCanvas);
	activeCanvas = clipCanvas;
}

void ShapePuzzleCanvas::EndClip()
{
	activeCanvas = currentDrawCanvas;
}

void ShapePuzzleCanvas::DrawRect(int x, int y, int w, int h, int color)
{
	if (w <= 0 || h <= 0)
		return;

	Rectangle ^rect = ref new Rectangle();
	rect->Fill = GetColor(color);
	rect->Margin = Thickness(x, y, 0, 0);
	rect->Width = w;
	rect->Height = h;
	activeCanvas->Children->Append(rect);
}

void ShapePuzzleCanvas::DrawCircle(int x, int y, int r, int fill, int outline)
{
	if (r <= 0)
		return;

	Ellipse ^ell = ref new Ellipse();
	ell->Width = r*2;
	ell->Height = r*2;
	ell->Margin = Thickness(x-r, y-r, 0, 0);
	if (fill >= 0)
		ell->Fill = GetColor(fill);
	ell->Stroke = GetColor(outline);

	if (_lineDotted)
	{
		auto strokeDots = ref new DoubleCollection();
		strokeDots->Append(1);
		strokeDots->Append(1);
		ell->StrokeDashArray = strokeDots;
	}

	ell->StrokeThickness = _lineWidth;
	
	activeCanvas->Children->Append(ell);
}

void ShapePuzzleCanvas::DrawLine(int x1, int y1, int x2, int y2, int color)
{
	Line ^line = ref new Line();
	line->Margin = Thickness(0, 0, 0, 0);
	line->X1 = x1;
	line->X2 = x2;
	line->Y1 = y1;
	line->Y2 = y2;
	line->Stroke = GetColor(color);
	line->StrokeThickness = _lineWidth;
	
	if (_lineDotted)
	{
		auto strokeDots = ref new DoubleCollection();
		strokeDots->Append(1);
		strokeDots->Append(1);
		line->StrokeDashArray = strokeDots;
	}

	activeCanvas->Children->Append(line);
}

void ShapePuzzleCanvas::DrawText(double x, double y, GameFontType fonttype, GameFontHAlign halign, GameFontVAlign valign, int size, int color, Platform::String ^text)
{
	if (size <= 0)
		return;

	TextBlock ^tb = ref new TextBlock();
	tb->FontSize = size;
	tb->Text = text;
	tb->Foreground = GetColor(color);
	tb->TextWrapping = TextWrapping::NoWrap;
	tb->TextTrimming = TextTrimming::None;

	if (fonttype == GameFontType::FixedWidth)
		tb->FontFamily = ref new Windows::UI::Xaml::Media::FontFamily("Consolas");

	double width = size * text->Length() * 2;
	tb->Width = width;
	if (halign == GameFontHAlign::HorizontalCenter)
	{
		x -= width / 2;
		tb->TextAlignment = TextAlignment::Center;
	}
	else if (halign == GameFontHAlign::HorizontalRight)
	{
		x -= width;
		tb->TextAlignment = TextAlignment::Right;
	}

	if (valign == GameFontVAlign::VerticalCentre)
		y -= size / 2;
	else
		y -= size;
	
	tb->Margin = Thickness(x, y, 0, 0);

	activeCanvas->Children->Append(tb);
}

void ShapePuzzleCanvas::DrawPolygon(IVector<Point> ^points, int fill, int outline)
{
	Polygon ^poly = ref new Polygon();

	PointCollection ^pc = ref new PointCollection();
	for each (Point p in points)
	{
		pc->Append(p);
	}

	poly->Points = pc;

	if (fill >= 0)
		poly->Fill = GetColor(fill);
	poly->Stroke = GetColor(outline);
	poly->StrokeThickness = _lineWidth;

	if (_lineDotted)
	{
		auto strokeDots = ref new DoubleCollection();
		strokeDots->Append(1);
		strokeDots->Append(1);
		poly->StrokeDashArray = strokeDots;
	}

	activeCanvas->Children->Append(poly);
}

void ShapePuzzleCanvas::UpdateArea(int x, int y, int w, int h)
{
	// FIXME: Remove all geometry inside the given area...
}

bool ShapePuzzleCanvas::StartDraw()
{
	Canvas ^drawCanvas = ref new Canvas();
	drawCanvas->Width = RootCanvas->ActualWidth;
	drawCanvas->Height = RootCanvas->ActualHeight;
	_lineWidth = 1;
	_lineDotted = false;

	currentDrawCanvas = activeCanvas = drawCanvas;
	return true;
}

void ShapePuzzleCanvas::EndDraw()
{
	RootCanvas->Children->Append(currentDrawCanvas);
	currentDrawCanvas = activeCanvas = nullptr;
}

void ShapePuzzleCanvas::SetLineWidth(float width)
{
	_lineWidth = width;
}
void ShapePuzzleCanvas::SetLineDotted(bool dotted)
{
	_lineDotted = dotted;
}

int ShapePuzzleCanvas::BlitterNew(int w, int h)
{
	return 0;
}
void ShapePuzzleCanvas::BlitterFree(int id){}
void ShapePuzzleCanvas::BlitterSave(int id, int x, int y, int w, int h){}
void ShapePuzzleCanvas::BlitterLoad(int id, int x, int y, int w, int h){}
