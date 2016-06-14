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
 * \file OriConfig.h
 * \author Yann LEYDIER
 */

#ifndef OriConfig_HEADER
#define OriConfig_HEADER

#include <oriflamms_config.h>
#include <CRNUtils/CRNConfigurationFile.h>

namespace ori
{
	/*! \brief Configuration for Oriflamms
	 *
	 *  Configuration for Oriflamms
	 *
	 * \date	October 2012
	 * \author	Yann LEYDIER
	 * \version	0.1
	 */
	class Config
	{
		public:
			virtual ~Config() { Save(); }
			/*! \brief Saves to user's local config */
			static bool Save();

			static crn::Path GetLocaleDir();
			static void SetLocaleDir(const crn::Path &dir);
			static crn::Path GetStaticDataDir();
			static void SetStaticDataDir(const crn::Path &dir);
			static crn::Path GetUserDirectory();
			static void SetFont(const crn::StringUTF8 &dir);
			static crn::StringUTF8 GetFont();

			static Config& GetInstance();
		private:
			static const crn::String localeDirKey;
			static const crn::String staticDataDirKey;
			static const crn::String fontKey;
			Config();
			crn::ConfigurationFile appconf;
			crn::ConfigurationFile userconf;
			struct Init
			{
				Init();
			};
			static Init init;
	};
}

#endif


