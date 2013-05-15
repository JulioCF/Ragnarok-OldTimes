// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _NULLPO_H_
#define _NULLPO_H_

#include "../common/cbasetypes.h"

#define NLP_MARK __FILE__, __LINE__, __func__

// enabled by default on debug builds
#if defined(DEBUG) && !defined(NULLPO_CHECK)
#define NULLPO_CHECK
#endif

#if defined(NULLPO_CHECK)

#define nullpo_ret(t) \
	if (nullpo_chk(NLP_MARK, (void *)(t))) {return(0);}

#define nullpo_retv(t) \
	if (nullpo_chk(NLP_MARK, (void *)(t))) {return;}

#define nullpo_retr(ret, t) \
	if (nullpo_chk(NLP_MARK, (void *)(t))) {return(ret);}

#define nullpo_retb(t) \
	if (nullpo_chk(NLP_MARK, (void *)(t))) {break;}

#if __STDC_VERSION__ >= 199901L
#define nullpo_ret_f(t, fmt, ...) \
	if (nullpo_chk_f(NLP_MARK, (void *)(t), (fmt), __VA_ARGS__)) {return(0);}

#define nullpo_retv_f(t, fmt, ...) \
	if (nullpo_chk_f(NLP_MARK, (void *)(t), (fmt), __VA_ARGS__)) {return;}

#define nullpo_retr_f(ret, t, fmt, ...) \
	if (nullpo_chk_f(NLP_MARK, (void *)(t), (fmt), __VA_ARGS__)) {return(ret);}

#define nullpo_retb_f(t, fmt, ...) \
	if (nullpo_chk_f(NLP_MARK, (void *)(t), (fmt), __VA_ARGS__)) {break;}

#elif __GNUC__ >= 2
#define nullpo_ret_f(t, fmt, args...) \
	if (nullpo_chk_f(NLP_MARK, (void *)(t), (fmt), ## args)) {return(0);}

#define nullpo_retv_f(t, fmt, args...) \
	if (nullpo_chk_f(NLP_MARK, (void *)(t), (fmt), ## args)) {return;}

#define nullpo_retr_f(ret, t, fmt, args...) \
	if (nullpo_chk_f(NLP_MARK, (void *)(t), (fmt), ## args)) {return(ret);}

#define nullpo_retb_f(t, fmt, args...) \
	if (nullpo_chk_f(NLP_MARK, (void *)(t), (fmt), ## args)) {break;}

#else
#endif

#else /* NULLPO_CHECK */

#define nullpo_ret(t) (void)(t)
#define nullpo_retv(t) (void)(t)
#define nullpo_retr(ret, t) (void)(t)
#define nullpo_retb(t) (void)(t)

#if __STDC_VERSION__ >= 199901L
#define nullpo_ret_f(t, fmt, ...) (void)(t)
#define nullpo_retv_f(t, fmt, ...) (void)(t)
#define nullpo_retr_f(ret, t, fmt, ...) (void)(t)
#define nullpo_retb_f(t, fmt, ...) (void)(t)

#elif __GNUC__ >= 2
#define nullpo_ret_f(t, fmt, args...) (void)(t)
#define nullpo_retv_f(t, fmt, args...) (void)(t)
#define nullpo_retr_f(ret, t, fmt, args...) (void)(t)
#define nullpo_retb_f(t, fmt, args...) (void)(t)

#else
#endif

#endif /* NULLPO_CHECK */


int nullpo_chk(const char *file, int line, const char *func, const void *target);

int nullpo_chk_f(const char *file, int line, const char *func,
				 const void *target, const char *fmt, ...)
				 __attribute__((format(printf,5,6)));

void nullpo_info(const char *file, int line, const char *func);

void nullpo_info_f(const char *file, int line, const char *func,
				   const char *fmt, ...)__attribute__((format(printf,4,5)));

#endif /* _NULLPO_H_ */
