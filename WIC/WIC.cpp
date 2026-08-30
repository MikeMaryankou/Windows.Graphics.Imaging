/**
 * @file
 * @brief Реализация класса Image предоставляющего функционал для операций над изображениями
 */

#include "pch.h"
#include "WIC.h"

using Microsoft::WRL::ComPtr;

using namespace WIC;

namespace
{
	const std::map<WIC::EImageType, IID> g_imageTypeEncoderDictionary =
	{
		{WIC::EImageType::Bmp, CLSID_WICBmpEncoder},
		{WIC::EImageType::Gif, CLSID_WICGifEncoder},
		{WIC::EImageType::Jpeg, CLSID_WICJpegEncoder},
		{WIC::EImageType::Png, CLSID_WICPngEncoder},
		{WIC::EImageType::Tiff, CLSID_WICTiffEncoder},
	};

	const std::map<WIC::EImageType, IID> g_imageTypeDecoderDictionary =
	{
		{WIC::EImageType::Bmp, CLSID_WICBmpDecoder},
		{WIC::EImageType::Gif, CLSID_WICGifDecoder},
		{WIC::EImageType::Jpeg, CLSID_WICJpegDecoder},
		{WIC::EImageType::Png, CLSID_WICPngDecoder},
		{WIC::EImageType::Tiff, CLSID_WICTiffDecoder},
	};
}


Image::Image(const Image& image)
{
	m_comInitializer.Initialize();

	m_imageType = image.m_imageType;
	m_factory = image.m_factory;
	m_bitmap = image.m_bitmap;
}


Image& Image::operator=(const Image& image)
{
	if (this != &image)
	{
		//m_comInitializer.Initialize();

		m_imageType = image.m_imageType;
		m_factory = image.m_factory;
		m_bitmap = image.m_bitmap;
	}

	return *this;
}


Image::~Image()
{
	Dispose();
}


std::string Image::GetErrorMsg(std::string_view method, const HRESULT errorCode, const std::source_location& location)
{
	return std::format(fmt, location.function_name(), method, static_cast<unsigned long>(errorCode));
}


void Image::Dispose()
{
	if (m_bitmap != nullptr)
	{
		m_bitmap.Reset();
	}

	if (m_factory != nullptr)
	{
		m_factory.Reset();
	}
}


ComPtr<IWICImagingFactory> Image::CreateFactory()
{
	ComPtr<IWICImagingFactory> factory;

	if (const HRESULT res = CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory2, std::bit_cast<LPVOID*>(factory.GetAddressOf()));
		FAILED(res) || factory == nullptr)
	{
		throw Exception("Factory not initialized!");
	}

	return factory;
}


EImageType Image::GetImageType(const ComPtr<IWICBitmapDecoder>& decoder) noexcept(false)
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

	for (const auto& [type, guid] : g_imageTypeDecoderDictionary)
	{
		if (guid == sid)
		{
			return type;
		}
	}

	auto stringFromGuid = [](const GUID guid)
		{
			return std::format("{:08X}-{:04X}-{:04X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}",
				guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
				guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
		};

	throw Exception(std::format("DecoderInfo return unsupported sid [{}]", stringFromGuid(sid)));
}


EImageType Image::SelectEncoderImageType(const EImageType type) const
{
	const auto imageType = type == EImageType::Unknown ? m_imageType : type;

	return imageType == EImageType::Ico ? EImageType::Png : imageType;
}


Image::Image(const HICON source)
{
	m_imageType = EImageType::Ico;
	m_comInitializer.Initialize();
	m_factory = CreateFactory();

	if (const HRESULT res = m_factory->CreateBitmapFromHICON(source, m_bitmap.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::CreateBitmapFromHICON", res));
	}
}


Image::Image(const HBITMAP source)
{
	m_imageType = EImageType::Bmp;
	m_comInitializer.Initialize();
	m_factory = CreateFactory();

	if (const HRESULT res = m_factory->CreateBitmapFromHBITMAP(source, nullptr, WICBitmapUseAlpha, m_bitmap.GetAddressOf());
		FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::CreateBitmapFromHBITMAP", res));
	}
}


Image::Image(const uint32_t width, const uint32_t height, std::vector<uint8_t>& bytes, const bool isRgb)
{
	m_comInitializer.Initialize();
	m_factory = CreateFactory();

	const auto pixelFormat = isRgb ? GUID_WICPixelFormat32bppPRGBA : GUID_WICPixelFormat32bppPBGRA;

	if (const HRESULT res = m_factory->CreateBitmapFromMemory(width, height, pixelFormat, width * 4,
		static_cast<UINT>(bytes.size()), bytes.data(), m_bitmap.GetAddressOf());
		FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::CreateBitmapFromMemory", res));
	}
}


Image::Image(std::vector<uint8_t>& bytes)
{
	m_comInitializer.Initialize();
	m_factory = CreateFactory();

	CreateWicBitmap(GetDecoder(GetStream(bytes)));
}


Image::Image(const ComPtr<IStream>& stream)
{
	m_comInitializer.Initialize();
	m_factory = CreateFactory();

	CreateWicBitmap(GetDecoder(GetStream(stream)));
}


Image::Image(const std::filesystem::path& path)
{
	m_comInitializer.Initialize();
	m_factory = CreateFactory();

	CreateWicBitmap(GetDecoder(path));
}


void Image::CreateWicBitmap(const ComPtr<IWICBitmapDecoder>& decoder)
{
	ComPtr<IWICBitmapFrameDecode> frame;

	if (const HRESULT res = decoder->GetFrame(0, frame.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapDecoder::GetFrame", res));
	}

	WICPixelFormatGUID pixelFormat;

	if (const HRESULT res = frame->GetPixelFormat(&pixelFormat); FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapFrameDecode::GetPixelFormat", res));
	}

	if (const HRESULT res = WICConvertBitmapSource(pixelFormat, frame.Get(), std::bit_cast<IWICBitmapSource**>(m_bitmap.GetAddressOf()));
		FAILED(res))
	{
		throw Exception(GetErrorMsg("WICConvertBitmapSource()", res));
	}
}


void Image::ConvertToGrayscale()
{
	ComPtr<IWICFormatConverter> converter;

	if (const HRESULT res = m_factory->CreateFormatConverter(converter.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::CreateFormatConverter", res));
	}

	if (const HRESULT res = converter->Initialize(m_bitmap.Get(), GUID_WICPixelFormat32bppGrayFixedPoint, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom); 
		FAILED(res))
	{
		throw Exception(GetErrorMsg("FormatConverter::Initialize", res));
	}

	m_bitmap.Reset();

	if (const HRESULT res = converter->QueryInterface(IID_IWICBitmapSource, std::bit_cast<void**>(m_bitmap.GetAddressOf()));
		FAILED(res))
	{
		throw Exception(GetErrorMsg("FormatConverter::QueryInterface", res));
	}
}


std::tuple<uint32_t, uint32_t> Image::GetImageSize() const
{
	uint32_t width = 0;
	uint32_t height = 0;

	if (const HRESULT res = m_bitmap->GetSize(&width, &height); FAILED(res))
	{
		return {};
	}

	return { width, height };
}


EImageType Image::GetImageType() const
{
	return m_imageType;
}


void Image::SaveToFile(const std::filesystem::path& destination, const EImageType type, const int imageQuality) const
{
	const EImageType encoderImageType = SelectEncoderImageType(type);
	const ComPtr<IWICStream> outputStream = GetStream(destination);
	const ComPtr<IWICBitmapEncoder> encoder = GetEncoder(outputStream, encoderImageType);

	SaveEncoderData(encoder, imageQuality);
}


void Image::Scale(const unsigned int width, const unsigned int height)
{
	ComPtr<IWICBitmapScaler> scaledBitmap = nullptr;

	if (const HRESULT res = m_factory->CreateBitmapScaler(scaledBitmap.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::CreateBitmapScaler", res));
	}

	if (const auto res = scaledBitmap->Initialize(m_bitmap.Get(), width, height, WICBitmapInterpolationModeFant); FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapScaler::Initialize", res));
	}

	const ComPtr<IWICStream> outputStream = GetStream();
	const ComPtr<IWICBitmapEncoder> encoder = GetEncoder(outputStream, SelectEncoderImageType(m_imageType));

	SaveEncoderData(encoder, scaledBitmap);
	m_bitmap.Reset();

	if (const HRESULT res = m_factory->CreateBitmapFromSource(scaledBitmap.Get(), WICBitmapCacheOnDemand, m_bitmap.GetAddressOf());
		FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::CreateBitmapFromMemory", res));
	}
}


void Image::SaveToBuffer(std::vector<uint8_t>& buffer, const EImageType type, const int imageQuality) const
{
	const EImageType encoderImageType = SelectEncoderImageType(type);
	const ComPtr<IWICStream> stream = GetStream();
	const ComPtr<IWICBitmapEncoder> encoder = GetEncoder(stream, encoderImageType);

	SaveEncoderData(encoder, encoderImageType, imageQuality);
	SaveToBufferDetails(buffer, stream);
}


void Image::SaveToBufferDetails(std::vector<uint8_t>& buffer, const ComPtr<IWICStream>& stream)
{
	buffer.clear();
	STATSTG stats{};

	if (const HRESULT res = stream->Stat(&stats, STATFLAG_NONAME); FAILED(res))
	{
		throw Exception(GetErrorMsg("Stream::Stat", res));
	}

	constexpr LARGE_INTEGER zero{};

	if (const HRESULT res = stream->Seek(zero, STREAM_SEEK_SET, nullptr); FAILED(res))
	{
		throw Exception(GetErrorMsg("Stream::Seek", res));
	}

	ULONG bytesRead = 0;
	buffer.resize(static_cast<size_t>(stats.cbSize.QuadPart));

	if (const HRESULT res = stream->Read(buffer.data(), static_cast<ULONG>(buffer.size()), &bytesRead); FAILED(res))
	{
		buffer.clear();

		throw Exception(GetErrorMsg("Stream::Read", res));
	}
}


void Image::SaveToBuffer(std::vector<char>& buffer, const EImageType type, const int imageQuality) const
{
	const EImageType encoderImageType = SelectEncoderImageType(type);
	const ComPtr<IWICStream> stream = GetStream();
	const ComPtr<IWICBitmapEncoder> encoder = GetEncoder(stream, encoderImageType);

	SaveEncoderData(encoder, encoderImageType, imageQuality);
	SaveToBufferDetails(buffer, stream);
}


void Image::SaveToBufferDetails(std::vector<char>& buffer, const ComPtr<IWICStream>& stream)
{
	buffer.clear();
	STATSTG stats{};

	if (const HRESULT res = stream->Stat(&stats, STATFLAG_NONAME); FAILED(res))
	{
		throw Exception(GetErrorMsg("Stream::Stat", res));
	}

	constexpr LARGE_INTEGER zero{};

	if (const HRESULT res = stream->Seek(zero, STREAM_SEEK_SET, nullptr); FAILED(res))
	{
		throw Exception(GetErrorMsg("Stream::Seek", res));
	}

	ULONG bytesRead = 0;
	buffer.resize(static_cast<size_t>(stats.cbSize.QuadPart));

	if (const HRESULT res = stream->Read(buffer.data(), static_cast<ULONG>(buffer.size()), &bytesRead); FAILED(res))
	{
		buffer.clear();

		throw Exception(GetErrorMsg("Stream::Read", res));
	}
}


ComPtr<IWICStream> Image::GetStream(const ComPtr<IStream>& buffer) const
{
	ComPtr<IWICStream> stream = nullptr;

	if (const HRESULT res = m_factory->CreateStream(stream.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::CreateStream", res));
	}

	if (const HRESULT res = stream->InitializeFromIStream(buffer.Get()); FAILED(res))
	{
		throw Exception(GetErrorMsg("Stream::InitializeFromIStream", res));
	}

	return stream;
}


ComPtr<IWICStream> Image::GetStream() const
{
	ComPtr<IWICStream> stream = nullptr;

	if (const HRESULT res = m_factory->CreateStream(stream.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::CreateStream", res));
	}

	ComPtr<IStream> buffer;

	if (const HRESULT res = CreateStreamOnHGlobal(nullptr, true, buffer.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("CreateStreamOnHGlobal", res));
	}

	if (const HRESULT res = stream->InitializeFromIStream(buffer.Get()); FAILED(res))
	{
		throw Exception(GetErrorMsg("Stream::InitializeFromIStream", res));
	}

	return stream;
}


ComPtr<IWICStream> Image::GetStream(const std::filesystem::path& source) const
{
	ComPtr<IWICStream> stream = nullptr;

	if (const HRESULT res = m_factory->CreateStream(stream.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::InitializeFromIStream", res));
	}

	if (const HRESULT res = stream->InitializeFromFilename(source.c_str(), GENERIC_WRITE); FAILED(res))
	{
		throw Exception(GetErrorMsg("Stream::InitializeFromFilename", res));
	}

	return stream;
}


ComPtr<IWICStream> Image::GetStream(std::vector<uint8_t>& source) const
{
	ComPtr<IWICStream> stream = nullptr;

	if (const HRESULT res = m_factory->CreateStream(stream.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::InitializeFromIStream", res));
	}

	if (const HRESULT res = stream->InitializeFromMemory(source.data(), static_cast<unsigned long>(source.size()));
		FAILED(res))
	{
		throw Exception(GetErrorMsg("Stream::InitializeFromMemory", res));
	}

	return stream;
}


ComPtr<IWICBitmapEncoder> Image::GetEncoder(const ComPtr<IWICStream>& stream, const EImageType type)
{
	ComPtr<IWICBitmapEncoder> encoder = nullptr;

	if (const HRESULT res = ::CoCreateInstance(g_imageTypeEncoderDictionary.at(type), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(encoder.GetAddressOf())); 
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


ComPtr<IWICBitmapDecoder> Image::GetDecoder(const ComPtr<IWICStream>& stream)
{
	for (const auto& [type, guid] : g_imageTypeDecoderDictionary)
	{
		ComPtr<IWICBitmapDecoder> decoder = nullptr;

		if (const HRESULT res = ::CoCreateInstance(guid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(decoder.GetAddressOf())); FAILED(res))
		{
			continue;
		}

		if (const HRESULT res = decoder->Initialize(stream.Get(), WICDecodeMetadataCacheOnLoad); SUCCEEDED(res))
		{
			m_imageType = type;
			return decoder;
		}
	}

	ComPtr<IWICBitmapDecoder> decoder = nullptr;

	if (const HRESULT res = m_factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf());
		FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::CreateDecoderFromStream", res));
	}

	m_imageType = GetImageType(decoder);
	return decoder;
}


ComPtr<IWICBitmapDecoder> Image::GetDecoder(const std::filesystem::path& path)
{
	ComPtr<IWICBitmapDecoder> decoder = nullptr;

	if (const HRESULT res = m_factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf()); 
		FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::CreateDecoderFromStream", res));
	}

	m_imageType = GetImageType(decoder);

	return decoder;
}


void Image::SetImageQuality(const int imageQuality, const ComPtr<IPropertyBag2>& properties)
{
	const float imageQualityValue = std::truncf(static_cast<float>(imageQuality)) / 100;\
	VARIANT value;
	::VariantInit(&value);
	value.vt = VT_R8;
	value.dblVal = static_cast<double>(imageQualityValue);

	PROPBAG2 data{};
	data.dwType = PROPBAG2_TYPE_DATA;
	data.vt = VT_R8;
	data.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
	
	const HRESULT res = properties->Write(1, &data, &value);

	::VariantClear(&value);

	if (FAILED(res))
	{
		throw Exception(GetErrorMsg("IPropertyBag2::Write", res));
	}
}


void Image::SaveEncoderData(const ComPtr<IWICBitmapEncoder>& encoder, const EImageType type, const int imageQuality) const
{
	ComPtr<IPropertyBag2> properties;
	ComPtr<IWICBitmapFrameEncode> targetFrame;

	if (const HRESULT res = encoder->CreateNewFrame(targetFrame.GetAddressOf(), properties.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapEncoder::CreateNewFrame", res));
	}

	if (type == EImageType::Jpeg && imageQuality > 0 && imageQuality <= 100)
	{
		SetImageQuality(imageQuality, properties);
	}

	if (const HRESULT res = targetFrame->Initialize(properties.Get()); FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapFrameEncode::Initialize", res));
	}

	if (const HRESULT res = targetFrame->WriteSource(m_bitmap.Get(), nullptr); FAILED(res))
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


void Image::SaveEncoderData(const ComPtr<IWICBitmapEncoder>& encoder, const ComPtr<IWICBitmapScaler>& scaledBitmap)
{
	ComPtr<IPropertyBag2> properties;
	ComPtr<IWICBitmapFrameEncode> targetFrame;

	if (const HRESULT res = encoder->CreateNewFrame(targetFrame.GetAddressOf(), properties.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapEncoder::CreateNewFrame", res));
	}

	if (const HRESULT res = targetFrame->Initialize(properties.Get()); FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapFrameEncode::Initialize", res));
	}

	if (const HRESULT res = targetFrame->WriteSource(scaledBitmap.Get(), nullptr); FAILED(res))
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


void Image::SaveEncoderData(const ComPtr<IWICBitmapEncoder>& encoder, const bool asGraysScale) const
{
	ComPtr<IPropertyBag2> properties;
	ComPtr<IWICBitmapFrameEncode> targetFrame;

	const std::string functionName = std::source_location::current().function_name();

	if (const HRESULT res = encoder->CreateNewFrame(targetFrame.GetAddressOf(), properties.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapEncoder::CreateNewFrame", res));
	}

	if (const HRESULT res = targetFrame->Initialize(properties.Get()); FAILED(res))
	{
		throw Exception(GetErrorMsg("BitmapFrameEncode::Initialize", res));
	}

	if (asGraysScale)
	{
		auto grayScalePixelFormat = GUID_WICPixelFormat32bppGrayFixedPoint;

		if (const HRESULT res = targetFrame->SetPixelFormat(&grayScalePixelFormat); FAILED(res))
		{
			throw Exception(GetErrorMsg("BitmapFrameEncode::SetPixelFormat", res));
		}
	}

	if (const HRESULT res = targetFrame->WriteSource(m_bitmap.Get(), nullptr); FAILED(res))
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
