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
 * \file OriFeatures.h
 * \author Yann LEYDIER
 */

#ifndef OriFeatures_HEADER
#define OriFeatures_HEADER

#include <oriflamms_config.h>
#include <CRNMath/CRNLinearInterpolation.h>
#include <CRNBlock.h>

namespace ori
{
	/*! \brief A signature element on the image */
	struct ImageSignature
	{
		ImageSignature(const crn::Rect &box, char desc, uint8_t cp = 127):bbox(box),code(desc),cutproba(cp) { }
		crn::Rect bbox; /*!< bounding box */
		char code; /*!< description of the signature element */
		uint8_t cutproba; /*!< likeliness to be last element of a string */
	};

	/*! \brief A signature element in the transcribed text */
	struct TextSignature
	{
		TextSignature(bool startword, char desc):start(startword),code(desc) { }
		bool start; /*!< is this the first element of a string? */
		char code; /*!< description of the signature element */
	};

	/*! \brief Aligns two signature strings */
	std::vector<std::pair<crn::Rect, crn::StringUTF8>> Align(const std::vector<ImageSignature> &isig, const std::vector<TextSignature> &tsig);
}
#endif


