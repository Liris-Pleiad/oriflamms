/* Copyright 2015-2016 Université Paris Descartes, ENS-Lyon
 *
 * file: OriDocument.cpp
 * \author Yann LEYDIER
 */

#include <oriflamms_config.h>
#include <OriConfig.h>
#include <OriDocument.h>
#include <CRNIO/CRNIO.h>
#include <OriLines.h>
#include <CRNAI/CRNPathFinding.h>
#include <CRNImage/CRNDifferential.h>
#include <OriViewImpl.h>
#include <OriTextSignature.h>
#include <OriFeatures.h>
#include <CRNIO/CRNZip.h>
#include <CRNIO/CRNFileShield.h>
#include <CRNi18n.h>

#include <iostream>
#include <fstream>

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

const auto GLOBALGLYPH = "ggly:"_s;
const auto LOCALGLYPH = "lgly:"_s;

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

void Zone::Clear()
{
	pos = crn::Rect{};
	el.RemoveAttribute("ulx");
	el.RemoveAttribute("uly");
	el.RemoveAttribute("lrx");
	el.RemoveAttribute("lry");
	box.clear();
	el.RemoveAttribute("points");
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
View::Impl::Impl(const Id &surfid, Document::ViewStructure &s, const crn::Path &base, const crn::StringUTF8 &projname, bool clean):
	id(surfid),
	struc(s)
{
	std::lock_guard<std::mutex> flock(crn::FileShield::GetMutex("views://" + id));

	// read ZONEDIR/projname_id-zones.xml for boxes and image
	const auto zonesdocpath = base / ZONEDIR / projname + "_" + crn::Path{id} + "-zones.xml";
	zonesdoc = crn::xml::Document{zonesdocpath};
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
	//img = crn::Block::New(crn::NewImageFromFile(imagename));
	readZoneElements(el);

	// read LINKDIR/projname_id-links.xml for boxes
	const auto linksdocpath = base / LINKDIR / projname + "_" + crn::Path{id} + "-links.xml";
	linksdoc = crn::xml::Document{linksdocpath};
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
	readLinkElements(el, clean);
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

	// read or create ONTOLINKDIR/projname_id-links.xml for links between characters and classes
	const auto ontolinksdocpath = base / ONTOLINKDIR / projname + "_" + crn::Path{id} + "-ontolinks.xml";
	try
	{
		ontolinksdoc = crn::xml::Document{ontolinksdocpath};
	}
	catch (...)
	{
		ontolinksdoc = crn::xml::Document{};
		root = ontolinksdoc.PushBackElement("TEI");
		auto hel = root.PushBackElement("teiHeader");
		el = hel.PushBackElement("fileDesc");
		auto tmpel = el.PushBackElement("titleStmt");
		tmpel = tmpel.PushBackElement("title");
		tmpel.PushBackText(_("Allograph links"));
		tmpel = el.PushBackElement("publicationStmt");
		tmpel = tmpel.PushBackElement("p");
		tmpel.PushBackText(_("Oriflamms Project, http://oriflamms.hypotheses.org"));
		tmpel = el.PushBackElement("sourceDesc");
		tmpel = tmpel.PushBackElement("p");
		tmpel.PushBackText(_("Created by Oriflamms."));
		el = hel.PushBackElement("encodingDesc");
		el = el.PushBackElement("listPrefixDef");
		tmpel = el.PushBackElement("prefixDef");
		tmpel.SetAttribute("ident", "txt");
		tmpel.SetAttribute("matchPattern", "([a-z]+)");
		tmpel.SetAttribute("replacementPattern", "../texts/" + projname + "-c.xml#$1");
		tmpel = el.PushBackElement("prefixDef");
		tmpel.SetAttribute("ident", LOCALGLYPH.SubString(0, LOCALGLYPH.Size() - 1));
		tmpel.SetAttribute("matchPattern", "([a-z]+)");
		tmpel.SetAttribute("replacementPattern", "../ontologies/" + projname + "_ontology.xml#$1");
		tmpel = el.PushBackElement("prefixDef");
		tmpel.SetAttribute("ident", GLOBALGLYPH.SubString(0, GLOBALGLYPH.Size() - 1));
		tmpel.SetAttribute("matchPattern", "([a-z]+)");
		tmpel.SetAttribute("replacementPattern", "../../charDecl.xml#$1");
		el = root.PushBackElement("text");
		el = el.PushBackElement("body");
		el = el.PushBackElement("ab");
		el = el.PushBackElement("linkGrp");
		el.SetAttribute("type", "words");
		ontolinksdoc.Save(base / ONTOLINKDIR / projname + "_" + crn::Path{id} + "-ontolinks.xml");
	}
	root = ontolinksdoc.GetRoot();
	el = root.GetFirstChildElement("text");
	if (!el)
		throw crn::ExceptionNotFound(projname + "_" + id + "-ontolinks.xml: " + _("no text element."));
	el = el.GetFirstChildElement("body");
	if (!el)
		throw crn::ExceptionNotFound(projname + "_" + id + "-ontolinks.xml: " + _("no body element."));
	el = el.GetFirstChildElement("ab");
	if (!el)
		throw crn::ExceptionNotFound(projname + "_" + id + "-ontolinks.xml: " + _("no ab element."));
	ontolinkgroup = std::make_unique<crn::xml::Element>(el.GetFirstChildElement("linkGrp"));
	if (!*ontolinkgroup)
		throw crn::ExceptionNotFound(projname + "_" + id + "-ontolinks.xml: " + _("no linkGrp element."));
	el = ontolinkgroup->GetFirstChildElement("link");
	while (el)
	{
		const auto target = el.GetAttribute<crn::StringUTF8>("target").Split(" \t");
		auto cid = Id{};
		auto gids = std::vector<Id>{};
		for (const auto &part : target)
		{
			if (part.StartsWith("txt:"))
				cid = part.SubString(4);
			else if (part.StartsWith(LOCALGLYPH))
				gids.push_back(part);
			else if (part.StartsWith(GLOBALGLYPH))
				gids.push_back(part);
			else
				throw crn::ExceptionInvalidArgument(projname + "_" + id + "-ontolinks.xml: " + _("invalid target base."));
		}
		if (cid.IsEmpty())
			throw crn::ExceptionInvalidArgument(projname + "_" + id + "-ontolinks.xml: " + _("incomplete target."));

		std::sort(gids.begin(), gids.end());
		gids.erase(std::unique(gids.begin(), gids.end()), gids.end());
		onto_links.emplace(std::move(cid), std::move(gids));

		el = el.GetNextSiblingElement("link");
	}

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
		//save();
	}
}

View::Impl::~Impl()
{
	save();
}

void View::Impl::readZoneElements(crn::xml::Element &el)
{
	if (el.GetName() == "zone")
	{
		const auto id = el.GetAttribute<Id>("xml:id", true);
		if (id.IsEmpty())
			throw crn::ExceptionNotFound(this->id + "-zones.xml: " + _("zone without an id."));
		zones.emplace(id, Zone{el});
	}
	for (auto sel = el.BeginElement(); sel != el.EndElement(); ++sel)
		readZoneElements(sel);
}

void View::Impl::readLinkElements(crn::xml::Element &el, bool clean)
{
	auto grel = el.GetFirstChildElement("linkGrp");
	while (grel)
	{
		const auto type = grel.GetAttribute<crn::StringUTF8>("type", false);
		link_groups.emplace(type, grel);

		auto lel = grel.GetFirstChildElement("link");
		while (lel)
		{
			auto delnode = false;
			const auto targets = lel.GetAttribute<crn::StringUTF8>("target", false).Split(" \t");
			auto txtid = ""_s;
			auto zoneid = ""_s;
			for (const auto &target : targets)
			{
				if (target.StartsWith("txt:"))
					txtid = target.SubString(4, 0);
				else if (target.StartsWith("img:"))
					zoneid = target.SubString(4, 0);
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
				{
					if (clean)
						delnode = true;
					else
						logmsg += id + "-links: "_s + _("target points to an unknown page: ") + txtid + "\n";
				}
				else if (zones.find(zoneid) == zones.end())
				{
					if (clean)
						delnode = true;
					else
						logmsg += id + "-links: "_s + _("target points to an unknown zone: ") + zoneid + "\n";
				}
				else
					it->second.zone = zoneid;
			}
			else if (type == COLUMNLINKS )
			{
				auto it = struc.columns.find(txtid);
				if (it == struc.columns.end())
				{
					if (clean)
						delnode = true;
					else
						logmsg += id + "-links: "_s + _("target points to an unknown column: ") + txtid + "\n";
				}
				else if (zones.find(zoneid) == zones.end())
				{
					if (clean)
						delnode = true;
					else
						logmsg += id + "-links: "_s + _("target points to an unknown zone: ") + zoneid + "\n";
				}
				else
					it->second.zone = zoneid;
			}
			else if (type == LINELINKS )
			{
				auto it = struc.lines.find(txtid);
				if (it == struc.lines.end())
				{
					if (clean)
						delnode = true;
					else
						logmsg += id + "-links: "_s + _("target points to an unknown line: ") + txtid + "\n";
				}
				else if (zones.find(zoneid) == zones.end())
				{
					if (clean)
						delnode = true;
					else
						logmsg += id + "-links: "_s + _("target points to an unknown zone: ") + zoneid + "\n";
				}
				else
					it->second.zone = zoneid;
			}
			else if (type == WORDLINKS )
			{
				auto it = struc.words.find(txtid);
				if (it == struc.words.end())
				{
					if (clean)
						delnode = true;
					else
						logmsg += id + "-links: "_s + _("target points to an unknown word: ") + txtid + "\n";
				}
				else if (zones.find(zoneid) == zones.end())
				{
					if (clean)
						delnode = true;
					else
						logmsg += id + "-links: "_s + _("target points to an unknown zone: ") + zoneid + "\n";
				}
				else
					it->second.zone = zoneid;
			}
			else if (type == CHARLINKS )
			{
				auto it = struc.characters.find(txtid);
				if (it == struc.characters.end())
				{
					if (clean)
						delnode = true;
					else
						logmsg += id + "-links: "_s + _("target points to an unknown character: ") + txtid + "\n";
				}
				else if (zones.find(zoneid) == zones.end())
				{
					if (clean)
						delnode = true;
					else
						logmsg += id + "-links: "_s + _("target points to an unknown zone: ") + zoneid + "\n";
				}
				else
					it->second.zone = zoneid;
			}

			auto nel = lel.GetNextSiblingElement("link");
			if (delnode)
			{
				grel.RemoveChild(lel);
			}
			lel = nel;
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
	{
		auto res = validation.emplace(el.GetAttribute<Id>("id", false), crn::Prop3{el.GetAttribute<int>("ok", false)});
		res.first->second.left_corr = el.GetAttribute<int>("left", true);
		res.first->second.right_corr = el.GetAttribute<int>("right", true);
		res.first->second.imgsig = el.GetAttribute<crn::StringUTF8>("imgsig", true);
	}
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
	std::lock_guard<std::mutex> flock(crn::FileShield::GetMutex("views://" + id));
	zonesdoc.Save();
	linksdoc.Save();

	//////////////////////////////////////////////
	// ontology links
	//////////////////////////////////////////////
	ontolinkgroup->Clear();
	for (const auto &l : onto_links)
	{
		if (l.second.empty())
			continue;
		auto val = "txt:" + l.first;
		for (const auto &gid : l.second)
			val += " " + gid;
		auto el = ontolinkgroup->PushBackElement("link");
		el.SetAttribute("target", val);
	}
	ontolinksdoc.Save();

	//////////////////////////////////////////////
	// custom data
	//////////////////////////////////////////////
	const auto f = datapath + "-oridata.xml";
	auto doc = crn::xml::Document{};
	doc.PushBackComment("oriflamms data file");
	auto root = doc.PushBackElement("OriData");
	// save validation
	auto var = root.PushBackElement("validation");
	for (const auto &p : validation)
	{
		auto el = var.PushBackElement("v");
		el.SetAttribute("id", p.first);
		el.SetAttribute("ok", p.second.ok.GetValue());
		el.SetAttribute("left", p.second.left_corr);
		el.SetAttribute("right", p.second.right_corr);
		el.SetAttribute("imgsig", p.second.imgsig);
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
	doc.Save(f);
}

//////////////////////////////////////////////////////////////////////////////////
// View
//////////////////////////////////////////////////////////////////////////////////
View::View() = default;

View::~View() = default;

void View::Save()
{
	if (pimpl)
	{
		pimpl->save();
	}
}

const crn::Path& View::GetImageName() const noexcept { return pimpl->imagename; }

/*! Gets the image */
crn::Block& View::GetBlock() const
{
	if (!pimpl->img)
		pimpl->img = crn::Block::New(crn::NewImageFromFile(pimpl->imagename));
	return *pimpl->img;
}

const std::vector<Id>& View::GetPages() const noexcept { return pimpl->struc.pageorder; }

/*!
 * \throws	crn::ExceptionNotFound	invalid id
 */
const Page& View::GetPage(const Id &page_id) const
{
	auto it = pimpl->struc.pages.find(page_id);
	if (it == pimpl->struc.pages.end())
		throw crn::ExceptionNotFound{"View::GetPage(): "_s + _("Invalid page id: ") + page_id};
	return it->second;
}

/*!
 * \throws	crn::ExceptionNotFound	invalid id
 */
Page& View::GetPage(const Id &page_id)
{
	auto it = pimpl->struc.pages.find(page_id);
	if (it == pimpl->struc.pages.end())
		throw crn::ExceptionNotFound{"View::GetPage(): "_s + _("Invalid page id: ") + page_id};
	return it->second;
}

const std::unordered_map<Id, Column>& View::GetColumns() const
{
	return pimpl->struc.columns;
}
/*!
 * \throws	crn::ExceptionNotFound	invalid id
 */
const Column& View::GetColumn(const Id &col_id) const
{
	auto it = pimpl->struc.columns.find(col_id);
	if (it == pimpl->struc.columns.end())
		throw crn::ExceptionNotFound{"View::GetColumn(): "_s + _("Invalid column id: ") + col_id};
	return it->second;
}

/*!
 * \throws	crn::ExceptionNotFound	invalid id
 */
Column& View::GetColumn(const Id &col_id)
{
	auto it = pimpl->struc.columns.find(col_id);
	if (it == pimpl->struc.columns.end())
		throw crn::ExceptionNotFound{"View::GetColumn(): "_s + _("Invalid column id: ") + col_id};
	return it->second;
}

/*!
 * \throws	crn::ExceptionNotFound	invalid id
 * \param[in]	col_id	the id of a column
 * \return all graphical lines of the column
 */
const std::vector<GraphicalLine>& View::GetGraphicalLines(const Id &col_id) const
{
	auto it = pimpl->medlines.find(col_id);
	if (it == pimpl->medlines.end())
		throw crn::ExceptionNotFound{"View::GetGraphicalLines(): "_s + _("Invalid column id: ") + col_id};
	return it->second;
}

/*! Adds a median line to a column
 * \param[in]	pts	the median line
 * \param[in]	col_id	the id of a column
 */
void View::AddGraphicalLine(const std::vector<crn::Point2DInt> &pts, const Id &col_id)
{
	auto &col = pimpl->medlines[col_id];
	// estimate line height
	auto lh = std::vector<size_t>{};
	for (const auto &l : col)
		lh.push_back(l.GetLineHeight());
	auto h = size_t(0);
	if (!lh.empty())
	{
		std::sort(lh.begin(), lh.end());
		h = lh[lh.size() / 2];
	}
	else
	{ // compute
		h = crn::EstimateLeading(*GetBlock().GetGray());
	}
	// add line
	col.emplace_back(std::make_shared<crn::LinearInterpolation>(pts.begin(), pts.end()), h);
	std::sort(col.begin(), col.end(),
			[](const GraphicalLine &l1, const GraphicalLine &l2)
			{
				return l1.GetFront().Y < l2.GetFront().Y;
			});
}

/*! Removes a median line from a column
 * \throws	crn::ExceptionDomain	index > number of lines
 * \param[in]	col_id	the id of a column
 * \param[in]	index	the number of the line
 */
void View::RemoveGraphicalLine(const Id &col_id, size_t index)
{
	auto &col = pimpl->medlines[col_id];
	if (index >= col.size())
		throw crn::ExceptionDomain("View::RemoveGraphicalLine(): "_s + _("index is greater than the number of lines."));
	col.erase(col.begin() + index);
}

/*! Removes median lines from a column
 * \throws	crn::ExceptionDomain	index > number of lines
 * \param[in]	col_id	the id of a column
 */
void View::RemoveGraphicalLines(const Id &col_id)
{
	auto &col = pimpl->medlines[col_id];
	col.clear();
}

/*! Removes all aligned coordinates in a column
 * \param[in]	col_id	the id of the column
 */
void View::ClearAlignment(const Id &col_id)
{
	auto &col = GetColumn(col_id);
	GetZone(col.GetZone()).Clear();
	for (const auto &lid : col.GetLines())
	{
		auto &line = GetLine(lid);
		GetZone(line.GetZone()).Clear();
		for (const auto &wid : line.GetWords())
		{
			auto &word = GetWord(wid);
			GetZone(word.GetZone()).Clear();
			for (const auto &cid : word.GetCharacters())
				GetZone(GetCharacter(cid).GetZone()).Clear();
		}
	}
}

const std::unordered_map<Id, Line>& View::GetLines() const
{
	return pimpl->struc.lines;
}
/*!
 * \throws	crn::ExceptionNotFound	invalid id
 */
const Line& View::GetLine(const Id &line_id) const
{
	auto it = pimpl->struc.lines.find(line_id);
	if (it == pimpl->struc.lines.end())
		throw crn::ExceptionNotFound{"View::GetLine(): "_s + _("Invalid line id: ") + line_id};
	return it->second;
}

/*!
 * \throws	crn::ExceptionNotFound	invalid id
 */
Line& View::GetLine(const Id &line_id)
{
	auto it = pimpl->struc.lines.find(line_id);
	if (it == pimpl->struc.lines.end())
		throw crn::ExceptionNotFound{"View::GetLine(): "_s + _("Invalid line id: ") + line_id};
	return it->second;
}

/*!
 * \throws	crn::ExceptionNotFound	invalid id
 * \throws	crn::ExceptionDomain	no graphical line associated to the text line
 * \param[in]	line_id	the id of a line
 * \return the medline of the line
 */
const GraphicalLine& View::GetGraphicalLine(const Id &line_id) const
{
	auto it = pimpl->line_links.find(line_id);
	if (it == pimpl->line_links.end())
		throw crn::ExceptionNotFound{"View::GetGraphicalLine(): "_s + _("Invalid line id: ") + line_id};

	if (it->second.second >= pimpl->medlines[it->second.first].size())
		throw crn::ExceptionDomain{"View::GetGraphicalLine(): "_s + _("No graphical line associated to text line: ") + line_id};

	return pimpl->medlines[it->second.first][it->second.second];
}

/*!
 * \throws	crn::ExceptionNotFound	invalid id
 * \throws	crn::ExceptionDomain	no graphical line associated to the text line
 * \param[in]	line_id	the id of a line
 * \return the medline of the line
 */
GraphicalLine& View::GetGraphicalLine(const Id &line_id)
{
	auto it = pimpl->line_links.find(line_id);
	if (it == pimpl->line_links.end())
		throw crn::ExceptionNotFound{"View::GetGraphicalLine(): "_s + _("Invalid line id: ") + line_id};

	if (it->second.second >= pimpl->medlines[it->second.first].size())
		throw crn::ExceptionDomain{"View::GetGraphicalLine(): "_s + _("No graphical line associated to text line: ") + line_id};

	return pimpl->medlines[it->second.first][it->second.second];
}

/*!
 * \throws	crn::ExceptionNotFound	invalid id
 * \param[in]	line_id	the id of a line
 * \return	the median line's index
 */
size_t View::GetGraphicalLineIndex(const Id &line_id) const
{
	auto it = pimpl->line_links.find(line_id);
	if (it == pimpl->line_links.end())
		throw crn::ExceptionNotFound{"View::GetGraphicalLineIndex(): "_s + _("Invalid line id: ") + line_id};
	return it->second.second;
}

const std::unordered_map<Id, Word>& View::GetWords() const
{
	return pimpl->struc.words;
}
/*!
 * \throws	crn::ExceptionNotFound	invalid id
 */
const Word& View::GetWord(const Id &word_id) const
{
	auto it = pimpl->struc.words.find(word_id);
	if (it == pimpl->struc.words.end())
		throw crn::ExceptionNotFound{"View::GetWord(): "_s + _("Invalid word id: ") + word_id};
	return it->second;
}

/*!
 * \throws	crn::ExceptionNotFound	invalid id
 */
Word& View::GetWord(const Id &word_id)
{
	auto it = pimpl->struc.words.find(word_id);
	if (it == pimpl->struc.words.end())
		throw crn::ExceptionNotFound{"View::GetWord(): "_s + _("Invalid word id: ") + word_id};
	return it->second;
}

/*!
 * \throws	crn::ExceptionNotFound	invalid id
 * \param[in]	word_id	the id of a word
 * \returns the validation state of the word
 */
const crn::Prop3& View::IsValid(const Id &word_id) const
{
	const auto it = pimpl->validation.find(word_id);
	if (it == pimpl->validation.end())
		throw crn::ExceptionNotFound{"View::IsValid(): "_s + _("Invalid word id: ") + word_id};
	return it->second.ok;
}

/*!
 * \throws	crn::ExceptionNotFound	invalid id
 * \param[in]	word_id	the id of a word
 * \param[in]	val	the validation state of the word
 */
void View::SetValid(const Id &word_id, const crn::Prop3 &val)
{
	auto it = pimpl->validation.find(word_id);
	if (it == pimpl->validation.end())
		throw crn::ExceptionNotFound{"View::SetValid(): "_s + _("Invalid word id: ") + word_id};
	it->second.ok = val;
}

/*!
 * \throws	crn::ExceptionNotFound	invalid id
 * \param[in]	word_id	the id of the word
 * \return the alignable characters in word
 */
crn::String View::GetAlignableText(const Id &word_id) const
{
	const auto &word = GetWord(word_id);
	auto str = crn::String{};
	for (const auto &cid : word.GetCharacters())
	{
		str += GetCharacter(cid).GetText();
	}
	return str;
}

/*! Sets a word's image signature after alignment
 * \throws	crn::ExceptionNotFound	invalid id
 */
void View::SetWordImageSignature(const Id &word_id, const crn::StringUTF8 &s)
{
	const auto it = pimpl->validation.find(word_id);
	if (it == pimpl->validation.end())
		throw crn::ExceptionNotFound{"View::SetWordImageSignature(): "_s + _("Invalid word id: ") + word_id};
	it->second.imgsig = s;
}

/*! Gets a word's image signature after alignment
 * \throws	crn::ExceptionNotFound	invalid id
 */
const crn::StringUTF8& View::GetWordImageSignature(const Id &word_id) const
{
	const auto it = pimpl->validation.find(word_id);
	if (it == pimpl->validation.end())
		throw crn::ExceptionNotFound{"View::GetWordImageSignature(): "_s + _("Invalid word id: ") + word_id};
	return it->second.imgsig;
}

/*! Clears alignment for characters in a word
 * \throws	crn::ExceptionNotFound	invalid id
 */
void View::ClearCharactersAlignment(const Id &word_id)
{
	const auto &word = GetWord(word_id);
	for (const auto &cid : word.GetCharacters())
		GetZone(GetCharacter(cid).GetZone()).Clear();
}

/*!
 * \param[in]	char_id	the id of the character
 * \return	the list of glyph Ids associated to a character
 */
const std::vector<Id>& View::GetClusters(const Id &char_id) const
{
	static const auto nullvector = std::vector<Id>{};

	auto it = pimpl->onto_links.find(char_id);
	if (it == pimpl->onto_links.end())
		return nullvector;
	return it->second;
}

/*!
 * \param[in]	word_id	the id of the character
 * \return	the list of glyph Ids associated to a character
 */
std::vector<Id>& View::GetClusters(const Id &char_id)
{
	return pimpl->onto_links[char_id];
}

const std::unordered_map<Id, Character>& View::GetCharacters() const
{
return pimpl->struc.characters;
}

/*!
* \throws	crn::ExceptionNotFound	invalid id
*/
const Character& View::GetCharacter(const Id &char_id) const
{
auto it = pimpl->struc.characters.find(char_id);
if (it == pimpl->struc.characters.end())
	throw crn::ExceptionNotFound{"View::GetCharacter(): "_s + _("Invalid character id: ") + char_id};
return it->second;
}

/*!
* \throws	crn::ExceptionNotFound	invalid id
*/
Character& View::GetCharacter(const Id &char_id)
{
auto it = pimpl->struc.characters.find(char_id);
if (it == pimpl->struc.characters.end())
	throw crn::ExceptionNotFound{"View::GetCharacter(): "_s + _("Invalid character id: ") + char_id};
return it->second;
}

/*!
* \throws	crn::ExceptionNotFound	invalid id
*/
const Zone& View::GetZone(const Id &zone_id) const
{
auto it = pimpl->zones.find(zone_id);
if (it == pimpl->zones.end())
	throw crn::ExceptionNotFound{"View::GetZone(): "_s + _("Invalid zone id: ") + zone_id};
return it->second;
}

/*!
* \throws	crn::ExceptionNotFound	invalid id
*/
Zone& View::GetZone(const Id &zone_id)
{
auto it = pimpl->zones.find(zone_id);
if (it == pimpl->zones.end())
	throw crn::ExceptionNotFound{"View::GetZone(): "_s + _("Invalid zone id: ") + zone_id};
return it->second;
}

/*! Sets the bounding box of an element
* \throws	crn::ExceptionNotFound	invalid id
* \param[in]	id	the id of the element
* \param[in]	r	the bounding box
* \param[in]	compute_contour	shall the contour be automatically set? (computes curves at left and right ends)
*/
void View::SetPosition(const Id &id, const crn::Rect &r, bool compute_contour)
{
	auto zid = id;
	auto wit = pimpl->struc.words.find(id);
	if (wit != pimpl->struc.words.end())
	{
		zid = wit->second.GetZone();
	}
	else
	{
		auto cit = pimpl->struc.characters.find(id);
		if (cit != pimpl->struc.characters.end())
		{
			zid = cit->second.GetZone();
		}
	}
	auto zit = pimpl->zones.find(zid);
	if (zit != pimpl->zones.end())
	{
		zit->second.SetPosition(r);
		if (compute_contour)
			ComputeContour(zid);
	}
	else
		throw crn::ExceptionNotFound("View::SetPosition(): "_s + _("Invalid zone id: ") + id);
}

/*! Sets the contour of an element
 * \throws	crn::ExceptionNotFound	invalid id
 * \param[in]	id	the id of the element
 * \param[in]	c	the contour
 * \param[in]	set_position	shall the bounding box be automatically set?
 */
void View::SetContour(const Id &id, const std::vector<crn::Point2DInt> &c, bool set_position)
{
	auto zid = id;
	auto wit = pimpl->struc.words.find(id);
	if (wit != pimpl->struc.words.end())
	{
		zid = wit->second.GetZone();
	}
	else
	{
		auto cit = pimpl->struc.characters.find(id);
		if (cit != pimpl->struc.characters.end())
		{
			zid = cit->second.GetZone();
		}
	}
	auto zit = pimpl->zones.find(zid);
	if (zit != pimpl->zones.end())
	{
		zit->second.SetContour(c);
		if (set_position)
		{
			auto r = crn::Rect{c.front()};
			for (const auto p : c)
				r |= p;
			zit->second.SetPosition(r);
		}
	}
	else
		throw crn::ExceptionNotFound("View::SetContour(): "_s + _("Invalid zone id: ") + id);
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

namespace crn
{
	bool operator<(const crn::Point2DInt &p1, const crn::Point2DInt &p2)
	{
		if (p1.X < p2.X)
			return true;
		else if (p1.X == p2.X)
			return p1.Y < p2.Y;
		else
			return false;
	}
}
std::vector<crn::Point2DInt> View::ComputeFrontier(size_t x, size_t y1, size_t y2) const
{
	try
	{
		return SimplifyCurve(crn::AStar(
					crn::Point2DInt(int(x), int(y1)),
					crn::Point2DInt(int(x), int(y2)),
					stepcost(getWeight(), int(x)),
					heuristic(),
					get_neighbors(GetBlock().GetAbsoluteBBox())), 1);
	}
	catch (crn::ExceptionNotFound&)
	{
		return std::vector<crn::Point2DInt>{crn::Point2DInt{int(x), int(y1)}, crn::Point2DInt{int(x), int(y2)}};
	}
}

/*! Computes the contour of a zone from its bounding box
 * \throws	crn::ExceptionNotFound	invalid id
 * \param[in]	zone_id	the id of a zone
 */
void View::ComputeContour(const Id &zone_id)
{
	auto zit = pimpl->zones.find(zone_id);
	if (zit == pimpl->zones.end())
		throw crn::ExceptionNotFound("View::CumputeContour(): "_s + _("Invalid zone id: ") + zone_id);
	const auto &r = zit->second.GetPosition();
	auto contour = ComputeFrontier(r.GetLeft(), r.GetTop(), r.GetBottom());
	auto contour2 = ComputeFrontier(r.GetRight(), r.GetTop(), r.GetBottom());
	std::move(contour2.rbegin(), contour2.rend(), std::back_inserter(contour));
	zit->second.SetContour(contour);
}

/*! Gets the image of a zone. If the zone has a contour, the exterior will be filled with white pixels or null gradients.
 * \throws	crn::ExceptionNotFound	invalid id
 * \throws	crn::ExceptionUninitialized	uninitialized zone
 * \param[in]	zone_id	the id of the zone
 * \return	a block containing a RGB and gradient buffers
 */
crn::SBlock View::GetZoneImage(const Id &zone_id) const
{
	auto zit = pimpl->zones.find(zone_id);
	if (zit == pimpl->zones.end())
		throw crn::ExceptionNotFound("View::GetZoneImage(): "_s + _("Invalid zone id: ") + zone_id);
	const auto &pos = zit->second.GetPosition();
	if (!pos.IsValid())
		throw crn::ExceptionUninitialized("View::GetZoneImage(): "_s + _("Uninitialized zone id: ") + zone_id);
	auto &b = GetBlock();
	b.GetGradient(); // precompute
	try
	{
		return b.GetChild(U"zones", zone_id);
	}
	catch (crn::ExceptionNotFound&)
	{
		auto contour = zit->second.GetContour();
		auto frontier = size_t(0);
		if (contour.empty())
		{
			contour.emplace_back(pos.GetLeft(), pos.GetTop());
			contour.emplace_back(pos.GetLeft(), pos.GetBottom());
			frontier = 2;
			contour.emplace_back(pos.GetRight(), pos.GetBottom());
			contour.emplace_back(pos.GetRight(), pos.GetTop());
		}
		else
		{
			for (; frontier < contour.size() - 1; ++frontier)
				if (contour[frontier + 1].Y < contour[frontier].Y)
					break;
		}

		auto min_x = std::numeric_limits<int>::max();
		auto max_x = 0;
		auto min_y = std::numeric_limits<int>::max();
		auto max_y = 0;

		for (auto i : crn::Range(contour))
		{
			if (contour[i].X < min_x)
				min_x = contour[i].X;
			if (contour[i].X > max_x)
				max_x = contour[i].X;
			if (contour[i].Y < min_y)
				min_y = contour[i].Y;
			if (contour[i].Y > max_y)
				max_y = contour[i].Y;
		}

		auto img = b.AddChildAbsolute(U"zones", {min_x, min_y, max_x, max_y}, zone_id);

		for (auto j = size_t(0); j < img->GetRGB()->GetHeight(); ++j)
		{ // left frontier
			auto x = 0;
			for (auto i = size_t(0); i < frontier - 1; ++i)
			{
				const auto x1 = contour[i].X - min_x;
				const auto y1 = contour[i].Y - min_y;
				const auto x2 = contour[i + 1].X - min_x;
				const auto y2 = contour[i + 1].Y - min_y;

				if (j == y2)
					x = x2;
				if (j == y1)
					x = x1;
				if ((j < y2) && (j > y1))
					x = int(x1 + (x2 - x1) * ((double(j) - y1)/(y2 - y1)));

			}
			for (auto k = 0; k <= x; ++k)
			{
				img->GetRGB()->At(k, j) = {255, 255, 255};
				img->GetGradient()->At(k, j).rho = 0;
			}
		}
		for (auto j = size_t(0); j < img->GetRGB()->GetHeight(); ++j)
		{ // right frontier
			auto x = 0;
			for (auto i = frontier; i < contour.size() - 1; ++i)
			{
				const auto x2 = contour[i].X - min_x;
				const auto y2 = contour[i].Y - min_y;
				const auto x1 = contour[i + 1].X - min_x;
				const auto y1 = contour[i + 1].Y - min_y;
				if (j == y2)
					x = x2;
				if (j == y1)
					x = x1;
				if ((j < y2) && (j > y1))
					x = int(x1 + (x2 - x1) * ((double(j) - y1)/(y2 - y1)));
			}
			for (auto k = x; k < img->GetRGB()->GetWidth(); ++k)
			{
				img->GetRGB()->At(k, j) = {255, 255, 255};
				img->GetGradient()->At(k, j).rho = 0;
			}
		}
		return img;
	}
}

/*! Sets the left frontier of a word
 * \throws	crn::ExceptionNotFound	invalid id
 * \throws	crn::ExceptionUninitialized	the word's zone is currently unset
 * \param[in]	id	the id of the word
 * \param[in]	x	position of the new left frontier
 */
void View::UpdateLeftFrontier(const Id &id, int x)
{
	auto wit = pimpl->struc.words.find(id);
	if (wit == pimpl->struc.words.end())
		throw crn::ExceptionNotFound("View::UpdateLeftFrontier(): "_s + _("Invalid word id: ") + id);
	const auto zid = wit->second.GetZone();
	auto &zone = pimpl->zones.find(zid)->second;
	auto r = zone.GetPosition();
	if (r.IsValid())
	{
		if (r.GetLeft() == x)
			return;
		auto &val = pimpl->validation.find(id)->second;
		val.left_corr += r.GetLeft() - x;
		r.SetLeft(x);
		zone.SetPosition(r);
		if (val.right_corr)
			val.ok = true;
	}
	else
		throw crn::ExceptionUninitialized("View::UpdateLeftFrontier(): "_s + _("uninitialized zone: ") + zid);
}

/*! Sets the right frontier of a word
 * \throws	crn::ExceptionNotFound	invalid id
 * \throws	crn::ExceptionUninitialized	the word's zone is currently unset
 * \param[in]	id	the id of the word
 * \param[in]	x	position of the new right frontier
 */
void View::UpdateRightFrontier(const Id &id, int x)
{
	auto wit = pimpl->struc.words.find(id);
	if (wit == pimpl->struc.words.end())
		throw crn::ExceptionNotFound("View::UpdateRightFrontier(): "_s + _("Invalid word id: ") + id);
	const auto zid = wit->second.GetZone();
	auto &zone = pimpl->zones.find(zid)->second;
	auto r = zone.GetPosition();
	if (r.IsValid())
	{
		if (r.GetRight() == x)
			return;
		auto &val = pimpl->validation.find(id)->second;
		val.right_corr += r.GetRight() - x;
		r.SetRight(x);
		zone.SetPosition(r);
		if (val.left_corr)
			val.ok = true;
	}
	else
		throw crn::ExceptionUninitialized("View::UpdateRightFrontier(): "_s + _("uninitialized zone: ") + zid);
}

/*!
 * \throws	crn::ExceptionNotFound	invalid id
 * \param[in]	id	the id of the word
 * \return	the total left correction of the word
 */
int View::GetLeftCorrection(const Id &id) const
{
	auto wit = pimpl->struc.words.find(id);
	if (wit == pimpl->struc.words.end())
		throw crn::ExceptionNotFound("View::GetLeftCorrection(): "_s + _("Invalid word id: ") + id);
	return pimpl->validation.find(id)->second.left_corr;
}

/*!
 * \throws	crn::ExceptionNotFound	invalid id
 * \param[in]	id	the id of the word
 * \return	the total right correction of the word
 */
int View::GetRightCorrection(const Id &id) const
{
	auto wit = pimpl->struc.words.find(id);
	if (wit == pimpl->struc.words.end())
		throw crn::ExceptionNotFound("View::GetRightCorrection(): "_s + _("Invalid word id: ") + id);
	return pimpl->validation.find(id)->second.right_corr;
}

/*! Resets the left and right corrections of a word
 * \throws	crn::ExceptionNotFound	invalid id
 * \param[in]	id	the id of the word
 */
void View::ResetCorrections(const Id &id)
{
	if (pimpl->struc.words.find(id) == pimpl->struc.words.end())
		throw crn::ExceptionNotFound("View::ResetCorrections(): "_s + _("Invalid word id: ") + id);

	auto &val = pimpl->validation.find(id)->second;
	val.left_corr = val.right_corr = 0;
}

/*! Computes alignment on the view
 * \param[in]	conf	alignment options
 * \param[in]	viewprog	progress bar on pages
 * \param[in]	pageprog	progress bar on columns
 * \param[in]	colprog	progress bar on lines
 * \param[in]	linprog	progress bar on words
 */
void View::AlignAll(AlignConfig conf, crn::Progress *viewprog, crn::Progress *pageprog, crn::Progress *colprog, crn::Progress *linprog)
{
	if (viewprog)
		viewprog->SetMaxCount(int(GetPages().size()));
	for (const auto pid : GetPages())
	{
		AlignPage(conf, pid, pageprog, colprog, linprog);
		if (viewprog)
			viewprog->Advance();
	}
}

/*! Computes alignment on a page
 * \param[in]	conf	alignment options
 * \param[in]	page_id	the id of the page
 * \param[in]	pageprog	progress bar on columns
 * \param[in]	colprog	progress bar on lines
 * \param[in]	linprog	progress bar on words
 */
void View::AlignPage(AlignConfig conf, const Id &page_id, crn::Progress *pageprog, crn::Progress *colprog, crn::Progress *linprog)
{
	auto &page = GetPage(page_id);
	if (pageprog)
		pageprog->SetMaxCount(int(page.GetColumns().size()));
	for (const auto cid : page.GetColumns())
	{
		AlignColumn(conf, cid, colprog, linprog);
		if (pageprog)
			pageprog->Advance();
	}
}

/*! Computes alignment on a column
 * \param[in]	conf	alignment options
 * \param[in]	col_id	the id of the column
 * \param[in]	colprog	progress bar on lines
 * \param[in]	linprog	progress bar on words
 */
void View::AlignColumn(AlignConfig conf, const Id &col_id, crn::Progress *colprog, crn::Progress *linprog)
{
	auto &col = GetColumn(col_id);
	if (colprog)
		colprog->SetMaxCount(int(col.GetLines().size()));
	for (const auto &lid : col.GetLines())
	{
		AlignLine(conf, lid, linprog);
		if (colprog)
			colprog->Advance();
	}
}

/*! Computes alignment on a line
 * \param[in]	conf	alignment options
 * \param[in]	line_id	the id of the line
 * \param[in]	prog	progress bar on words
 */
void View::AlignLine(AlignConfig conf, const Id &line_id, crn::Progress *prog)
{
	auto &line = GetLine(line_id);
	if (line.GetWords().empty()) // Is it even possible?
		return;

	// Align words
	if (!!(conf & AlignConfig::AllWords))
	{
		AlignRange(conf, line_id, 0, line.GetWords().size() - 1);
	}
	else if (!!(conf & AlignConfig::NOKWords))
	{
		// gather ranges of words to align
		auto wranges = std::vector<std::vector<size_t>>{};
		auto in = false;
		for (auto w : crn::Range(line.GetWords()))
		{
			if (!GetZone(GetWord(line.GetWords()[w]).GetZone()).GetPosition().IsValid() ||
					!IsValid(line.GetWords()[w]).IsTrue())
			{
				if (!in)
				{
					in = true;
					wranges.push_back(std::vector<size_t>{});
				}
				wranges.back().push_back(w);
			}
			else
			{
				in = false;
			}
		}
		// align
		if (prog)
			prog->SetMaxCount(int(wranges.size()));
		for (const auto &r : wranges)
		{
			const auto &firstwid = line.GetWords()[r.front()];
			if ((r.size() == 1) && GetZone(GetWord(firstwid).GetZone()).GetPosition().IsValid())
			{ // a single word between validated words
				SetValid(firstwid, crn::Prop3::True);
			}
			else
			{
				AlignRange(conf, line_id, r.front(), r.back());
			}
			if (prog)
				prog->Advance();
		}
	}
	else if (!!(conf & AlignConfig::NAlWords))
	{
		// gather ranges of words to align
		auto wranges = std::vector<std::vector<size_t>>{};
		auto in = false;
		for (auto w : crn::Range(line.GetWords()))
		{
			if (!GetZone(GetWord(line.GetWords()[w]).GetZone()).GetPosition().IsValid())
			{
				if (!in)
				{
					in = true;
					wranges.push_back(std::vector<size_t>{});
				}
				wranges.back().push_back(w);
			}
			else
			{
				in = false;
			}
		}
		// align
		if (prog)
			prog->SetMaxCount(int(wranges.size()));
		for (const auto &r : wranges)
		{
			AlignRange(conf, line_id, r.front(), r.back());
			if (prog)
				prog->Advance();
		}
	}
	// Only update frontiers
	if (!!(conf & AlignConfig::WordFrontiers))
	{
		for (const auto &wid : line.GetWords())
		{
			const auto &wzid = GetWord(wid).GetZone();
			if (GetZone(wzid).GetPosition().IsValid())
				ComputeContour(wzid);
		}
	}
	// Align characters
	for (const auto &wid : line.GetWords())
	{
		if (!GetZone(GetWord(wid).GetZone()).GetPosition().IsValid())
			continue;

		const auto &val = IsValid(wid);
		if (!!(conf & AlignConfig::CharsAllWords) ||
				(!!(conf & AlignConfig::CharsOKWords) && val.IsTrue()) ||
				(!!(conf & AlignConfig::CharsNKOWords) && !val.IsFalse()))
			AlignWordCharacters(conf, line_id, wid);
	}
}

/*! Computes alignment on a range of words
 * \throws	crn::ExceptionDomain	the last word is before the first of range error
 * \throws	crn::ExceptionUninitialized	the word before or after the range is not aligned
 * \param[in]	conf	alignment options
 * \param[in]	line_id	the id of the line
 * \param[in]	first_word	index of the first word to align
 * \param[in]	lastçword 	index of the last word to align (included!)
 */
void View::AlignRange(AlignConfig conf, const Id &line_id, size_t first_word, size_t last_word)
{
	if (last_word < first_word)
		throw crn::ExceptionDomain("View::AlignRange(): "_s + _("the last word is located before the first."));
	auto &line = GetLine(line_id);
	if (first_word >= line.GetWords().size() || last_word >= line.GetWords().size())
		throw crn::ExceptionDomain("View::AlignRange(): "_s + _("out of range."));

	// check if the text line is associated to an image line
	try { GetGraphicalLine(line_id); }
	catch (crn::ExceptionDomain&) { return; }

	auto &bl = GetGraphicalLine(line_id);
	// range on image
	auto bx = size_t(0);
	if (first_word == 0)
	{
		// start of the line
		bx = bl.GetFront().X;
	}
	else
	{
		bx = GetZone(GetWord(line.GetWords()[first_word - 1]).GetZone()).GetPosition().GetRight() + 1; // may throw
	}
	auto ex = size_t(0);
	if (last_word == line.GetWords().size() - 1)
	{
		// end of the line
		ex = bl.GetBack().X;
	}
	else
	{
		ex = GetZone(GetWord(line.GetWords()[last_word + 1]).GetZone()).GetPosition().GetLeft() - 1; // may throw
	}
	// extract image signature
	const auto &isig = bl.ExtractFeatures(GetBlock());
	auto risig = std::vector<ImageSignature>{};
	for (const auto &is : isig)
	{
		if (is.bbox.GetRight() > ex)
			break;
		if (is.bbox.GetLeft() >= bx)
			risig.push_back(is);
	}

	// text signature
	auto lsig = std::vector<TextSignature>{};
	for (auto w = first_word; w <= last_word; ++w)
	{
		auto wsig = TextSignatureDB::Sign(GetAlignableText(line.GetWords()[w]));
		for (auto tmp = size_t(1); tmp < wsig.size(); ++tmp)
			wsig[tmp].start = false;
		std::copy(wsig.begin(), wsig.end(), std::back_inserter(lsig));
	}

	// perform alignment
	const auto align = Align(risig, lsig);
	auto bbn = size_t(0);
	auto bbox = crn::Rect{};
	for (auto w = first_word; w <= last_word; ++w)
	{
		if (bbn >= align.size())
		{
			CRNError(_("Range align misfit at line ") + line_id);
			break;
		}
		const auto &wid = line.GetWords()[w];
		auto &word = GetWord(wid);
		if (word.GetText().IsEmpty())
			continue; // XXX Is this necessary???

		auto &wzone = GetZone(word.GetZone());
		SetWordImageSignature(wid, align[bbn].second);
		if (wzone.GetPosition() != align[bbn].first)
		{ // BBox changed
			ClearCharactersAlignment(wid);
			SetValid(wid, crn::Prop3::Unknown);
			if (align[bbn].first.IsValid())
				wzone.SetPosition(align[bbn].first);
		}
		bbox |= align[bbn].first;
		ResetCorrections(wid); // reset left/right corrections
		ComputeContour(word.GetZone());

		bbn += 1;
	} // for each word

	// recompute line's bbox
	auto &lzone = GetZone(line.GetZone());
	bbox |= lzone.GetPosition();
	if (bbox.IsValid())
		lzone.SetPosition(bbox);
}

/*! Aligns the characters in a word
 * \throws	crn::ExceptionNotFound	a character in the word was not found
 * \param[in]	conf	alignment options
 * \param[in]	line_id	the id of the line
 * \param[in]	word_id	the id of the word
 */
void View::AlignWordCharacters(AlignConfig conf, const Id &line_id, const Id &word_id)
{
	// check if the text line is associated to an image line
	try { GetGraphicalLine(line_id); }
	catch (crn::ExceptionDomain&) { return; }

	auto &word = GetWord(word_id);

	if (!!(conf & AlignConfig::NAlChars))
	{
		const auto &chars = word.GetCharacters();
		if (chars.empty())
			return;
		const auto &cha = GetCharacter(chars.front()); // may throw
		if (GetZone(cha.GetZone()).GetPosition().IsValid())
			return; // do not realign
	}

	const auto isig = GetGraphicalLine(line_id).ExtractFeatures(GetBlock());
	auto wsig = std::vector<TextSignature>{};
	for (const auto cid : word.GetCharacters())
	{
		const auto ctxt = GetCharacter(cid).GetText();
		auto csig = TextSignatureDB::Sign(ctxt);
		if (csig.empty())
		{
			CRNWarning(cid + U" (" + ctxt + U") at line " + line_id + U": " + _("empty signature."));
			csig.emplace_back(true, 'l');
		}
		std::move(csig.begin(), csig.end(), std::back_inserter(wsig));
	}

	auto wisig = std::vector<ImageSignature>{};
	const auto wordbox = GetZone(word.GetZone()).GetPosition();
	for (const auto &is : isig)
	{
		const auto inter = is.bbox & wordbox;
		if (inter.IsValid())
			wisig.emplace_back(inter, is.code, is.cutproba);
	}
	// align
	const auto align = Align(wisig, wsig);
	if (align.empty())
		return; // XXX

	auto abox = size_t(0);
	for (const auto &cid : word.GetCharacters())
	{
		const auto &czid = GetCharacter(cid).GetZone();
		auto &czone = GetZone(czid);
		czone.SetPosition(align[abox++].first);
		ComputeContour(czid);
		if (abox >= align.size())
		{
			// XXX
			break;
		}
	}
}

/*! Checks if an element is associated to a non-empty zone */
bool View::IsAligned(const Id &id) const
{
	auto zid = id;

	// is it a page?
	auto pit = pimpl->struc.pages.find(id);
	if (pit != pimpl->struc.pages.end())
	{
		zid = pit->second.GetZone();
	}
	else
	{ // is it a column?
		auto cit = pimpl->struc.columns.find(id);
		if (cit != pimpl->struc.columns.end())
		{
			zid = cit->second.GetZone();
		}
		else
		{ // is it a line?
			auto lit = pimpl->struc.lines.find(id);
			if (lit != pimpl->struc.lines.end())
			{
				zid = lit->second.GetZone();
			}
			else
			{ // is it a word?
				auto wit = pimpl->struc.words.find(id);
				if (wit != pimpl->struc.words.end())
				{
					zid = wit->second.GetZone();
				}
				else
				{ // is it a character?
					auto chit = pimpl->struc.characters.find(id);
					if (chit != pimpl->struc.characters.end())
					{
						zid = chit->second.GetZone();
					}
				}
			}
		}
	}
	// have we found a zone?
	auto zit = pimpl->zones.find(zid);
	if (zit != pimpl->zones.end())
		return zit->second.GetPosition().IsValid();
	else
		return false;
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
			auto diff = crn::Differential::NewGaussian(*GetBlock().GetRGB(), crn::Differential::RGBProjection::ABSMAX, 0);
			diff.Diffuse(5);
			pimpl->weight = std::make_shared<crn::ImageGray>(crn::Downgrade<crn::ImageGray>(diff.MakeLaplacian()));
			for (auto tmp : Range(*pimpl->weight))
				pimpl->weight->At(tmp) = uint8_t(pimpl->weight->At(tmp) / 2 + 127 - GetBlock().GetGray()->At(tmp) / 2);
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
// Glyph
//////////////////////////////////////////////////////////////////////////////////
Glyph::Glyph(crn::xml::Element &elem):
	el(elem)
{
}

crn::StringUTF8 Glyph::GetDescription() const
{
	return el.GetFirstChildElement("desc").GetFirstChildText();
}

/*! \return	the id of the parent glyph (with prefix) or empty string */
Id Glyph::GetParent() const
{
	auto note = el.GetFirstChildElement("note");
	if (!note)
		return "";
	const auto str = note.GetFirstChildText();
	if (!str.StartsWith("parent="))
		return "";
	return str.SubString(7);
}

/*! \para[in]	parent_id	the id of the parent glyph (with prefix) or empty string */
void Glyph::SetParent(const Id &parent_id)
{
	auto note = el.GetFirstChildElement("note");
	if (!note)
	{
		if (parent_id.IsEmpty())
			return;
		note = el.PushBackElement("note");
	}
	note.Clear();
	if (parent_id.IsNotEmpty())
		note.PushBackText("parent=" + parent_id);
}

bool Glyph::IsAuto() const
{
	return el.GetAttribute<crn::StringUTF8>("change", true) == "#auto";
}

Id Glyph::LocalId(const Id &id)
{
	return LOCALGLYPH + id;
}

Id Glyph::GlobalId(const Id &id)
{
	return GLOBALGLYPH + id;
}

Id Glyph::BaseId(const Id &id)
{
	if (IsLocal(id))
		return id.SubString(LOCALGLYPH.Size());
	else if (IsGlobal(id))
		return id.SubString(GLOBALGLYPH.Size());
	else
		return id;
}

bool Glyph::IsLocal(const Id &id)
{
	return id.StartsWith(LOCALGLYPH);
}

bool Glyph::IsGlobal(const Id &id)
{
	return id.StartsWith(GLOBALGLYPH);
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

	// TODO multiple choices
	TextSignatureDB::Load(Config::GetInstance().GetStaticDataDir() / "orisig.xml");

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
		throw crn::ExceptionInvalidArgument{"Document::Document(): "_s + _("Cannot find main transcription filename.")};

	// read structure up to word level
	auto milestones = std::multimap<int, Id>{};
	{
		auto doc = crn::xml::Document{base / TEXTDIR / name + "-w.xml"_p};
		auto root = doc.GetRoot();
		auto pos = ElementPosition{};
		readTextWElements(root, pos, milestones, 'l'); // may throw
	}
	if (prog)
		prog->SetMaxCount(milestones.size() + 3);
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
		report += "\n\n";
	}
	// read structure at character level
	{
		auto doc = crn::xml::Document{base / TEXTDIR / name + "-c.xml"_p};
		auto root = doc.GetRoot();
		auto pos = ElementPosition{};
		readTextCElements(root, pos);
	}
	// compute words transcription
	for (auto &v : view_struct)
	{
		for (auto &w : v.second.words)
		{
			auto trans = crn::String{};
			for (const auto &cid : w.second.GetCharacters())
				trans += v.second.characters[cid].GetText();
			w.second.text = trans;
		}
	}
	if (prog)
		prog->Advance();

	// read global ontology file
	try
	{
		global_onto = crn::xml::Document{base / ".." / "charDecl.xml"};
		auto root = global_onto.GetRoot();
		auto el = root.GetFirstChildElement("teiHeader");
		if (!el) throw 1;
		el = el.GetFirstChildElement("encodingDesc");
		if (!el) throw 1;
		el = el.GetFirstChildElement("charDecl");
		if (!el) throw 1;
		el = el.GetFirstChildElement("glyph");
		while (el)
		{
			glyphs.emplace(Glyph::GlobalId(el.GetAttribute<Id>("xml:id")), Glyph{el});
			el = el.GetNextSiblingElement("glyph");
		}
	}
	catch (...)
	{
		report += base + "/../charDecl.xml";
		report += ": ";
		report += _("file not found.");
		report += "\n\n";
	}
	// read or create local ontology file
	try
	{
		local_onto = crn::xml::Document{base / ONTODIR / name + "_ontology.xml"};
		auto root = local_onto.GetRoot();
		auto el = root.GetFirstChildElement("teiHeader");
		if (!el) throw 1;
		el = el.GetFirstChildElement("encodingDesc");
		if (!el) throw 1;
		charDecl = std::make_unique<crn::xml::Element>(el.GetFirstChildElement("charDecl"));
		if (!*charDecl) throw 1;
		el = charDecl->GetFirstChildElement("glyph");
		while (el)
		{
			glyphs.emplace(Glyph::LocalId(el.GetAttribute<Id>("xml:id")), Glyph{el});
			el = el.GetNextSiblingElement("glyph");
		}
	}
	catch (int)
	{
		throw crn::ExceptionIO(crn::StringUTF8(name) + "_ontology.xml: "_s + _("malformed file."));
	}
	catch (...)
	{
		local_onto = crn::xml::Document{};
		auto root = local_onto.PushBackElement("TEI");
		auto hel = root.PushBackElement("teiHeader");
		auto el = hel.PushBackElement("fileDesc");
		auto tmpel = el.PushBackElement("titleStmt");
		tmpel = tmpel.PushBackElement("title");
		tmpel.PushBackText(_("Allograph declaration"));
		tmpel = el.PushBackElement("publicationStmt");
		tmpel = tmpel.PushBackElement("p");
		tmpel.PushBackText(_("Oriflamms Project, http://oriflamms.hypotheses.org"));
		tmpel = el.PushBackElement("sourceDesc");
		tmpel = tmpel.PushBackElement("p");
		tmpel.PushBackText(_("Created by Oriflamms."));
		el = hel.PushBackElement("encodingDesc");
		charDecl = std::make_unique<crn::xml::Element>(el.PushBackElement("charDecl"));
		el = root.PushBackElement("text");
		el = el.PushBackElement("body");
		el = el.PushBackElement("p");
		el.PushBackText(_("Allograph declaration"));
		local_onto.Save(base / ONTODIR / name + "_ontology.xml");
	}
	if (prog)
		prog->Advance();

	// read distance matrices
	try
	{
		auto dmdoc = crn::xml::Document{base / ORIDIR / "char_dm.xml"};
		auto root = dmdoc.GetRoot();
		auto el = root.GetFirstChildElement("dm");
		while (el)
		{
			auto iel = el.GetFirstChildElement("ids");
			if (!iel)
			{
				report += "char_dm.xml";
				report += ": ";
				report += _("missing id list.");
				report += "\n\n";
				continue;
			}
			auto idlist = iel.GetFirstChildText().Split(" ");
			/*
				 auto mel = el.GetFirstChildElement("SquareMatrixDouble");
				 if (!mel)
				 {
				 report += "char_dm.xml";
				 report += ": ";
				 report += _("missing distance matrix.");
				 report += "\n";
				 continue;
				 }
				 chars_dm.emplace(el.GetAttribute<crn::StringUTF8>("charname", false), std::make_pair(std::move(idlist), crn::SquareMatrixDouble{mel}));
				 */
			const auto num = el.GetAttribute<int>("num", false);
			std::ifstream matfile;
			matfile.open((base / ORIDIR / "dm"_p + num + ".dat"_p).CStr(), std::ios::in|std::ios::binary);
			auto dm = crn::SquareMatrixDouble{idlist.size()};
			matfile.read(reinterpret_cast<char *>(const_cast<double*>(dm.Std().data())), dm.Std().size() * sizeof(double));
			chars_dm.emplace(el.GetAttribute<crn::StringUTF8>("charname", false), std::make_pair(std::move(idlist), std::move(dm)));
			el = el.GetNextSiblingElement("dm");
		}
	}
	catch (std::exception &ex)
	{
		report += "char_dm.xml";
		report += ": ";
		report += ex.what();
		report += "\n\n";
	}
	if (prog)
		prog->Advance();

	// read all files
	for (const auto &id : views)
	{
		auto v = GetView(id);
		warningreport += v.pimpl->logmsg;
		if (prog)
			prog->Advance();
	}
}

Document::~Document()
{
	Save();
}

void Document::TidyUp(crn::Progress *prog)
{
	if (prog)
		prog->SetMaxCount(views.size());
	// read all files
	for (const auto &id : views)
	{
		auto v = getCleanView(id);
		auto need_lines = false;
		// index and compute boxes if needed
		for (auto &p : v.pimpl->struc.pages)
		{ // for each page
			positions.emplace(p.first, ElementPosition{id, p.first});

			if (p.second.GetZone().IsEmpty())
			{
				// add zone to XML
				auto root = v.pimpl->zonesdoc.GetRoot();
				auto el = root.GetFirstChildElement("facsimile");
				el = el.GetFirstChildElement("surface");
				el = el.PushBackElement("zone");
				el.SetAttribute("type", "page");
				const auto &pbox = v.GetBlock().GetAbsoluteBBox();
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
			{ // columns
				positions.emplace(cid, ElementPosition{id, p.first, cid});

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
				{ // lines
					positions.emplace(lid, ElementPosition{id, p.first, cid, lid});

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
					auto medianline = std::vector<crn::Point2DInt>{};
					for (auto &wid : line.GetWords())
					{ // words
						positions.emplace(wid, ElementPosition{id, p.first, cid, lid, wid});

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
						{ // characters
							positions.emplace(cid, ElementPosition{id, p.first, cid, lid, wid});

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
						if (wpos.IsValid())
						{
							if (medianline.empty())
								medianline.emplace_back(wpos.GetLeft(), wpos.GetCenterY());
							medianline.emplace_back(wpos.GetRight(), wpos.GetCenterY());
							if (wzone.GetContour().empty())
							{ // compute contour
								auto contour = v.ComputeFrontier(wpos.GetLeft(), wpos.GetTop(), wpos.GetBottom());
								auto contour2 = v.ComputeFrontier(wpos.GetRight(), wpos.GetTop(), wpos.GetBottom());
								std::move(contour2.rbegin(), contour2.rend(), std::back_inserter(contour));
								wzone.SetContour(contour);
							}
						}
						lbox |= wpos;
					} // words

					if (!lzone.GetPosition().IsValid() && lbox.IsValid())
						lzone.SetPosition(lbox);

					if (!medianline.empty())
					{
						try
						{
							auto &gl = v.GetGraphicalLine(lid);
							if (gl.GetMidline().empty())
								gl.SetMidline(medianline);
						}
						catch (...)
						{
							v.pimpl->medlines[cid].emplace_back(std::make_shared<crn::LinearInterpolation>(medianline.begin(), medianline.end()), lbox.GetHeight());
							v.pimpl->line_links.emplace(lid, std::make_pair(cid, v.pimpl->medlines[cid].size() - 1));
						}
					}

					cbox |= lzone.GetPosition();
				} // lines

				if (!czone.GetPosition().IsValid())
				{
					if (cbox.IsValid())
					{
						czone.SetPosition(cbox);
					}
					else
					{ // need to compute median lines from scratch
						if (v.GetGraphicalLines(cid).empty())
							need_lines = true;
					}
				}
				pbox |= czone.GetPosition();
			} // columns

			if (!pzone.GetPosition().IsValid())
			{
				if (pbox.IsValid())
					pzone.SetPosition(pbox);
			}

		} // pages
		if (need_lines)
			v.detectLines();
		if (prog)
			prog->Advance();
	}
}

void Document::Save() const
{
	local_onto.Save();
	// save distance matrices
	auto dmdoc = crn::xml::Document{};
	auto root = dmdoc.PushBackElement("charsdm");
	auto cnt = 0;
	for (const auto &dm : chars_dm)
	{
		auto el = root.PushBackElement("dm");
		el.SetAttribute("charname", dm.first.CStr());
		el.SetAttribute("num", cnt);
		std::ofstream matfile;
		matfile.open((base / ORIDIR / "dm"_p + cnt + ".dat"_p).CStr(), std::ios::out|std::ios::binary);
		matfile.write(reinterpret_cast<const char *>(dm.second.second.Std().data()), dm.second.second.Std().size() * sizeof(double));
		//dm.second.second.Serialize(el);
		el = el.PushBackElement("ids");
		auto idlist = crn::StringUTF8{};
		for (const auto &id : dm.second.first)
		{
			idlist += id;
			idlist += ' ';
		}
		el.PushBackText(idlist);
		cnt += 1;
	}
	dmdoc.Save(base / ORIDIR / "char_dm.xml");
}

const ElementPosition& Document::GetPosition(const Id &elem_id) const
{
	auto it = positions.find(elem_id);
	if (it == positions.end())
		throw crn::ExceptionNotFound("Document::GetPosition(): "_s + _("invalid id: ") + elem_id);
	return it->second;
}

View Document::GetView(const Id &id)
{
	std::lock_guard<std::mutex> flock(crn::FileShield::GetMutex("views://doc/" + id));

	auto it = view_refs.find(id);
	if (it == view_refs.end())
		throw crn::ExceptionNotFound("Document::GetView(): "_s + _("Cannot find view with id ") + id);

	auto v = std::shared_ptr<View::Impl>{};
	if (it->second.expired())
	{
		v = std::make_shared<View::Impl>(id, view_struct[id], base, name, false); // may throw
		it->second = v;
	}
	else
		v = it->second.lock();
	return View(v);
}

/*! Erases image signatures in the document
 * \param[in]	prog	a progress bar
 */
void Document::ClearSignatures(crn::Progress *prog)
{
	if (prog)
		prog->SetMaxCount(int(views.size()));
	for (const auto vid : views)
	{
		auto v = GetView(vid);
		for (auto &col : v.pimpl->medlines)
			for (auto &gl : col.second)
				gl.ClearFeatures();
		if (prog)
			prog->Advance();
	}
}

/*! Computes alignment on the whole document
 * \param[in]	conf	alignment options
 * \param[in]	docprog	progress bar on views
 * \param[in]	viewprog	progress bar on pages
 * \param[in]	pageprog	progress bar on columns
 * \param[in]	colprog	progress bar on lines
 * \param[in]	linprog	progress bar on words
 */
void Document::AlignAll(AlignConfig conf, crn::Progress *docprog, crn::Progress *viewprog, crn::Progress *pageprog, crn::Progress *colprog, crn::Progress *linprog)
{
	if (docprog)
		docprog->SetMaxCount(int(views.size()));
	for (const auto &vid : views)
	{
		auto view = GetView(vid);
		view.AlignAll(conf, viewprog, pageprog, colprog, linprog);

		if (docprog)
			docprog->Advance();
	}
}

/*! Propagates the validation of word alignment
 * \param[in]	prog	a progress bar
 */
void Document::PropagateValidation(crn::Progress *prog)
{
	if (prog)
		prog->SetMaxCount(int(views.size()));
	for (const auto &vid : views)
	{
		auto v = GetView(vid);
		for (const auto &lp : v.pimpl->struc.lines)
		{
			if (lp.second.GetWords().size() < 3)
				continue;
			for (size_t w = 1; w < lp.second.GetWords().size() - 1; ++w)
			{
				auto &precid = lp.second.GetWords()[w - 1];
				auto &currid = lp.second.GetWords()[w];
				auto &nextid = lp.second.GetWords()[w + 1];
				if (v.IsValid(precid).IsTrue() && v.IsValid(currid).IsUnknown() && v.IsValid(nextid).IsTrue())
				{
					// 1 ? 1 -> 1 1 1
					v.SetValid(currid, true);
				}
				if (v.IsValid(precid).IsTrue() && v.IsValid(currid).IsFalse() && v.IsValid(nextid).IsUnknown())
				{
					// 1 0 ? -> 1 0 0
					v.SetValid(nextid, false);
				}
				if (v.IsValid(precid).IsUnknown() && v.IsValid(currid).IsFalse() && v.IsValid(nextid).IsTrue())
				{
					// ? 0 1 -> 0 0 1
					v.SetValid(precid, false);
				}
			} // words
		} // lines
		if (prog)
			prog->Advance();
	} // views
}

/*! \return	the list of characters sorted firstly by Unicode value and then by view id */
std::map<crn::String, std::unordered_map<Id, std::vector<Id>>> Document::CollectCharacters() const
{
	auto res = std::map<crn::String, std::unordered_map<Id, std::vector<Id>>>{};
	for (const auto &v : view_struct)
	{
		for (const auto &c : v.second.characters)
			res[c.second.GetText()][v.first].push_back(c.first);
	}
	return res;
}

const Glyph& Document::GetGlyph(const Id &id) const
{
	auto it = glyphs.find(id);
	if (it == glyphs.end())
		throw crn::ExceptionNotFound("Document::GetGlyph(): "_s + _("invalid glyph id: ") + id);
	return it->second;
}

Glyph& Document::GetGlyph(const Id &id)
{
	auto it = glyphs.find(id);
	if (it == glyphs.end())
		throw crn::ExceptionNotFound("Document::GetGlyph(): "_s + _("invalid glyph id: ") + id);
	return it->second;
}

/* Adds a glyph to the local ontology file
 * \throws	crn::ExceptionDomain the glyph already exists
 * \param[in]	id	the id of the new glyph
 * \param[in]	desc	a description for the glyph
 * \param[in]	parent	the id of the parent glyph (with prefix) or empty string (default = empty)
 * \param[in]	automatic	was the glyph automatically created? (default = false)
 * \return	the new glyph
 */
Glyph& Document::AddGlyph(const Id &id, const crn::StringUTF8 &desc, const Id &parent, bool automatic)
{
	const auto lid = Glyph::LocalId(id);
	if (glyphs.find(lid) != glyphs.end())
		throw crn::ExceptionDomain("Document::AddGlyph(): "_s + _("the glyph already exists: ") + id);
	auto el = charDecl->PushBackElement("glyph");
	el.SetAttribute("xml:id", id);
	el.PushBackElement("desc").PushBackText(desc);
	if (parent.IsNotEmpty())
		el.PushBackElement("note").PushBackText("parent=" + parent);
	if (automatic)
		el.SetAttribute("change", "#auto");
	return glyphs.emplace(lid, Glyph{el}).first->second;
}

static crn::xml::Element addcell(crn::xml::Element &row)
{
	crn::xml::Element cell = row.PushBackElement("table:table-cell");
	return cell;
}

static crn::xml::Element addcell(crn::xml::Element &row, const crn::StringUTF8 &s)
{
	crn::xml::Element cell = row.PushBackElement("table:table-cell");
	cell.SetAttribute("office:value-type", "string");
	//cell.SetAttribute("calcext:value-type", "string");
	crn::xml::Element el = cell.PushBackElement("text:p");
	el.PushBackText(s);
	return cell;
}

static crn::xml::Element addcell(crn::xml::Element &row, int v)
{
	crn::xml::Element cell = row.PushBackElement("table:table-cell");
	cell.SetAttribute("office:value-type", "float");
	cell.SetAttribute("office:value", v);
	//cell.SetAttribute("calcext:value-type", "float");
	crn::xml::Element el = cell.PushBackElement("text:p");
	el.PushBackText(crn::StringUTF8(v));
	return cell;
}

static crn::xml::Element addcell(crn::xml::Element &row, double v)
{
	crn::xml::Element cell = row.PushBackElement("table:table-cell");
	cell.SetAttribute("office:value-type", "float");
	cell.SetAttribute("office:value", v);
	//cell.SetAttribute("calcext:value-type", "float");
	crn::xml::Element el = cell.PushBackElement("text:p");
	el.PushBackText(crn::StringUTF8(v));
	return cell;
}

static crn::xml::Element addcellp(crn::xml::Element &row, double v)
{
	crn::xml::Element cell = row.PushBackElement("table:table-cell");
	cell.SetAttribute("office:value-type", "percentage");
	cell.SetAttribute("office:value", v);
	//cell.SetAttribute("calcext:value-type", "percentage");
	cell.SetAttribute("table:style-name", "percent");
	crn::xml::Element el = cell.PushBackElement("text:p");
	el.PushBackText(crn::StringUTF8(v * 100) + "%");
	return cell;
}

struct statelem
{
	statelem():ok(0),ko(0),un(0),nleft(0),leftc(0),leftac(0),nright(0),rightc(0),rightac(0) { }
	statelem(int o, int k, int u):ok(o),ko(k),un(u),nleft(0),leftac(0),nright(0),rightc(0),rightac(0) { }
	int total() const
	{
		return ok + ko + un;
	}
	int ok, ko, un;
	int nleft, leftc, leftac, nright, rightc, rightac;
};
/*! Exports statistics on alignment validation to an ODS file
 * \param[in]	fname	the filename of the spreadsheet
 */
void Document::ExportStats(const crn::Path &fname)
{
	if (crn::IO::Access(fname, crn::IO::EXISTS))
		crn::IO::Rm(fname);

	auto ods = crn::Zip::Create(fname, true);
	// stuff
	const auto mime = "application/vnd.oasis.opendocument.spreadsheet"_s;
	ods.AddFile("mimetype", mime.CStr(), mime.Size());
	ods.AddDirectory("META-INF");
	auto midoc = crn::xml::Document{};
	auto el = midoc.PushBackElement("manifest:manifest");
	el.SetAttribute("xmlns:manifest", "urn:oasis:names:tc:opendocument:xmlns:manifest:1.0");
	el.SetAttribute("manifest:version", "1.2");
	auto manel = el.PushBackElement("manifest:file-entry");
	manel.SetAttribute("manifest:full-path", "/");
	manel.SetAttribute("manifest:version", "1.2");
	manel.SetAttribute("manifest:media-type", "application/vnd.oasis.opendocument.spreadsheet");
	manel = el.PushBackElement("manifest:file-entry");
	manel.SetAttribute("manifest:full-path", "styles.xml");
	manel.SetAttribute("manifest:media-type", "text/xml");
	manel = el.PushBackElement("manifest:file-entry");
	manel.SetAttribute("manifest:full-path", "content.xml");
	manel.SetAttribute("manifest:media-type", "text/xml");
	manel = el.PushBackElement("manifest:file-entry");
	manel.SetAttribute("manifest:full-path", "meta.xml");
	manel.SetAttribute("manifest:media-type", "text/xml");
	manel = el.PushBackElement("manifest:file-entry");
	manel.SetAttribute("manifest:full-path", "settings.xml");
	manel.SetAttribute("manifest:media-type", "text/xml");

	const auto mistr = midoc.AsString();
	ods.AddFile("META-INF/manifest.xml", mistr.CStr(), mistr.Size());

	// create document-styles
	auto sdoc = crn::xml::Document{};
	el = sdoc.PushBackElement("office:document-styles");
	el.SetAttribute("xmlns:office", "urn:oasis:names:tc:opendocument:xmlns:office:1.0");
	el.SetAttribute("xmlns:style", "urn:oasis:names:tc:opendocument:xmlns:style:1.0");
	el.SetAttribute("xmlns:text", "urn:oasis:names:tc:opendocument:xmlns:text:1.0");
	el.SetAttribute("xmlns:table", "urn:oasis:names:tc:opendocument:xmlns:table:1.0");
	el.SetAttribute("xmlns:draw", "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0");
	el.SetAttribute("xmlns:fo", "urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0");
	el.SetAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
	el.SetAttribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
	el.SetAttribute("xmlns:meta", "urn:oasis:names:tc:opendocument:xmlns:meta:1.0");
	el.SetAttribute("xmlns:number", "urn:oasis:names:tc:opendocument:xmlns:datastyle:1.0");
	el.SetAttribute("xmlns:presentation", "urn:oasis:names:tc:opendocument:xmlns:presentation:1.0");
	el.SetAttribute("xmlns:svg", "urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0");
	el.SetAttribute("xmlns:chart", "urn:oasis:names:tc:opendocument:xmlns:chart:1.0");
	el.SetAttribute("xmlns:dr3d", "urn:oasis:names:tc:opendocument:xmlns:dr3d:1.0");
	el.SetAttribute("xmlns:math", "http://www.w3.org/1998/Math/MathML");
	el.SetAttribute("xmlns:form", "urn:oasis:names:tc:opendocument:xmlns:form:1.0");
	el.SetAttribute("xmlns:script", "urn:oasis:names:tc:opendocument:xmlns:script:1.0");
	el.SetAttribute("xmlns:ooo", "http://openoffice.org/2004/office");
	el.SetAttribute("xmlns:ooow", "http://openoffice.org/2004/writer");
	el.SetAttribute("xmlns:oooc", "http://openoffice.org/2004/calc");
	el.SetAttribute("xmlns:dom", "http://www.w3.org/2001/xml-events");
	el.SetAttribute("xmlns:rpt", "http://openoffice.org/2005/report");
	el.SetAttribute("xmlns:of", "urn:oasis:names:tc:opendocument:xmlns:of:1.2");
	el.SetAttribute("xmlns:xhtml", "http://www.w3.org/1999/xhtml");
	el.SetAttribute("xmlns:grddl", "http://www.w3.org/2003/g/data-view#");
	el.SetAttribute("xmlns:tableooo", "http://openoffice.org/2009/table");
	el.SetAttribute("xmlns:drawooo", "http://openoffice.org/2010/draw");
	el.SetAttribute("xmlns:calcext", "urn:org:documentfoundation:names:experimental:calc:xmlns:calcext:1.0");
	el.SetAttribute("xmlns:css3t", "http://www.w3.org/TR/css3-text/");
	el.SetAttribute("office:version", "1.2");
	auto styles = el.PushBackElement("office:styles");
	auto style = styles.PushBackElement("style:default-style");
	style.SetAttribute("style:family", "table-cell");
	auto style2 = style.PushBackElement("style:table-column-properties");
	style2.SetAttribute("style:use-optimal-column-width", "true"); // not working…

	style = styles.PushBackElement("style:style");
	style.SetAttribute("style:name", "percent");
	style.SetAttribute("style:family","table-cell");
	style.SetAttribute("style:data-style-name", "N11");

	style = styles.PushBackElement("style:style");
	style.SetAttribute("style:name", "title");
	style.SetAttribute("style:family","table-cell");
	style2 = style.PushBackElement("style:text-properties");
	style2.SetAttribute("fo:font-weight", "bold");

	const auto str = sdoc.AsString();
	ods.AddFile("styles.xml", str.CStr(), str.Size());

	// create document-meta
	auto mdoc = crn::xml::Document{};
	el = mdoc.PushBackElement("office:document-meta");
	el.SetAttribute("xmlns:office", "urn:oasis:names:tc:opendocument:xmlns:office:1.0");
	el.SetAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
	el.SetAttribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
	el.SetAttribute("xmlns:meta", "urn:oasis:names:tc:opendocument:xmlns:meta:1.0");
	el.SetAttribute("xmlns:ooo", "http://openoffice.org/2004/office");
	el.SetAttribute("xmlns:grddl", "http://www.w3.org/2003/g/data-view#");
	el.SetAttribute("office:version", "1.2");

	const auto mstr = mdoc.AsString();
	ods.AddFile("meta.xml", mstr.CStr(), mstr.Size());

	// create document-settings
	auto setdoc = crn::xml::Document{};
	el = setdoc.PushBackElement("office:document-settings");
	el.SetAttribute("xmlns:office", "urn:oasis:names:tc:opendocument:xmlns:office:1.0");
	el.SetAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
	el.SetAttribute("xmlns:draw", "urn:oasis:names:tc:opendocument:xmlns:config:1.0");
	el.SetAttribute("xmlns:ooo", "http://openoffice.org/2004/office");
	el.SetAttribute("office:version", "1.2");

	const auto sstr = setdoc.AsString();
	ods.AddFile("settings.xml", sstr.CStr(), sstr.Size());

	// create document-content
	auto doc = crn::xml::Document{};
	el = doc.PushBackElement("office:document-content");
	el.SetAttribute("xmlns:office", "urn:oasis:names:tc:opendocument:xmlns:office:1.0");
	el.SetAttribute("xmlns:style", "urn:oasis:names:tc:opendocument:xmlns:style:1.0");
	el.SetAttribute("xmlns:text", "urn:oasis:names:tc:opendocument:xmlns:text:1.0");
	el.SetAttribute("xmlns:table", "urn:oasis:names:tc:opendocument:xmlns:table:1.0");
	el.SetAttribute("xmlns:draw", "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0");
	el.SetAttribute("xmlns:fo", "urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0");
	el.SetAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
	el.SetAttribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
	el.SetAttribute("xmlns:meta", "urn:oasis:names:tc:opendocument:xmlns:meta:1.0");
	el.SetAttribute("xmlns:number", "urn:oasis:names:tc:opendocument:xmlns:datastyle:1.0");
	el.SetAttribute("xmlns:presentation", "urn:oasis:names:tc:opendocument:xmlns:presentation:1.0");
	el.SetAttribute("xmlns:svg", "urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0");
	el.SetAttribute("xmlns:chart", "urn:oasis:names:tc:opendocument:xmlns:chart:1.0");
	el.SetAttribute("xmlns:dr3d", "urn:oasis:names:tc:opendocument:xmlns:dr3d:1.0");
	el.SetAttribute("xmlns:math", "http://www.w3.org/1998/Math/MathML");
	el.SetAttribute("xmlns:form", "urn:oasis:names:tc:opendocument:xmlns:form:1.0");
	el.SetAttribute("xmlns:script", "urn:oasis:names:tc:opendocument:xmlns:script:1.0");
	el.SetAttribute("xmlns:config", "urn:oasis:names:tc:opendocument:xmlns:config:1.0");
	el.SetAttribute("xmlns:ooo", "http://openoffice.org/2004/office");
	el.SetAttribute("xmlns:ooow", "http://openoffice.org/2004/writer");
	el.SetAttribute("xmlns:oooc", "http://openoffice.org/2004/calc");
	el.SetAttribute("xmlns:dom", "http://www.w3.org/2001/xml-events");
	el.SetAttribute("xmlns:xforms", "http://www.w3.org/2002/xforms");
	el.SetAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema");
	el.SetAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
	el.SetAttribute("xmlns:rpt", "http://openoffice.org/2005/report");
	el.SetAttribute("xmlns:of", "urn:oasis:names:tc:opendocument:xmlns:of:1.2");
	el.SetAttribute("xmlns:xhtml", "http://www.w3.org/1999/xhtml");
	el.SetAttribute("xmlns:grddl", "http://www.w3.org/2003/g/data-view#");
	el.SetAttribute("xmlns:tableooo", "http://openoffice.org/2009/table");
	el.SetAttribute("xmlns:drawooo", "http://openoffice.org/2010/draw");
	el.SetAttribute("xmlns:calcext", "urn:org:documentfoundation:names:experimental:calc:xmlns:calcext:1.0");
	el.SetAttribute("xmlns:field", "urn:openoffice:names:experimental:ooo-ms-interop:xmlns:field:1.0");
	el.SetAttribute("xmlns:formx", "urn:openoffice:names:experimental:ooxml-odf-interop:xmlns:form:1.0");
	el.SetAttribute("xmlns:css3t", "http://www.w3.org/TR/css3-text/");
	el.SetAttribute("office:version", "1.2");

	el = el.PushBackElement("office:body");
	el = el.PushBackElement("office:spreadsheet");

	// gather stats
	statelem globalstat;
	auto imagestat = std::vector<statelem>{};
	auto wordstat = std::map<crn::String, statelem>{};
	auto lenstat = std::map<size_t, statelem>{};
	auto inistat = std::map<char32_t, statelem>{};
	auto endstat = std::map<char32_t, statelem>{};
	auto nextinistat = std::map<char32_t, statelem>{};
	auto precendstat = std::map<char32_t, statelem>{};
	auto precend_startstat = std::map<char32_t, std::map<char32_t, statelem>>{};
	auto end_nextinistat = std::map<char32_t, std::map<char32_t, statelem>>{};
	auto nblineerr = 0, nblines = 0;
	auto nblinecor = 0, nbwordcor = 0;
	auto totwordcor = int64_t(0);
	auto totawordcor = uint64_t(0);
	for (const auto &vid : views)
	{
		auto v = GetView(vid);
		auto wok = 0, wko = 0, wun = 0;
		for (const auto &l : v.pimpl->struc.lines)
		{
			auto err = false;
			auto precend = U' ';
			for (auto tmpw = size_t(0); tmpw < l.second.GetWords().size(); ++tmpw)
			{
				const auto &wid = l.second.GetWords()[tmpw];
				const auto &w = v.GetWord(wid);
				if (w.GetText().IsEmpty())
					continue;
				auto nextini = U' ';
				if ((tmpw + 1 < l.second.GetWords().size()) && !v.GetWord(l.second.GetWords()[tmpw + 1]).GetText().IsEmpty())
					nextini = v.GetWord(l.second.GetWords()[tmpw + 1]).GetText()[0];
				if (v.IsValid(wid).IsTrue())
				{
					wok += 1;
					wordstat[w.GetText()].ok += 1;
					lenstat[w.GetText().Length()].ok += 1;
					inistat[w.GetText()[0]].ok += 1;
					endstat[w.GetText()[w.GetText().Length() - 1]].ok += 1;
					nextinistat[nextini].ok += 1;
					precendstat[precend].ok += 1;
					precend_startstat[precend][w.GetText()[0]].ok += 1;
					end_nextinistat[w.GetText()[w.GetText().Length() - 1]][nextini].ok += 1;
				}
				else if (v.IsValid(wid).IsFalse())
				{
					wko += 1;
					wordstat[w.GetText()].ko += 1;
					lenstat[w.GetText().Length()].ko += 1;
					inistat[w.GetText()[0]].ko += 1;
					endstat[w.GetText()[w.GetText().Length() - 1]].ko += 1;
					nextinistat[nextini].ko += 1;
					precendstat[precend].ko += 1;
					precend_startstat[precend][w.GetText()[0]].ko += 1;
					end_nextinistat[w.GetText()[w.GetText().Length() - 1]][nextini].ko += 1;
					err = true;
				}
				else
				{
					wun += 1;
					wordstat[w.GetText()].un += 1;
					lenstat[w.GetText().Length()].un += 1;
					inistat[w.GetText()[0]].un += 1;
					endstat[w.GetText()[w.GetText().Length() - 1]].un += 1;
					nextinistat[nextini].un += 1;
					precendstat[precend].un += 1;
					precend_startstat[precend][w.GetText()[0]].un += 1;
					end_nextinistat[w.GetText()[w.GetText().Length() - 1]][nextini].un += 1;
				}
				precend = w.GetText()[w.GetText().Length() - 1];
				auto corr = false;
				const auto lcorr = v.GetLeftCorrection(wid);
				if (lcorr != 0)
				{
					if (tmpw == 0)
					{
						corr = true;
						totwordcor += lcorr;
						totawordcor += crn::Abs(lcorr);
					}
					inistat[w.GetText()[0]].nleft += 1;
					inistat[w.GetText()[0]].leftc += lcorr;
					inistat[w.GetText()[0]].leftac += crn::Abs(lcorr);
				}
				const auto rcorr = v.GetRightCorrection(wid);
				if (rcorr != 0)
				{
					corr = true;
					totwordcor += rcorr;
					totawordcor += crn::Abs(rcorr);
					endstat[w.GetText()[w.GetText().Length() - 1]].nright += 1;
					endstat[w.GetText()[w.GetText().Length() - 1]].rightc += rcorr;
					endstat[w.GetText()[w.GetText().Length() - 1]].rightac += crn::Abs(rcorr);
				}
				if (corr)
					nbwordcor += 1;
			} // word
			nblines += 1;
			if (err)
				nblineerr += 1;
			//if (l.second.GetCorrected()) TODO
			//nblinecor += 1;
		} // line
		globalstat.ok += wok;
		globalstat.ko += wko;
		globalstat.un += wun;
		imagestat.push_back(statelem(wok, wko, wun));
	}

	// Global statistics
	auto tab = el.PushBackElement("table:table");
	tab.SetAttribute("table:name", _("Global"));
	auto row = tab.PushBackElement("table:table-row");
	addcell(row, _("Document name"));
	addcell(row, name);
	row = tab.PushBackElement("table:table-row");
	addcell(row, _("Number of pages"));
	addcell(row, int(views.size()));
	row = tab.PushBackElement("table:table-row");

	auto wordstot = globalstat.total();
	row = tab.PushBackElement("table:table-row");
	addcell(row, _("Validated words"));
	addcell(row, globalstat.ok);
	addcellp(row, globalstat.ok / double(wordstot));
	row = tab.PushBackElement("table:table-row");
	addcell(row, _("Rejected words"));
	addcell(row, globalstat.ko);
	addcellp(row, globalstat.ko / double(wordstot));
	row = tab.PushBackElement("table:table-row");
	addcell(row, _("Unchecked words"));
	addcell(row, globalstat.un);
	addcellp(row, globalstat.un / double(wordstot));
	row = tab.PushBackElement("table:table-row");
	addcell(row, _("Total"));
	addcell(row, wordstot);
	row = tab.PushBackElement("table:table-row");
	addcell(row, _("Accuracy on checked words"));
	addcellp(row, (globalstat.ok + globalstat.ko) == 0 ? 0.0 : globalstat.ok / double(globalstat.ok + globalstat.ko));
	row = tab.PushBackElement("table:table-row");

	row = tab.PushBackElement("table:table-row");
	addcell(row, _("Number of lines"));
	addcell(row, nblines);
	row = tab.PushBackElement("table:table-row");
	addcell(row, _("Lines containing errors"));
	addcell(row, nblineerr);
	addcellp(row, nblineerr / double(nblines));
	row = tab.PushBackElement("table:table-row");
	addcell(row, _("Number of manually corrected lines"));
	addcell(row, nblinecor);
	addcellp(row, nblinecor / double(nblines));
	row = tab.PushBackElement("table:table-row");
	addcell(row, _("Number of manually corrected words"));
	addcell(row, nbwordcor);
	addcellp(row, nbwordcor / double(wordstot));
	row = tab.PushBackElement("table:table-row");
	addcell(row, _("Mean word correction length"));
	addcell(row, nbwordcor == 0 ? 0 : double(totwordcor) / nbwordcor);
	row = tab.PushBackElement("table:table-row");
	addcell(row, _("Mean word correction absolute length"));
	addcell(row, nbwordcor == 0 ? 0 : double(totawordcor) / nbwordcor);

	// Page statistics
	tab = el.PushBackElement("table:table");
	tab.SetAttribute("table:name", _("Image"));
	row = tab.PushBackElement("table:table-row");
	addcell(row, _("Page id")).SetAttribute("table:style-name", "title");
	addcell(row, _("Image name")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Total")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Accuracy on checked words")).SetAttribute("table:style-name", "title");
	for (auto p = size_t(0); p < imagestat.size(); ++p)
	{
		row = tab.PushBackElement("table:table-row");
		addcell(row, views[p]);
		addcell(row, GetView(views[p]).GetImageName());
		addcell(row, imagestat[p].ok);
		addcell(row, imagestat[p].ko);
		addcell(row, imagestat[p].un);
		const auto tot = imagestat[p].total();
		addcell(row, tot);
		addcellp(row, imagestat[p].ok / double(tot));
		addcellp(row, imagestat[p].ko / double(tot));
		addcellp(row, imagestat[p].un / double(tot));
		addcellp(row, (imagestat[p].ok + imagestat[p].ko) == 0 ? 0.0 : imagestat[p].ok / double(imagestat[p].ok + imagestat[p].ko));
	}

	// Word statistics
	tab = el.PushBackElement("table:table");
	tab.SetAttribute("table:name", _("Word"));
	row = tab.PushBackElement("table:table-row");
	row.SetAttribute("table:style-name", "title");
	addcell(row, _("Transcription")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Total")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Accuracy on checked words")).SetAttribute("table:style-name", "title");
	for (const auto &it : wordstat)
	{
		row = tab.PushBackElement("table:table-row");
		addcell(row, it.first.CStr());
		addcell(row, it.second.ok);
		addcell(row, it.second.ko);
		addcell(row, it.second.un);
		const auto tot = it.second.total();
		addcell(row, tot);
		addcellp(row, it.second.ok / double(tot));
		addcellp(row, it.second.ko / double(tot));
		addcellp(row, it.second.un / double(tot));
		addcellp(row, (it.second.ok + it.second.ko) == 0 ? 0.0 : it.second.ok / double(it.second.ok + it.second.ko));
	}

	// Length statistics
	tab = el.PushBackElement("table:table");
	tab.SetAttribute("table:name", _("Length"));
	row = tab.PushBackElement("table:table-row");
	row.SetAttribute("table:style-name", "title");
	addcell(row, _("Length")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Total")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Accuracy on checked words")).SetAttribute("table:style-name", "title");
	for (const auto &it : lenstat)
	{
		row = tab.PushBackElement("table:table-row");
		addcell(row, int(it.first));
		addcell(row, it.second.ok);
		addcell(row, it.second.ko);
		addcell(row, it.second.un);
		const auto tot = it.second.total();
		addcell(row, tot);
		addcellp(row, it.second.ok / double(tot));
		addcellp(row, it.second.ko / double(tot));
		addcellp(row, it.second.un / double(tot));
		addcellp(row, (it.second.ok + it.second.ko) == 0 ? 0.0 : it.second.ok / double(it.second.ok + it.second.ko));
	}

	// Initial statistics
	tab = el.PushBackElement("table:table");
	tab.SetAttribute("table:name", _("Initial letter"));
	row = tab.PushBackElement("table:table-row");
	row.SetAttribute("table:style-name", "title");
	addcell(row, _("Initial letter")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Total")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Accuracy on checked words")).SetAttribute("table:style-name", "title");
	addcell(row, _("Mean manual correction")).SetAttribute("table:style-name", "title");
	addcell(row, _("Mean absolute manual correction")).SetAttribute("table:style-name", "title");
	for (const auto &it : inistat)
	{
		row = tab.PushBackElement("table:table-row");
		addcell(row, crn::String(it.first).CStr());
		addcell(row, it.second.ok);
		addcell(row, it.second.ko);
		addcell(row, it.second.un);
		const auto tot = it.second.total();
		addcell(row, tot);
		addcellp(row, it.second.ok / double(tot));
		addcellp(row, it.second.ko / double(tot));
		addcellp(row, it.second.un / double(tot));
		addcellp(row, (it.second.ok + it.second.ko) == 0 ? 0.0 : it.second.ok / double(it.second.ok + it.second.ko));
		addcell(row, it.second.leftc / double(it.second.nleft));
		addcell(row, it.second.leftac / double(it.second.nleft));
	}

	// Final statistics
	tab = el.PushBackElement("table:table");
	tab.SetAttribute("table:name", _("Final letter"));
	row = tab.PushBackElement("table:table-row");
	row.SetAttribute("table:style-name", "title");
	addcell(row, _("Final letter")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Total")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Accuracy on checked words")).SetAttribute("table:style-name", "title");
	addcell(row, _("Mean manual correction")).SetAttribute("table:style-name", "title");
	addcell(row, _("Mean absolute manual correction")).SetAttribute("table:style-name", "title");
	for (const auto &it : endstat)
	{
		row = tab.PushBackElement("table:table-row");
		addcell(row, crn::String(it.first).CStr());
		addcell(row, it.second.ok);
		addcell(row, it.second.ko);
		addcell(row, it.second.un);
		const auto tot = it.second.total();
		addcell(row, tot);
		addcellp(row, it.second.ok / double(tot));
		addcellp(row, it.second.ko / double(tot));
		addcellp(row, it.second.un / double(tot));
		addcellp(row, (it.second.ok + it.second.ko) == 0 ? 0.0 : it.second.ok / double(it.second.ok + it.second.ko));
		addcell(row, it.second.rightc / double(it.second.nright));
		addcell(row, it.second.rightac / double(it.second.nright));
	}

	// Preceding final statistics
	tab = el.PushBackElement("table:table");
	tab.SetAttribute("table:name", _("Preceding final letter"));
	row = tab.PushBackElement("table:table-row");
	row.SetAttribute("table:style-name", "title");
	addcell(row, _("Preceding final letter")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Total")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Accuracy on checked words")).SetAttribute("table:style-name", "title");
	for (const auto &it : precendstat)
	{
		row = tab.PushBackElement("table:table-row");
		addcell(row, crn::String(it.first).CStr());
		addcell(row, it.second.ok);
		addcell(row, it.second.ko);
		addcell(row, it.second.un);
		const auto tot = it.second.total();
		addcell(row, tot);
		addcellp(row, it.second.ok / double(tot));
		addcellp(row, it.second.ko / double(tot));
		addcellp(row, it.second.un / double(tot));
		addcellp(row, (it.second.ok + it.second.ko) == 0 ? 0.0 : it.second.ok / double(it.second.ok + it.second.ko));
	}

	// Next initial statistics
	tab = el.PushBackElement("table:table");
	tab.SetAttribute("table:name", _("Next initial letter"));
	row = tab.PushBackElement("table:table-row");
	row.SetAttribute("table:style-name", "title");
	addcell(row, _("Next initial letter")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Total")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Accuracy on checked words")).SetAttribute("table:style-name", "title");
	for (const auto &it : nextinistat)
	{
		row = tab.PushBackElement("table:table-row");
		addcell(row, crn::String(it.first).CStr());
		addcell(row, it.second.ok);
		addcell(row, it.second.ko);
		addcell(row, it.second.un);
		const auto tot = it.second.total();
		addcell(row, tot);
		addcellp(row, it.second.ok / double(tot));
		addcellp(row, it.second.ko / double(tot));
		addcellp(row, it.second.un / double(tot));
		addcellp(row, (it.second.ok + it.second.ko) == 0 ? 0.0 : it.second.ok / double(it.second.ok + it.second.ko));
	}

	// Preceding final + initial statistics
	tab = el.PushBackElement("table:table");
	tab.SetAttribute("table:name", _("Preceding final + initial"));
	row = tab.PushBackElement("table:table-row");
	row.SetAttribute("table:style-name", "title");
	addcell(row, _("Digram")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Total")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Accuracy on checked words")).SetAttribute("table:style-name", "title");
	for (const auto &it : precend_startstat)
		for (const auto &it2 : it.second)
		{
			row = tab.PushBackElement("table:table-row");
			auto label = crn::String(it.first);
			label += " - ";
			label += it2.first;
			addcell(row, label.CStr());
			addcell(row, it2.second.ok);
			addcell(row, it2.second.ko);
			addcell(row, it2.second.un);
			const auto tot = it2.second.total();
			addcell(row, tot);
			addcellp(row, it2.second.ok / double(tot));
			addcellp(row, it2.second.ko / double(tot));
			addcellp(row, it2.second.un / double(tot));
			addcellp(row, (it2.second.ok + it2.second.ko) == 0 ? 0.0 : it2.second.ok / double(it2.second.ok + it2.second.ko));
		}

	// Final + next initial statistics
	tab = el.PushBackElement("table:table");
	tab.SetAttribute("table:name", _("Final + next initial"));
	row = tab.PushBackElement("table:table-row");
	row.SetAttribute("table:style-name", "title");
	addcell(row, _("Digram")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Total")).SetAttribute("table:style-name", "title");
	addcell(row, _("Validated")).SetAttribute("table:style-name", "title");
	addcell(row, _("Rejected")).SetAttribute("table:style-name", "title");
	addcell(row, _("Unchecked")).SetAttribute("table:style-name", "title");
	addcell(row, _("Accuracy on checked words")).SetAttribute("table:style-name", "title");
	for (const auto &it : end_nextinistat)
		for (const auto &it2 : it.second)
		{
			row = tab.PushBackElement("table:table-row");
			auto label = crn::String(it.first);
			label += " - ";
			label += it2.first;
			addcell(row, label.CStr());
			addcell(row, it2.second.ok);
			addcell(row, it2.second.ko);
			addcell(row, it2.second.un);
			const auto tot = it2.second.total();
			addcell(row, tot);
			addcellp(row, it2.second.ok / double(tot));
			addcellp(row, it2.second.ko / double(tot));
			addcellp(row, it2.second.un / double(tot));
			addcellp(row, (it2.second.ok + it2.second.ko) == 0 ? 0.0 : it2.second.ok / double(it2.second.ok + it2.second.ko));
		}

	const auto dstr = doc.AsString();
	ods.AddFile("content.xml", dstr.CStr(), dstr.Size());
	ods.Save();
}

/*!
 * \throws	crn::ExceptionNotFound	the distance matrix was not computed for this character
 * \param[in]	character	the character string
 * \return	the distance matrix for the character
 */
const std::pair<std::vector<Id>, crn::SquareMatrixDouble>& Document::GetDistanceMatrix(const crn::String &character) const
{
	auto it = chars_dm.find(character);
	if (it == chars_dm.end())
		throw crn::ExceptionNotFound{"Document::GetDistanceMatrix(): "_s + _("character not found.")};
	return it->second;
}

/*! Erases the distance matrix for a character
 * \throws	crn::ExceptionNotFound	the distance matrix was not computed for this character
 * \param[in]	character	the character string
 */
void Document::EraseDistanceMatrix(const crn::String &character)
{
	auto it = chars_dm.find(character);
	if (it == chars_dm.end())
		throw crn::ExceptionNotFound{"Document::EraseDistanceMatrix(): "_s + _("character not found.")};
	chars_dm.erase(it);
}

static crn::StringUTF8 allTextInElement(crn::xml::Element &el)
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
			if (sel.GetAttribute<crn::StringUTF8>("ana", true) != "ori:align-no")
				txt += allTextInElement(sel);
		}
	}
	return txt;
}

void Document::readTextWElements(crn::xml::Element &el, ElementPosition &pos, std::multimap<int, Id> &milestones, char lpos)
{
	for (auto sel = el.BeginElement(); sel != el.EndElement(); ++sel)
	{
		const auto elname = sel.GetName();
		if (sel.GetAttribute<crn::StringUTF8>("ana", true) != "ori:align-no")
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
				if (sel.GetAttribute<crn::StringUTF8>("type", true) == "rejet")
				{
					auto corresp = sel.GetAttribute<Id>("corresp", true);
					if (corresp.IsEmpty())
						throw crn::ExceptionNotFound(name + "-w: "_s + _("line @type=\"rejet\" without @corresp."));
					if (corresp[0] != '#')
						throw crn::ExceptionNotFound(name + "-w: "_s + _("line @type=\"rejet\" with malformed @corresp."));
					corresp.Crop(1);
					if (sel.GetAttribute<crn::StringUTF8>("rend", true) == "center")
						lpos = 'c';
					else
						lpos = 'r';
					if (view_struct[pos.view].lines.find(corresp) == view_struct[pos.view].lines.end())
					{
						view_struct[pos.view].lines.emplace(corresp, Line{});
						//view_struct[pos.view].columns[pos.column].lines.push_back(corresp);
					}
					pos.line = corresp;
				}
				else
				{
					if (view_struct[pos.view].lines.find(elid) == view_struct[pos.view].lines.end())
						view_struct[pos.view].lines.emplace(elid, Line{});
					view_struct[pos.view].columns[pos.column].lines.push_back(elid);
					pos.line = elid;
					lpos = 'l';
				}
			}
			else if ((elname == "w") || (elname == "pc"))
			{
				if (!sel.GetFirstChildElement("seg"))
				{
					if (elid.IsEmpty())
						throw crn::ExceptionNotFound(name + "-w: "_s + _("word or pc without an id."));
					view_struct[pos.view].words.emplace(elid, Word{});
					//view_struct[pos.view].words[elid].text = allTextInElement(sel);
					switch (lpos)
					{
						case 'r':
							view_struct[pos.view].lines[pos.line].right.push_back(elid);
							break;
						case 'c':
							view_struct[pos.view].lines[pos.line].center.push_back(elid);
							break;
						default:
							view_struct[pos.view].lines[pos.line].left.push_back(elid);
					}
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
					//view_struct[pos.view].words[elid].text = allTextInElement(sel);
					switch (lpos)
					{
						case 'r':
							view_struct[pos.view].lines[pos.line].right.push_back(elid);
							break;
						case 'c':
							view_struct[pos.view].lines[pos.line].center.push_back(elid);
							break;
						default:
							view_struct[pos.view].lines[pos.line].left.push_back(elid);
					}
				}
			}

			readTextWElements(sel, pos, milestones, lpos);
		}
	}
}

void Document::readTextCElements(crn::xml::Element &el, ElementPosition &pos)
{
	for (auto sel = el.BeginElement(); sel != el.EndElement(); ++sel)
	{
		const auto elname = sel.GetName();
		if (sel.GetAttribute<crn::StringUTF8>("ana", true) != "ori:align-no")
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
				//pos.page = elid;
			}
			else if (elname == "cb")
			{
				if (elid.IsEmpty())
					throw crn::ExceptionNotFound(name + "-c: "_s + _("column without an id."));
				//pos.column = elid;
			}
			else if (elname == "lb")
			{
				if (elid.IsEmpty())
					throw crn::ExceptionNotFound(name + "-c: "_s + _("line without an id."));
				//pos.line = elid;
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
				view_struct[pos.view].characters[elid].text = allTextInElement(sel);
				view_struct[pos.view].words[pos.word].characters.push_back(elid);
			}

			readTextCElements(sel, pos);
		}
	}
}

View Document::getCleanView(const Id &id)
{
	std::lock_guard<std::mutex> flock(crn::FileShield::GetMutex("views://doc/" + id));

	auto it = view_refs.find(id);
	if (it == view_refs.end())
		throw crn::ExceptionNotFound("Document::GetView(): "_s + _("Cannot find view with id ") + id);

	auto v = std::shared_ptr<View::Impl>{};
	if (it->second.expired())
	{
		v = std::make_shared<View::Impl>(id, view_struct[id], base, name, true); // may throw
		it->second = v;
	}
	else
		v = it->second.lock();
	return View(v);
}

