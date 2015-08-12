/* Copyright 2015 Université Paris Descartes
 * 
 * file: OriAlignConfig.h
 * \author Yann LEYDIER
 */

#ifndef OriAlignConfig_HEADER
#define OriAlignConfig_HEADER

#include <CRNType.h>

namespace ori
{
	enum class AlignConfig { 
		None = 0, // Do nothing

		AllWords = 1, // align all words
		NOKWords = 2, // align non-validated words (i.e.: unknown and rejected)

		CharsAllWords = 4, // align characters in all aligned words
		CharsOKWords = 8, // align characters in validated words
		CharsNKOWords = 16, // align characters in non-rejected words (i.e.: validated and unknown)

		AllChars = 32, // align all characters (in previous selection)
		NOKChars = 64 // align non-validated characters
	};
}

CRN_DECLARE_ENUM_OPERATORS(ori::AlignConfig)

#endif


