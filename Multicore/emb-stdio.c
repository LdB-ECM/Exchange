#include <stdbool.h>			// Standard C library needed for bool
#include <stdint.h>				// Standard C library needed for uint8_t, uint32_t etc
#include <stdarg.h>				// Standard C library needed for varadic arguments
#include <float.h>				// Standard C library needed for DBL_EPSILON
#include <string.h>				// Standard C librray needed for strnlen used
#include <stdlib.h>				// Standard C library needed for strtol
#include <math.h>				// Standard C library needed for modf
#include <stddef.h>				// Standard C library needed for ptrdiff_t
#include <ctype.h>				// Standard C library needed for toupper, isdigit etc
#include <wchar.h>				// Standard C library needed for wchar_t
#ifndef _WIN32					// Testing for this unit is done in Windows console need to exclude SmartStart
#include "rpi-SmartStart.h"     // SmartStart system when compiling for embedded system
#endif
#include "emb-stdio.h"			// This units header

/*--------------------------------------------------------------------------}
{ EMBEDDED SYSTEMS DO NOT HAVE M/SOFT BUFFER OVERRUN SAFE STRING FUNCTIONS  }
{    ALL WE CAN DO IS MACRO THE SAFE CALLS TO NORMAL C STANDARD CALLS		} 
{--------------------------------------------------------------------------*/
#ifndef _WIN32					
#define	strncpy_s(a,b,c,d) strncpy(a,c,d)
#define strcpy_s(a,b,c) strcpy(a,c)
#define mbstowcs_s(a,b,c,d,e) mbstowcs(b,d,e)
#define wcscpy_s(a,b,c) wcscpy(a,c)
#endif

/***************************************************************************}
{		 		    PRIVATE VARIABLE DEFINITIONS				            }
****************************************************************************/

/*--------------------------------------------------------------------------}
{				   CONSOLE WRITE CHARACTER HANDLER							}
{--------------------------------------------------------------------------*/
static CHAR_OUTPUT_HANDLER Console_WriteChar = 0;


static int checkdigit (char c) {
	if ((c >= '0') && (c <= '9')) return 1;
	return 0;
}

static int skip_atoi(const char **s)
{
	int i = 0;

	while (checkdigit(**s))
		i = i * 10 + *((*s)++) - '0';
	return i;
}

#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SMALL	32		/* Must be 32 == 0x20 */
#define SPECIAL	64		/* 0x */

struct cvt_struct 
{
	unsigned int ndigits : 7;         // Gives max 128 characters
	unsigned int decpt : 7;           // Gives max 128 decimals
	unsigned int sign : 1;            // Sign 0 = positive   1 = negative
	unsigned int eflag : 1;			  // Exponent flag
};

static char *cvt(double arg, struct cvt_struct* cvs, char *buf, int CVTBUFSIZE) {
	int r2;
	double fi, fj;
	char *p, *p1;

	if (CVTBUFSIZE > 128) CVTBUFSIZE = 128;
	r2 = 0;
	cvs->sign = false;
	p = &buf[0];
	if (arg < 0) {
		cvs->sign = true;
		arg = -arg;
	}
	arg = modf(arg, &fi);
	p1 = &buf[CVTBUFSIZE];

	if (fi != 0) {
		p1 = &buf[CVTBUFSIZE];
		while (fi != 0) {
			fj = modf(fi / 10, &fi);
			*--p1 = (int)((fj + .03) * 10) + '0';
			r2++;
			cvs->ndigits++;
		}
		while (p1 < &buf[CVTBUFSIZE]) *p++ = *p1++;
	}
	else if (arg > 0) {
		while ((fj = arg * 10) < 1) {
			arg = fj;
			r2--;
		}
	}
	p1 = &buf[CVTBUFSIZE - 2];
	if (cvs->eflag == 0) p1 += r2;
	cvs->decpt = r2;
	if (p1 < &buf[0]) {
		buf[0] = '\0';
		return buf;
	}
	while (p <= p1 && p < &buf[CVTBUFSIZE] && arg > DBL_EPSILON) {
		arg *= 10;
		arg = modf(arg, &fj);
		*p++ = (int)fj + '0';
		cvs->ndigits++;
	}
	/*p = p1;
	*p1 += 5;
	while (*p1 > '9') {
		*p1 = '0';
		if (p1 > buf) {
			++*--p1;
		}
		else {
			*p1 = '1';
			(cvs->decpt)++;
			if (cvs->eflag == 0) {
				if (p > buf) *p = '0';
				p++;
			}
		}
	}*/
	*p = '\0';
	return buf;
}

/*--------------------------------------------------------------------------}
. This formats a number and returns a string pointed to by txt
. 13Oct2017 LdB
.--------------------------------------------------------------------------*/
static char* NumStr (double value, char* retStr, int retStrSize) 
{
	uint_fast8_t spos = 0;
	struct cvt_struct cvs = { 0 };
	char Ns[256];

	if (retStr == 0 || retStrSize == 0) return (retStr);			// String buffer error
	retStr[0] = '\0';												// Clear return string
	cvt(value, &cvs, &Ns[0], sizeof(Ns));							// Create number string of given dp
	if (cvs.sign) {
		retStr[spos++] = '-';										// We have a negative number
	}
	if (cvs.decpt == 0) {
		retStr[spos++] = '0';										// We must be "0.xxxxx
	} else {
		for (uint_fast8_t i = 0; i < cvs.decpt; i++)
			retStr[spos++] = Ns[i];									// Copy the whole number
	}
	if (cvs.decpt > 0) {											// We have decimal point
		retStr[spos++] = '.';										// Add decimal point
		for (uint_fast8_t i = cvs.decpt; i < cvs.ndigits; i++)
			retStr[spos++] = Ns[i];									// Move decimal number
	}
	retStr[spos] = '\0';											// Make asciiz
	return (&retStr[spos]);											// String end returned
}

int do_div (long* n, int base)
{
	int res = *n % base;
	*n /= base;
	return res;
}

static char *number(char *str, long num, int base, int size, int precision,
	int type)
{
	/* we are called with base 8, 10 or 16, only, thus don't need "G..."  */
	static const char digits[16] = "0123456789ABCDEF"; /* "GHIJKLMNOPQRSTUVWXYZ"; */

	char tmp[66];
	char c, sign, locase;
	int i;

	/* locase = 0 or 0x20. ORing digits or letters with 'locase'
	* produces same digits or (maybe lowercased) letters */
	locase = (type & SMALL);
	if (type & LEFT)
		type &= ~ZEROPAD;
	if (base < 2 || base > 16)
		return NULL;
	c = (type & ZEROPAD) ? '0' : ' ';
	sign = 0;
	if (type & SIGN) {
		if (num < 0) {
			sign = '-';
			num = -num;
			size--;
		}
		else if (type & PLUS) {
			sign = '+';
			size--;
		}
		else if (type & SPACE) {
			sign = ' ';
			size--;
		}
	}
	if (type & SPECIAL) {
		if (base == 16)
			size -= 2;
		else if (base == 8)
			size--;
	}
	i = 0;
	if (num == 0)
		tmp[i++] = '0';
	else
		while (num != 0)
			tmp[i++] = (digits[do_div(&num, base)] | locase);
	if (i > precision)
		precision = i;
	size -= precision;
	if (!(type & (ZEROPAD + LEFT)))
		while (size-- > 0)
			*str++ = ' ';
	if (sign)
		*str++ = sign;
	if (type & SPECIAL) {
		if (base == 8)
			*str++ = '0';
		else if (base == 16) {
			*str++ = '0';
			*str++ = ('X' | locase);
		}
	}
	if (!(type & LEFT))
		while (size-- > 0)
			*str++ = c;
	while (i < precision--)
		*str++ = '0';
	while (i-- > 0)
		*str++ = tmp[i];
	while (size-- > 0)
		*str++ = ' ';
	return str;
}






/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{				 	PUBLIC FORMATTED OUTPUT ROUTINES					    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*-[ vsscanf ]--------------------------------------------------------------}
{ Reads data from buf and stores them according to parameter format in the  }
{ locations pointed by the elements in the variable argument list arg.		}
{ Internally, the function retrieves arguments from the list identified as  }
{ arg as if va_arg was used on it, and thus the state of arg is likely to   }
{ be altered by the call. In any case, arg should have been initialized by  }
{ va_start at some point before the call, and it is expected to be released }
{ by va_end at some point after the call.									}
{ 14Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
/* Built around: http://www.cplusplus.com/reference/Cstdio/scanf/           }
{  Format specifier takes form:   "%[*][width][length]specifier"            }
{--------------------------------------------------------------------------*/
int	vsscanf (const char *str, const char *fmt, va_list arg) 
{
	int count = 0;													// Count of specifiers matched
	int base = 10;													// Specifier base
	int width = 0;													// Specifier width
	const char* tp = str;											// Temp pointer starts at str
	const char* spos;												// Start specifier position
	char specifier, lenSpecifier;									// Specifier characters
	char* endPtr;													// Temp end ptr
	char tmp[256];													// Temp buffer
	bool dontStore = false;											// Dont store starts false
	bool notSet = false;											// Not in set starts false
	bool doubleLS = false;											// Double letter of length specifier

	if (fmt != 0 || str != 0) return (0);							// One of the pointers invalid
	while (*tp != '\0' && *fmt != '\0') {
		while (isspace(*fmt)) fmt++;								// find next format specifier start
		if (*fmt == '%')											// Format is a specifier otherwise its a literal
		{
			fmt++;													// Advance format pointer
			if (*fmt == '*') {										// Check for dont store flag
				dontStore = true;									// Set dont store flag
				tp++;												// Next character
			}
			if (isdigit(*fmt)) {									// Look for width specifier
				int cc;
				const char* wspos = fmt;						    // Width specifier start pos
				while (isdigit(*fmt)) fmt++;						// Find end of width specifier
				cc = fmt - wspos;									// Character count for width
				strncpy_s(&tmp[0], _countof(tmp), wspos, cc);		// Copy width specifier
				tmp[cc] = '\0';										// Terminate string
				width = strtol(wspos, &endPtr, 10);					// Calculate specifier width
			}
			else width = 256;
			if ((*fmt == 'h') || (*fmt == 'l') ||
				(*fmt == 'j') || (*fmt == 'z') ||
				(*fmt == 't') || (*fmt == 'L')) {					// Special length specifier
				lenSpecifier = *fmt;								// Hold length specifier
				if ((*fmt == 'h') || (*fmt == 'l')) {				// Check for hh and ll
					if (fmt[1] == *fmt) 							// Peek at next character for match ... 'hh' or 'll'
					{
						doubleLS = true;							// Set the double LS flag
						fmt++;										// Next format character
					}
				}
				fmt++;												// Next format character
			} else lenSpecifier = '\0';								// No length specifier
			specifier = *fmt++;										// Load specifier					

			/* FIND START OF SPECIFIER DATA  */
			switch (specifier) {
			case '\0':												// Terminator found on format string
			{
				return (count);										// Format string ran out of characters
			}

			case '[':												// We have a Set marker
			{
				if (*fmt == '^') {									// Not in set requested
					notSet = true;									// Set notSet flag
					fmt++;											// Move to next character
				}
				break;
			}

			/* string or character find start */
			case 'c':
			case 's':
			{
				// We are looking for non white space to start
				while (isspace(*tp)) tp++;
				break;
			}

			/* Integer/decimal/unsigned/hex/octal and pointer find start */
			case 'i':
			case 'd':
			{
				// We are looking for  -, + or digit to start number
				while (!(isdigit(*tp) || (*tp == '-') || (*tp == '+')) && (*tp != '\0'))  tp++;
				break;
			}

			/* unsigned/hex/octal and pointer find start */
			case 'u':
			case 'x':
			case 'X':
			case 'o':
			case 'p':
			{
				// We are looking for  digit to start number
				while (!isdigit(*tp) && (*tp != '\0'))  tp++;
				break;
			}


			/* Floats/double/double double find start */
			case 'a':
			case 'A':
			case 'e':
			case 'E':
			case 'f':
			case 'F':
			case 'g':
			case 'G':
			{
				// We are looking for  -,'.' or digit to start number
				while (!(isdigit(*tp) || (*tp == '.') || (*tp == '-') || (*tp == '+')) && (*tp != '\0'))  tp++;
				break;
			}

			}  /* end switch */

			/* HOLD MATCH STARTED POSITION */
			if (specifier != '[') {									// Not a creating set characters
				if (*tp == '\0') return(count);						// End of string reached

				spos = tp;											// String match starts here
				tp++;												// Move to next character

				if (*tp == '\0') return(count);						// End of string reached
			}
			else {
				if (*fmt == '\0') return(count);					// End of format reached

				spos = fmt;											// String match starts here
				fmt++;												// Move to next format character

				if (*fmt == '\0') return(count);					// End of format reached
			}

			/* FIND END OF SPECIFIER DATA */
			switch (specifier) {


			/* character find end */
			case 'c':
			{
				break;
			}

			/* set find end */
			case '[':
			{
				while (*fmt != ']' && *fmt != '\0') fmt++;			// Keep reading until set end
				break;
			}

			/* string find end */
			case 's':
			{
				// We are looking for white space to end
				while (!isspace(*tp) && (*tp != '\0') && width != 0) {
					tp++;
					width--;
				}
				break;
			}

			/* decimal and unsigned integer find end */
			case 'd':
			case 'u':
			{
				// We are looking for non digit to end number
				while (isdigit(*tp))  tp++;
				break;
			}

			/* octal find end */
			case 'o':
			{
				base = 8;													// Set base to 8
				while (*tp >= '0' && *tp <= '7') tp++;						// Keep reading while characters are 0-7
				break;
			}

			/* octal find end */
			case 'x':
			case 'X':
			{
				base = 16;													// Set base to 16
				while (isdigit(*tp) || (*tp >= 'a' && *tp <= 'f')
					|| (*tp >= 'A' && *tp <= 'F'))  tp++;					// keep advancing so long as 0-9, a-f, A-F
				break;
			}

			/* integer and pointer find end ... allows 0x  0b 0 at start for binary,hex and octal and ends when no digits */
			case 'i':
			case 'p':
			{
				if (*spos == '0') {
					switch (*tp) {
					case 'x': {												// Hex entry
						base = 16;											// Set base to 16
						tp++;												// Advance tp
						while (isdigit(*tp) || (*tp >= 'a' && *tp <= 'f')
							|| (*tp >= 'A' && *tp <= 'F'))  tp++;			// keep advancing so long as 0-9, a-f, A-F
						break;
					}
					case 'b': {												// Binary entry
						base = 2;											// Set base to 2
						tp++;												// Advance tp
						while (*tp == '0' || *tp == '1') tp++;				// Keep scanning so long as we have 0 or 1's
						break;
					}
					default:												// Octal entry
						base = 8;											// Set base to 8
						tp++;												// Advance tp
						while (*tp >= '0' && *tp <= '7') tp++;				// Keep reading while characters are 0-7
						break;
					} /* switch */
				}
				else {
					// We are looking for any non decimal digit to end number
					while (isdigit(*tp))  tp++;
				}
				break;
			}

			/* Floats/double/double double find end */
			case 'a':
			case 'A':
			case 'e':
			case 'E':
			case 'f':
			case 'F':
			case 'g':
			case 'G':
			{
				bool exitflag = false;
				bool decflag = false;
				bool eflag = false;
				if (*spos == '.') decflag = true;
				do {
					if (!decflag && *tp == '.') {
						decflag = true;
						tp++;
					}
					if (!eflag && (*tp == 'e' || *tp == 'E')) {
						eflag = true;
						tp++;
						/* look for negative index  E-10 etc*/
						if (*tp == '-') tp++;
					}
					if (isdigit(*tp)) tp++;
					else exitflag = true;
				} while (!exitflag);
				break;
			}

			}  /* switch end */


			/* COPY DATA FROM MATCH START TO END TO TEMP BUFFER */
			if (specifier != 'c') {
				int cc;
				if (specifier != '[') cc = tp - spos;				// Not set specifier is from data
					else cc = fmt - spos;							// Set specifier is from format
				strncpy_s(&tmp[0], _countof(tmp), spos, cc);		// Copy to temp buffer
				tmp[cc] = '\0';										// Terminate string
			}

			/* PROCESS THE FOUND SPECIFIER */
			switch (specifier) {

			case 'u':												// Unsigned integer ... base = 10
			case 'o':												// Octal .. same as 'u' but base = 8
			case 'x':												// Hex .. same as 'u' but base = 16
			case 'X':												// Hex .. same as 'u' but base = 16
			{
				if (dontStore) break;								// Request is to not store result
				switch (lenSpecifier) {
				case '\0':											// Length not specified 
				{
					unsigned int i;
					i = strtoul(&tmp[0], &endPtr, base);			// Convert unsigned integer	
					*va_arg(arg, unsigned int *) = i;				// Store the value
					break;
				}
				case 'h':
				{
					if (doubleLS) {									// Length specifier was "hh"
						unsigned char i;
						i = (unsigned char) strtoul(&tmp[0], &endPtr, base);		// Convert unsigned char
						*va_arg(arg, unsigned char *) = i;			// Store the value
						break;
					}
					else {											// Length specifier was "h"
						unsigned short i;
						i = (unsigned short) strtoul(&tmp[0], &endPtr, base);		// Convert unsigned char
						*va_arg(arg, unsigned short *) = i;			// Store the value
						break;
					}
				}
				case 'l':
				{
					if (doubleLS) {									// Length specifier was "ll"
						unsigned long long i;
						i = strtoull(&tmp[0], &endPtr, base);		// Convert unsigned char
						*va_arg(arg, unsigned long long *) = i;		// Store the value
						break;
					}
					else {											// Length specifier was "l"
						unsigned long i;
						i = strtoul(&tmp[0], &endPtr, base);		// Convert unsigned char
						*va_arg(arg, unsigned long *) = i;			// Store the value
						break;
					}
				}
				case 'j':											// Length specifier was 'j'
				{
					uintmax_t i;
					i = strtoul(&tmp[0], &endPtr, base);			// Convert unsigned char
					*va_arg(arg, uintmax_t *) = i;					// Store the value
					break;
				}
				case 'z':											// Length specifier was 'z'
				{
					size_t i;
					i = strtoul(&tmp[0], &endPtr, base);			// Convert unsigned char
					*va_arg(arg, size_t *) = i;						// Store the value
					break;
				}
				case 't':											// Length specifier was 't'
				{
					ptrdiff_t i;
					i = strtoul(&tmp[0], &endPtr, base);			// Convert unsigned char
					*va_arg(arg, ptrdiff_t *) = i;					// Store the value
					break;
				}
				} /* end switch lenSpecifier */
				break;
			}
			case 'd':												// Decimal integer .. base = 10
			case 'i':												// Integer .. same as d but base got from string
			{
				if (dontStore) break;								// Request is to not store result
				switch (lenSpecifier) {
				case '\0':											// Length not specified 
				{
					int i;
					i = strtol(&tmp[0], &endPtr, base);				// Convert integer
					*va_arg(arg, int *) = i;						// Store the value 
					break;
				}
				case 'h':
				{
					if (doubleLS) {									// Length specifier was "hh"
						signed char i;
						i = (signed char) strtol(&tmp[0], &endPtr, base); // Convert integer
						*va_arg(arg, signed char *) = i;			// Store the value 
						break;
					}
					else {											// Length specifier was "h"
						short i;
						i = (short) strtol(&tmp[0], &endPtr, base);	// Convert integer
						*va_arg(arg, short *) = i;					// Store the value 
						break;
					}
				}
				case 'l':
				{
					if (doubleLS) {									// Length specifier was "ll"
						long long i;
						i = strtoll(&tmp[0], &endPtr, base);		// Convert integer
						*va_arg(arg, long long *) = i;				// Store the value 
						break;
					}
					else {											// Length specifier was "l"
						long i;
						i = strtol(&tmp[0], &endPtr, base);			// Convert integer
						*va_arg(arg, long *) = i;					// Store the value 
						break;
					}
				}
				case 'j':											// Length specifier was 'j' 
				{
					intmax_t i;
					i = strtol(&tmp[0], &endPtr, base);				// Convert integer
					*va_arg(arg, intmax_t *) = i;					// Store the value 
					break;
				}
				case 'z':											// Length specifier was 'z' 
				{
					size_t i;
					i = strtol(&tmp[0], &endPtr, base);				// Convert integer
					*va_arg(arg, size_t *) = i;						// Store the value 
					break;
				}
				case 't':											// Length specifier was 't' 
				{
					ptrdiff_t i;
					i = strtol(&tmp[0], &endPtr, base);				// Convert integer
					*va_arg(arg, ptrdiff_t *) = i;					// Store the value 
					break;
				}
				} /* end switch lenSpecifier */
				break;
			}
			case 'e':
			case 'g':
			case 'E':
			case 'G':
			case 'f':
			case 'F':												// Float
			{
				if (dontStore) break;								// Request is to not store result
				switch (lenSpecifier) {
				case '\0':											// Length not specified 
				{
					float f;
					f = strtof(&tmp[0], &endPtr);					// Convert float
					*va_arg(arg, float *) = f;						// Store the value
					break;
				}
				case 'l':
				{
					double d;
					d = strtod(&tmp[0], &endPtr);					// Convert double
					*va_arg(arg, double *) = d;						// Store the value
					break;
				}
				case 'L':
				{
					long double ld;
					ld = strtold(&tmp[0], &endPtr);					// Convert long double
					*va_arg(arg, long double *) = ld;				// Store the value
					break;
				}
				} /* end switch lenSpecifier */
				break;
			}


			case 'p':												// Pointer  -- special no length specifier possible
			{
				void* p;
				p = (void*)(uintptr_t)strtol(&tmp[0], &endPtr, 16);	// Convert hexadecimal
				if (!dontStore) *va_arg(arg, void **) = p;			// If dontstore flag false then Store the value
				break;
			}


			case 's':												// String
			{
				if (dontStore) break;								// Request is to not store result
				switch (lenSpecifier) {
				case '\0':											// Length not specified 
				{
					strcpy_s(va_arg(arg, char *), _countof(tmp), &tmp[0]);// If dontstore flag false then Store the value
					break;
				}
				case 'l':											// Length specifier 'l' 
				{
					wchar_t wc[256];								// Temp buffer
					size_t cSize = strlen(&tmp[0]) + 1;				// Size of text scanned
                    #ifdef _WIN32
					size_t oSize;
					#endif
					mbstowcs_s(&oSize, &wc[0], cSize, &tmp[0], cSize); // Convert to wide character
					wcscpy_s(va_arg(arg, wchar_t *), _countof(wc), &wc[0]); // Store the wide string
					break;
				}
				} /* end switch lenSpecifier */
				break;
			}

			case 'c':												// Character
			{
				if (dontStore) break;								// Request is to not store result
				switch (lenSpecifier) {
				case '\0':											// Length not specified 
				{
					char c = *spos;
					*va_arg(arg, char *) = c;						// Store the value
					break;
				}
				case 'l':											// Length specifier 'l' 
				{
					wchar_t wc;
					#ifdef _WIN32
					size_t oSize;
					#endif
					mbstowcs_s(&oSize, &wc, 1, spos, 1);			// Convert to wide character
					*va_arg(arg, wchar_t *) = wc;					// Store the value
					break;
				}
				} /* end switch lenSpecifier */
				break;
			}

			case '[':
			{
				if (notSet) {										// Not in set request
					while (!strchr(&tmp[0], *tp) && width != 0) {	// While not matching set advance
						tp++;										// Next chracter
						width--;									// Decrease width
					}
				}
				else {
					while (strchr(&tmp[0], *tp) && width != 0) {	// While matching set advance
						tp++;										// Next chracter
						width--;									// Decrease width
					}
				}
				break;
			}
			}
			if (specifier != '[') count++;							// If not create set the we have matched a specifier
		}
		else {														// Literal character match
			if (*tp != *fmt) return(count);							// literal match fail time to exit
			fmt++;													// Next format character
			tp++;													// Next string character
		}
	}
	return (count);													// Return specifier match count
}

/*-[ sscanf ]---------------------------------------------------------------}
{ Reads data from str and stores them according to parameter format in the  }
{ locations given by the additional arguments, as if scanf was used, but    }
{ reading from buf instead of the standard input (stdin).					}
{ The additional arguments should point to already allocated variables of   }
{ the type specified by their corresponding specifier in the format string. }
{ 14Sep2017 LdB                                                             }
{--------------------------------------------------------------------------*/
int	sscanf (const char *str, const char *fmt, ...)
{
	va_list args;
	int i = 0;
	if (str) {														// Check buffer is valid
		va_start(args, fmt);										// Start variadic argument
		i = vsscanf(str, fmt, args);								// Run vsscanf
		va_end(args);												// End of variadic arguments
	}
	return (i);														// Return characters place in buffer
}



/***************************************************************************}
{                       PUBLIC C INTERFACE ROUTINES                         }
{***************************************************************************/

/*-[Init_EmbStdio]----------------------------------------------------------}
. Initialises the EmbStdio by setting the handler that will be called for
. Each character to be output to the standard console. That routine could be 
. a function that puts the character to a screen or something like a UART.
. Until this function is called with a valid handler output will not occur.
. RETURN: Will be the current handler that was active before the new handler 
. was set. For the first ever handler it will be NULL.
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
CHAR_OUTPUT_HANDLER Init_EmbStdio (CHAR_OUTPUT_HANDLER handler)
{
	CHAR_OUTPUT_HANDLER ret = Console_WriteChar;					// Hold current handler for return
	Console_WriteChar = handler;									// Set new handler function
	return (ret);													// Return the old handler													
}


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++}
{				 	PUBLIC FORMATTED OUTPUT ROUTINES					    }
{++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*-[ printf ]---------------------------------------------------------------}
. Writes the C string pointed by fmt to the standard console, replacing any
. format specifier with the value from the provided variadic list.
.
. RETURN: 
. The number of characters written to the standard console function
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
int printf (const char *fmt, ...)
{
	char buf[512];													// Temp buffer
	va_list args;													// Argument list
	int printed = 0;												// Number of charcters printed

	va_start(args, fmt);											// Create argument list
	printed = vsnprintf(buf, _countof(buf), fmt, args);				// Create string to print in temp buffer
	va_end(args);													// Done with argument list
	if (Console_WriteChar)											// Console write character valid
		for (int i = 0; i < printed; i++)							// For each character in buffer
			Console_WriteChar(buf[i]);								// Output it via console routine
	return printed;													// Return number of characters printed
}

/*-[ sprintf ]--------------------------------------------------------------}
. Writes the C string formatted by fmt to the given buffer, replacing any
. format specifier in the same way as printf.
.
. DEPRECATED:
. Using sprintf, there is no way to limit the number of characters written,
. which means the function is susceptible to buffer overruns. It is suggested
. the new function sprintf_s should be used as a replacement. For at least
. some safety the call is limited to writing a maximum of 256 characters.
.
. RETURN:
. The number of characters written to the provided buffer
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
int sprintf (char* buf, const char* fmt, ...)
{
	va_list args;													// Argument list
	int printed = 0;												// Number of charcters printed
	va_start(args, fmt);											// Create argument list
	printed = vsnprintf(buf, 256, fmt, args);						// Create string to print in buffer
	va_end(args);													// Done with argument list
	return printed;													// Return characters added to buffer
}

/*-[ snprintf ]-------------------------------------------------------------}
. Writes the C string formatted by fmt to the given buffer, replacing any
. format specifier in the same way as printf. This function has protection
. for output buffer size but not for the format buffer. Care should be taken
. to make user provided buffers are not used for format strings which would
. allow users to exploit buffer overruns.
.
. RETURN:
. The number of characters written to the provided buffer
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
int snprintf (char *buf, size_t bufSize, const char *fmt, ...)
{
	va_list args;													// Argument list
	int printed = 0;												// Number of charcters printed
	va_start(args, fmt);											// Create argument list
	printed = vsnprintf(buf, bufSize, fmt, args);					// Create string to print in buffer
	va_end(args);													// Done with argument list
	return printed;													// Return characters added to buffer
}

/*-[ vprintf ]--------------------------------------------------------------}
. Writes the C string formatted by fmt to the standard console, replacing 
. any format specifier in the same way as printf, but using the elements in 
. variadic argument list identified by arg instead of additional variadics.
.
. RETURN: 
. The number of characters written to the standard console function
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
int vprintf (const char* fmt, va_list arg)
{
	char buf[512];													// Temp buffer
	int printed = 0;												// Number of charcters printed
	printed = vsnprintf(buf, _countof(buf), fmt, arg);				// Create string to print in temp buffer
	if (Console_WriteChar)											// Console write character valid
		for (int i = 0; i < printed; i++)							// For each character in buffer
			Console_WriteChar(buf[i]);								// Output it via console routine
	return printed;													// Return number of characters printed
}

/*-[ vsprintf ]-------------------------------------------------------------}
. Writes the C string formatted by fmt to the given buffer, replacing any 
. format specifier in the same way as printf.
.
. DEPRECATED: 
. Using vsprintf, there is no way to limit the number of characters written, 
. which means the function is susceptible to buffer overruns. It is suggested 
. the new function vsprintf_s should be used as a replacement. For at least
. some safety the call is limited to read/write a maximum of 256 characters.
.
. RETURN: 
. The number of characters written to the provided buffer
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
int vsprintf (char *buf, const char *fmt, va_list args)
{
	return vsnprintf(buf, 256, fmt, args);							// Call secure version limiting to 256 char buffer
}

/*-[ vsnprintf ]-----------------------------------------------------------}
. Writes the C string formatted by fmt to the given buffer, replacing any
. format specifier in the same way as printf. This function has protection
. for output buffer size but not for the format buffer. Care should be taken
. to make user provided buffers are not used for format strings which would
. allow users to exploit buffer overruns.
.
. RETURN:
. The number of characters written to the provided buffer
. 19Oct17 LdB
.--------------------------------------------------------------------------*/
int vsnprintf (char *buf, size_t bufSize, const char *fmt, va_list args)
{
	int len;
	unsigned long long num;
	int i, base;
	char *str;


	int flags;														// flags to number()
	int field_width;												// width of output field
	int precision;													// min. # of digits for integers; max number of chars for from string
	int qualifier;													// 'h', 'l', 'j', 'z', 't' or 'L' 
	bool doubleLS = false;											// Double letter of length specifier

	for (str = buf; *fmt; ++fmt)
	{
		bool processed = false;
		if (*fmt != '%')
		{
			*str++ = *fmt;
			continue;
		}

		/* process flags */
		flags = 0;
		bool procFlag = true;
		do {
			++fmt;
			switch (*fmt) {
			case '-':
				flags |= LEFT;
				break;
			case '+':
				flags |= PLUS;
				break;
			case ' ':
				flags |= SPACE;
				break;
			case '#':
				flags |= SPECIAL;
				break;
			case '0':
				flags |= ZEROPAD;
				break;
			default:
				procFlag = false;
			}
		} while (procFlag);

		/* get field width */
		field_width = -1;
		if (checkdigit(*fmt))
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {
			++fmt;
			/* it's the next argument */
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		/* get the precision */
		precision = -1;
		if (*fmt == '.') {
			++fmt;
			if (checkdigit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				++fmt;
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier = -1;											// Preset -1 for qualifier
		if ((*fmt == 'h') || (*fmt == 'l') ||
			(*fmt == 'j') || (*fmt == 'z') ||
			(*fmt == 't') || (*fmt == 'L')) {					// Special conversion qualifier
			qualifier = *fmt;									// Hold conversion qualifier
			if ((*fmt == 'h') || (*fmt == 'l')) {				// Check for hh and ll
				if (fmt[1] == *fmt) 							// Peek at next character for match ... 'hh' or 'll'
				{
					doubleLS = true;							// Set the double LS flag
					fmt++;										// Next format character
				}
			}
			fmt++;												// Next format character
		}

		/* default base */
		base = 10;

		switch (*fmt) {
		case 'c':
			if (!(flags & LEFT))
				while (--field_width > 0)
					*str++ = ' ';
			if (qualifier == 'l')								// Length specifier 'l' 
			{
				wchar_t* tp = (wchar_t*)str;
				*tp++ = va_arg(args, wchar_t);					// Store the value
				str = (char*)tp;								// Update pointer
			}
			else	*str++ = (unsigned char)va_arg(args, int);
			while (--field_width > 0)
				*str++ = ' ';
			continue;

		case 's':
		{
			const char *s;
			s = va_arg(args, char *);
			len = strnlen(s, precision);

			if (!(flags & LEFT))
				while (len < field_width--)
					*str++ = ' ';

			for (i = 0; i < len; ++i)
				*str++ = *s++;

			while (len < field_width--)
				*str++ = ' ';
			continue;
		}
		case 'p':
			if (field_width == -1) {
				field_width = 2 * sizeof(void *);
				flags |= ZEROPAD;
			}
			str = number(str,
				(unsigned long)va_arg(args, void *), 16,
				field_width, precision, flags);
			continue;

		case 'n':
			if (qualifier == 'l') {
				long *ip = va_arg(args, long *);
				*ip = (str - buf);
			}
			else {
				int *ip = va_arg(args, int *);
				*ip = (str - buf);
			}
			continue;

		case '%':
			*str++ = '%';
			continue;

			/* integer number formats - set up the flags and "break" */
		case 'o':
			base = 8;
			break;

		case 'x':
			flags |= SMALL;
		case 'X':
			base = 16;
			break;

		case 'd':
		case 'i':
			flags |= SIGN;
		case 'u':
			break;

			/* Floats/double/double double find end */
		case 'a':
		case 'A':
		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
		{
			processed = true;									// Set processed flag if float/double/long double handled
			switch (qualifier) {
			case -1:											// qualifier not specified 
			{
				float f = (float)va_arg(args, double);			// Load float value
				if (field_width == -1) field_width = 53;
				if (precision == -1) precision = 10;
				str = NumStr(f, str, bufSize - (str - buf));
				break;
			}
			case 'l':
			{
				double d = va_arg(args, double);				// Load the double value
				str = NumStr(d, str, bufSize - (str - buf));
				break;
			}
			case 'L':
			{
				long double ld = va_arg(args, long double);		// Load the long value
				str = NumStr(ld, str, bufSize - (str - buf));
				break;
			}
			} /* end switch qualifier */
			break;
		}
		default:
		{
			*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				--fmt;
			continue;
		}
		}
		if (processed == false) {
			if (qualifier == 'l')
				if (doubleLS)
					num = va_arg(args, unsigned long long);
				else
					num = va_arg(args, unsigned long);
			else if (qualifier == 'h') {
				if (doubleLS)
					num = (unsigned char)va_arg(args, int);
				else
					num = (unsigned short)va_arg(args, int);
				if (flags & SIGN)
					num = (short)num;
			}
			else if (qualifier == 'z') {
				num = va_arg(args, size_t);
			}
			else if (qualifier == 't') {
				num = va_arg(args, ptrdiff_t);
			}
			else if (flags & SIGN)
				num = va_arg(args, int);
			else
				num = va_arg(args, unsigned int);
			str = number(str, num, base, field_width, precision, flags);
		}
	}
	*str = '\0';
	return str - buf;
}
