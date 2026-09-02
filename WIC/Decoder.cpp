#include "pch.h"
#include "Decoder.h"
#include "Common.h"


using Microsoft::WRL::ComPtr;


namespace
{
	constexpr auto GUID_STRING_FMT = "{:08X}-{:04X}-{:04X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}";
	
	
	std::map<WIC::EImageType, IID> GetImageTypeDecoderDictionary()
	{
		static const std::map<WIC::EImageType, IID> imageTypeDecoderDictionary =
		{
			{WIC::EImageType::Bmp, CLSID_WICBmpDecoder},
			{WIC::EImageType::Gif, CLSID_WICGifDecoder},
			{WIC::EImageType::Jpeg, CLSID_WICJpegDecoder},
			{WIC::EImageType::Png, CLSID_WICPngDecoder},
			{WIC::EImageType::Tiff, CLSID_WICTiffDecoder},
		};
		
		return imageTypeDecoderDictionary;
	}


	std::string GuidStringify(const GUID guid)
	{
		return std::format(GUID_STRING_FMT, guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
			guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
	};
}


ComPtr<IWICBitmapDecoder> WIC::GetDecoder(const ComPtr<IWICImagingFactory>& factory, const ComPtr<IWICStream>& stream)
{
	for (const auto& guid : GetImageTypeDecoderDictionary() | std::views::values)
	{
		ComPtr<IWICBitmapDecoder> decoder = nullptr;

		if (const HRESULT res = ::CoCreateInstance(guid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(decoder.GetAddressOf())); FAILED(res))
		{
			continue;
		}

		if (const HRESULT res = decoder->Initialize(stream.Get(), WICDecodeMetadataCacheOnLoad); SUCCEEDED(res))
		{
			return decoder;
		}
	}

	ComPtr<IWICBitmapDecoder> decoder = nullptr;

	if (const HRESULT res = factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf());
		FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::CreateDecoderFromStream", res));
	}

	return decoder;
}


ComPtr<IWICBitmapDecoder> WIC::GetDecoder(const ComPtr<IWICImagingFactory>& factory, const std::filesystem::path& path)
{
	ComPtr<IWICBitmapDecoder> decoder = nullptr;

	if (const HRESULT res = factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf());
		FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::CreateDecoderFromStream", res));
	}

	return decoder;
}


WIC::EImageType WIC::GetImageType(const ComPtr<IWICBitmapDecoder>& decoder)
{
	ComPtr<IWICBitmapDecoderInfo> info = nullptr;

	if (const HRESULT res = decoder->GetDecoderInfo(info.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("Decoder::GetDecoderInfo", res));
	}

	CLSID sid = {};

	if (const HRESULT res = info->GetCLSID(&sid); FAILED(res))
	{
		throw Exception(GetErrorMsg("DecoderInfo::GetCLSID", res));
	}

	for (const auto& [type, guid] : GetImageTypeDecoderDictionary())
	{
		if (guid == sid)
		{
			return type;
		}
	}
	
	throw Exception(std::format("DecoderInfo return unsupported sid [{}]", GuidStringify(sid)));
}