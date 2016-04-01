/* Copyright 2013-2014 INSA-Lyon & IRHT
 * 
 * This file is part of oriflamms.
 * 
 * file: OriConfig.cpp
 * \author Yann LEYDIER
 */

#include <OriConfig.h>
#include <CRNIO/CRNIO.h>
#include <CRNi18n.h>

using namespace ori;

const crn::String Config::localeDirKey(U"LocalePath");
const crn::String Config::staticDataDirKey(U"StaticDataPath");

Config::Init::Init()
{
	Config::GetInstance();
}
Config::Init Config::init;

Config::Config():
	conf("oriflamms")
{
	if (conf.Load().IsEmpty())
	{
		conf.SetData(localeDirKey, crn::Path(ORIFLAMMS_LOCALE_FULL_PATH));
		conf.SetData(staticDataDirKey, crn::Path(ORIFLAMMS_DATA_FULL_PATH));
		conf.Save();
	}

	char *loc = setlocale(LC_ALL, "");
	loc = bindtextdomain(GETTEXT_PACKAGE, conf.GetPath(localeDirKey).CStr());
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
	return GetInstance().conf.Save().IsNotEmpty();
}

crn::Path Config::GetLocaleDir()
{
	try
	{
		return GetInstance().conf.GetPath(GetInstance().localeDirKey);
	}
	catch (...)
	{
		crn::Path p(ORIFLAMMS_LOCALE_FULL_PATH);
		GetInstance().conf.SetData(GetInstance().localeDirKey, p);
		return p;
	}
}

void Config::SetLocaleDir(const crn::Path &dir)
{
	GetInstance().conf.SetData(GetInstance().localeDirKey, dir);
}

crn::Path Config::GetStaticDataDir()
{
	try
	{
		return GetInstance().conf.GetPath(GetInstance().staticDataDirKey);
	}
	catch (...)
	{
		crn::Path p(ORIFLAMMS_DATA_FULL_PATH);
		GetInstance().conf.SetData(GetInstance().staticDataDirKey, p);
		return p;
	}
}

void Config::SetStaticDataDir(const crn::Path &dir)
{
	GetInstance().conf.SetData(GetInstance().staticDataDirKey, dir);
}

crn::Path Config::GetUserDirectory()
{
	return GetInstance().conf.GetUserDirectory();
}

