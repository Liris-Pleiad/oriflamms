/* Copyright 2015 Université Paris Descartes
 * 
 * file: OriTextSignature.h
 * \author Yann LEYDIER
 */

#ifndef OriTextSignature_HEADER
#define OriTextSignature_HEADER

#include <oriflamms_config.h>
#include <CRNString.h>
#include <CRNIO/CRNPath.h>
#include <unordered_map>

namespace ori
{
	class TextSignature;

	class TextSignatureDB
	{
		public:
			static void Load(const crn::Path &fname);
			static crn::StringUTF8 Convert(char32_t c);
			static crn::StringUTF8 Convert(const crn::String &str);
			static std::vector<TextSignature> Sign(const crn::String &str);
		private:
			TextSignatureDB();
			static TextSignatureDB& getInstance();
			std::unordered_map<char32_t, crn::StringUTF8> signature_db; /*!< signature database */
	};
}
#endif


