/*
 * Copyright 2021-2022 Qorvo US, Inc.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef QORVO_UWBMAC_LOG_H_
#define QORVO_UWBMAC_LOG_H_

#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

#if defined(__ANDROID__)
#include <log/log_main.h>

#define QLOGI(...) IF_ALOGI() ALOGI(__VA_ARGS__)
#define QLOGE(...) IF_ALOGE() ALOGE(__VA_ARGS__)
#define QLOGW(...) IF_ALOGW() ALOGW(__VA_ARGS__)
#define QLOGD(...) IF_ALOGD() ALOGD(__VA_ARGS__)

#elif defined(__linux__)
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

enum qorvo_log_levels {
	LOG_LEVEL_DEBUG = 3,
	LOG_LEVEL_INFORMATION = 4,
	LOG_LEVEL_WARNING = 6,
	LOG_LEVEL_ERROR = 7,
};

extern enum qorvo_log_levels qorvo_log_level;

void qorvo_log_print(enum qorvo_log_levels level, const char *tag, const char *fmt, ...);

#define QLOGE(FORMAT, ...)                                                        \
	do {                                                                      \
		qorvo_log_print(LOG_LEVEL_ERROR, LOG_TAG, FORMAT, ##__VA_ARGS__); \
	} while (0)

#define QLOGW(FORMAT, ...)                                                          \
	do {                                                                        \
		qorvo_log_print(LOG_LEVEL_WARNING, LOG_TAG, FORMAT, ##__VA_ARGS__); \
	} while (0)

#ifndef NDEBUG

#define QLOGI(FORMAT, ...)                                                              \
	do {                                                                            \
		qorvo_log_print(LOG_LEVEL_INFORMATION, LOG_TAG, FORMAT, ##__VA_ARGS__); \
	} while (0)

#define QLOGD(FORMAT, ...)                                                        \
	do {                                                                      \
		qorvo_log_print(LOG_LEVEL_DEBUG, LOG_TAG, FORMAT, ##__VA_ARGS__); \
	} while (0)

#else

#define QLOGI(...) \
	do {       \
	} while (0)

#define QLOGD(...) \
	do {       \
	} while (0)

#endif

/* Zephyr */
#elif defined(__ZEPHYR__)
#include <qlog.h>
#else
#define QLOGI(...) \
	do {       \
	} while (0)
#define QLOGE(...) \
	do {       \
	} while (0)
#define QLOGW(...) \
	do {       \
	} while (0)
#define QLOGD(...) \
	do {       \
	} while (0)

#endif

#endif // QORVO_UWBMAC_LOG_H_
