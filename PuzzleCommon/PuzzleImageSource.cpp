//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "pch.h"
#include "IPuzzleCanvas.h"
#include "windows.ui.xaml.media.dxinterop.h"

#include "PuzzleImageSource.h"

using namespace Platform;
using namespace Microsoft::WRL;
using namespace Windows::ApplicationModel;
using namespace Windows::Graphics::Display;
using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::Foundation::Collections;

namespace PuzzleCommon
{
	PuzzleImageSource::PuzzleImageSource()
	{
		Application::Current->Suspending += ref new SuspendingEventHandler(this, &PuzzleImageSource::OnSuspending);
	}

	SurfaceImageSource ^PuzzleImageSource::CreateImageSource(int pixelWidth, int pixelHeight)
	{
		m_imageSource = ref new SurfaceImageSource(pixelWidth, pixelHeight, true);
		m_width = pixelWidth;
		m_height = pixelHeight;
		m_clipActive = false;
		CreateDeviceResources();
		return m_imageSource;
	}

	// Initialize hardware-dependent resources.
	// Initialize hardware-dependent resources.
	void PuzzleImageSource::CreateDeviceResources()
	{

		// This flag adds support for surfaces with a different color channel ordering
		// than the API default. It is required for compatibility with Direct2D.
		UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)    
		// If the project is in a debug build, enable debugging via SDK Layers.
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		// This array defines the set of DirectX hardware feature levels this app will support.
		// Note the ordering should be preserved.
		// Don't forget to declare your application's minimum required feature level in its
		// description.  All applications are assumed to support 9.1 unless otherwise stated.
		const D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1,
		};

		D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			__uuidof(ID2D1Factory1),
			nullptr,
			&m_d2dFactory
			);

		// Create the Direct3D 11 API device object.
		ThrowIfFailed(
			D3D11CreateDevice(
			nullptr,                        // Specify nullptr to use the default adapter.
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			creationFlags,                  // Set debug and Direct2D compatibility flags.
			featureLevels,                  // List of feature levels this app can support.
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,              // Always set this to D3D11_SDK_VERSION for Windows Store apps.
			&m_d3dDevice,                   // Returns the Direct3D device created.
			nullptr,
			nullptr
			)
			);

		// Get the Direct3D 11.1 API device.
		ComPtr<IDXGIDevice> dxgiDevice;
		ThrowIfFailed(
			m_d3dDevice.As(&dxgiDevice)
			);

		// Create the Direct2D device object and a corresponding context.
		ThrowIfFailed(
			m_d2dFactory->CreateDevice(
			dxgiDevice.Get(),
			&m_d2dDevice
			)
			);

		ThrowIfFailed(
			m_d2dDevice->CreateDeviceContext(
			D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
			&m_d2dBufferContext
			)
			);

		ThrowIfFailed(
			DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			&m_dwriteFactory
			)
			);

		// Query for ISurfaceImageSourceNative interface.
		Microsoft::WRL::ComPtr<ISurfaceImageSourceNative> sisNative;
		ThrowIfFailed(
			reinterpret_cast<IUnknown*>(m_imageSource)->QueryInterface(IID_PPV_ARGS(&sisNative))
			);

		// Associate the DXGI device with the SurfaceImageSource.
		ThrowIfFailed(
			sisNative->SetDevice(dxgiDevice.Get())
			);

		m_d2dContext = nullptr;
		m_nextBlitterId = 0;
		m_blitters.clear();
	}

	void PuzzleImageSource::LoadFontMetrics(ComPtr<IDWriteTextFormat> textFormat, DWRITE_FONT_METRICS *target)
	{
		IDWriteFontCollection* collection;
		TCHAR name[64]; UINT32 findex; BOOL exists;
		textFormat->GetFontFamilyName(name, 64);
		textFormat->GetFontCollection(&collection);
		collection->FindFamilyName(name, &findex, &exists);
		IDWriteFontFamily *ffamily;
		collection->GetFontFamily(findex, &ffamily);
		IDWriteFont* font;
		ffamily->GetFirstMatchingFont(textFormat->GetFontWeight(), textFormat->GetFontStretch(), textFormat->GetFontStyle(), &font);
		font->GetMetrics(target);
	}

	int PuzzleImageSource::BlitterNew(uint32 w, uint32 h)
	{
		int id = m_nextBlitterId++;
		return id;
	}

	void PuzzleImageSource::BlitterSave(int id, int x, int y, int w, int h)
	{
		if (!m_blitters[id])
		{
			ComPtr<ID2D1Bitmap> bitmap;

			auto size = D2D1::SizeU(w, h);
			D2D1_BITMAP_PROPERTIES props;
			props.pixelFormat = m_d2dBufferContext->GetPixelFormat();
			m_d2dBufferContext->GetDpi(&props.dpiX, &props.dpiY);
			ThrowIfFailed(
				m_d2dBufferContext->CreateBitmap(size, nullptr, 0, props, &bitmap)
				);

			m_blitters[id] = bitmap;
		}

		auto rect = D2D1::RectU(x, y, x + w, y + h);

		m_blitters[id]->CopyFromRenderTarget(nullptr, m_d2dContext.Get(), &rect);
	}
	void PuzzleImageSource::BlitterLoad(int id, int x, int y, int w, int h)
	{
		auto rect = D2D1::RectF(x, y, x + w, y + h);
		auto srcRect = D2D1::RectF(0, 0, w, h);
		m_d2dContext->DrawBitmap(m_blitters[id].Get(), &rect, 1.0, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &srcRect);
	}

	void PuzzleImageSource::BlitterFree(int id)
	{
		m_blitters.erase(id);
	}

	// Begins drawing, allowing updates to content in the specified area.
	bool PuzzleImageSource::BeginDraw(Windows::Foundation::Rect updateRect)
	{
		POINT offset;
		ComPtr<IDXGISurface> surface;

		// Express target area as a native RECT type.
		RECT updateRectNative;
		updateRectNative.left = static_cast<LONG>(updateRect.Left);
		updateRectNative.top = static_cast<LONG>(updateRect.Top);
		updateRectNative.right = static_cast<LONG>(updateRect.Right);
		updateRectNative.bottom = static_cast<LONG>(updateRect.Bottom);

		// Query for ISurfaceImageSourceNative interface.
		Microsoft::WRL::ComPtr<ISurfaceImageSourceNative> sisNative;
		ThrowIfFailed(
			reinterpret_cast<IUnknown*>(m_imageSource)->QueryInterface(IID_PPV_ARGS(&sisNative))
			);

		// Begin drawing - returns a target surface and an offset to use as the top left origin when drawing.
		HRESULT beginDrawHR = sisNative->BeginDraw(updateRectNative, &surface, &offset);

		if (SUCCEEDED(beginDrawHR))
		{
			Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;

			ThrowIfFailed(
				m_d2dBufferContext->CreateBitmapFromDxgiSurface(
				surface.Get(),
				nullptr,
				&bitmap
				)
				);
			// Begin drawing using D2D context.
			m_d2dBufferContext->BeginDraw();

			// Set context's render target.
			m_d2dBufferContext->SetTarget(bitmap.Get());

			if (!m_d2dContext)
			{
				m_d2dBufferContext->CreateCompatibleRenderTarget(&m_d2dContext);
			}

			auto clip = D2D1::RectF(
				static_cast<float>(offset.x),
				static_cast<float>(offset.y),
				static_cast<float>(offset.x + updateRect.Width),
				static_cast<float>(offset.y + updateRect.Height)
				);
			auto translation = D2D1::Matrix3x2F::Translation(
				static_cast<float>(offset.x),
				static_cast<float>(offset.y)
				);

			// Apply a clip and transform to constrain updates to the target update area.
			// This is required to ensure coordinates within the target surface remain
			// consistent by taking into account the offset returned by BeginDraw, and
			// can also improve performance by optimizing the area that is drawn by D2D.
			// Apps should always account for the offset output parameter returned by 
			// BeginDraw, since it may not match the passed updateRect input parameter's location.
			m_d2dBufferContext->PushAxisAlignedClip(clip,
				D2D1_ANTIALIAS_MODE_ALIASED
				);

			m_d2dBufferContext->SetTransform(translation);

			m_d2dContext->BeginDraw();
			return true;
		}
		else if (beginDrawHR == DXGI_ERROR_DEVICE_REMOVED || beginDrawHR == DXGI_ERROR_DEVICE_RESET)
		{
			// If the device has been removed or reset, attempt to recreate it and continue drawing.
			CreateDeviceResources();
			return BeginDraw(updateRect);
		}
		else
		{
			OutputDebugString(("Skipping BeginDraw " + beginDrawHR.ToString() + "\n")->Data());
			return false;
		}
	}

	// Ends drawing updates started by a previous BeginDraw call.
	void PuzzleImageSource::EndDraw()
	{
		ThrowIfFailed(
			m_d2dContext->EndDraw()
			);
		
		ComPtr<ID2D1Bitmap> bitmap;
		m_d2dContext->GetBitmap(&bitmap);

		m_d2dBufferContext->DrawBitmap(bitmap.Get());

		// Remove the transform and clip applied in BeginDraw since
		// the target area can change on every update.
		m_d2dBufferContext->SetTransform(D2D1::IdentityMatrix());
		m_d2dBufferContext->PopAxisAlignedClip();

		// Remove the render target and end drawing.
		ThrowIfFailed(
			m_d2dBufferContext->EndDraw()
			);

		m_d2dBufferContext->SetTarget(nullptr);
		// Query for ISurfaceImageSourceNative interface.
		Microsoft::WRL::ComPtr<ISurfaceImageSourceNative> sisNative;
		ThrowIfFailed(
			reinterpret_cast<IUnknown*>(m_imageSource)->QueryInterface(IID_PPV_ARGS(&sisNative))
			);

		ThrowIfFailed(
			sisNative->EndDraw()
			);
	}

	// Clears the background with the given color.
	void PuzzleImageSource::Clear(Windows::UI::Color color)
	{
		m_d2dContext->Clear(ConvertToColorF(color));
	}

	// Draws a filled rectangle with the given color and position.
	void PuzzleImageSource::FillSolidRect(uint32 color, Windows::Foundation::Rect rect)
	{
		// Create a solid color D2D brush.
		ComPtr<ID2D1SolidColorBrush> brush;
		ThrowIfFailed(
			m_d2dContext->CreateSolidColorBrush(
			ConvertToColorF(color),
			&brush
			)
			);

		// Draw a filled rectangle.
		m_d2dContext->FillRectangle(ConvertToRectF(rect), brush.Get());
	}

	void PuzzleImageSource::DrawLine(uint32 color, float x1, float y1, float x2, float y2, float thick)
	{
		ComPtr<ID2D1SolidColorBrush> brush;
		ThrowIfFailed(
			m_d2dContext->CreateSolidColorBrush(
			ConvertToColorF(color),
			&brush
			)
			);

		auto p1 = D2D1::Point2F(x1, y1);
		auto p2 = D2D1::Point2F(x2, y2);

		m_d2dContext->DrawLine(p1, p2, brush.Get(), thick);
	}

	void PuzzleImageSource::DrawEllipse(uint32 outline, uint32 fill, float x, float y, float rw, float rh)
	{
		auto ellipse = D2D1::Ellipse(
			D2D1::Point2F(x, y),
			rw, rh
			);

		if (outline != ~0)
		{
			ComPtr<ID2D1SolidColorBrush> outlineBrush;
			ThrowIfFailed(
				m_d2dContext->CreateSolidColorBrush(
				ConvertToColorF(outline),
				&outlineBrush
				)
				);

			m_d2dContext->DrawEllipse(ellipse, outlineBrush.Get());
		}
		if (fill != ~0)
		{
			ComPtr<ID2D1SolidColorBrush> fillBrush;
			ThrowIfFailed(
				m_d2dContext->CreateSolidColorBrush(
				ConvertToColorF(fill),
				&fillBrush
				)
				);

			m_d2dContext->FillEllipse(ellipse, fillBrush.Get());
		}
	}

	void PuzzleImageSource::DrawGeometry(uint32 outline, uint32 fill, IVector<Windows::Foundation::Point> ^points)
	{
		ComPtr<ID2D1PathGeometry> pathGeometry;
		ThrowIfFailed(
			m_d2dFactory->CreatePathGeometry(&pathGeometry)
			);

		ComPtr<ID2D1GeometrySink> geometrySink;
		ThrowIfFailed(
			pathGeometry->Open(&geometrySink)
			);

		geometrySink->SetFillMode(D2D1_FILL_MODE_WINDING);
		geometrySink->BeginFigure(ConvertToPoint(points->GetAt(points->Size - 1)), D2D1_FIGURE_BEGIN_FILLED);
		for (uint32 i = 0; i < points->Size; i++)
		{
			geometrySink->AddLine(ConvertToPoint(points->GetAt(i)));
		}
		geometrySink->EndFigure(D2D1_FIGURE_END_CLOSED);
		ThrowIfFailed(
			geometrySink->Close()
			);

		if (outline != ~0)
		{
			ComPtr<ID2D1SolidColorBrush> outlineBrush;
			ThrowIfFailed(
				m_d2dContext->CreateSolidColorBrush(
				ConvertToColorF(outline),
				&outlineBrush
				)
				);

			m_d2dContext->DrawGeometry(pathGeometry.Get(), outlineBrush.Get());
		}
		if (fill != ~0)
		{
			ComPtr<ID2D1SolidColorBrush> fillBrush;
			ThrowIfFailed(
				m_d2dContext->CreateSolidColorBrush(
				ConvertToColorF(fill),
				&fillBrush
				)
				);

			m_d2dContext->FillGeometry(pathGeometry.Get(), fillBrush.Get());
		}
	}

	void PuzzleImageSource::StartClip(Windows::Foundation::Rect rect)
	{
		if (m_clipActive)
			m_d2dContext->PopAxisAlignedClip();

		m_d2dContext->PushAxisAlignedClip(ConvertToRectF(rect), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		m_clipActive = true;
	}

	void PuzzleImageSource::EndClip()
	{
		if (m_clipActive)
			m_d2dContext->PopAxisAlignedClip();
		m_clipActive = false;
	}

	void PuzzleImageSource::DrawText(uint32 color, int x, int y, float size, GameFontType fonttype, GameFontHAlign halign, GameFontVAlign valign, Platform::String ^text)
	{
		ComPtr<IDWriteTextFormat> textFormat;

		ThrowIfFailed(
			m_dwriteFactory->CreateTextFormat(
			fonttype == GameFontType::VariableWidth ? L"Segoe UI" : L"Courier New",
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			size,
			L"en-us",
			&textFormat
			)
			);

		if (fonttype == GameFontType::VariableWidth && !m_variableFontMetrics.designUnitsPerEm)
			LoadFontMetrics(textFormat, &m_variableFontMetrics);
		else if (fonttype == GameFontType::FixedWidth && !m_fixedFontMetrics.designUnitsPerEm)
			LoadFontMetrics(textFormat, &m_fixedFontMetrics);

		DWRITE_FONT_METRICS *metrics = fonttype == GameFontType::VariableWidth ? &m_variableFontMetrics : &m_fixedFontMetrics;

		ThrowIfFailed(
			textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING)
			);

		ComPtr<IDWriteTextLayout> textLayout;

		ThrowIfFailed(
			m_dwriteFactory->CreateTextLayout(
			text->Data(),
			text->Length(),
			textFormat.Get(),
			m_width,
			m_height,
			&textLayout
			)
			);
		
		ComPtr<ID2D1SolidColorBrush> textBrush;
		ThrowIfFailed(
			m_d2dContext->CreateSolidColorBrush(
			ConvertToColorF(color),
			&textBrush
			)
			);
		
		float fx = x, fy = y, width;

		if (valign == GameFontVAlign::VerticalBase)
			fy -= metrics->ascent * (size / metrics->designUnitsPerEm);
		else
			fy -= (metrics->ascent + metrics->descent) * (size / (2*metrics->designUnitsPerEm));
		textLayout->DetermineMinWidth(&width);
		if (halign == GameFontHAlign::HorizontalCenter)
			fx -= width / 2;
		else if (halign == GameFontHAlign::HorizontalRight)
			fx -= width;

		m_d2dContext->DrawTextLayout(
			D2D1::Point2F(fx, fy),
			textLayout.Get(),
			textBrush.Get(),
			D2D1_DRAW_TEXT_OPTIONS_NO_SNAP
			);
	}

	void PuzzleImageSource::OnSuspending(Object ^sender, SuspendingEventArgs ^e)
	{
		ComPtr<IDXGIDevice3> dxgiDevice;
		m_d3dDevice.As(&dxgiDevice);

		// Hints to the driver that the app is entering an idle state and that its memory can be used temporarily for other apps.
		dxgiDevice->Trim();
	}
}
