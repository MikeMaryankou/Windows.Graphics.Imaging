#pragma once
#include "Common.h"

namespace WIC
{
	using Microsoft::WRL::ComPtr;

	ComPtr<IWICBitmapDecoder> GetDecoder(const ComPtr<IWICImagingFactory>& factory, const ComPtr<IWICStream>& stream);

	ComPtr<IWICBitmapDecoder> GetDecoder(const ComPtr<IWICImagingFactory>& factory, const std::filesystem::path& path);

	EImageType GetImageType(const ComPtr<IWICBitmapDecoder>& decoder);
}