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

		WordFrontiers = 4, // update aligned words' frontiers

		CharsAllWords = 8, // align characters in all aligned words
		CharsOKWords = 16, // align characters in validated words
		CharsNKOWords = 32, // align characters in non-rejected words (i.e.: validated and unknown)

		AllChars = 64, // align all characters (in previous selection)
		NOKChars = 128 // align non-validated characters
	};
}

CRN_DECLARE_ENUM_OPERATORS(ori::AlignConfig)

#endif


