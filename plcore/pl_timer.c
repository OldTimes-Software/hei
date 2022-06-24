/**
 * Hei Platform Library
 * Copyright (C) 2017-2022 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "pl_private.h"

/* MSC doesn't provide clock_gettime, so blergh */
#if defined( _MSC_VER )
#	include <Windows.h>
#endif

#define NSEC_PER_SEC 1000000000

/** \fn struct timespec timespec_normalise(struct timespec ts)
 *  \brief Normalises a timespec structure.
 *
 * Returns a normalised version of a timespec structure, according to the
 * following rules:
 *
 * 1) If tv_nsec is >1,000,000,00 or <-1,000,000,000, flatten the surplus
 *    nanoseconds into the tv_sec field.
 *
 * 2) If tv_sec is >0 and tv_nsec is <0, decrement tv_sec and roll tv_nsec up
 *    to represent the same value on the positive side of the new tv_sec.
 *
 * 3) If tv_sec is <0 and tv_nsec is >0, increment tv_sec and roll tv_nsec down
 *    to represent the same value on the negative side of the new tv_sec.
 *
 * Originally written by Daniel Collins (2017), used with permission.
 * https://github.com/solemnwarning/timespec
 */
static struct timespec timespec_normalise( struct timespec ts ) {
	while ( ts.tv_nsec >= NSEC_PER_SEC ) {
		++( ts.tv_sec );
		ts.tv_nsec -= NSEC_PER_SEC;
	}

	while ( ts.tv_nsec <= -NSEC_PER_SEC ) {
		--( ts.tv_sec );
		ts.tv_nsec += NSEC_PER_SEC;
	}

	if ( ts.tv_nsec < 0 && ts.tv_sec > 0 ) {
		/* Negative nanoseconds while seconds is positive.
		 * Decrement tv_sec and roll tv_nsec over.
		 */
		--( ts.tv_sec );
		ts.tv_nsec = NSEC_PER_SEC - ( -1 * ts.tv_nsec );
	} else if ( ts.tv_nsec > 0 && ts.tv_sec < 0 ) {
		/* Positive nanoseconds while seconds is negative.
		 * Increment tv_sec and roll tv_nsec over.
		 */
		++( ts.tv_sec );
		ts.tv_nsec = -NSEC_PER_SEC - ( -1 * ts.tv_nsec );
	}

	return ts;
}

/** \fn struct timespec timespec_sub(struct timespec ts1, struct timespec ts2)
 *  \brief Returns the result of subtracting ts2 from ts1.
 *  Originally written by Daniel Collins (2017), used with permission.
 *  https://github.com/solemnwarning/timespec
 */
static struct timespec timespec_sub( struct timespec ts1, struct timespec ts2 ) {
	/* Normalise inputs to prevent tv_nsec rollover if whole-second values
	 * are packed in it.
	 */
	ts1 = timespec_normalise( ts1 );
	ts2 = timespec_normalise( ts2 );

	ts1.tv_sec -= ts2.tv_sec;
	ts1.tv_nsec -= ts2.tv_nsec;

	return timespec_normalise( ts1 );
}

/** \fn double timespec_to_double(struct timespec ts)
 *  \brief Converts a timespec to a fractional number of seconds.
 *  Originally written by Daniel Collins (2017), used with permission.
 */
static double timespec_to_double( const struct timespec *ts ) {
	return ( ( double ) ( ts->tv_sec ) + ( ( double ) ( ts->tv_nsec ) / NSEC_PER_SEC ) );
}

/** \fn long timespec_to_ms(struct timespec ts)
 *  \brief Converts a timespec to an integer number of milliseconds.
 *  Originally written by Daniel Collins (2017), used with permission.
 *  https://github.com/solemnwarning/timespec
 */
static long timespec_to_ms( const struct timespec *ts ) {
	return ( ts->tv_sec * 1000 ) + ( ts->tv_nsec / 1000000 );
}

static bool GetTime( struct timespec *ts ) {
#if defined( _MSC_VER )
	/* based on public-domain implementation here;
	 * https://github.com/msys2-contrib/mingw-w64/blob/master/mingw-w64-libraries/winpthreads/src/clock.c
	 */

	LARGE_INTEGER pf;
	if ( QueryPerformanceFrequency( &pf ) == 0 ) {
		PlReportBasicError( PL_RESULT_FAIL );
		return false;
	}

	LARGE_INTEGER pc;
	if ( QueryPerformanceFrequency( &pc ) == 0 ) {
		PlReportBasicError( PL_RESULT_FAIL );
		return false;
	}

#	define POW10_7 10000000
#	define POW10_9 1000000000

	ts->tv_sec = pc.QuadPart / pf.QuadPart;
	ts->tv_nsec = ( int ) ( ( ( pc.QuadPart % pf.QuadPart ) * POW10_9 + ( pf.QuadPart >> 1 ) ) / pf.QuadPart );
	if ( ts->tv_nsec >= POW10_9 ) {
		ts->tv_sec++;
		ts->tv_nsec -= POW10_9;
	}
#else
	if ( clock_gettime( CLOCK_MONOTONIC, ts ) != 0 ) {
		PlReportBasicError( PL_RESULT_FAIL );
		return false;
	}
#endif
	return true;
}

/****************************************
 * Public Interface
 ****************************************/

double PlGetCurrentSeconds( void ) {
	struct timespec ts;
	if ( !GetTime( &ts ) ) {
		return 0;
	}
	return timespec_to_double( &ts );
}

long PlGetCurrentMilliseconds( void ) {
	struct timespec ts;
	if ( !GetTime( &ts ) ) {
		return 0;
	}
	return timespec_to_ms( &ts );
}
