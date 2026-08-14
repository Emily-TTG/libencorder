#pragma once

#include <encorder/encorder.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ENC_TEST_RESULT(...) (enc_test_log_result(#__VA_ARGS__, __VA_ARGS__))
#define ENC_TEST_FATAL(...) do { if(enc_test_log_result(#__VA_ARGS__, __VA_ARGS__)) exit(EXIT_FAILURE); } while(0)

static bool enc_test_log_result(const char* context, const enum enc_result result) {
	const char* error_context = enc_last_error_context();

	if(fprintf(
			stderr,
			"%s:%s%s (%s)\n",
			context,
			error_context || error_context[0] ? " " : "",
			error_context ? error_context : "",
			enc_result_name(result)) < 0) perror("fprintf");

	return result < 0;
}

static const char* enc_test_level_name(const enum enc_log_level level) {
	switch(level) {
		case ENC_LOG_TRACE: return "trace";
		case ENC_LOG_DEBUG: return "debug";
		case ENC_LOG_INFO: return "info";
		case ENC_LOG_WARN: return "warn";
		case ENC_LOG_ERROR: return "error";
		default: return "?";
	}
}

static void enc_test_on_log(const enum enc_log_level level, const char* const message, void*) {
	fprintf(stderr, "(encorder) [%s] %s\n", enc_test_level_name(level), message);
}

static void enc_test_dump_codecs(const enum enc_codec codecs, const char* base_indent) {
	if(codecs == ENC_CODEC_NONE) {
		printf("%scodecs: none\n", base_indent);
		return;
	}

	printf("%s- codecs:\n", base_indent);

	for(unsigned bit = 1; bit; bit <<= 1u) {
		if(codecs & bit) {
			printf("%s\t- %s\n", base_indent, enc_codec_name((enum enc_codec) bit));
		}
	}
}

static void enc_test_dump_backends(const enum enc_backend backends, const char* base_indent) {
	printf("%s- backends:\n", base_indent);

	for(unsigned bit = 1; bit; bit <<= 1u) {
		if(backends & bit) {
			printf("%s\t- %s\n", base_indent, enc_backend_name((enum enc_backend) bit));
		}
	}
}

#ifdef __cplusplus
}
#endif
