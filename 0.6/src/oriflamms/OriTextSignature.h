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
 * \file OriTextSignature.h
 * \author Yann LEYDIER
 */

#ifndef OriTextSignature_HEADER
#define OriTextSignature_HEADER

#include <oriflamms_config.h>
#include <CRNString.h>
#include <CRNIO/CRNPath.h>
#include <unordered_map>
#include <OriFeatures.h> // MSVC cannot link if we forward declare class TextSignature

namespace ori
{
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


