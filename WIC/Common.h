#pragma once

namespace WIC
{
	enum class EImageType : std::int8_t
	{
		Unknown = 0,
		Bmp = 1,
		Jpeg = 2,
		Gif = 3,
		Tiff = 4,
		Png = 5,
		Ico = 6,
	};


	constexpr auto fmt = "{}==>{} failed with error [{:#08x}]";

	std::string GetErrorMsg(std::string_view method, HRESULT errorCode, const std::source_location& location = std::source_location::current());


	class Exception final : public std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};
};

