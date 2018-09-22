//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "pch.h"

using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml::Media::Imaging;

namespace PuzzleCommon
{
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw Platform::Exception::CreateException(hr);
		}
	}
	inline D2D1_COLOR_F ConvertToColorF(uint32 color)
	{
		UCHAR R = color >> 16;
		UCHAR G = (color >> 8) & 0xFF;
		UCHAR B = color & 0xFF;

		return D2D1::ColorF(R / 255.0f, G / 255.0f, B / 255.0f, 1.0f);
	}
	inline D2D1_COLOR_F ConvertToColorF(Windows::UI::Color color)
	{
		return D2D1::ColorF(color.R / 255.0f, color.G / 255.0f, color.B / 255.0f, color.A / 255.0f);
	}

	inline D2D1_RECT_F ConvertToRectF(Windows::Foundation::Rect source)
	{
		return D2D1::RectF(source.Left, source.Top, source.Right, source.Bottom);
	}

	inline D2D1_POINT_2F ConvertToPoint(Windows::Foundation::Point point)
	{
		return D2D1::Point2F(point.X, point.Y);
	}

	ref class PuzzleImageSource sealed
	{
	public:
		PuzzleImageSource();

		bool BeginDraw(Windows::Foundation::Rect updateRect);
		bool BeginDraw()    { return BeginDraw(Windows::Foundation::Rect(0, 0, (float)m_width, (float)m_height)); }
		void EndDraw();

		void Clear(Windows::UI::Color color);
		void FillSolidRect(uint32 color, Windows::Foundation::Rect rect);
		void DrawLine(uint32 color, float x1, float y1, float x2, float y2, float thick);
		void DrawEllipse(uint32 outline, uint32 fill, float x, float y, float rw, float rh);
		void DrawGeometry(uint32 outline, uint32 fill, IVector<Windows::Foundation::Point> ^points);
		void DrawText(uint32 color, int x, int y, float size, GameFontType fonttype, GameFontHAlign halign, GameFontVAlign valign, Platform::String ^text);
		void StartClip(Windows::Foundation::Rect rect);
		void EndClip();

		void BlitterNew(int id, uint32 w, uint32 h);
		void BlitterFree(int id);
		void BlitterSave(int id, int x, int y, int w, int h);
		void BlitterLoad(int id, int x, int y, int w, int h);

		SurfaceImageSource ^CreateImageSource(int w, int h);

	private protected:
		void OnSuspending(Platform::Object ^sender, Windows::ApplicationModel::SuspendingEventArgs ^e);
		void CreateDeviceResources();
		void LoadFontMetrics(Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat, DWRITE_FONT_METRICS *target);

		SurfaceImageSource ^m_imageSource;

		// Direct3D device
		Microsoft::WRL::ComPtr<ID3D11Device>            m_d3dDevice;

		// Direct2D objects
		Microsoft::WRL::ComPtr<ID2D1Device>             m_d2dDevice;
		Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> m_d2dContext;
		Microsoft::WRL::ComPtr<ID2D1DeviceContext>      m_d2dBufferContext;
		Microsoft::WRL::ComPtr<ID2D1Factory1>           m_d2dFactory;

		// DirectWrite objects
		Microsoft::WRL::ComPtr<IDWriteFactory>          m_dwriteFactory;
		DWRITE_FONT_METRICS                             m_variableFontMetrics;
		DWRITE_FONT_METRICS                             m_fixedFontMetrics;

		std::unordered_map<int, Microsoft::WRL::ComPtr<ID2D1Bitmap>> m_blitters;

		int                                             m_width;
		int                                             m_height;
		bool                                            m_clipActive;
	};
}
