#pragma once
#include "Common.h"
#include "WIC.h"

namespace WIC
{
	using Microsoft::WRL::ComPtr;

	ComPtr<IWICBitmapEncoder> GetEncoder(const ComPtr<IWICStream>& stream, EImageType type);

	void SaveEncoderData(const ComPtr<IWICBitmapEncoder>& encoder, const Configuration& configuration);
}
