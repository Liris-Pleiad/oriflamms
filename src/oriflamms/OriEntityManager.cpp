/* Copyright 2014 INSA-Lyon, IRHT, ZHAO Xiaojuan
 *
 * file: OriEntityManager.cpp
 * \author ZHAO Xiaojuan, Yann LEYDIER
 */

#include <OriEntityManager.h>
#include <OriConfig.h>
#include <CRNString.h>
#include <fstream>
#include <CRNi18n.h>

#include <iostream>

using namespace ori;
using namespace crn::literals;

static int fraction(int j)
{
	if(j == 0)
		return 1;
	else
		return 16*fraction(j-1);
}

static int char_to_unicode(char c) noexcept
{
	if(c <= '9' && c >= '0')
		return c-'0';
	else
		return 10+(c-'A');
}

EntityManager::EntityManager()
{
	crn::Path path = ori::Config::GetUserDirectory() / "orient.xml";
	try
	{
		load(path);
	}
	catch (...)
	{
		path = ori::Config::GetStaticDataDir() / "orient.xml";
		try
		{
			load(path);
		}
		catch (...) { }
	}
}

/*! Adds entities from a file
 * \param[in]	path	path to the file to read
 * \param[in]	update	shall existing entities be overwritten or kept?
 * \throws	ExceptionIO	cannot open file
 */
void EntityManager::AddEntityFile(const crn::Path &file_path, bool update)
{
	std::ifstream in(file_path.CStr());
	if (!in.good())
	{
		throw	crn::ExceptionIO(crn::StringUTF8("void EntityManager::AddEntityFile(const crn::Path &file_path, bool update): ") + _("Cannot open file."));
	}
	if (file_path.Find("dtd") == file_path.NPos()) // XXX ???????????????????????????????????????????
		while (!in.eof())
		{
			crn::StringUTF8 s;
			std::getline(in, s.Std());
			// look for entity
			size_t pos = s.Find("!ENTITY");
			if (pos == crn::StringUTF8::NPos())
				pos = s.Find("!entity");
			if (pos == crn::StringUTF8::NPos())
				pos = s.Find("!Entity");
			if (pos == crn::StringUTF8::NPos())
				continue;
			// goto space
			pos = s.FindAnyOf(" \t", pos);
			if (pos == crn::StringUTF8::NPos())
				continue;
			// goto entity name
			pos = s.FindNotOf(" \t", pos);
			if (pos == crn::StringUTF8::NPos())
				continue;
			// find next space
			size_t n = s.FindAnyOf(" \t", pos);
			if (n == crn::StringUTF8::NPos())
				continue;
			// extract entity name
			crn::StringUTF8 entname = s.SubString(pos, n - pos);
			// goto entity value
			pos = s.Find("\"", n);
			if (pos == crn::StringUTF8::NPos())
				continue;
			pos += 1;
			// goto next "
			n = s.Find("\"", pos);
			if (n == crn::StringUTF8::NPos())
				continue;
			// extract entity value
			crn::StringUTF8 entval = s.SubString(pos, n - pos);

			AddEntity(entname, entval, update);
		}
}

/*!
 * Adds an entity
 * \param[in]	key	the name of the entity
 * \param[in]	value	the value of the entity
 * \param[in]	update	shall existing entities be overwritten or kept?
 * \return	true if the entity was added or updated, false else
 */
bool EntityManager::AddEntity(const crn::StringUTF8 &key, const crn::StringUTF8 &value, bool update)
{
	auto it = getMap().find(key);
	if (it == getMap().end())
	{
		getMap().insert(std::make_pair(key, value));
		return true;
	}
	else if (update)
	{
		it->second = value;
		return true;
	}
	else
		return false;
}

void EntityManager::RemoveEntity(const crn::StringUTF8 &key)
{
	getMap().erase(key);
}

EntityManager& EntityManager::getInstance()
{
	static EntityManager map_manager;
	return map_manager;
}

/*! Converts XML hexadecimal entities to Unicode
 * \param[in]	xmltext	the string to expand
 * \return	the string with Unicode characters instead of &x....;
 */
crn::StringUTF8 EntityManager::ExpandUnicodeEntities(const crn::StringUTF8 &xmltext)
{
	crn::StringUTF8 textxml(xmltext);
	while (textxml.Find("&#x") != textxml.NPos())
	{
		crn::StringUTF8 s_child = textxml.SubString(textxml.Find("&#x") + 3, textxml.Find(";") - textxml.Find("&#x")-3);
		int code_unicode = 0;
		for (int j = 0; j < s_child.Length(); ++j)
		{
			code_unicode = code_unicode + char_to_unicode(s_child[s_child.Length()-1-j]) * fraction(j);
		}
		crn::String character((char32_t)code_unicode);
		textxml.Replace(character.CStr(), textxml.Find("&#x"), textxml.Find(";") - textxml.Find("&#x") + 1);
	}
	return textxml;
}

void EntityManager::Save()
{
	crn::xml::Document doc;

	doc.PushBackComment("Oriflamms entity table");
	crn::xml::Element root(doc.PushBackElement("orient"));
	for (const auto &ent : GetMap())
	{
		crn::xml::Element el(root.PushBackElement("ent"));
		el.SetAttribute("name", ent.first);

		el.PushBackText(ent.second, true);
	}

	crn::Path path = ori::Config::GetUserDirectory();
	doc.Save(path / "orient.xml");
}

void EntityManager::Reload()
{
	getMap().clear();
	crn::Path path = ori::Config::GetUserDirectory() / "orient.xml";
	try
	{
		getInstance().load(path);
	}
	catch (...)
	{
		path = ori::Config::GetStaticDataDir() / "orient.xml";
		try
		{
			getInstance().load(path);
		}
		catch (...) { }
	}
}

/*!
 * Loads a entity manager saved file
 * \param[in]	fname	the file to load
 * \throws	ExceptionNotFound	file not found
 * \throws	ExceptionIO	cannot open file
 * \throws	ExceptionRuntime	error parsing the file
 * \throws	ExceptionDomain	not a valid entity file
 */
void EntityManager::load(const crn::Path &fname)
{
	crn::xml::Document doc(fname); // might throw
	crn::xml::Element root(doc.GetRoot());
	if (root.GetName() != "orient")
		throw crn::ExceptionDomain(crn::StringUTF8("void EntityManager::load(const crn::Path &fname): ") + _("not an Oriflamms entity file."));
	crn::xml::Element ep = root.GetFirstChildElement("ent");
	while (ep)
	{
		try
		{ entities.insert(std::make_pair(ep.GetAttribute<crn::StringUTF8>("name", false), ep.GetFirstChildText())); }
		catch (...) {}
		ep = ep.GetNextSiblingElement("ent");
	}
}

std::unique_ptr<crn::xml::Document> EntityManager::ExpandXML(const crn::Path &fname)
{
	EntityManager& mapwords = EntityManager::getInstance();

	std::ifstream in(fname.CStr());
	crn::StringUTF8 content;

	if (in.good())
	{
		while (!in.eof() )
		{
			std::string s;
			getline(in,s);
			auto unicode_string = std::string{};
			unicode_string.reserve(s.size());
			for (size_t tmp = 0; tmp < s.size(); ++tmp)
			{
				if (s[tmp] == '&')
				{
					auto endpos = s.find(';', tmp);
					if (endpos != std::string::npos)
					{
						auto name = s.substr(tmp, endpos - tmp + 1);
						auto it = mapwords.GetMap().find(name);
						if (it != mapwords.GetMap().end())
						{
							content += it->second;
							tmp = endpos;
							continue;
						}
					}
				}
				content += s[tmp];
			}
			content += unicode_string + "\n"_s;
		}
	}
	else
	{
		throw crn::ExceptionIO(crn::StringUTF8("EntityManager::ExpandXML(const crn::Path &fname): ") + strerror(errno));
	}
	return std::unique_ptr<crn::xml::Document>{new crn::xml::Document{ExpandUnicodeEntities(content).CStr()}}; // may throw
}

