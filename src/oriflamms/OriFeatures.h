/* Copyright 2013 INSA-Lyon & IRHT
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
	struct ImageSignature
	{
		ImageSignature(const crn::Rect &box, char desc):bbox(box),code(desc) { }
		crn::Rect bbox;
		char code;
	};

	struct TextSignature
	{
		TextSignature(bool startword, char desc):start(startword),code(desc) { }
		bool start;
		char code;
	};

	const std::vector<std::pair<crn::Rect, crn::StringUTF8> > Align(const std::vector<ImageSignature> &isig, const std::vector<TextSignature> &tsig);
}
#endif


