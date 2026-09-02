#pragma once

namespace WIC
{
	using Microsoft::WRL::ComPtr;

	ComPtr<IWICStream> GetStream(const ComPtr<IWICImagingFactory>& factory, const ComPtr<IStream>& buffer);

	ComPtr<IWICStream> GetStream(const ComPtr<IWICImagingFactory>& factory);

	ComPtr<IWICStream> GetStream(const ComPtr<IWICImagingFactory>& factory, const std::filesystem::path& source);

	ComPtr<IWICStream> GetStream(const ComPtr<IWICImagingFactory>& factory, std::vector<uint8_t>& source);
}