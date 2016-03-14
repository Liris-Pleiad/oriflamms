/* Copyright 2015-2016 Université Paris Descartes, ENS-Lyon
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
		NAlWords = 4, // align not aligned words

		WordFrontiers = 8, // update aligned words' frontiers

		CharsAllWords = 16, // align characters in all aligned words
		CharsOKWords = 32, // align characters in validated words
		CharsNKOWords = 64, // align characters in non-rejected words (i.e.: validated and unknown)

		AllChars = 128, // align all characters (in previous selection)
		NAlChars = 256 // align not aligned characters
	};
}

CRN_DECLARE_ENUM_OPERATORS(ori::AlignConfig)

#endif


