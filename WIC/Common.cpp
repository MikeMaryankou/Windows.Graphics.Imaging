#include "pch.h"
#include "Common.h"

namespace WIC
{
	std::string GetErrorMsg(std::string_view method, const HRESULT errorCode, const std::source_location& location)
	{
		return std::format(fmt, location.function_name(), method, static_cast<unsigned long>(errorCode));
	}
}