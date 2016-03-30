/* Copyright 2013-2015 INSA-Lyon & IRHT
 * 
 * file: OriFeatures.h
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


