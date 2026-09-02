#include "pch.h"
#include "Encoder.h"
#include "Common.h"

using Microsoft::WRL::ComPtr;

namespace
{
	std::map<WIC::EImageType, IID> GetImageTypeEncoderDictionary()
	{
		static const std::map<WIC::EImageType, IID> g_imageTypeEncoderDictionary =
		{
			{WIC::EImageType::Bmp, CLSID_WICBmpEncoder},
			{WIC::EImageType::Gif, CLSID_WICGifEncoder},
			{WIC::EImageType::Jpeg, CLSID_WICJpegEncoder},
			{WIC::EImageType::Png, CLSID_WICPngEncoder},
			{WIC::EImageType::Tiff, CLSID_WICTiffEncoder},
		};

		return g_imageTypeEncoderDictionary;
	}


	void SetImageQuality(const int imageQuality, const ComPtr<IPropertyBag2>& properties)
	{
		const float imageQualityValue = std::truncf(static_cast<float>(imageQuality)) / 100;
		VARIANT value;
		VariantInit(&value);
		value.vt = VT_R8;
		value.dblVal = static_cast<double>(imageQualityValue);

		PROPBAG2 data{};
		data.dwType = PROPBAG2_TYPE_DATA;
		data.vt = VT_R8;
		data.pstrName = const_cast<LPOLESTR>(L"ImageQuality");

		const HRESULT res = properties->Write(1, &data, &value);

		std::ignore = VariantClear(&value);

		if (FAILED(res))
		{
			throw WIC::Exception(WIC::GetErrorMsg("IPropertyBag2::Write", res));
		}
	}
}


ComPtr<IWICBitmapEncoder> WIC::GetEncoder(const ComPtr<IWICStream>& stream, const EImageType type)
{
	ComPtr<IWICBitmapEncoder> encoder = nullptr;

	const auto& dictionary = GetImageTypeEncoderDictionary();

	if (const HRESULT res = ::CoCreateInstance(dictionary.at(type), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(encoder.GetAddressOf()));
		FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapEncoder::CoCreateInstance", res));
	}

	if (const HRESULT res = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache); FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapEncoder::Initialize", res));
	}

	return encoder;
}


void WIC::SaveEncoderData(const ComPtr<IWICBitmapEncoder>& encoder, const Configuration& configuration)
{
	if (configuration.destination == nullptr)
	{
		return;
	}
	
	ComPtr<IPropertyBag2> properties;
	ComPtr<IWICBitmapFrameEncode> targetFrame;

	if (const HRESULT res = encoder->CreateNewFrame(targetFrame.GetAddressOf(), properties.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapEncoder::CreateNewFrame", res));
	}

	if (configuration.grayscale)
	{
		auto grayScalePixelFormat = GUID_WICPixelFormat32bppGrayFixedPoint;

		if (const HRESULT res = targetFrame->SetPixelFormat(&grayScalePixelFormat); FAILED(res))
		{
			throw Exception(GetErrorMsg("BitmapFrameEncode::SetPixelFormat", res));
		}
	}

	if (configuration.type == EImageType::Jpeg && configuration.quality > 0 && configuration.quality <= 100)
	{
		SetImageQuality(configuration.quality, properties);
	}

	if (const HRESULT res = targetFrame->Initialize(properties.Get()); FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapFrameEncode::Initialize", res));
	}

	if (const HRESULT res = targetFrame->WriteSource(configuration.destination, nullptr); FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapFrameEncode::WriteSource", res));
	}

	if (const HRESULT res = targetFrame->Commit(); FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapFrameEncode::Commit", res));
	}

	if (const HRESULT res = encoder->Commit(); FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapEncoder::Commit", res));
	}
}