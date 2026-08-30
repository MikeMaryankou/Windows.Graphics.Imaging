/**
 * @file
 * @brief Definiton of class Image which provide functionality for image processing.
 */
#pragma once
#include "Common.h"

namespace WIC
{
	class ComInitializer
	{
	public:
		ComInitializer() noexcept = default;

		ComInitializer(const ComInitializer&) = delete;
		ComInitializer& operator=(const ComInitializer&) = delete;
		ComInitializer(ComInitializer&&) = delete;
		ComInitializer& operator=(ComInitializer&&) = delete;

		~ComInitializer() noexcept
		{
			Dispose();
		}

		bool Initialize(std::initializer_list<DWORD> coInitOrder = { COINIT_MULTITHREADED, COINIT_APARTMENTTHREADED }) noexcept
		{
			if (m_initialized)
			{
				return true;
			}

			for (const auto& coInitFlag : coInitOrder)
			{
				m_comError = ::CoInitializeEx(nullptr, coInitFlag);

				if (SUCCEEDED(m_comError))
				{
					m_initialized = true;
					break;
				}

				if (m_comError == RPC_E_CHANGED_MODE)
				{
					continue;
				}
			}

			return m_initialized;
		}

		void Dispose() noexcept
		{
			if (m_initialized)
			{
				return;
			}

			::CoUninitialize();
			m_initialized = false;
			m_comError = S_OK;
		}

		bool IsInitialized() const noexcept { return m_initialized; }
		HRESULT GetComError() const noexcept { return m_comError; }

	private:
		bool m_initialized{ false };
		HRESULT m_comError{ S_OK };
	};
}



namespace WIC
{
	class Image
	{
	public:
		Image(const Image& image);
		Image& operator=(const Image& image);

		Image(Image&& image) = delete;
		Image& operator=(Image&& moved) = delete;

		~Image();

		explicit Image(HICON source);

		explicit Image(HBITMAP source);

		Image(uint32_t width, uint32_t height, std::vector<uint8_t>& bytes, bool isRgb = false);

		explicit Image(std::vector<uint8_t>& bytes);

		explicit Image(const Microsoft::WRL::ComPtr<IStream>& stream);

		explicit Image(const std::filesystem::path& path);

		void ConvertToGrayscale();

		void Scale(unsigned int width, unsigned int height);

		[[nodiscard]] std::tuple<uint32_t, uint32_t> Size() const;

		EImageType Type() const;

		void SaveToFile(const std::filesystem::path& destination, EImageType type = EImageType::Unknown, int imageQuality = 0) const;

		void SaveToBuffer(std::vector<uint8_t>& buffer, EImageType type = EImageType::Unknown, int imageQuality = 0) const;

		void SaveToBuffer(std::vector<char>& buffer, EImageType type = EImageType::Unknown, int imageQuality = 0) const;

	private:
		void Dispose();

		static Microsoft::WRL::ComPtr<IWICImagingFactory> CreateFactory();

		EImageType SelectEncoderImageType(EImageType type) const;

		void CreateWicBitmap(const Microsoft::WRL::ComPtr<IWICBitmapDecoder>& decoder);

		static Microsoft::WRL::ComPtr<IWICBitmapEncoder> GetEncoder(const Microsoft::WRL::ComPtr<IWICStream>& stream, EImageType type);

		static void SetImageQuality(int imageQuality, const Microsoft::WRL::ComPtr<IPropertyBag2>& properties);

		void SaveEncoderData(const Microsoft::WRL::ComPtr<IWICBitmapEncoder>& encoder, EImageType type, int imageQuality = 0) const;

		static void SaveEncoderData(const Microsoft::WRL::ComPtr<IWICBitmapEncoder>& encoder, const Microsoft::WRL::ComPtr<IWICBitmapScaler>& scaledBitmap);

		void SaveEncoderData(const Microsoft::WRL::ComPtr<IWICBitmapEncoder>& encoder, bool asGraysScale = false) const;

		static void SaveToBufferDetails(std::vector<char>& buffer, const Microsoft::WRL::ComPtr<IWICStream>& stream);

		static void SaveToBufferDetails(std::vector<uint8_t>& buffer, const Microsoft::WRL::ComPtr<IWICStream>& stream);

		Microsoft::WRL::ComPtr<IWICImagingFactory> m_factory;
		Microsoft::WRL::ComPtr<IWICBitmap> m_bitmap;
		EImageType m_imageType = EImageType::Unknown;
		ComInitializer m_comInitializer;
	};
}