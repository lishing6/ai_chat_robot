
#include "wav.h"
#include <bit>
#include <algorithm>
#include <string_view>

extern "C" wav_header wav_header_construct(
	uint_least32_t file_size,
	uint_least32_t format_size,
	uint_least16_t audio_format,
	uint_least16_t num_channels,
	uint_least32_t sample_rate,
	uint_least32_t byte_rate,
	uint_least16_t block_align,
	uint_least16_t bits_per_sample,
	uint_least32_t data_size
)
{
	if constexpr (::std::endian::native == ::std::endian::little)
	{
		return {
			{ 'R', 'I', 'F', 'F' },
			file_size,
			{ 'W', 'A', 'V', 'E' },
			{ 'f', 'm', 't', ' ' },
			format_size,
			audio_format,
			num_channels,
			sample_rate,
			byte_rate,
			block_align,
			bits_per_sample,
			{ 'd', 'a', 't', 'a' },
			data_size
		};
	}
	else
	{
		return {
			{ 'R', 'I', 'F', 'F' },
			::std::byteswap(file_size),
			{ 'W', 'A', 'V', 'E' },
			{ 'f', 'm', 't', ' ' },
			::std::byteswap(format_size),
			::std::byteswap(audio_format),
			::std::byteswap(num_channels),
			::std::byteswap(sample_rate),
			::std::byteswap(byte_rate),
			::std::byteswap(block_align),
			::std::byteswap(bits_per_sample),
			{ 'd', 'a', 't', 'a' },
			::std::byteswap(data_size)
		};
	}
	
}

extern "C" bool wav_header_valid(wav_header const* wav)
{
	using namespace ::std::string_view_literals;

	return
		::std::ranges::equal(wav->riff, "RIFF"sv.substr(0, 4))
		&& ::std::ranges::equal(wav->wave, "WAVE"sv.substr(0, 4))
		&& ::std::ranges::equal(wav->format, "fmt "sv.substr(0, 4))
		&& ::std::ranges::equal(wav->data, "data"sv.substr(0, 4))
		;
}