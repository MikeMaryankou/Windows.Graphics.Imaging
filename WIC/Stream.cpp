#include "pch.h"
#include "Stream.h"
#include "Common.h"


using Microsoft::WRL::ComPtr;


ComPtr<IWICStream> WIC::GetStream(const ComPtr<IWICImagingFactory>& factory, const ComPtr<IStream>& buffer)
{
	ComPtr<IWICStream> stream = nullptr;

	if (const HRESULT res = factory->CreateStream(stream.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::CreateStream", res));
	}

	if (const HRESULT res = stream->InitializeFromIStream(buffer.Get()); FAILED(res))
	{
		throw Exception(GetErrorMsg("Stream::InitializeFromIStream", res));
	}

	return stream;
}


ComPtr<IWICStream> WIC::GetStream(const ComPtr<IWICImagingFactory>& factory)
{
	ComPtr<IWICStream> stream = nullptr;

	if (const HRESULT res = factory->CreateStream(stream.GetAddressOf()); FAILED(res))
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


ComPtr<IWICStream> WIC::GetStream(const ComPtr<IWICImagingFactory>& factory, const std::filesystem::path& source)
{
	ComPtr<IWICStream> stream = nullptr;

	if (const HRESULT res = factory->CreateStream(stream.GetAddressOf()); FAILED(res))
	{
		throw Exception(GetErrorMsg("Factory::InitializeFromIStream", res));
	}

	if (const HRESULT res = stream->InitializeFromFilename(source.c_str(), GENERIC_WRITE); FAILED(res))
	{
		throw Exception(GetErrorMsg("Stream::InitializeFromFilename", res));
	}

	return stream;
}


ComPtr<IWICStream> WIC::GetStream(const ComPtr<IWICImagingFactory>& factory, std::vector<uint8_t>& source)
{
	ComPtr<IWICStream> stream = nullptr;

	if (const HRESULT res = factory->CreateStream(stream.GetAddressOf()); FAILED(res))
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