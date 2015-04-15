/* Copyright 2013 INSA-Lyon & IRHT
 * 
 * This file is part of oriflamms.
 * 
 * file: OriConfig.h
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

			static const crn::Path GetLocaleDir();
			static void SetLocaleDir(const crn::Path &dir);
			static const crn::Path GetStaticDataDir();
			static void SetStaticDataDir(const crn::Path &dir);

			static Config& GetInstance();
		private:
			const crn::String localeDirKey;
			const crn::String staticDataDirKey;
			Config();
			CRNConfigurationFile conf;
			struct Init
			{
				Init();
			};
			static Init init;
	};
}

#endif


