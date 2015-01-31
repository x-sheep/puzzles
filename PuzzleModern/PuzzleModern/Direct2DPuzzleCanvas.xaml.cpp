//
// Direct2DPuzzleCanvas.xaml.cpp
// Implementation of the Direct2DPuzzleCanvas class
//

#include "pch.h"
#include "Direct2DPuzzleCanvas.xaml.h"

using namespace PuzzleModern;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

Direct2DPuzzleCanvas::Direct2DPuzzleCanvas()
{
	InitializeComponent();

	firstColor = Color();
	firstColor.A = 0;
	colors = ref new Vector<uint32>();

	this->SizeChanged += ref new Windows::UI::Xaml::SizeChangedEventHandler(this, &PuzzleModern::Direct2DPuzzleCanvas::OnSizeChanged);

	img = ref new PuzzleImageSource();
	CreateSource(320, 320);
}

void Direct2DPuzzleCanvas::CreateSource(int w, int h)
{
	if (isDrawing)
		img->EndDraw();

	float scale = Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi / 96;
	float invscale = 1 / scale;

	DrawImage->Source = img->CreateImageSource(ceil(w*scale), ceil(h*scale));
	
	auto transform = ref new ScaleTransform();
	transform->ScaleX = invscale;
	transform->ScaleY = invscale;
	DrawImage->RenderTransform = transform;

	DrawImage->Width = w*scale;
	DrawImage->Height = h*scale;

	img->BeginDraw();
	img->Clear(firstColor);
	img->EndDraw();
	
	isDrawing = false;
}

Windows::UI::Xaml::Media::Brush^ Direct2DPuzzleCanvas::GetFirstColor()
{
	return ref new SolidColorBrush(firstColor);
}

void Direct2DPuzzleCanvas::AddColor(float r, float g, float b)
{
	unsigned char rc, gc, bc;
	rc = (unsigned char)(255 * r);
	gc = (unsigned char)(255 * g);
	bc = (unsigned char)(255 * b);

	if (!firstColor.A)
	{
		firstColor = Color();
		firstColor.R = rc;
		firstColor.G = gc;
		firstColor.B = bc;
		firstColor.A = 255;
	}

	colors->Append((rc << 16) | (gc << 8) | bc);
}

void Direct2DPuzzleCanvas::SetPrintMode(bool printing)
{
	if (printing)
		throw ref new NotImplementedException("This canvas does not support printing.");
}

void Direct2DPuzzleCanvas::StartDraw()
{
	if (!isDrawing)
		img->BeginDraw();
	isDrawing = true;
}

void Direct2DPuzzleCanvas::EndDraw()
{
	if (isDrawing)
		img->EndDraw();
	isDrawing = false;
}

void Direct2DPuzzleCanvas::UpdateArea(int x, int y, int w, int h)
{
}

void Direct2DPuzzleCanvas::StartClip(int x, int y, int w, int h)
{
	if (x >= 0 && y >= 0 && w > 0 && h > 0)
		img->StartClip(Windows::Foundation::Rect(x, y, w, h));
}

void Direct2DPuzzleCanvas::EndClip()
{
	img->EndClip();
}

void Direct2DPuzzleCanvas::DrawRect(int x, int y, int w, int h, int color)
{
	if (w < 1 || h < 1)
		return;

	img->FillSolidRect(colors->GetAt(color), Rect(x, y, w, h));
}

void Direct2DPuzzleCanvas::DrawCircle(int x, int y, int r, int fill, int outline)
{
	img->DrawEllipse(
		outline == -1 ? ~0 : colors->GetAt(outline),
		fill == -1 ? ~0 : colors->GetAt(fill),
		x, y, r, r);
}
void Direct2DPuzzleCanvas::DrawLine(int x1, int y1, int x2, int y2, int color)
{
	img->DrawLine(colors->GetAt(color), x1, y1, x2, y2, 1);
}


void Direct2DPuzzleCanvas::DrawText(double x, double y, GameFontType fonttype, GameFontHAlign halign, GameFontVAlign valign, int size, int color, Platform::String ^text)
{
	if (size <= 0) return;
	if (text->Length() == 0) return;

	img->DrawText(colors->GetAt(color), x, y, size, fonttype, halign, valign, text);
}

void Direct2DPuzzleCanvas::DrawPolygon(IVector<Windows::Foundation::Point> ^points, int fill, int outline)
{
	img->DrawGeometry(
		outline == -1 ? ~0 : colors->GetAt(outline),
		fill == -1 ? ~0 : colors->GetAt(fill),
		points);
}

void Direct2DPuzzleCanvas::ClearAll(){}

void Direct2DPuzzleCanvas::SetLineWidth(float width){}
void Direct2DPuzzleCanvas::SetLineDotted(bool dotted){}

int Direct2DPuzzleCanvas::BlitterNew(int w, int h)
{
	return img->BlitterNew(w, h);
}
void Direct2DPuzzleCanvas::BlitterFree(int id)
{
	img->BlitterFree(id);
}
void Direct2DPuzzleCanvas::BlitterSave(int id, int x, int y, int w, int h)
{
	img->BlitterSave(id, x, y, w, h);
}

void Direct2DPuzzleCanvas::BlitterLoad(int id, int x, int y, int w, int h)
{
	img->BlitterLoad(id, x, y, w, h);
}

void PuzzleModern::Direct2DPuzzleCanvas::OnSizeChanged(Platform::Object ^sender, Windows::UI::Xaml::SizeChangedEventArgs ^e)
{
	if (isDrawing)
	{
		img->EndDraw();
		isDrawing = false;
	}

	CreateSource(ActualWidth, ActualHeight);
	NeedsRedraw();
}