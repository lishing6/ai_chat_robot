#pragma once

#ifdef __cplusplus
#	include <cstdint>

static_assert(sizeof(uint_least8_t) == 1 * sizeof(char));
static_assert(sizeof(uint_least16_t) == 2 * sizeof(char));
static_assert(sizeof(uint_least32_t) == 4 * sizeof(char));

#else
#	include <stdint.h>
#endif

#ifdef _MSC_VER
#	pragma pack(1)
#endif
struct
#if defined(__GNUC__) || defined(__clang__)
#	if defined(__has_cpp_attribute) && __has_cpp_attribute(gnu::packed)
	[[gnu::packed]]
#	else
	__attribute__((packed))
#	endif
#endif
wav_header
{
	uint_least8_t riff[4];
	uint_least32_t file_size;
	uint_least8_t wave[4];
	uint_least8_t format[4];
	uint_least32_t format_size;
	uint_least16_t audio_format;
	uint_least16_t num_channels;
	uint_least32_t sample_rate;
	uint_least32_t byte_rate;
	uint_least16_t block_align;
	uint_least16_t bits_per_sample;
	uint_least8_t data[4];
	uint_least32_t data_size;
};
#ifdef _MSC_VER
#	pragma pack()
#endif

typedef struct wav_header wav_header;

#ifdef __cplusplus
static_assert(sizeof(wav_header) == 44 * sizeof(char));
extern "C" {
#endif

	wav_header wav_header_construct(
		uint_least32_t file_size,
		uint_least32_t format_size,
		uint_least16_t audio_format,
		uint_least16_t num_channels,
		uint_least32_t sample_rate,
		uint_least32_t byte_rate,
		uint_least16_t block_align,
		uint_least16_t bits_per_sample,
		uint_least32_t data_size
		);

	bool wav_header_valid(wav_header const* wav);


#ifdef __cplusplus
}
#endif