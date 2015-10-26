#include <oriflamms_config.h>
#include "test.h"
#include <CRNIO/CRNIO.h>
#include <CRNi18n.h>

using namespace ori;
using namespace crn::literals;

const auto SURFACELINKS = "surfaces"_s;
const auto PAGELINKS = "pages"_s;
const auto COLUMNLINKS = "columns"_s;
const auto LINELINKS = "lines"_s;
const auto WORDLINKS = "words"_s;
const auto CHARLINKS = "characters"_s;
const auto GLYPHLINKS = "character-classes"_s;

const auto TEXTDIR = "texts"_p;
const auto IMGDIR = "img"_p;
const auto LINKDIR = "links"_p;
const auto ZONEDIR = "zones"_p;
const auto ONTODIR = "ontologies"_p;

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
// View
//////////////////////////////////////////////////////////////////////////////////
View::View() = default;

View::~View() = default;

const Page& View::GetPage(const Id &id) const
{
	auto it = pimpl->pages.find(id);
	if (it == pimpl->pages.end())
		throw crn::ExceptionNotFound(_("Invalid page id."));
	return it->second;
}

Page& View::GetPage(const Id &id)
{
	auto it = pimpl->pages.find(id);
	if (it == pimpl->pages.end())
		throw crn::ExceptionNotFound(_("Invalid page id."));
	return it->second;
}

const Column& View::GetColumn(const Id &id) const
{
	auto it = pimpl->columns.find(id);
	if (it == pimpl->columns.end())
		throw crn::ExceptionNotFound(_("Invalid column id."));
	return it->second;
}

Column& View::GetColumn(const Id &id)
{
	auto it = pimpl->columns.find(id);
	if (it == pimpl->columns.end())
		throw crn::ExceptionNotFound(_("Invalid column id."));
	return it->second;
}

const Word& View::GetWord(const Id &id) const
{
	auto it = pimpl->words.find(id);
	if (it == pimpl->words.end())
		throw crn::ExceptionNotFound(_("Invalid word id."));
	return it->second;
}

Word& View::GetWord(const Id &id)
{
	auto it = pimpl->words.find(id);
	if (it == pimpl->words.end())
		throw crn::ExceptionNotFound(_("Invalid word id."));
	return it->second;
}

const Character& View::GetCharacter(const Id &id) const
{
	auto it = pimpl->characters.find(id);
	if (it == pimpl->characters.end())
		throw crn::ExceptionNotFound(_("Invalid character id."));
	return it->second;
}

Character& View::GetCharacter(const Id &id)
{
	auto it = pimpl->characters.find(id);
	if (it == pimpl->characters.end())
		throw crn::ExceptionNotFound(_("Invalid character id."));
	return it->second;
}

const Zone& View::GetZone(const Id &id) const
{
	auto it = pimpl->zones.find(id);
	if (it == pimpl->zones.end())
		throw crn::ExceptionNotFound(_("Invalid zone id."));
	return it->second;
}

Zone& View::GetZone(const Id &id)
{
	auto it = pimpl->zones.find(id);
	if (it == pimpl->zones.end())
		throw crn::ExceptionNotFound(_("Invalid zone id."));
	return it->second;
}

View::Impl::Impl(const Id &surfid, const crn::Path &base, const crn::StringUTF8 &projname):
	id(surfid)
{
	// read TEXTDIR/projname-w/projname_id-w.xml for structure and words' transcription
	auto doc = crn::xml::Document{base / TEXTDIR / projname + "-w" / projname + "_" + crn::Path{id} + "-w.xml"};
	auto root = doc.GetRoot();
	auto pos = ElementPosition{id};
	readTextWElement(root, pos);
	// read TEXTDIR/projname-w/projname_id-c.xml for characters
	// read ZONEDIR/projname_id-zones-w.xml for boxes and image
	// read LINKDIR/projname_id-links-w.xml for boxes
	// read if it exists ZONEDIR/projname_id-zones-c.xml for boxes
	// read if it exists LINKDIR/projname_id-links-c.xml for boxes
	// read if it exists ONTODIR/projname_*.xml for character classes
}

void View::Impl::readTextWElement(crn::xml::Element &el, ElementPosition &pos)
{
	const auto elname = el.GetName();
	if (elname == "pb")
	{
		const auto id = el.GetAttribute<Id>("xml:id", false);
		pages.emplace(id, Page{});
		pos.page = id;
	}
	else if (elname == "cb")
	{
		const auto id = el.GetAttribute<Id>("xml:id", false);
		columns.emplace(id, Column{});
		pages[pos.page].columns.push_back(id);
		pos.column = id;
	}
	else if (elname == "lb")
	{
		const auto id = el.GetAttribute<Id>("xml:id", false);
		lines.emplace(id, Line{});
		columns[pos.column].lines.push_back(id);
		pos.line = id;
		// TODO renvois
	}
	else if ((elname == "w") || (elname == "seg") || (elname == "pc"))
	{
		if (!el.GetFirstChildElement("seg"))
		{
			const auto id = el.GetAttribute<Id>("xml:id", false);
			words.emplace(id, Word{});
			lines[pos.line].left.push_back(id); // TODO renvois
			words[id].text = ""; // TODO lire la structure jusqu'au bout
		}
	}
	for (auto sel = el.BeginElement(); sel != el.EndElement(); ++sel)
		readTextWElement(sel, pos);
}


//////////////////////////////////////////////////////////////////////////////////
// Document
//////////////////////////////////////////////////////////////////////////////////
static void findMilestones(crn::xml::Element &el, std::multimap<int, Id> &milestones)
{
	if (el.GetName() == "milestone")
		if (el.GetAttribute<crn::StringUTF8>("unit", true) == "surface")
		{
			try
			{
				const auto id = el.GetAttribute<crn::StringUTF8>("xml:id", false);
				const auto n = el.GetAttribute<int>("n", false);
				milestones.emplace(n, id);
			}
			catch (...)
			{
				throw crn::ExceptionNotFound(_("Malformed milestone element."));
			}
		}

	for (auto sel = el.BeginElement(); sel != el.EndElement(); ++sel)
		findMilestones(sel, milestones);
}

/*!
 * \param[in]	dirpath	base directory of the project
 * \param[in]	prog	a progress bar
 *
 * \throws	crn::ExceptionInvalidArgument	malformed main transcription filename
 */
Document::Document(const crn::Path &dirpath, crn::Progress *prog):
	base(dirpath)
{
	// get base name
	const auto txtdir = crn::IO::Directory{base / TEXTDIR};
	auto dir = crn::IO::Directory{txtdir};
	auto fname = dir.GetFiles().front();
	name = fname.GetBase();
	if (!name.EndsWith("-w"_s))
		throw crn::ExceptionInvalidArgument{_("malformed main transcription filename.")};
	name.Crop(0, name.Size() - 2);

	// read milestones
	auto milestones = std::multimap<int, Id>{};
	try
	{
		auto doc = crn::xml::Document{fname};
		auto root = doc.GetRoot();
		findMilestones(root, milestones);
	}
	catch (std::exception &ex)
	{
		report += fname;
		report += ": ";
		report += ex.what();
		report += "\n";
	}
	// check if milestones are well ordered
	auto cnt = 1;
	auto msok = true;
	for (const auto &ms : milestones)
	{
		if (ms.first != cnt++)
			msok = false;
		views.push_back(ms.second);
	}
	if (!msok)
	{
		report += fname;
		report += ": ";
		report += _("milestones are not numbered correctly.");
		report += "\n";
	}
	
	/*
	for (const auto fname : txtdir.GetFiles())
	{
		try
	for (const auto &fname : filenames)
	{
		//files.emplace_back(fname);
		// TODO
	}
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
					// TODO
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
*/
}

Document::~Document() = default;

View Document::GetView(const Id &id)
{
	auto it = view_refs.find(id);
	if (it == view_refs.end())
		throw crn::ExceptionNotFound(_("Cannot find view with id ") + id);

	auto v = std::shared_ptr<View::Impl>{};
	if (it->second.expired())
	{
		v = std::make_shared<View::Impl>(id, base, name);
		it->second = v;
	}
	else
		v = it->second.lock();
	return View(v);
}









int main(int argc, char *argv[])
{
	auto doc = Document("/data/Dropbox/ORIFLAMMS-Fichiers-exemple/Référentiels/Encodage XML/conversions vers TEI Oriflamms");
	return 0;
}

