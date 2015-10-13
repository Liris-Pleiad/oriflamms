#include <oriflamms_config.h>
#include "test.h"
#include <CRNIO/CRNIO.h>
#include <CRNi18n.h>

using namespace ori;

//////////////////////////////////////////////////////////////////////////////////
// Utils
//////////////////////////////////////////////////////////////////////////////////
enum class XMLType { Text, Zone, Link, Unknown };
static XMLType getType(crn::xml::Document &doc)
{
	auto root = doc.GetRoot();
	if (root.GetName() != "TEI")
		return XMLType::Unknown;
	auto el = root.GetFirstChildElement("facsimile");
	if (el)
		return XMLType::Zone;
	el = root.GetFirstChildElement("text");
	if (el)
	{
		el = el.GetFirstChildElement("body");
		if (!el)
			return XMLType::Unknown;
		if (el.GetFirstChildElement().GetName() == "ab")
			return XMLType::Link;
		else
			return XMLType::Text;
	}
	return XMLType::Unknown;
}

//////////////////////////////////////////////////////////////////////////////////
// Page
//////////////////////////////////////////////////////////////////////////////////
Page::Page() = default;

Page::~Page() = default;

const Column& Page::GetColumn(const Id &id) const
{
	auto it = columns.find(id);
	if (it == columns.end())
		throw crn::ExceptionNotFound(_("Invalid column id."));
	return it->second;
}

Column& Page::GetColumn(const Id &id)
{
	auto it = columns.find(id);
	if (it == columns.end())
		throw crn::ExceptionNotFound(_("Invalid column id."));
	return it->second;
}

const Word& Page::GetWord(const Id &id) const
{
	auto it = words.find(id);
	if (it == words.end())
		throw crn::ExceptionNotFound(_("Invalid word id."));
	return it->second;
}

Word& Page::GetWord(const Id &id)
{
	auto it = words.find(id);
	if (it == words.end())
		throw crn::ExceptionNotFound(_("Invalid word id."));
	return it->second;
}

const Character& Page::GetCharacter(const Id &id) const
{
	auto it = characters.find(id);
	if (it == characters.end())
		throw crn::ExceptionNotFound(_("Invalid character id."));
	return it->second;
}

Character& Page::GetCharacter(const Id &id)
{
	auto it = characters.find(id);
	if (it == characters.end())
		throw crn::ExceptionNotFound(_("Invalid character id."));
	return it->second;
}

const Zone& Page::GetZone(const Id &id) const
{
	auto it = zones.find(id);
	if (it == zones.end())
		throw crn::ExceptionNotFound(_("Invalid zone id."));
	return it->second;
}

Zone& Page::GetZone(const Id &id)
{
	auto it = zones.find(id);
	if (it == zones.end())
		throw crn::ExceptionNotFound(_("Invalid zone id."));
	return it->second;
}

//////////////////////////////////////////////////////////////////////////////////
// Document
//////////////////////////////////////////////////////////////////////////////////
Document::Document(const crn::Path &dirpath, crn::Progress *prog)
{
	const auto txtdir = crn::IO::Directory{dirpath / "texts"};
	for (const auto fname : txtdir.GetFiles())
	{
		try
		{
			auto xdoc = crn::xml::Document(fname); // may throw
			auto el = xdoc.GetRoot(); // may throw
			if (el.GetName() != "TEI")
				throw crn::ExceptionInvalidArgument(_("not a TEI file."));
			el = el.GetFirstChildElement("text");
			if (!el)
				throw crn::ExceptionNotFound(_("no <text> element."));
			el = el.GetFirstChildElement("body");
			if (!el)
				throw crn::ExceptionNotFound(_("no <body> element."));
			el = el.GetFirstChildElement("pb");
			if (!el)
				throw crn::ExceptionNotFound(_("no <pb> element."));
			const auto id = el.GetAttribute<crn::StringUTF8>("xml:id", true); // may throw
			page_refs[id].files.push_back(fname);
		}
		catch (std::exception &ex)
		{
			report += fname;
			report += ": ";
			report += ex.what();
			report += "\n";
		}
	}
	const auto zonedir = crn::IO::Directory{dirpath / "zones"};
	for (const auto fname : zonedir.GetFiles())
	{
		try
		{
			auto xdoc = crn::xml::Document(fname); // may throw
			auto el = xdoc.GetRoot(); // may throw
			if (el.GetName() != "TEI")
				throw crn::ExceptionInvalidArgument(_("not a TEI file."));
			el = el.GetFirstChildElement("facsimile");
			if (!el)
				throw crn::ExceptionNotFound(_("no <facsimile> element."));
			el = el.GetFirstChildElement("surface");
			if (!el)
				throw crn::ExceptionNotFound(_("no <surface> element."));
			el = el.GetFirstChildElement("graphic");
			if (!el)
				throw crn::ExceptionNotFound(_("no <graphic> element."));
			const auto id = el.GetAttribute<crn::StringUTF8>("xml:id", true); // may throw
			//page_refs[id].files.push_back(fname);
			// RAH! ÇA MARCHE PAS!!!
		}
		catch (std::exception &ex)
		{
			report += fname;
			report += ": ";
			report += ex.what();
			report += "\n";
		}
	}
	const auto linkdir = crn::IO::Directory{dirpath / "links"};
	for (const auto fname : linkdir.GetFiles())
	{
		try
		{
			auto xdoc = crn::xml::Document(fname); // may throw
			auto el = xdoc.GetRoot(); // may throw
			if (el.GetName() != "TEI")
				throw crn::ExceptionInvalidArgument(_("not a TEI file."));
			el = el.GetFirstChildElement("text");
			if (!el)
				throw crn::ExceptionNotFound(_("no <text> element."));
			el = el.GetFirstChildElement("body");
			if (!el)
				throw crn::ExceptionNotFound(_("no <body> element."));
			el = el.GetFirstChildElement("ab");
			if (!el)
				throw crn::ExceptionNotFound(_("no <ab> element."));
			for (auto gel = el.GetFirstChildElement("linkGrp"); gel; gel = gel.GetNextSiblingElement("linkGrp"))
				if (gel.GetAttribute<crn::StringUTF8>("type") == "pages")
				{
					el = gel.GetFirstChildElement("link");
					if (!el)
						throw crn::ExceptionNotFound(_("no <link> element in pages' <linkGrp>."));
					const auto link = el.GetAttribute<crn::StringUTF8>("target", true).Split(" \t"); // may throw
					break;
				}
		}
		catch (std::exception &ex)
		{
			report += fname;
			report += ": ";
			report += ex.what();
			report += "\n";
		}
	}
}

Document::~Document() = default;

std::shared_ptr<Page> Document::PageRef::GetPage()
{
	auto p = std::shared_ptr<Page>{};
	if (page.expired())
	{
		p = std::make_shared<Page>();
		page = p;
	}
	else
		p = page.lock();
	return p;
}









int main(int argc, char *argv[])
{
	auto doc = Document("/data/Dropbox/ORIFLAMMS-Fichiers-exemple/Référentiels/Encodage XML/conversions vers TEI Oriflamms");
	return 0;
}

