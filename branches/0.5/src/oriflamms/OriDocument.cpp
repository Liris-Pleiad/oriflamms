/* Copyright 2015 Université Paris Descartes
 * 
 * file: OriDocument.cpp
 * \author Yann LEYDIER
 */

#include <oriflamms_config.h>
#include <OriDocument.h>
#include <CRNIO/CRNIO.h>
#include <OriLines.h>
#include <CRNAI/CRNPathFinding.h>
#include <CRNImage/CRNDifferential.h>
#include <OriTEIImporter.h>
#include <CRNi18n.h>

#include <iostream>

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
const auto LINKDIR = "img_links"_p;
const auto ZONEDIR = "zones"_p;
const auto ONTODIR = "ontologies"_p;
const auto ONTOLINKDIR = "ontologies_links"_p;
const auto ORIDIR = "oriflamms"_p;

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
// Zone
//////////////////////////////////////////////////////////////////////////////////
Zone::Zone(crn::xml::Element &elem):
	el(elem)
{
	try
	{
		auto x1 = el.GetAttribute<int>("ulx", false);
		auto y1 = el.GetAttribute<int>("uly", false);
		auto x2 = el.GetAttribute<int>("lrx", false);
		auto y2 = el.GetAttribute<int>("lry", false);
		pos = {x1, y1, x2, y2};
	}
	catch (...) { }
	try
	{
		auto str = el.GetAttribute<crn::StringUTF8>("points", false);
		auto pts = str.Split(" \t");
		decltype(box) tmpbox;
		for (const auto pt : pts)
		{
			const auto coma = pt.Find(",");
			if (coma == crn::StringUTF8::NPos())
				throw 0;
			tmpbox.emplace_back(pt.SubString(0, coma).ToInt(), pt.SubString(coma + 1, 0).ToInt());
		}
		box.swap(tmpbox);
	}
	catch (...) { }
}

const crn::Rect& Zone::GetPosition() const noexcept
{
	if (!pos.IsValid() && !box.empty())
	{
		auto ulx = std::numeric_limits<int>::max();
		auto uly = std::numeric_limits<int>::max();
		auto lrx = 0;
		auto lry = 0;
		for (const auto &pt : box)
		{
			if (pt.X < ulx) ulx = pt.X;
			if (pt.X > lrx) lrx = pt.X;
			if (pt.Y < uly) uly = pt.Y;
			if (pt.Y > lry) lry = pt.Y;
		}
		pos = {ulx, uly, lrx, lry};
	}
	return pos;
}

void Zone::SetPosition(const crn::Rect &r)
{
	if (!r.IsValid())
		throw crn::ExceptionInvalidArgument{"Zone::SetPosition(): "_s + _("null rectangle.")};
	pos = r;
	el.SetAttribute("ulx", pos.GetLeft());
	el.SetAttribute("uly", pos.GetTop());
	el.SetAttribute("lrx", pos.GetRight());
	el.SetAttribute("lry", pos.GetBottom());
}

void Zone::SetContour(const std::vector<crn::Point2DInt> &c)
{
	if (c.size() < 3)
		throw crn::ExceptionInvalidArgument{"Zone::SetContour(): "_s + _("the contour has less than 3 points.")};
	box = c;
	auto str = ""_s;
	for (const auto &pt : box)
		str += pt.X + ","_s + pt.Y + " "_s;
	str.Crop(0, str.Size() - 1);
	el.SetAttribute("points", str);
}

//////////////////////////////////////////////////////////////////////////////////
// Line
//////////////////////////////////////////////////////////////////////////////////
const std::vector<Id>& Line::GetWords() const
{
	if (!center.empty())
	{
		std::move(center.begin(), center.end(), std::back_inserter(left));
		center.clear();
		center.shrink_to_fit();
	}
	if (!right.empty())
	{
		std::move(right.begin(), right.end(), std::back_inserter(left));
		right.clear();
		right.shrink_to_fit();
	}
	return left;
}

//////////////////////////////////////////////////////////////////////////////////
// View::Impl
//////////////////////////////////////////////////////////////////////////////////
struct View::Impl
{
	Impl(const Id &surfid, Document::ViewStructure &s, const crn::Path &base, const crn::StringUTF8 &projname);
	~Impl();
	void readZoneWElements(crn::xml::Element &el);
	void readLinkElements(crn::xml::Element &el);
	void load();
	void save();

	Id id;
	crn::SBlock img;
	crn::Path imagename;
	mutable crn::SImageGray weight;

	Document::ViewStructure &struc;
	std::unordered_map<Id, Zone> zones;

	crn::xml::Document zonesdoc;
	crn::xml::Document linksdoc;
	std::unordered_map<crn::StringUTF8, crn::xml::Element> link_groups;

	crn::Path datapath;
	std::unordered_map<Id, crn::Prop3> validation; // word Id
	std::unordered_map<Id, std::vector<GraphicalLine>> medlines; // column Id
	std::unordered_map<Id, std::pair<Id, size_t>> line_links; // line Id -> column Id + index
};

View::Impl::Impl(const Id &surfid, Document::ViewStructure &s, const crn::Path &base, const crn::StringUTF8 &projname):
	id(surfid),
	struc(s)
{
	// read ZONEDIR/projname_id-zones.xml for boxes and image
	zonesdoc = crn::xml::Document{base / ZONEDIR / projname + "_" + crn::Path{id} + "-zones.xml"};
	auto root = zonesdoc.GetRoot();
	auto el = root.GetFirstChildElement("facsimile");
	if (!el)
		throw crn::ExceptionNotFound(projname + "_" + id + "-zones.xml: " + _("no facsimile element."));
	el = el.GetFirstChildElement("surface");
	if (!el)
		throw crn::ExceptionNotFound(projname + "_" + id + "-zones.xml: " + _("no surface element."));
	auto grel = el.GetFirstChildElement("graphic");
	if (!grel)
		throw crn::ExceptionNotFound(projname + "_" + id + "-zones.xml: " + _("no graphic element."));
	imagename = base / IMGDIR / grel.GetAttribute<crn::StringUTF8>("url", false);
	img = crn::Block::New(crn::NewImageFromFile(imagename));
	readZoneWElements(el);

	// read LINKDIR/projname_id-links.xml for boxes
	linksdoc = crn::xml::Document{base / LINKDIR / projname + "_" + crn::Path{id} + "-links.xml"};
	root = linksdoc.GetRoot();
	el = root.GetFirstChildElement("text");
	if (!el)
		throw crn::ExceptionNotFound(projname + "_" + id + "-links.xml: " + _("no text element."));
	el = el.GetFirstChildElement("body");
	if (!el)
		throw crn::ExceptionNotFound(projname + "_" + id + "-links.xml: " + _("no body element."));
	el = el.GetFirstChildElement("ab");
	if (!el)
		throw crn::ExceptionNotFound(projname + "_" + id + "-links.xml: " + _("no ab element."));
	readLinkElements(el);
	if (link_groups.find(SURFACELINKS) == link_groups.end())
		throw crn::ExceptionNotFound(projname + "_" + id + "-links.xml: " + _("no linkGrp element for the surfaces."));
	if (link_groups.find(PAGELINKS) == link_groups.end())
		link_groups.emplace(PAGELINKS, el.PushBackElement("linkGrp")).first->second.SetAttribute("type", PAGELINKS);
	if (link_groups.find(COLUMNLINKS) == link_groups.end())
		link_groups.emplace(COLUMNLINKS, el.PushBackElement("linkGrp")).first->second.SetAttribute("type", COLUMNLINKS);
	if (link_groups.find(LINELINKS) == link_groups.end())
		link_groups.emplace(LINELINKS, el.PushBackElement("linkGrp")).first->second.SetAttribute("type", LINELINKS);
	if (link_groups.find(WORDLINKS) == link_groups.end())
		link_groups.emplace(WORDLINKS, el.PushBackElement("linkGrp")).first->second.SetAttribute("type", WORDLINKS);
	if (link_groups.find(CHARLINKS) == link_groups.end())
		link_groups.emplace(CHARLINKS, el.PushBackElement("linkGrp")).first->second.SetAttribute("type", CHARLINKS);
	
	// read if it exists ONTODIR/??? for character classes
	// TODO
	// read if it exists ONTOLINKDIR/??? for links between characters and classes
	// TODO

	// read or create oriflamms file
	datapath = base / ORIDIR / projname + "_" + crn::Path{id};
	if (crn::IO::Access(datapath + "-oridata.xml", crn::IO::EXISTS))
	{
		load();
	}
	else
	{ // create file
		for (const auto &p : struc.words)
			validation.emplace(p.first, crn::Prop3::Unknown);
		for (const auto &p : struc.columns)
		{
			medlines.emplace(p.first, std::vector<GraphicalLine>{}); // create empty line
			auto cnt = size_t(0);
			for (const auto &lid : p.second.GetLines())
				line_links.emplace(lid, std::make_pair(p.first, cnt++));
		}
		save();
	}
}

View::Impl::~Impl()
{
	zonesdoc.Save();
	linksdoc.Save();
	save();
}

void View::Impl::readZoneWElements(crn::xml::Element &el)
{
	if (el.GetName() == "zone")
	{
		const auto id = el.GetAttribute<Id>("xml:id", false);
		zones.emplace(id, Zone{el});
	}
	for (auto sel = el.BeginElement(); sel != el.EndElement(); ++sel)
		readZoneWElements(sel);
}

void View::Impl::readLinkElements(crn::xml::Element &el)
{
	auto grel = el.GetFirstChildElement("linkGrp");
	while (grel)
	{
		const auto type = grel.GetAttribute<crn::StringUTF8>("type", false);
		link_groups.emplace(type, grel);

		auto lel = grel.GetFirstChildElement("link");
		while (lel)
		{
			const auto targets = lel.GetAttribute<crn::StringUTF8>("target", false).Split(" \t");
			auto txtid = ""_s;
			auto zoneid = ""_s;
			for (const auto &target : targets)
			{
				if (target.StartsWith("txt:"))
					txtid = target.SubString(4, 0);
				else if (target.StartsWith("img:"))
				{
					zoneid = target.SubString(4, 0);
				}
				else
					throw crn::ExceptionInvalidArgument(id + "-links: " + _("invalid target base."));
			}
			if (txtid.IsEmpty() || zoneid.IsEmpty())
				throw crn::ExceptionInvalidArgument(id + "-links: " + _("incomplete target."));
			if (type == SURFACELINKS)
			{ // well... errrrrrrrrr... we could check if the target is consistent
			}
			else if (type == PAGELINKS )
			{
				auto it = struc.pages.find(txtid);
				if (it == struc.pages.end())
					throw crn::ExceptionInvalidArgument(id + "-links: " + _("target points to an unknown page: ") + txtid);
				if (zones.find(zoneid) == zones.end())
					throw crn::ExceptionInvalidArgument(id + "-links: " + _("target points to an unknown zone: ") + zoneid);
				it->second.zone = zoneid;
			}
			else if (type == COLUMNLINKS )
			{
				auto it = struc.columns.find(txtid);
				if (it == struc.columns.end())
					throw crn::ExceptionInvalidArgument(id + "-links: " + _("target points to an unknown column: ") + txtid);
				if (zones.find(zoneid) == zones.end())
					throw crn::ExceptionInvalidArgument(id + "-links: " + _("target points to an unknown zone: ") + zoneid);
				it->second.zone = zoneid;
			}
			else if (type == LINELINKS )
			{
				auto it = struc.lines.find(txtid);
				if (it == struc.lines.end())
					throw crn::ExceptionInvalidArgument(id + "-links: " + _("target points to an unknown line: ") + txtid);
				if (zones.find(zoneid) == zones.end())
					throw crn::ExceptionInvalidArgument(id + "-links: " + _("target points to an unknown zone: ") + zoneid);
				it->second.zone = zoneid;
			}
			else if (type == WORDLINKS )
			{
				auto it = struc.words.find(txtid);
				if (it == struc.words.end())
					throw crn::ExceptionInvalidArgument(id + "-links: " + _("target points to an unknown word: ") + txtid);
				if (zones.find(zoneid) == zones.end())
					throw crn::ExceptionInvalidArgument(id + "-links: " + _("target points to an unknown zone: ") + zoneid);
				it->second.zone = zoneid;
			}
			else if (type == CHARLINKS )
			{
				auto it = struc.characters.find(txtid);
				if (it == struc.characters.end())
					throw crn::ExceptionInvalidArgument(id + "-links: " + _("target points to an unknown character: ") + txtid);
				if (zones.find(zoneid) == zones.end())
					throw crn::ExceptionInvalidArgument(id + "-links: " + _("target points to an unknown zone: ") + zoneid);
				it->second.zone = zoneid;
			}

			lel = lel.GetNextSiblingElement("link");
		} // links
		grel = grel.GetNextSiblingElement("linkGrp");
	} // link groups
}

void View::Impl::load()
{
	const auto f = datapath + "-oridata.xml";
	// open file
	auto doc = crn::xml::Document{f};
	auto root = doc.GetRoot();
	if (root.GetName() != "OriData")
		throw crn::ExceptionInvalidArgument{_("Not an Oriflamms data file: ") + crn::StringUTF8(f)};
	// read validation
	auto var = root.GetFirstChildElement("validation");
	if (!var)
		throw crn::ExceptionNotFound{_("No validation element in ") + crn::StringUTF8(f)};
	for (auto el = var.BeginElement(); el != var.EndElement(); ++el)
		validation.emplace(el.GetAttribute<Id>("id", false), el.GetAttribute<int>("ok", false));
	// read medlines
	var = root.GetFirstChildElement("medlines");
	if (!var)
		throw crn::ExceptionNotFound{_("No medlines element in ") + crn::StringUTF8(f)};
	for (auto cel = var.BeginElement(); cel != var.EndElement(); ++cel)
	{
		const auto colid = cel.GetAttribute<Id>("id", false);
		medlines.emplace(colid, std::vector<GraphicalLine>{}); // create empty line
		for (auto el = cel.BeginElement(); el != cel.EndElement(); ++el)
		{
			medlines[colid].emplace_back(el);
		}
	}
	// read line_links
	var = root.GetFirstChildElement("lineLinks");
	if (!var)
		throw crn::ExceptionNotFound{_("No lineLinks element in ") + crn::StringUTF8(f)};
	for (auto el = var.BeginElement(); el != var.EndElement(); ++el)
		line_links.emplace(el.GetAttribute<Id>("id", false), std::make_pair(el.GetAttribute<Id>("col", false), el.GetAttribute<int>("n", false)));
}

void View::Impl::save()
{
	// create document
	auto doc = crn::xml::Document{};
	doc.PushBackComment("oriflamms data file");
	auto root = doc.PushBackElement("OriData");
	// save validation
	auto var = root.PushBackElement("validation");
	for (const auto &p : validation)
	{
		auto el = var.PushBackElement("v");
		el.SetAttribute("id", p.first);
		el.SetAttribute("ok", p.second.GetValue());
	}
	// save medlines
	var = root.PushBackElement("medlines");
	for (const auto &c : medlines)
	{
		auto col = var.PushBackElement("column");
		col.SetAttribute("id", c.first);
		for (const auto &l : c.second)
		{
			l.Serialize(col);
		}
	}
	// save line_lines
	var = root.PushBackElement("lineLinks");
	for (const auto &l : line_links)
	{
		auto el = var.PushBackElement("ll");
		el.SetAttribute("id", l.first);
		el.SetAttribute("col", l.second.first);
		el.SetAttribute("n", l.second.second);
	}
	// save file
	doc.Save(datapath + "-oridata.xml");
}

//////////////////////////////////////////////////////////////////////////////////
// View
//////////////////////////////////////////////////////////////////////////////////
View::View() = default;

View::~View() = default;

const crn::Path& View::GetImageName() const noexcept { return pimpl->imagename; }
const std::vector<Id>& View::GetPages() const noexcept { return pimpl->struc.pageorder; }

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 */
const Page& View::GetPage(const Id &id) const
{
	auto it = pimpl->struc.pages.find(id);
	if (it == pimpl->struc.pages.end())
		throw crn::ExceptionNotFound{_("Invalid page id: ") + id};
	return it->second;
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 */
Page& View::GetPage(const Id &id)
{
	auto it = pimpl->struc.pages.find(id);
	if (it == pimpl->struc.pages.end())
		throw crn::ExceptionNotFound{_("Invalid page id: ") + id};
	return it->second;
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 */
const Column& View::GetColumn(const Id &id) const
{
	auto it = pimpl->struc.columns.find(id);
	if (it == pimpl->struc.columns.end())
		throw crn::ExceptionNotFound{_("Invalid column id: ") + id};
	return it->second;
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 */
Column& View::GetColumn(const Id &id)
{
	auto it = pimpl->struc.columns.find(id);
	if (it == pimpl->struc.columns.end())
		throw crn::ExceptionNotFound{_("Invalid column id: ") + id};
	return it->second;
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 * \param[in]	id	the id of a column
 * \return all graphical lines of the column
 */
const std::vector<GraphicalLine>& View::GetGraphicalLines(const Id &id) const
{
	auto it = pimpl->medlines.find(id);
	if (it == pimpl->medlines.end())
		throw crn::ExceptionNotFound{_("Invalid column id: ") + id};
	return it->second;
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 */
const Line& View::GetLine(const Id &id) const
{
	auto it = pimpl->struc.lines.find(id);
	if (it == pimpl->struc.lines.end())
		throw crn::ExceptionNotFound{_("Invalid line id: ") + id};
	return it->second;
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 */
Line& View::GetLine(const Id &id)
{
	auto it = pimpl->struc.lines.find(id);
	if (it == pimpl->struc.lines.end())
		throw crn::ExceptionNotFound{_("Invalid line id: ") + id};
	return it->second;
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 * \throws	crn::ExceptionDomain	no graphical line associated to the text line
 * \param[in]	id	the id of a line
 * \return the medline of the line
 */
const GraphicalLine& View::GetGraphicalLine(const Id &id) const
{
	auto it = pimpl->line_links.find(id);
	if (it == pimpl->line_links.end())
		throw crn::ExceptionNotFound{_("Invalid line id: ") + id};

	if (it->second.second >= pimpl->medlines[it->second.first].size())
		throw crn::ExceptionDomain{_("No graphical line associated to text line: ") + id};

	return pimpl->medlines[it->second.first][it->second.second];
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 * \throws	crn::ExceptionDomain	no graphical line associated to the text line
 * \param[in]	id	the id of a line
 * \return the medline of the line
 */
GraphicalLine& View::GetGraphicalLine(const Id &id)
{
	auto it = pimpl->line_links.find(id);
	if (it == pimpl->line_links.end())
		throw crn::ExceptionNotFound{_("Invalid line id: ") + id};

	if (it->second.second >= pimpl->medlines[it->second.first].size())
		throw crn::ExceptionDomain{_("No graphical line associated to text line: ") + id};

	return pimpl->medlines[it->second.first][it->second.second];
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 */
const Word& View::GetWord(const Id &id) const
{
	auto it = pimpl->struc.words.find(id);
	if (it == pimpl->struc.words.end())
		throw crn::ExceptionNotFound{_("Invalid word id: ") + id};
	return it->second;
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 */
Word& View::GetWord(const Id &id)
{
	auto it = pimpl->struc.words.find(id);
	if (it == pimpl->struc.words.end())
		throw crn::ExceptionNotFound{_("Invalid word id: ") + id};
	return it->second;
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 * \param[in]	id	the id of a word
 * \returns the validation state of the word
 */
const crn::Prop3& View::IsValid(const Id &id) const
{
	const auto it = pimpl->validation.find(id);
	if (it == pimpl->validation.end())
		throw crn::ExceptionNotFound{_("Invalid word id: ") + id};
	return it->second;
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 * \param[in]	id	the id of a word
 * \param[in]	val	the validation state of the word
 */
void View::SetValid(const Id &id, const crn::Prop3 &val)
{
	auto it = pimpl->validation.find(id);
	if (it == pimpl->validation.end())
		throw crn::ExceptionNotFound{_("Invalid word id: ") + id};
	it->second = val;
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 */
const Character& View::GetCharacter(const Id &id) const
{
	auto it = pimpl->struc.characters.find(id);
	if (it == pimpl->struc.characters.end())
		throw crn::ExceptionNotFound{_("Invalid character id: ") + id};
	return it->second;
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 */
Character& View::GetCharacter(const Id &id)
{
	auto it = pimpl->struc.characters.find(id);
	if (it == pimpl->struc.characters.end())
		throw crn::ExceptionNotFound{_("Invalid character id: ") + id};
	return it->second;
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 */
const Zone& View::GetZone(const Id &id) const
{
	auto it = pimpl->zones.find(id);
	if (it == pimpl->zones.end())
		throw crn::ExceptionNotFound{_("Invalid zone id: ") + id};
	return it->second;
}

/*! 
 * \throws	crn::ExceptionNotFound	invalid id
 */
Zone& View::GetZone(const Id &id)
{
	auto it = pimpl->zones.find(id);
	if (it == pimpl->zones.end())
		throw crn::ExceptionNotFound{_("Invalid zone id: ") + id};
	return it->second;
}

/*! Sets the bounding box of an element
 * \param[in]	id	the id of the element
 * \param[in]	r	the bounding box
 * \param[in]	compute_contour	shall the contour be automatically set? (computes curves at left and right ends)
 */
void View::SetPosition(const Id &id, const crn::Rect &r, bool compute_contour)
{
	// TODO
	auto zone = (Zone*)nullptr;
	auto wit = pimpl->struc.words.find(id);
	if (wit != pimpl->struc.words.end())
	{
		auto zid = wit->second.GetZone();
	}
	else
	{
		auto cit = pimpl->struc.characters.find(id);
		if (cit != pimpl->struc.characters.end())
		{
			const auto &zid = cit->second.GetZone();
		}
		else
		{
			auto zit = pimpl->zones.find(id);
			if (zit != pimpl->zones.end())
				zone = &zit->second;
		}
	}
	if (!zone)
		return;
}

/*! Sets the contour of an element
 * \param[in]	id	the id of the element
 * \param[in]	c	the contour
 * \param[in]	set_position	shall the bounding box be automatically set?
 */
void View::SetContour(const Id &id, const std::vector<crn::Point2DInt> &c, bool set_position)
{
	// TODO
}

struct stepcost
{
	stepcost(const crn::ImageGray &ig, int x):img(ig),ref(x) { }
	const crn::ImageGray &img;
	const int ref;
	inline double operator()(const crn::Point2DInt &p1, const crn::Point2DInt &p2) const
	{
		return img.At(p2.X, p2.Y) + 2 * crn::Abs(ref - p2.X);
	}
};

struct heuristic
{
	inline double operator()(const crn::Point2DInt &p1, const crn::Point2DInt &p2) const
	{
		return crn::Abs(p2.X - p1.X) * 255 + crn::Abs(p2.Y - p1.Y) * 64;
	}
};

struct get_neighbors
{
	get_neighbors(const crn::Rect &r):bbox(r) { }
	const crn::Rect bbox;
	inline const std::vector<crn::Point2DInt> operator()(const crn::Point2DInt &p) const
	{
		std::vector<crn::Point2DInt> n;
		if (p.Y < bbox.GetBottom())
		{
			n.push_back(crn::Point2DInt(p.X, p.Y + 1));
			if (p.X < bbox.GetRight())
				n.push_back(crn::Point2DInt(p.X + 1, p.Y + 1));
			if (p.X > bbox.GetLeft())
				n.push_back(crn::Point2DInt(p.X - 1, p.Y + 1));
		}
		return n;
	}
};

std::vector<crn::Point2DInt> View::ComputeFrontier(size_t x, size_t y1, size_t y2) const
{
	return SimplifyCurve(crn::AStar(
			crn::Point2DInt(int(x), int(y1)),
			crn::Point2DInt(int(x), int(y2)),
			stepcost(getWeight(), int(x)),
			heuristic(),
			get_neighbors(pimpl->img->GetAbsoluteBBox())), 1);
}

const crn::ImageGray& View::getWeight() const
{
	if (!pimpl->weight)
	{
		const auto wname = pimpl->datapath + "-weight.png";
		try
		{
			pimpl->weight = crn::Block::New(crn::NewImageFromFile(wname))->GetGray();
		}
		catch (...)
		{
			auto diff = crn::Differential::NewGaussian(*pimpl->img->GetRGB(), crn::Differential::RGBProjection::ABSMAX, 0);
			diff.Diffuse(5);
			pimpl->weight = std::make_shared<crn::ImageGray>(crn::Downgrade<crn::ImageGray>(diff.MakeLaplacian()));
			for (auto tmp : Range(*pimpl->weight))
				pimpl->weight->At(tmp) = uint8_t(pimpl->weight->At(tmp) / 2 + 127 - pimpl->img->GetGray()->At(tmp) / 2);
			pimpl->weight->SavePNG(wname);
		}
	}
	return *pimpl->weight;
}

Id View::addZone(Id id_base, crn::xml::Element &elem)
{
	while (pimpl->zones.find(id_base) != pimpl->zones.end())
		id_base += "+";
	pimpl->zones.emplace(id_base, elem);
	elem.SetAttribute("xml:id", id_base);
	return id_base;
}

//////////////////////////////////////////////////////////////////////////////////
// Document
//////////////////////////////////////////////////////////////////////////////////
/*!
 * \param[in]	dirpath	base directory of the project
 * \param[in]	prog	a progress bar
 *
 * \throws	crn::ExceptionInvalidArgument	malformed main transcription filename
 */
Document::Document(const crn::Path &dirpath, crn::Progress *prog):
	base(dirpath)
{
	// create oriflamms directory if needed
	if (!crn::IO::Access(base / ORIDIR, crn::IO::EXISTS))
	{
		crn::IO::Mkdir(base / ORIDIR);
	}
	auto teiselection = TEISelectionNode{"TEI"};
	try
	{
		teiselection.Load(base / ORIDIR / "tei_selection.xml");
	}
	catch (std::exception &ex)
	{
		throw ExceptionTEISelection(ex.what());
	}

	// get base name
	const auto txtdir = crn::IO::Directory{base / TEXTDIR};
	auto dir = crn::IO::Directory{txtdir};
	for (const auto &fname : dir.GetFiles())
	{
		const auto &fbase = fname.GetBase();
		if (fbase.EndsWith("-w"_s))
		{
			name = fbase;
			name.Crop(0, name.Size() - 2);
			break;
		}
	}
	if (name.IsEmpty())
		throw crn::ExceptionInvalidArgument{_("Cannot find main transcription filename.")};

	// read structure up to word level
	auto milestones = std::multimap<int, Id>{};
	try
	{
		auto doc = crn::xml::Document{base / TEXTDIR / name + "-w.xml"_p};
		auto root = doc.GetRoot();
		auto pos = ElementPosition{};
		readTextWElements(root, pos, teiselection, milestones);
	}
	catch (std::exception &ex)
	{
		report += name + "-w.xml";
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
		view_refs.emplace(ms.second, ViewRef{});
	}
	if (!msok)
	{
		report += name + "-w.xml";
		report += ": ";
		report += _("milestones are not numbered correctly: ");
		for (const auto &ms : milestones)
			report += ms.first + " "_s;
		report += "\n";
	}
	// read structure at character level
	try
	{
		auto doc = crn::xml::Document{base / TEXTDIR / name + "-c.xml"_p};
		auto root = doc.GetRoot();
		auto pos = ElementPosition{};
		readTextCElements(root, pos, teiselection);
	}
	catch (std::exception &ex)
	{
		report += name + "-c.xml";
		report += ": ";
		report += ex.what();
		report += "\n";
	}
	
	// read all files
	for (const auto &id : views)
	{
		auto v = GetView(id);
		// index and compute boxes if needed
		for (auto &p : v.pimpl->struc.pages)
		{ // for each page
			positions.emplace(p.first, id);

			if (p.second.GetZone().IsEmpty())
			{
				// add zone to XML
				auto root = v.pimpl->zonesdoc.GetRoot();
				auto el = root.GetFirstChildElement("facsimile");
				el = el.GetFirstChildElement("surface");
				el = el.PushBackElement("zone");
				el.SetAttribute("type", "page");
				const auto &pbox = v.pimpl->img->GetAbsoluteBBox();
				el.SetAttribute("ulx", pbox.GetLeft());
				el.SetAttribute("uly", pbox.GetTop());
				el.SetAttribute("lrx", pbox.GetRight());
				el.SetAttribute("lry", pbox.GetBottom());
				// add zone to page
				p.second.zone = v.addZone("zone-" + p.first, el);
				// add link
				auto linkit = v.pimpl->link_groups.find(PAGELINKS);
				el = linkit->second.PushBackElement("link");
				el.SetAttribute("target", "txt:" + p.first + " img:" + p.second.zone);
			}
			auto &pzone = v.GetZone(p.second.GetZone());

			auto pbox = crn::Rect{};
			for (auto &cid : p.second.GetColumns())
			{
				positions.emplace(cid, ElementPosition{id, p.first});

				auto &col = v.GetColumn(cid);
				if (col.GetZone().IsEmpty())
				{
					// add zone to XML
					auto el = pzone.el.PushBackElement("zone");
					el.SetAttribute("type", "column");
					// add zone to page
					col.zone = v.addZone("zone-" + cid, el);
					// add link
					auto linkit = v.pimpl->link_groups.find(COLUMNLINKS);
					el = linkit->second.PushBackElement("link");
					el.SetAttribute("target", "txt:" + cid + " img:" + col.zone);
				}
				auto &czone = v.GetZone(col.GetZone());

				auto cbox = crn::Rect{};
				for (auto &lid : col.GetLines())
				{
					positions.emplace(lid, ElementPosition{id, p.first, cid});

					auto &line = v.GetLine(lid);
					if (line.GetZone().IsEmpty())
					{
						// add zone to XML
						auto el = czone.el.PushBackElement("zone");
						el.SetAttribute("type", "line");
						// add zone to page
						line.zone = v.addZone("zone-" + lid, el);
						// add link
						auto linkit = v.pimpl->link_groups.find(LINELINKS);
						el = linkit->second.PushBackElement("link");
						el.SetAttribute("target", "txt:" + lid + " img:" + line.zone);
					}
					auto &lzone = v.GetZone(line.GetZone());

					auto lbox = crn::Rect{};
					for (auto &wid : line.GetWords())
					{
						positions.emplace(wid, ElementPosition{id, p.first, cid, lid});

						auto &word = v.GetWord(wid);
						if (word.GetZone().IsEmpty())
						{
							// add zone to XML
							auto el = lzone.el.PushBackElement("zone");
							el.SetAttribute("type", "word");
							// add zone to page
							word.zone = v.addZone("zone-" + wid, el);
							// add link
							auto linkit = v.pimpl->link_groups.find(WORDLINKS);
							el = linkit->second.PushBackElement("link");
							el.SetAttribute("target", "txt:" + wid + " img:" + word.zone);
						}
						auto &wzone = v.GetZone(word.GetZone());

						for (const auto &cid : word.GetCharacters())
						{
							positions.emplace(wid, ElementPosition{id, p.first, cid, lid, wid});

							auto &cha = v.GetCharacter(cid);
							if (cha.GetZone().IsEmpty())
							{
								// add zone to XML
								auto el = wzone.el.PushBackElement("zone");
								el.SetAttribute("type", "character");
								// add zone to page
								cha.zone = v.addZone("zone-" + cid, el);
								// add link
								auto linkit = v.pimpl->link_groups.find(CHARLINKS);
								el = linkit->second.PushBackElement("link");
								el.SetAttribute("target", "txt:" + cid + " img:" + cha.zone);
							}
							else
							{
								auto &czone = v.GetZone(cha.GetZone());
								const auto &cpos = czone.GetPosition();
								if (cpos.IsValid() && czone.GetContour().empty())
								{
									// compute contour
									auto contour = v.ComputeFrontier(cpos.GetLeft(), cpos.GetTop(), cpos.GetBottom());
									auto contour2 = v.ComputeFrontier(cpos.GetRight(), cpos.GetTop(), cpos.GetBottom());
									std::move(contour2.rbegin(), contour2.rend(), std::back_inserter(contour));
									czone.SetContour(contour);
								}
							}
						}

						const auto &wpos = wzone.GetPosition();
						if (wpos.IsValid() && wzone.GetContour().empty())
						{
							// compute contour
							auto contour = v.ComputeFrontier(wpos.GetLeft(), wpos.GetTop(), wpos.GetBottom());
							auto contour2 = v.ComputeFrontier(wpos.GetRight(), wpos.GetTop(), wpos.GetBottom());
							std::move(contour2.rbegin(), contour2.rend(), std::back_inserter(contour));
							wzone.SetContour(contour);
						}
						lbox |= wpos;
						
					} // words

					if (!lzone.GetPosition().IsValid() && lbox.IsValid())
						lzone.SetPosition(lbox);
					cbox |= lzone.GetPosition();
				} // lines

				if (!czone.GetPosition().IsValid() && cbox.IsValid())
					czone.SetPosition(cbox);
				pbox |= czone.GetPosition();
			} // columns

			if (!pzone.GetPosition().IsValid() && pbox.IsValid())
				pzone.SetPosition(pbox);
		} // pages
	}
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
		v = std::make_shared<View::Impl>(id, view_struct[id], base, name);
		it->second = v;
	}
	else
		v = it->second.lock();
	return View(v);
}

static crn::StringUTF8 allTextInElement(crn::xml::Element &el, const TEISelectionNode& teisel)
{
	auto txt = ""_s;
	for (auto n = el.BeginNode(); n != el.EndNode(); ++n)
	{
		if (n.IsText())
		{
			txt += n.AsText().GetValue();
		}
		else if (n.IsElement())
		{
			auto sel = n.AsElement();
			const auto elname = sel.GetName();
			for (const TEISelectionNode &cnode : teisel.GetChildren())
				if (cnode.GetName() == elname)
				{
					txt += allTextInElement(sel, cnode);
					break;
				}
		}
	}
	return txt;
}

void Document::readTextWElements(crn::xml::Element &el, ElementPosition &pos, const TEISelectionNode& teisel, std::multimap<int, Id> &milestones)
{
	for (auto sel = el.BeginElement(); sel != el.EndElement(); ++sel)
	{
		const auto elname = sel.GetName();
		for (const TEISelectionNode &cnode : teisel.GetChildren())
			if (cnode.GetName() == elname)
			{
				const auto elid = sel.GetAttribute<Id>("xml:id", true);
				if (elname == "milestone")
				{
					if (sel.GetAttribute<crn::StringUTF8>("unit", true) == "surface")
					{
						if (elid.IsEmpty())
							throw crn::ExceptionNotFound(name + "-w: "_s + _("milestone without an id."));
						milestones.emplace(sel.GetAttribute<int>("n", true), elid);
						pos.view = elid;
					}
				}
				else if (elname == "pb")
				{
					if (elid.IsEmpty())
						throw crn::ExceptionNotFound(name + "-w: "_s + _("page without an id."));
					view_struct[pos.view].pages.emplace(elid, Page{});
					view_struct[pos.view].pageorder.push_back(elid);
					pos.page = elid;
				}
				else if (elname == "cb")
				{
					if (elid.IsEmpty())
						throw crn::ExceptionNotFound(name + "-w: "_s + _("column without an id."));
					view_struct[pos.view].columns.emplace(elid, Column{});
					view_struct[pos.view].pages[pos.page].columns.push_back(elid);
					pos.column = elid;
				}
				else if (elname == "lb")
				{
					if (elid.IsEmpty())
						throw crn::ExceptionNotFound(name + "-w: "_s + _("line without an id."));
					view_struct[pos.view].lines.emplace(elid, Line{});
					view_struct[pos.view].columns[pos.column].lines.push_back(elid);
					pos.line = elid;
					// TODO rejets
				}
				else if ((elname == "w") || (elname == "pc"))
				{
					if (!sel.GetFirstChildElement("seg"))
					{
						if (elid.IsEmpty())
							throw crn::ExceptionNotFound(name + "-w: "_s + _("word or pc without an id."));
						view_struct[pos.view].words.emplace(elid, Word{});
						view_struct[pos.view].words[elid].text = allTextInElement(sel, cnode);
						view_struct[pos.view].lines[pos.line].left.push_back(elid); // TODO rejets
					}
				}
				else if (elname == "seg")
				{
					const auto type = sel.GetAttribute<crn::StringUTF8>("type", true);
					if (type == "wp" || type == "deleted")
					{
						if (elid.IsEmpty())
							throw crn::ExceptionNotFound(name + "-w: "_s + _("seg without an id."));
						view_struct[pos.view].words.emplace(elid, Word{});
						view_struct[pos.view].words[elid].text = allTextInElement(sel, cnode);
						view_struct[pos.view].lines[pos.line].left.push_back(elid); // TODO rejets
					}
				}

				readTextWElements(sel, pos, cnode, milestones);
				break;
			}
	}
}

void Document::readTextCElements(crn::xml::Element &el, ElementPosition &pos, const TEISelectionNode& teisel)
{
	for (auto sel = el.BeginElement(); sel != el.EndElement(); ++sel)
	{
		const auto elname = sel.GetName();
		for (const TEISelectionNode &cnode : teisel.GetChildren())
		{
			if (cnode.GetName() == elname)
			{
				const auto elid = sel.GetAttribute<Id>("xml:id", true);
				if (elname == "milestone")
				{
					if (sel.GetAttribute<crn::StringUTF8>("unit", true) == "surface")
					{
						if (elid.IsEmpty())
							throw crn::ExceptionNotFound(name + "-c: "_s + _("milestone without an id."));
						pos.view = elid;
					}
				}
				else if (elname == "pb")
				{
					if (elid.IsEmpty())
						throw crn::ExceptionNotFound(name + "-c: "_s + _("page without an id."));
					pos.page = elid;
				}
				else if (elname == "cb")
				{
					if (elid.IsEmpty())
						throw crn::ExceptionNotFound(name + "-c: "_s + _("column without an id."));
					pos.column = elid;
				}
				else if (elname == "lb")
				{
					if (elid.IsEmpty())
						throw crn::ExceptionNotFound(name + "-c: "_s + _("line without an id."));
					pos.line = elid;
					// TODO rejets
				}
				else if ((elname == "w") || (elname == "pc"))
				{
					if (!sel.GetFirstChildElement("seg"))
					{
						if (elid.IsEmpty())
							throw crn::ExceptionNotFound(name + "-c: "_s + _("word or pc without an id."));
						pos.word = elid;
					}
				}
				else if (elname == "seg")
				{
					const auto type = sel.GetAttribute<crn::StringUTF8>("type", true);
					if (type == "wp" || type == "deleted")
					{
						if (elid.IsEmpty())
							throw crn::ExceptionNotFound(name + "-c: "_s + _("seg without an id."));
						pos.word = elid;
					}
				}
				else if (elname == "c")
				{
					if (elid.IsEmpty())
						throw crn::ExceptionNotFound(name + "-c: "_s + _("character without an id."));
					view_struct[pos.view].characters.emplace(elid, Character{});
					view_struct[pos.view].characters[elid].text = allTextInElement(sel, cnode);
					view_struct[pos.view].words[pos.word].characters.push_back(elid);
				}

				readTextCElements(sel, pos, cnode);
				break;
			}
		}
	}
}

