/*! Copyright 2013-2016 A2IA, CNRS, École Nationale des Chartes, ENS Lyon, INSA Lyon, Université Paris Descartes, Université de Poitiers
 *
 * This file is part of Oriflamms.
 *
 * Oriflamms is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Oriflamms is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Oriflamms.  If not, see <http://www.gnu.org/licenses/>.
 *
 * \file OriAlignConfig.h
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


