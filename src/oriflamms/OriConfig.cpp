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
 * \file OriConfig.cpp
 * \author Yann LEYDIER
 */

#include <OriConfig.h>
#include <CRNIO/CRNIO.h>
#include <CRNi18n.h>

using namespace ori;

const crn::String Config::localeDirKey(U"LocalePath");
const crn::String Config::staticDataDirKey(U"StaticDataPath");
const crn::String Config::fontKey(U"Font");

Config::Init::Init()
{
	Config::GetInstance();
}
Config::Init Config::init;

Config::Config():
	appconf("oriflamms"),
	userconf("oriflamms", "user", crn::ConfigurationType::USER)
{
	if (appconf.Load().IsEmpty())
	{
		appconf.SetData(localeDirKey, crn::Path(ORIFLAMMS_LOCALE_FULL_PATH));
		appconf.SetData(staticDataDirKey, crn::Path(ORIFLAMMS_DATA_FULL_PATH));
		appconf.Save();
	}

	char *loc = setlocale(LC_ALL, "");
	loc = bindtextdomain(GETTEXT_PACKAGE, appconf.GetPath(localeDirKey).CStr());
	loc = bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	loc = textdomain(GETTEXT_PACKAGE);
}

Config& Config::GetInstance()
{
	static Config c;
	return c;
}

bool Config::Save()
{
	const auto appfname = GetInstance().appconf.Save();
	const auto userfname = GetInstance().userconf.Save();
	return appfname.IsNotEmpty() && userfname.IsNotEmpty();
}

crn::Path Config::GetLocaleDir()
{
	try
	{
		return GetInstance().appconf.GetPath(localeDirKey);
	}
	catch (...)
	{
		crn::Path p(ORIFLAMMS_LOCALE_FULL_PATH);
		GetInstance().appconf.SetData(localeDirKey, p);
		return p;
	}
}

void Config::SetLocaleDir(const crn::Path &dir)
{
	GetInstance().appconf.SetData(localeDirKey, dir);
	Save();
}

crn::Path Config::GetStaticDataDir()
{
	try
	{
		return GetInstance().appconf.GetPath(staticDataDirKey);
	}
	catch (...)
	{
		crn::Path p(ORIFLAMMS_DATA_FULL_PATH);
		GetInstance().appconf.SetData(staticDataDirKey, p);
		return p;
	}
}

void Config::SetStaticDataDir(const crn::Path &dir)
{
	GetInstance().appconf.SetData(staticDataDirKey, dir);
	Save();
}

crn::Path Config::GetUserDirectory()
{
	return GetInstance().appconf.GetUserDirectory();
}

void Config::SetFont(const crn::StringUTF8 &font)
{
	GetInstance().userconf.SetData(fontKey, font);
	Save();
}

crn::StringUTF8 Config::GetFont()
{
	try
	{
		return GetInstance().userconf.GetStringUTF8(fontKey);
	}
	catch (...)
	{
		return "";
	}
}

