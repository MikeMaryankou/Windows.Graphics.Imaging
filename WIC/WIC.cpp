/**
 * @file
 * @brief Реализация класса Image предоставляющего функционал для операций над изображениями
 */

#include "pch.h"
#include "WIC.h"
#include "Encoder.h"
#include "Decoder.h"
#include "Stream.h"
#include "Common.h"

using Microsoft::WRL::ComPtr;

using namespace WIC;


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
		m_comInitializer.Initialize();

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

	CreateWicBitmap(GetDecoder(m_factory, GetStream(m_factory, bytes)));
}


Image::Image(const ComPtr<IStream>& stream)
{
	m_comInitializer.Initialize();
	m_factory = CreateFactory();

	const auto decoder = GetDecoder(m_factory, GetStream(m_factory, stream));
	m_imageType = GetImageType(decoder);

	CreateWicBitmap(decoder);
}


Image::Image(const std::filesystem::path& path)
{
	m_comInitializer.Initialize();
	m_factory = CreateFactory();

	const auto decoder = GetDecoder(m_factory, path);
	m_imageType = GetImageType(decoder);

	CreateWicBitmap(GetDecoder(m_factory, path));
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


std::tuple<uint32_t, uint32_t> Image::Size() const
{
	uint32_t width = 0;
	uint32_t height = 0;

	if (const HRESULT res = m_bitmap->GetSize(&width, &height); FAILED(res))
	{
		return {};
	}

	return { width, height };
}


EImageType Image::Type() const
{
	return m_imageType;
}


void Image::SaveToFile(const std::filesystem::path& destination, const EImageType type, const int imageQuality) const
{
	const EImageType encoderImageType = SelectEncoderImageType(type);
	const ComPtr<IWICStream> outputStream = GetStream(m_factory, destination);

	const Configuration configuration { .type = encoderImageType, .quality = imageQuality, .destination = m_bitmap.Get() };
	const ComPtr<IWICBitmapEncoder> encoder = GetEncoder(outputStream, encoderImageType);

	SaveEncoderData(encoder, configuration);
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

	const ComPtr<IWICStream> outputStream = GetStream(m_factory);
	const ComPtr<IWICBitmapEncoder> encoder = GetEncoder(outputStream, SelectEncoderImageType(m_imageType));

	SaveEncoderData(encoder, { .destination = scaledBitmap.Get() });
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
	const ComPtr<IWICStream> stream = GetStream(m_factory);
	const ComPtr<IWICBitmapEncoder> encoder = GetEncoder(stream, encoderImageType);

	const Configuration configuration { .type = encoderImageType, .quality = imageQuality, .destination = m_bitmap.Get() };
	SaveEncoderData(encoder, configuration);
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