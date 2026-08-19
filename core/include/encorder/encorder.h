#ifndef ENC_ENCORDER_H
#define ENC_ENCORDER_H

#include <stdint.h> // NOLINT(*-deprecated-headers)
// ReSharper disable once CppUnusedIncludeDirective
#include <stdbool.h> // NOLINT(*-deprecated-headers)

#define ENC_VERSION (0x00'01'00u)

#define ENC_VERSION_MAJOR(version) (((version) & 0xFF'00'00u) >> 16)
#define ENC_VERSION_MINOR(version) (((version) & 0x00'FF'00u) >> 8)
#define ENC_VERSION_PATCH(version) (((version) & 0x00'00'FFu))

#ifdef __cplusplus
# define ENC_API_CXX extern "C"
#else
# define ENC_API_CXX
#endif

#ifndef __has_attribute
# define ENC_DEFINED_HAS_ATTRIBUTE
# define __has_attribute(...) 0
#endif

#if defined(_WIN32)
# if defined(ENC_BUILD_SHARED)
#  define ENC_API ENC_API_CXX __declspec(dllexport)
# elif defined(ENC_USE_SHARED)
#  define ENC_API ENC_API_CXX __declspec(dllimport)
# else
#  define ENC_API ENC_API_CXX
# endif
#else
# if defined(ENC_BUILD_SHARED) && __has_attribute(visibility)
#  define ENC_API ENC_API_CXX __attribute__((visibility("default")))
# else
#  define ENC_API ENC_API_CXX
# endif
#endif

#define ENC_PUBLIC_STRUCT uint32_t struct_size; uint32_t version;

#if  __has_attribute(enum_extensibility)
# define ENC_ENUM_OPEN __attribute__((enum_extensibility(open)))
# define ENC_ENUM_CLOSED __attribute__((enum_extensibility(closed)))
#else
# define ENC_ENUM_OPEN
# define ENC_ENUM_CLOSED
#endif

#if  __has_attribute(flag_enum)
# define ENC_ENUM_FLAG __attribute__((flag_enum))
#else
# define ENC_ENUM_FLAG
#endif

#ifdef NEARGYE_MAGIC_ENUM_HPP
# define ENC_MAGIC_ENUM_FLAG(name) \
		template<> \
		struct magic_enum::customize::enum_range<name> { \
			static constexpr bool is_flags = true; \
		};
#else
# define ENC_MAGIC_ENUM_FLAG(name)
#endif

#define ENC_ENUM_OPEN_FLAG(name, ...) enum ENC_ENUM_OPEN ENC_ENUM_FLAG name __VA_ARGS__; ENC_MAGIC_ENUM_FLAG(name)
#define ENC_ENUM_CLOSED_FLAG(name, ...) enum ENC_ENUM_CLOSED ENC_ENUM_FLAG name __VA_ARGS__; ENC_MAGIC_ENUM_FLAG(name)

#ifdef ENC_DEFINED_HAS_ATTRIBUTE
# undef __has_attribute
#endif

struct enc_instance;
struct enc_device;
struct enc_encoder;
struct enc_pool;
struct enc_frame;
struct enc_ticket;

struct enc_struct_header {
	ENC_PUBLIC_STRUCT
};

enum enc_result {
	ENC_RESULT_SUCCESS = 0,
	ENC_RESULT_INCOMPLETE = 1,
	ENC_RESULT_NOT_READY = 2,
	ENC_RESULT_TIMEOUT = 3,
	ENC_RESULT_WOULD_BLOCK = 4,

	ENC_RESULT_ERROR_UNKNOWN = -1,
	ENC_RESULT_ERROR_INVALID_ARGUMENT = -2,
	ENC_RESULT_ERROR_INVALID_STRUCT_SIZE = -3,
	ENC_RESULT_ERROR_INVALID_STRUCT_VERSION = -4,
	ENC_RESULT_ERROR_VERSION_MISMATCH = -5,
	ENC_RESULT_ERROR_UNSUPPORTED = -6,
	ENC_RESULT_ERROR_OUT_OF_MEMORY = -7,
	ENC_RESULT_ERROR_OUT_OF_DEVICE_MEMORY = -8,
	ENC_RESULT_ERROR_INITIALIZATION_FAILED = -9,

	ENC_RESULT_ERROR_NO_DEVICE = -100,
	ENC_RESULT_ERROR_NO_BACKEND = -101,
	ENC_RESULT_ERROR_DEVICE_LOST = -102,
	ENC_RESULT_ERROR_BACKEND_MISMATCH = -103,
	ENC_RESULT_ERROR_DEVICE_UUID_MISMATCH = -104,
	ENC_RESULT_ERROR_HARDWARE_REQUIRED = -105,

	ENC_RESULT_ERROR_CONFIG_UNSUPPORTED = -200,
	ENC_RESULT_ERROR_FORMAT_UNSUPPORTED = -201,
	ENC_RESULT_ERROR_CODEC_UNSUPPORTED = -202,
	ENC_RESULT_ERROR_RESOLUTION_UNSUPPORTED = -203,

	ENC_RESULT_ERROR_SESSION_LIMIT_REACHED = -300,
	ENC_RESULT_ERROR_IMPORT_MISMATCH = -301,
	ENC_RESULT_ERROR_POOL_EXHAUSTED = -302,
	ENC_RESULT_ERROR_FRAME_IN_USE = -303,

	ENC_RESULT_MINIMUM = -500,
	ENC_RESULT_MAXIMUM = 500
};

ENC_ENUM_OPEN_FLAG(enc_backend, {
	ENC_BACKEND_NONE = 0,
	ENC_BACKEND_VULKAN = 1,
	ENC_BACKEND_NVENC = 1 << 1,
	ENC_BACKEND_AMF = 1 << 2,
	ENC_BACKEND_VPL = 1 << 3,
	ENC_BACKEND_VAAPI = 1 << 4,
	ENC_BACKEND_VIDEOTOOLBOX = 1 << 5,
	ENC_BACKEND_D3D12 = 1 << 6,
	ENC_BACKEND_COMPUTE = 1 << 7,

	ENC_BACKEND_ALL = 0xFFFF
})

enum ENC_ENUM_CLOSED enc_native_kind {
	ENC_NATIVE_NONE = 0,
	ENC_NATIVE_VULKAN = 1,
	ENC_NATIVE_METAL = 2,
	ENC_NATIVE_D3D12 = 3,
	ENC_NATIVE_CUDA = 4,
	ENC_NATIVE_IOSURFACE = 5
};

ENC_ENUM_OPEN_FLAG(enc_codec, {
	ENC_CODEC_NONE = 0,
	ENC_CODEC_H264 = 1,
	ENC_CODEC_HEVC = 1 << 1,
	ENC_CODEC_AV1 = 1 << 2,
	ENC_CODEC_PRORES = 1 << 3,
	ENC_CODEC_MJPEG = 1 << 4
})

ENC_ENUM_OPEN_FLAG(enc_chroma, {
	ENC_CHROMA_NONE = 0,
	ENC_CHROMA_MONOCHROME = 1,
	ENC_CHROMA_420 = 1 << 1,
	ENC_CHROMA_422 = 1 << 2,
	ENC_CHROMA_444 = 1 << 3
})

/* Grouped by range with gaps for future formats. */
enum ENC_ENUM_OPEN enc_format {
	ENC_FORMAT_UNDEFINED = 0,

	ENC_FORMAT_NV12 = 0x0100,
	ENC_FORMAT_P010 = 0x0101,
	ENC_FORMAT_P012 = 0x0102,
	ENC_FORMAT_P016 = 0x0103,

	ENC_FORMAT_NV16 = 0x0200,
	ENC_FORMAT_P210 = 0x0201,
	ENC_FORMAT_P216 = 0x0202,
	ENC_FORMAT_Y210 = 0x0203,

	ENC_FORMAT_NV24 = 0x0300,
	ENC_FORMAT_P410 = 0x0301,
	ENC_FORMAT_P416 = 0x0302,
	ENC_FORMAT_AYUV = 0x0303,
	ENC_FORMAT_Y410 = 0x0304,
	ENC_FORMAT_Y416 = 0x0305,

	ENC_FORMAT_RGBA8 = 0x0400,
	ENC_FORMAT_BGRA8 = 0x0401,
	ENC_FORMAT_RGB10A2 = 0x0402,
	ENC_FORMAT_RGBA16F = 0x0403
};

/* ITU-T H.273 (V4) Table 2 from https://www.itu.int/rec/T-REC-H.273-202407-I/en. */
enum ENC_ENUM_CLOSED enc_h273_signal_type {
	/* 0 "Reserved ... [f]or future use by ITU-T | ISO/IEC" */

	ENC_SIGNAL_BT709_6 = 1,
	ENC_SIGNAL_BT1361_0 = 1, /* "(historical)" */
	ENC_SIGNAL_IEC61966_2_1_SRGB = 1,
	ENC_SIGNAL_IEC61966_2_1_SYCC = 1,
	ENC_SIGNAL_IEC61966_2_4 = 1,
	ENC_SIGNAL_SMPTE_RP_177_B = 1,

	/*
	 * "Image characteristics are unknown or are determined by the application"
	 * so we can use this for native frames.
	 */
	ENC_SIGNAL_UNSPECIFIED = 2,
	ENC_SIGNAL_LIBENCORDER_IDENTITY = 2,

	ENC_SIGNAL_RESERVED_3 = 3,

	ENC_SIGNAL_BT470_6_M = 4, /* "(historical)" */

	ENC_SIGNAL_BT470_6_BG = 5, /* "(historical)" */
	ENC_SIGNAL_BT601_7_625 = 5,
	ENC_SIGNAL_BT1358_0_625 = 5, /* "(historical)" */
	ENC_SIGNAL_BT1700_0_625_PAL = 5,
	ENC_SIGNAL_BT1700_0_625_SECAM = 5,

	ENC_SIGNAL_BT601_7_525 = 6,
	ENC_SIGNAL_BT1358_1_525 = 6, /* "(historical)" */
	ENC_SIGNAL_BT1358_1_625 = 6, /* "(historical)" */
	ENC_SIGNAL_BT1700_0_NTSC = 6,
	ENC_SIGNAL_SMPTE_ST_170 = 6,

	ENC_SIGNAL_SMPTE_ST_240 = 7, /* "functionally the same as the value 6" */

	/* 8 is just "Generic film". */

	ENC_SIGNAL_BT2020_2 = 9,
	ENC_SIGNAL_BT2100_2 = 9,

	ENC_SIGNAL_SMPTE_ST_428_1 = 10,
	/* TODO(Emily): What does "(CIE 1931 XYZ as in ISO/CIE 11664-1)" mean? */

	ENC_SIGNAL_SMPTE_RP_431_2 = 11,

	ENC_SIGNAL_SMPTE_EG_432_1 = 12,

	/* 13-21 "Reserved ... [f]or future use by ITU-T | ISO/IEC" */

	ENC_SIGNAL_SPECIAL_22 = 22, /* "No corresponding industry specification identified" but has specific value. */

	/* 23-255 "Reserved ... [f]or future use by ITU-T | ISO/IEC" */
};

enum ENC_ENUM_CLOSED enc_range {
	ENC_RANGE_LIMITED = 0,
	ENC_RANGE_FULL = 1
};

enum ENC_ENUM_CLOSED enc_chroma_siting {
	ENC_CHROMA_SITING_UNSPECIFIED = 0,
	ENC_CHROMA_SITING_LEFT = 1,
	ENC_CHROMA_SITING_CENTER = 2,
	ENC_CHROMA_SITING_TOP_LEFT = 3
};

struct enc_color_parameters {
	ENC_PUBLIC_STRUCT

	enum enc_h273_signal_type matrix;
	enum enc_h273_signal_type primaries;
	enum enc_h273_signal_type transfer;

	enum enc_range range;
	enum enc_chroma_siting siting;

	/* Incompatible with `ENC_SIGNAL_LIBENCORDER_IDENTITY` for `matrix`. */
	bool dither;
};

ENC_ENUM_OPEN_FLAG(enc_rate_mode, {
	ENC_RATE_NONE = 0,
	ENC_RATE_CONSTANT_QUANTISER = 1, /* QCP */
	ENC_RATE_CONSTANT_QUALITY = 1 << 1, /* uncapped CRF/ICQ, capped VBR/QVBR, target-quality VBR */
	ENC_RATE_AVERAGE_BITRATE = 1 << 2, /* VBR */
	ENC_RATE_CONSTANT_BITRATE = 1 << 3, /* CBR */
	ENC_RATE_CODEC_TIER = 1 << 4, /* Generic codec-defined quality level */
	ENC_RATE_LOSSLESS = 1 << 5
})

/*
 * Different codecs name "fixed input buffer with fill rate" differently
 * MPEG-2/4 -> VBV
 * H.264, HEVC -> HRD CPB
 * AV1 -> "Decoder model"
 */
struct enc_buffer_model {
	/*
	 * Size of the buffer per bitrate in _bits_; `ENC_BUFFER_UNCONSTRAINED`
	 * lets the backend choose, backends will clamp/convert as necessary.
	 */
	uint32_t size_bits;

	/* Startup "buffer fill mark" delay in bits, 0 means "full". */
	uint32_t initial_fullness_bits;
};

#define ENC_BUFFER_UNCONSTRAINED (0)
#define ENC_BITRATE_UNCAPPED (0)

/*
 * See enc_capabilities.quantizer_min/max.
 * Both 0 leaves the rate controller unbounded.
 */
struct enc_quantizer_bounds {
	uint32_t min;
	uint32_t max;
};

struct enc_rate_constant_quantizer {
	uint32_t quantizer_iframes;
	uint32_t quantizer_pframes;
	uint32_t quantizer_bframes;
};

struct enc_rate_constant_quality {
	/* Target on the codec-native quantizer scale. */
	uint32_t quality;

	/*
	 * `ENC_BITRATE_UNCAPPED` leaves quality unconstrained (CRF/ICQ).
	 * Otherwise sets the maximum (capped-VBR).
	 */
	uint64_t max_bitrate;

	struct enc_buffer_model buffer;
	struct enc_quantizer_bounds quantizer_bounds;
};

struct enc_rate_average_bitrate {
	uint64_t bitrate;

	/* `ENC_BITRATE_UNCAPPED` lets the backend pick a multiple of `bitrate`. */
	uint64_t max_bitrate;

	struct enc_buffer_model buffer;
	struct enc_quantizer_bounds quantizer_bounds;
};

struct enc_rate_constant_bitrate {
	uint64_t bitrate;

	struct enc_buffer_model buffer;
	struct enc_quantizer_bounds quantizer_bounds;

	/* Emits filler to hold the channel rate exactly. Required for CBR. */
	bool pad_to_rate;
};

struct enc_rate_codec_tier {
	/* Codec-defined index. ProRes 422 Proxy through 4444 XQ. */
	uint32_t tier;
};

struct enc_rate_control {
	ENC_PUBLIC_STRUCT

	enum enc_rate_mode mode;

	union {
		struct enc_rate_constant_quantizer constant_quantizer;
		struct enc_rate_constant_quality constant_quality;
		struct enc_rate_average_bitrate average_bitrate;
		struct enc_rate_constant_bitrate constant_bitrate;
		struct enc_rate_codec_tier codec_tier;

		uint32_t reserved_size[24];
	} parameters;
};

struct enc_gop {
	ENC_PUBLIC_STRUCT

	/*
	 * 0 selects delta-only, which is required for ProRes and MJPEG.
	 * `ENC_KEYFRAME_LEADING_ONLY` requests a single leading keyframe.
	 */
	uint32_t keyframe_interval;

	uint32_t b_frames;
	uint32_t reference_frames;
	uint32_t temporal_layers;

	bool closed;
};

#define ENC_KEYFRAME_LEADING_ONLY (UINT32_MAX)

/*
 * NOTE: Software encoding may incur significant memeory/PCIe bandwidth
 *       contention compared to hardware encoding.
 */
enum ENC_ENUM_CLOSED enc_acceleration_policy {
	ENC_ACCELERATION_REQUIRE_HARDWARE = 0,
	ENC_ACCELERATION_ALLOW_SOFTWARE = 1
};

/*
 * Overflow allows encoders like NVENC with low -- sometimes only ~3 for the
 * entire _system_ -- to transparently spill over into compute-based encode.
 */
enum ENC_ENUM_CLOSED enc_overflow_policy {
	ENC_OVERFLOW_FAIL = 0,
	ENC_OVERFLOW_SAME_CODEC = 1,
	ENC_OVERFLOW_ANY = 2
};

/*
 * A conversion pass costs bandwidth and a compute dispatch every frame. The
 * caller can usually avoid it by rendering into `preferred_input_format`, so
 * refusing is the default.
 */
enum ENC_ENUM_CLOSED enc_conversion_policy {
	ENC_CONVERSION_FORBID = 0,
	ENC_CONVERSION_ALLOW = 1
};

ENC_ENUM_CLOSED_FLAG(enc_surface_tier, {
	ENC_SURFACE_TIER_NONE = 0,
	ENC_SURFACE_TIER_FUSED = 1,
	ENC_SURFACE_TIER_SPLIT = 1 << 1,
	ENC_SURFACE_TIER_CONVERT = 1 << 2
})

ENC_ENUM_OPEN_FLAG(enc_feature, {
	ENC_FEATURE_NONE = 0,
	ENC_FEATURE_B_FRAMES = 1,
	ENC_FEATURE_RECORD_COMMANDS = 1 << 1,
	ENC_FEATURE_SUBFRAME_OUTPUT = 1 << 2,
	ENC_FEATURE_INTRA_REFRESH = 1 << 3,
	ENC_FEATURE_LONG_TERM_REFS = 1 << 4,
	ENC_FEATURE_TEMPORAL_LAYERS = 1 << 5,
	ENC_FEATURE_ROI_QP = 1 << 6,
	ENC_FEATURE_HDR_METADATA = 1 << 7,
	ENC_FEATURE_ALPHA = 1 << 8,
	ENC_FEATURE_DYNAMIC_BITRATE = 1 << 9,
	ENC_FEATURE_DYNAMIC_RESOLUTION = 1 << 10,
	ENC_FEATURE_IMPORT_FRAMES = 1 << 11
})

enum ENC_ENUM_CLOSED enc_log_level {
	ENC_LOG_OFF = 0,
	ENC_LOG_TRACE = 1,
	ENC_LOG_DEBUG = 2,
	ENC_LOG_INFO = 3,
	ENC_LOG_WARN = 4,
	ENC_LOG_ERROR = 5,
};

typedef void (*enc_log_callback_t)(enum enc_log_level, const char*, void*);

struct enc_instance_info {
	ENC_PUBLIC_STRUCT

	/* `ENC_VERSION` the caller compiled against. */
	uint32_t api_version;

	const char* application_name;

	enc_log_callback_t log;
	void* log_user;
	enum enc_log_level log_level;

	enum enc_backend enabled_backends;
};

struct enc_device_info {
	ENC_PUBLIC_STRUCT

	char name[256];

	enum enc_backend backends;
	enum enc_codec codecs;
	enum enc_native_kind native_kind;

	uint8_t uuid[16];
	// TODO(Emily): Provide basic vendor+device lookup/matching.
	uint32_t vendor_id;
	uint32_t device_id;
	uint32_t driver_version;

	bool is_hardware;
};

ENC_ENUM_OPEN_FLAG(enc_bit_depth, {
	ENC_BIT_DEPTH_NONE = 0,
	ENC_BIT_DEPTH_8 = 1,
	ENC_BIT_DEPTH_10 = 1 << 1,
	ENC_BIT_DEPTH_12 = 1 << 2,
	ENC_BIT_DEPTH_16 = 1 << 3
})

struct enc_capabilities {
	ENC_PUBLIC_STRUCT

	enum enc_backend backend;
	enum enc_codec codec;

	bool is_hardware;

	/* Whether the encoder reads input through the CPU. */
	bool host_access;

	uint32_t min_width;
	uint32_t min_height;
	uint32_t max_width;
	uint32_t max_height;
	uint32_t width_align;
	uint32_t height_align;

	uint32_t max_reference_frames;
	uint32_t max_temporal_layers;
	uint32_t max_retained_frames;

	uint32_t quantizer_min;
	uint32_t quantizer_max;

	enum enc_rate_mode rate_modes;
	enum enc_chroma chroma_formats;
	enum enc_surface_tier surface_tiers;
	enum enc_feature features;

	enum enc_bit_depth bit_depths;

	uint32_t input_format_count;
	enum enc_format input_formats[16];

	/*
	 * The input format the encoder consumes with no conversion and no copy.
	 * Render into this and `enc_instance_query_format` reports
	 * `ENC_SURFACE_TIER_FUSED`. `ENC_FORMAT_UNDEFINED` if none is preferable.
	 */
	enum enc_format preferred_input_format;
};

enum ENC_ENUM_CLOSED enc_session_limit_source {
	ENC_SESSION_LIMIT_UNKNOWN = 0,
	ENC_SESSION_LIMIT_QUERIED = 1,
	ENC_SESSION_LIMIT_PROBED = 2, /* Tried opening streams until error -- could be impacted by other processes. */
	ENC_SESSION_LIMIT_HEURISTIC = 3 /* Guessed based on driver/vendor defaults. */
};

struct enc_concurrency_capabilities {
	ENC_PUBLIC_STRUCT

	/* 0 means unknown or unbounded -- check `source` to disambiguate. */
	uint32_t max_sessions;

	enum enc_session_limit_source source;

	/* Only sessions opened by this instance. */
	uint32_t active_sessions;

	uint32_t encode_engine_count;
	bool sessions_share_engine;
};

struct enc_config {
	ENC_PUBLIC_STRUCT

	enum enc_codec codec;
	uint32_t profile;

	uint32_t width;
	uint32_t height;
	uint32_t frame_rate_numerator;
	uint32_t frame_rate_denominator;

	enum enc_format input_format;
	struct enc_color_parameters color;
	struct enc_rate_control rate;
	struct enc_gop gop;

	enum enc_acceleration_policy accel;
	enum enc_overflow_policy overflow;

	/* ENC_BACKEND_NONE selects automatically. */
	enum enc_backend preferred_backend;

	/*
	 * Whether the encoder may insert a conversion pass when `input_format` is
	 * not one the encoder consumes directly. See `enc_instance_query_format`.
	 */
	enum enc_conversion_policy conversion;

	uint32_t async_depth;
};

struct enc_fallback_event {
	ENC_PUBLIC_STRUCT

	enum enc_backend from_backend;
	enum enc_codec from_codec;
	enum enc_backend to_backend;
	enum enc_codec to_codec;

	enum enc_result reason;
};

typedef void (*enc_fallback_fn)(struct enc_encoder*, const struct enc_fallback_event*, void*);

ENC_API uint32_t enc_version(void);

ENC_API const char* enc_result_name(enum enc_result);

/* Valid until the next failing call on the same thread. */
ENC_API const char* enc_last_error_context(void);

ENC_API const char* enc_backend_name(enum enc_backend);

ENC_API const char* enc_codec_name(enum enc_codec);
ENC_API bool enc_codec_has_keyframes(enum enc_codec);

ENC_API const char* enc_format_name(enum enc_format);

ENC_API enum enc_chroma enc_format_chroma(enum enc_format);
ENC_API uint32_t enc_format_bit_depth(enum enc_format);
ENC_API uint32_t enc_format_plane_count(enum enc_format);
ENC_API bool enc_format_is_rgb(enum enc_format);

ENC_API void enc_instance_info_new(struct enc_instance_info*);
ENC_API void enc_config_new(struct enc_config*, enum enc_codec);

ENC_API enum enc_result enc_instance_new(const struct enc_instance_info*, struct enc_instance**);
ENC_API void enc_instance_delete(struct enc_instance*);

ENC_API enum enc_backend enc_instance_backends(const struct enc_instance*);

/*
 * Call with null array to get the required count, then call with array and
 * array size.
 * Returns `ENC_INCOMPLETE` when the supplied array was too small.
 */
ENC_API enum enc_result enc_enumerate_devices(struct enc_instance*, uint32_t*, struct enc_device_info*);

ENC_API enum enc_result enc_instance_query_capabilities(struct enc_instance*, uint32_t, enum enc_backend, enum enc_codec, struct enc_capabilities*);
ENC_API enum enc_result enc_instance_query_concurrency(struct enc_instance*, uint32_t, enum enc_codec, struct enc_concurrency_capabilities*);

ENC_API enum enc_result enc_instance_query_format(struct enc_instance*, uint32_t, enum enc_backend, enum enc_codec, enum enc_format, enum enc_surface_tier*);

/*
 * Adopt a device from the host renderer via. struct matching `enc_native_kind`.
 * e.g. `enc_vulkan_device_info` from <encorder/encorder_vulkan.h>.
 */
ENC_API enum enc_result enc_device_new_from_native(struct enc_instance*, enum enc_native_kind, const void*, struct enc_device**);

ENC_API void enc_device_delete(struct enc_device*);

/* Whether this device can create encoders, or only answer queries. */
ENC_API bool enc_device_can_encode(const struct enc_device*);

ENC_API enum enc_result enc_device_get_info(const struct enc_device*, struct enc_device_info*);
ENC_API enum enc_result enc_device_query_capabilities(struct enc_device*, enum enc_backend, enum enc_codec, struct enc_capabilities*);
ENC_API enum enc_result enc_device_query_concurrency(struct enc_device*, enum enc_codec, struct enc_concurrency_capabilities*);

#endif
