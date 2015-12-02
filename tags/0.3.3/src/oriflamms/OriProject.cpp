/* Copyright 2013 INSA-Lyon & IRHT
 * 
 * file: OriProject.h
 * \author Yann LEYDIER
 */

#include <OriProject.h>
#include <OriLines.h>
#include <CRNMath/CRNLinearInterpolation.h>
#include <OriFeatures.h>
#include <CRNIO/CRNIO.h>
#include <CRNUtils/CRNXml.h>
#include <iostream>
#include <CRNi18n.h>
#include <CRNIO/CRNZip.h>
#include <OriConfig.h>
#include <CRNAI/CRNPathFinding.h>
#include <CRNImage/CRNDifferential.h>

using namespace ori;

const crn::String Project::LinesKey(STR("ori::Lines"));

Project::Project(const crn::Path &name, const crn::Path &xmlpath, const crn::Path &imagedir, crn::Progress *prog):
	doc(new crn::Document)
{
	xdoc.Import(xmlpath);
	// look for images
	crn::IO::Directory imgdir(imagedir);
	CRN_FOREACH(const ori::View &v, xdoc.GetViews())
	{
		bool found = false;
		CRN_FOREACH(const crn::Path &f, imgdir.GetFiles())
		{
			if (f.GetFilename() == v.GetImageName())
			{
				doc->AddView(f);
				found = true;
				break;
			}
		}
		if (!found)
		{ // TODO
			throw crn::ExceptionNotFound(crn::StringUTF8(_("Cannot find image: ")) + v.GetImageName());
		}
	}
	doc->Save(name);

	// segment images
	if (prog)
		prog->SetMaxCount(int(xdoc.GetViews().size()));
	for (size_t v = 0; v < xdoc.GetViews().size(); ++v)
	{
		//std::cout << xdoc.GetViews()[v].GetImageName() << std::endl;
		//std::cout << xdoc.GetViews()[v].GetColumns().size() << " columns vs ";
		std::cout << doc->GetFilenames()[v] << std::endl;
		CRNBlock view(doc->GetView(v));
		const CRNVector cols(DetectLines(*view));
		//std::cout << lines->Size() << std::endl << "x\t";
		//CRN_FOREACH(const ori::Column &c, xdoc.GetViews()[v].GetColumns())
			//std::cout << c.GetLines().size() << " ";
		//std::cout << std::endl << "i\t";
		//CRN_FOREACH(const CRNVector &l, lines)
			//std::cout << l->Size() << " ";
		//std::cout << std::endl;
		/*
		CRN_FOREACH(const CRNVector col, cols)
		{
			CRN_FOREACH(const OriGraphicalLine l, col)
			{
				const std::vector<ImageSignature> sig = l->ExtractFeatures(*view);
				//std::cout << sig.size() << " ";
			}
			//std::cout << std::endl;
		}
		*/
		// XXX display
		view->GetRGB()->SavePNG(crn::Path("zzzzz") + int(v) + ".png");
		
		// store
		doc->GetView(v)->SetUserData(LinesKey, cols);
		if (prog)
			prog->Advance();
	}
	doc->Save();
	xdoc.Save(doc->GetBasename() + crn::Path::Separator() + "tei.xml");
	load_db(); // may throw
}

Project::Project(const crn::Path &fname):
	doc(new crn::Document)
{
	doc->Load(fname); // may throw
	xdoc.Load(doc->GetBasename() + crn::Path::Separator() + "tei.xml"); // may throw
	load_db(); // may throw
}

Project::View Project::GetView(size_t num)
{
	return View(doc->GetView(num), xdoc.GetViews()[num]);
}

void Project::Save(crn::Progress *prog) const
{
	xdoc.Save(doc->GetBasename() + crn::Path::Separator() + "tei.xml", prog);
}

void Project::ReloadTEI()
{
	xdoc.Import(xdoc.GetTEIPath());
	xdoc.Save(doc->GetBasename() + crn::Path::Separator() + "tei.xml");
}

void Project::AlignAll(crn::Progress *docprog, crn::Progress *viewprog, crn::Progress *colprog, crn::Progress *linprog)
{
	if (docprog)
		docprog->SetMaxCount(int(xdoc.GetViews().size()));
	for (size_t v = 0; v < xdoc.GetViews().size(); ++v)
	{
		AlignView(v, viewprog, colprog, linprog);
		if (docprog)
			docprog->Advance();
	}
}

void Project::AlignView(size_t view_num, crn::Progress *viewprog, crn::Progress *colprog, crn::Progress *linprog)
{
	CRNBlock b(doc->GetView(view_num)); // load once and for all (avoids to reload the image each time we have to compute features)
	size_t ncols = xdoc.GetViews()[view_num].GetColumns().size();
	if (viewprog)
		viewprog->SetMaxCount(int(ncols));
	for (size_t c = 0; c < ncols; ++c)
	{
		AlignColumn(view_num, c, colprog, linprog);
		if (viewprog)
			viewprog->Advance();
	}
	//b->GetRGB()->SavePNG(crn::Path("new sig ") + int(view_num) + crn::Path(".png"));
}

void Project::AlignColumn(size_t view_num, size_t col_num, crn::Progress *colprog, crn::Progress *linprog)
{
	CRNBlock b(doc->GetView(view_num)); // load once and for all (avoids to reload the image each time we have to compute features)
	size_t nlines = xdoc.GetViews()[view_num].GetColumns()[col_num].GetLines().size();
	if (colprog)
		colprog->SetMaxCount(int(nlines));
	for (size_t l = 0; l < nlines; ++l)
	{
		AlignLine(view_num, col_num, l, linprog);
		if (colprog)
			colprog->Advance();
	}
}

void Project::AlignLine(size_t view_num, size_t col_num, size_t line_num, crn::Progress *prog)
{
	/*
	if (prog)
		prog->SetMaxCount(2);
	CRNBlock b(doc->GetView(view_num));
	CRNVector cols(b->GetUserData(ori::Project::LinesKey));
	CRNVector lines(cols[col_num]);
	OriGraphicalLine l(lines[line_num]);
	ori::Line &ol = xdoc.GetViews()[view_num].GetColumns()[col_num].GetLines()[line_num];
	const std::vector<TextSignature> lsig(getSignature(ol));
	const std::vector<ImageSignature> isig(l->ExtractFeatures(*b));
	//if (sig.empty())
		//std::cout << "no sig " << view_num << " " << col_num << " " << line_num << std::endl;
	if (prog)
		prog->Advance();
	ol.Align(Align(isig, lsig));
	//ol.Align(sig);
	if (prog)
		prog->Advance();
		*/

	ori::Line &ol = xdoc.GetViews()[view_num].GetColumns()[col_num].GetLines()[line_num];
	std::vector<std::vector<size_t> > wranges;
	bool in = false;
	for (size_t w = 0; w < ol.GetWords().size(); ++w)
	{
		if (!ol.GetWords()[w].GetBBox().IsValid() || !ol.GetWords()[w].GetValid().IsTrue())
		{
			if (!in)
			{
				in = true;
				wranges.push_back(std::vector<size_t>());
			}
			wranges.back().push_back(w);
		}
		else
		{
			in = false;
		}
	}
	if (prog)
		prog->SetMaxCount(int(wranges.size()));
	CRN_FOREACH(const std::vector<size_t> &r, wranges)
	{
		if ((r.size() == 1) && ol.GetWords()[r.front()].GetBBox().IsValid())
		{ // a single word between validated words
			ol.GetWords()[r.front()].SetValid(crn::Prop3::True);
		}
		else
		{
			AlignRange(WordPath(view_num, col_num, line_num, r.front()), WordPath(view_num, col_num, line_num, r.back()));
		}
		if (prog)
			prog->Advance();
	}
}

/*!
 * Align text and image between two words
 * \param[in]	first	the first word to align
 * \param[in]	last	the last word to align (included!)
 * \throws	crn::ExceptionDomain	the last word is before the first
 * \throws	crn::ExceptionTODO	the range is not on a single line
 * \throws	crn::ExceptionUninitialized	the word before or after the range is not aligned
 */
void Project::AlignRange(const WordPath &first, const WordPath &last)
{
	using namespace std; // for operator<(WordPath, WordPath)
	if (last < first)
		throw crn::ExceptionDimension(_("The last word is locate before the first."));
	if ((first.view != last.view) || (first.col != last.col) || (first.line != last.line))
		throw crn::ExceptionTODO(_("The range must be within a single line."));
	//if (first == last)
		//return;

	CRNBlock bb(doc->GetView(first.view));
	CRNVector bcols(bb->GetUserData(ori::Project::LinesKey));
	CRNVector blines(bcols[first.col]);
	OriGraphicalLine bl(blines[first.line]);
	// range on image
	size_t bx;
	if (first.word == 0)
	{ // start of the line
		bx = bl->GetLeft();
	}
	else
	{
		bx = xdoc.GetWord(WordPath(first.view, first.col, first.line, first.word - 1)).GetBBox().GetRight() + 1; // may throw
	}
	size_t ex;
	if (last.word == (xdoc.GetLine(last).GetWords().size() - 1))
	{ // end of the line
		ex = bl->GetRight();
	}
	else
	{
		ex = xdoc.GetWord(WordPath(last.view, last.col, last.line, last.word + 1)).GetBBox().GetLeft() - 1; // may throw
	}
	// extract image signature
	const std::vector<ImageSignature> isig(bl->ExtractFeatures(*bb));
	std::vector<ImageSignature> risig;
	CRN_FOREACH(const ImageSignature &is, isig)
	{
		if (is.bbox.GetRight() > ex)
			break;
		if (is.bbox.GetLeft() >= bx)
			risig.push_back(is);
	}

	// text signature
	std::vector<TextSignature> lsig;
	for (size_t w = first.word; w <= last.word; ++w)
	{
		std::vector<TextSignature> wsig(getSignature(xdoc.GetWord(WordPath(first.view, first.col, first.line, w))));
		for (size_t tmp = 1; tmp < wsig.size(); ++tmp)
			wsig[tmp].start = false;
		std::copy(wsig.begin(), wsig.end(), std::back_inserter(lsig));
	}

	const std::vector<std::pair<crn::Rect, crn::StringUTF8> > align(Align(risig, lsig));
	size_t bbn = 0;
	for (size_t w = first.word; w <= last.word; ++w)
	{
		if (bbn >= align.size())
		{
			CRNError(_("Range align misfit."));
			break;
		}
		Word &word(xdoc.GetWord(WordPath(first.view, first.col, first.line, w)));
		if (word.GetText().IsNotEmpty())
		{
			word.SetImageSignature(align[bbn].second);
			if (word.GetBBox() != align[bbn].first)
			{
				word.SetValid(crn::Prop3::Unknown);
			}
			word.SetBBox(align[bbn].first); // always reset left/right corrections
			bbn += 1;
		}
	}
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
	int total() const { return ok + ko + un; }
	int ok, ko, un;
	int nleft, leftc, leftac, nright, rightc, rightac;
};

void Project::ExportStats(const crn::Path &fname) const
{
	if (crn::IO::Access(fname, crn::IO::EXISTS))
		crn::IO::Rm(fname);

	crn::Zip ods(crn::Zip::Create(fname, true));
	// stuff
	crn::StringUTF8 mime("application/vnd.oasis.opendocument.spreadsheet");
	ods.AddFile("mimetype", mime.CStr(), mime.Size());
	ods.AddDirectory("META-INF");
	crn::xml::Document midoc;
	crn::xml::Element el = midoc.PushBackElement("manifest:manifest");
	el.SetAttribute("xmlns:manifest", "urn:oasis:names:tc:opendocument:xmlns:manifest:1.0");
	el.SetAttribute("manifest:version", "1.2");
	crn::xml::Element manel = el.PushBackElement("manifest:file-entry");
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

	crn::StringUTF8 mistr(midoc.AsString());
	ods.AddFile("META-INF/manifest.xml", mistr.CStr(), mistr.Size());
	
	// create document-styles
	crn::xml::Document sdoc;
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
	crn::xml::Element styles = el.PushBackElement("office:styles");
	crn::xml::Element style = styles.PushBackElement("style:default-style");
	style.SetAttribute("style:family", "table-cell");
	crn::xml::Element style2 = style.PushBackElement("style:table-column-properties");
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

	crn::StringUTF8 str(sdoc.AsString());
	ods.AddFile("styles.xml", str.CStr(), str.Size());

	// create document-meta
	crn::xml::Document mdoc;
	el = mdoc.PushBackElement("office:document-meta");
	el.SetAttribute("xmlns:office", "urn:oasis:names:tc:opendocument:xmlns:office:1.0");
	el.SetAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
	el.SetAttribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
	el.SetAttribute("xmlns:meta", "urn:oasis:names:tc:opendocument:xmlns:meta:1.0");
	el.SetAttribute("xmlns:ooo", "http://openoffice.org/2004/office");
	el.SetAttribute("xmlns:grddl", "http://www.w3.org/2003/g/data-view#");
	el.SetAttribute("office:version", "1.2");
	
	crn::StringUTF8 mstr = mdoc.AsString();
	ods.AddFile("meta.xml", mstr.CStr(), mstr.Size());

	// create document-settings
	crn::xml::Document setdoc;
	el = setdoc.PushBackElement("office:document-settings");
	el.SetAttribute("xmlns:office", "urn:oasis:names:tc:opendocument:xmlns:office:1.0");
	el.SetAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
	el.SetAttribute("xmlns:draw", "urn:oasis:names:tc:opendocument:xmlns:config:1.0");
	el.SetAttribute("xmlns:ooo", "http://openoffice.org/2004/office");
	el.SetAttribute("office:version", "1.2");

	crn::StringUTF8 sstr = setdoc.AsString();
	ods.AddFile("settings.xml", sstr.CStr(), sstr.Size());

	// create document-content
	crn::xml::Document doc;
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
	std::vector<statelem> imagestat;
	std::map<crn::String, statelem> wordstat;
	std::map<size_t, statelem> lenstat;
	std::map<crn::Char, statelem> inistat, endstat;
	std::map<crn::Char, statelem> nextinistat, precendstat;
	std::map<crn::Char, std::map<crn::Char, statelem> > precend_startstat, end_nextinistat;
	int nblineerr = 0, nblines = 0;
	int nblinecor = 0, nbwordcor = 0;
	crn::Int64 totwordcor = 0;
	crn::UInt64 totawordcor = 0;
	CRN_FOREACH(const ori::View &v, xdoc.GetViews())
	{
		int wok = 0, wko = 0, wun = 0;
		CRN_FOREACH(const Column &c, v.GetColumns())
		{
			CRN_FOREACH(const Line &l, c.GetLines())
			{
				bool err = false;
				crn::Char precend = L' ';
				for (size_t tmpw = 0; tmpw < l.GetWords().size(); ++tmpw)
				{
					const Word &w(l.GetWords()[tmpw]);
					crn::Char nextini = L' ';
					if (tmpw + 1 < l.GetWords().size())
						nextini = l.GetWords()[tmpw + 1].GetText()[0];
					if (w.GetValid().IsTrue())
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
					else if (w.GetValid().IsFalse())
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
					bool corr = false;
					if (w.GetLeftCorrection() != 0)
					{
						if (tmpw == 0)
						{
							corr = true;
							totwordcor += w.GetLeftCorrection();
							totawordcor += crn::Abs(w.GetLeftCorrection());
						}
						inistat[w.GetText()[0]].nleft += 1;
						inistat[w.GetText()[0]].leftc += w.GetLeftCorrection();
						inistat[w.GetText()[0]].leftac += crn::Abs(w.GetLeftCorrection());
					}
					if (w.GetRightCorrection() != 0)
					{
						corr = true;
						totwordcor += w.GetRightCorrection();
						totawordcor += crn::Abs(w.GetRightCorrection());
						endstat[w.GetText()[w.GetText().Length() - 1]].nright += 1;
						endstat[w.GetText()[w.GetText().Length() - 1]].rightc += w.GetRightCorrection();
						endstat[w.GetText()[w.GetText().Length() - 1]].rightac += crn::Abs(w.GetRightCorrection());
					}
					if (corr)
						nbwordcor += 1;
				} // word
				nblines += 1;
				if (err)
					nblineerr += 1;
				if (l.GetCorrected())
					nblinecor += 1;
			} // line
		} // col
		globalstat.ok += wok;
		globalstat.ko += wko;
		globalstat.un += wun;
		imagestat.push_back(statelem(wok, wko, wun));
	}

	// Global statistics
	crn::xml::Element tab = el.PushBackElement("table:table");
	tab.SetAttribute("table:name", _("Global"));
	crn::xml::Element row = tab.PushBackElement("table:table-row");
	addcell(row, _("Document name"));
	addcell(row, xdoc.GetTEIPath().GetFilename());
	row = tab.PushBackElement("table:table-row");
	addcell(row, _("Number of pages"));
	addcell(row, int(GetNbViews()));
	row = tab.PushBackElement("table:table-row");

	int wordstot = globalstat.total();
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
	for (size_t p = 0; p < imagestat.size(); ++p)
	{
		row = tab.PushBackElement("table:table-row");
		addcell(row, xdoc.GetViews()[p].GetId());
		addcell(row, xdoc.GetViews()[p].GetImageName());
		addcell(row, imagestat[p].ok);
		addcell(row, imagestat[p].ko);
		addcell(row, imagestat[p].un);
		int tot = imagestat[p].total();
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
	for (std::map<crn::String, statelem>::iterator	it = wordstat.begin(); it != wordstat.end(); ++it)
	{
		row = tab.PushBackElement("table:table-row");
		addcell(row, it->first.CStr());
		addcell(row, it->second.ok);
		addcell(row, it->second.ko);
		addcell(row, it->second.un);
		int tot = it->second.total();
		addcell(row, tot);
		addcellp(row, it->second.ok / double(tot));
		addcellp(row, it->second.ko / double(tot));
		addcellp(row, it->second.un / double(tot));
		addcellp(row, (it->second.ok + it->second.ko) == 0 ? 0.0 : it->second.ok / double(it->second.ok + it->second.ko));
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
	for (std::map<size_t, statelem>::iterator	it = lenstat.begin(); it != lenstat.end(); ++it)
	{
		row = tab.PushBackElement("table:table-row");
		addcell(row, int(it->first));
		addcell(row, it->second.ok);
		addcell(row, it->second.ko);
		addcell(row, it->second.un);
		int tot = it->second.total();
		addcell(row, tot);
		addcellp(row, it->second.ok / double(tot));
		addcellp(row, it->second.ko / double(tot));
		addcellp(row, it->second.un / double(tot));
		addcellp(row, (it->second.ok + it->second.ko) == 0 ? 0.0 : it->second.ok / double(it->second.ok + it->second.ko));
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
	for (std::map<crn::Char, statelem>::iterator	it = inistat.begin(); it != inistat.end(); ++it)
	{
		row = tab.PushBackElement("table:table-row");
		addcell(row, crn::String(it->first).CStr());
		addcell(row, it->second.ok);
		addcell(row, it->second.ko);
		addcell(row, it->second.un);
		int tot = it->second.total();
		addcell(row, tot);
		addcellp(row, it->second.ok / double(tot));
		addcellp(row, it->second.ko / double(tot));
		addcellp(row, it->second.un / double(tot));
		addcellp(row, (it->second.ok + it->second.ko) == 0 ? 0.0 : it->second.ok / double(it->second.ok + it->second.ko));
		addcell(row, it->second.leftc / double(it->second.nleft));
		addcell(row, it->second.leftac / double(it->second.nleft));
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
	for (std::map<crn::Char, statelem>::iterator	it = endstat.begin(); it != endstat.end(); ++it)
	{
		row = tab.PushBackElement("table:table-row");
		addcell(row, crn::String(it->first).CStr());
		addcell(row, it->second.ok);
		addcell(row, it->second.ko);
		addcell(row, it->second.un);
		int tot = it->second.total();
		addcell(row, tot);
		addcellp(row, it->second.ok / double(tot));
		addcellp(row, it->second.ko / double(tot));
		addcellp(row, it->second.un / double(tot));
		addcellp(row, (it->second.ok + it->second.ko) == 0 ? 0.0 : it->second.ok / double(it->second.ok + it->second.ko));
		addcell(row, it->second.rightc / double(it->second.nright));
		addcell(row, it->second.rightac / double(it->second.nright));
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
	for (std::map<crn::Char, statelem>::iterator	it = precendstat.begin(); it != precendstat.end(); ++it)
	{
		row = tab.PushBackElement("table:table-row");
		addcell(row, crn::String(it->first).CStr());
		addcell(row, it->second.ok);
		addcell(row, it->second.ko);
		addcell(row, it->second.un);
		int tot = it->second.total();
		addcell(row, tot);
		addcellp(row, it->second.ok / double(tot));
		addcellp(row, it->second.ko / double(tot));
		addcellp(row, it->second.un / double(tot));
		addcellp(row, (it->second.ok + it->second.ko) == 0 ? 0.0 : it->second.ok / double(it->second.ok + it->second.ko));
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
	for (std::map<crn::Char, statelem>::iterator	it = nextinistat.begin(); it != nextinistat.end(); ++it)
	{
		row = tab.PushBackElement("table:table-row");
		addcell(row, crn::String(it->first).CStr());
		addcell(row, it->second.ok);
		addcell(row, it->second.ko);
		addcell(row, it->second.un);
		int tot = it->second.total();
		addcell(row, tot);
		addcellp(row, it->second.ok / double(tot));
		addcellp(row, it->second.ko / double(tot));
		addcellp(row, it->second.un / double(tot));
		addcellp(row, (it->second.ok + it->second.ko) == 0 ? 0.0 : it->second.ok / double(it->second.ok + it->second.ko));
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
	for (std::map<crn::Char, std::map<crn::Char, statelem> >::iterator it = precend_startstat.begin(); it != precend_startstat.end(); ++it)
		for (std::map<crn::Char, statelem>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
	{
		row = tab.PushBackElement("table:table-row");
		crn::String label(it->first);
		label += " - ";
		label += it2->first;
		addcell(row, label.CStr());
		addcell(row, it2->second.ok);
		addcell(row, it2->second.ko);
		addcell(row, it2->second.un);
		int tot = it2->second.total();
		addcell(row, tot);
		addcellp(row, it2->second.ok / double(tot));
		addcellp(row, it2->second.ko / double(tot));
		addcellp(row, it2->second.un / double(tot));
		addcellp(row, (it2->second.ok + it2->second.ko) == 0 ? 0.0 : it2->second.ok / double(it2->second.ok + it2->second.ko));
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
	for (std::map<crn::Char, std::map<crn::Char, statelem> >::iterator it = end_nextinistat.begin(); it != end_nextinistat.end(); ++it)
		for (std::map<crn::Char, statelem>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
	{
		row = tab.PushBackElement("table:table-row");
		crn::String label(it->first);
		label += " - ";
		label += it2->first;
		addcell(row, label.CStr());
		addcell(row, it2->second.ok);
		addcell(row, it2->second.ko);
		addcell(row, it2->second.un);
		int tot = it2->second.total();
		addcell(row, tot);
		addcellp(row, it2->second.ok / double(tot));
		addcellp(row, it2->second.ko / double(tot));
		addcellp(row, it2->second.un / double(tot));
		addcellp(row, (it2->second.ok + it2->second.ko) == 0 ? 0.0 : it2->second.ok / double(it2->second.ok + it2->second.ko));
	}

	crn::StringUTF8 dstr = doc.AsString();
	ods.AddFile("content.xml", dstr.CStr(), dstr.Size());
	ods.Save();
	
}

const crn::StringUTF8 Project::GetTitle() const
{
	return doc->GetBasename().GetFilename();
}

void Project::ClearSignatures(crn::Progress *prog)
{
	if (prog)
		prog->SetMaxCount(int(GetNbViews()));
	for (size_t tmp = 0; tmp < GetNbViews(); ++tmp)
	{
		CRNBlock b(doc->GetView(tmp));
		CRNVector cols(b->GetUserData(LinesKey));
		CRN_FOREACH(CRNVector col, cols)
		{
			CRN_FOREACH(OriGraphicalLine l, col)
				l->ClearFeatures();
		}
		if (prog)
			prog->Advance();
	}
}

struct stepcost
{
	stepcost(const crn::ImageGray &ig, int x):img(ig),ref(x) { }
	const crn::ImageGray &img;
	const int ref;
	inline double operator()(const crn::Point2DInt &p1, const crn::Point2DInt &p2) const
	{
		return img.GetPixel(p2.X, p2.Y) + 4 * crn::Abs(ref - p2.X); // 2 for gray, 8 for k2
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

void Project::ExportCharacters(const crn::Path &path, bool only_validated, crn::Progress *prog)
{
	if (!crn::IO::Access(path, crn::IO::EXISTS))
		crn::IO::Mkdir(path); // may throw
	if (!crn::IO::Access(path / "__words", crn::IO::EXISTS))
		crn::IO::Mkdir(path / "__words"); // may throw
	if (prog)
		prog->SetMaxCount(int(GetNbViews()));
	for (size_t v = 0; v < GetNbViews(); ++v)
	{
		View view(GetView(v));
		CRNVector cols(view.image->GetUserData(ori::Project::LinesKey));
		for (size_t c = 0; c < view.page.GetColumns().size(); ++c)
		{
			CRNVector lines(cols[c]);
			for (size_t l = 0; l < view.page.GetColumns()[c].GetLines().size(); ++l)
			{
				OriGraphicalLine line(lines[l]);
				const std::vector<ImageSignature> isig(line->ExtractFeatures(*view.image));
				for (size_t w = 0; w < view.page.GetColumns()[c].GetLines()[l].GetWords().size(); ++w)
				{
					Word &word(view.page.GetColumns()[c].GetLines()[l].GetWords()[w]);
					if (only_validated && !word.GetValid().IsTrue())
						continue;
					if (word.GetBBox().IsValid())
					{
						const std::vector<TextSignature> wsig(getSignature(word));
						// crop image signature
						std::vector<ImageSignature> wisig;
						CRN_FOREACH(const ImageSignature &is, isig)
						{
							if ((is.bbox & word.GetBBox()).IsValid())
								wisig.push_back(is);
						}
						// align
						const std::vector<std::pair<crn::Rect, crn::StringUTF8> > align(Align(wisig, wsig));
						crn::ImageRGB irgb;
						irgb.InitFromImage(*view.image->GetRGB(), word.GetBBox());
						
						CRNDifferential diff(crn::Differential::New(irgb, 0));
						diff->Diffuse(5);
						CRNImageDoubleGray roadmap(diff->MakeKappa2());
						CRNImageGray rmg(roadmap->MakeImageGray());
						CRNImageGray ig(irgb.MakeImageGray());
						FOREACHPIXEL(tmp, *rmg)
						{
							;
							rmg->GetPixels()[tmp] = crn::Byte(rmg->GetPixels()[tmp] / 2 + 127 - ig->GetPixel(tmp) / 2);
						}
						//irgb.InitFromImage(*rmg);
						for (size_t tmp = 0; tmp < align.size(); ++tmp)
						{
							crn::Rect r(align[tmp].first);

							crn::ImageRGB ic;
							ic.InitFromImage(*view.image->GetRGB(), r);
							crn::String ch(word.GetText()[tmp]);
							if (ch == ".") ch = "dot";
							try
							{
								if (!crn::IO::Access(path / ch.CStr(), crn::IO::EXISTS))
									crn::IO::Mkdir(path / ch.CStr()); // may throw
								ic.SavePNG(path / ch.CStr() / int(v) + "-" + int(c) + "-" + int(l) + "-" + int(w) + "-" + int(tmp) + ".png");
							}
							catch (...) {}

							r.Translate(-word.GetBBox().GetLeft(), -word.GetBBox().GetTop());
							//irgb.DrawRect(r, crn::PixelRGB(crn::Byte(255), 255, 255));

							const std::vector<crn::Point2DInt> path(crn::AStar(crn::Point2DInt(r.GetRight(), r.GetTop()), crn::Point2DInt(r.GetRight(), r.GetBottom()), stepcost(*rmg, r.GetRight()), heuristic(), get_neighbors(r)));
							for (size_t tmp = 1; tmp < path.size(); ++tmp)
							{
								irgb.DrawLine(path[tmp - 1].X, path[tmp - 1].Y, path[tmp].X, path[tmp].Y, crn::PixelRGB(crn::Byte(255), 0, 0));
							}
						}
						try
						{
							irgb.SavePNG(path / "__words" / int(v) + "-" + int(c) + "-" + int(l) + "-" + int(w) + ".png");
						}
						catch (...) {}
					} // word is aligned
				} // for each word
			} // for each line
		} // for each column
		if (prog)
			prog->Advance();
	} // for each view
}

void Project::PropagateValidation(crn::Progress *prog)
{
	if (prog)
		prog->SetMaxCount(int(xdoc.GetViews().size()));
	CRN_FOREACH(ori::View &v, xdoc.GetViews())
	{
		CRN_FOREACH(Column &c, v.GetColumns())
		{
			CRN_FOREACH(Line &l, c.GetLines())
			{
				for (size_t w = 1; w < l.GetWords().size() - 1; ++w)
				{
					Word &prec(l.GetWords()[w - 1]);
					Word &curr(l.GetWords()[w]);
					Word &next(l.GetWords()[w + 1]);
					if (prec.GetValid().IsTrue() && curr.GetValid().IsUnknown() && next.GetValid().IsTrue())
					{ // 1 ? 1 -> 1 1 1
						curr.SetValid(true);
					}
					if (prec.GetValid().IsTrue() && curr.GetValid().IsFalse() && next.GetValid().IsUnknown())
					{ // 1 0 ? -> 1 0 0
						next.SetValid(false);
					}
					if (prec.GetValid().IsUnknown() && curr.GetValid().IsFalse() && next.GetValid().IsTrue())
					{ // ? 0 1 -> 0 0 1
						prec.SetValid(false);
					}
				} // words
			} // lines
		} // columns
		if (prog)
			prog->Advance();
	} // views
}

void Project::load_db()
{
	signature_db.clear();
	crn::xml::Document doc(ori::Config::GetInstance().GetStaticDataDir() + crn::Path::Separator() + "orisig.xml"); // may throw
	crn::xml::Element root(doc.GetRoot());
	for (crn::xml::Element el = root.BeginElement(); el != root.EndElement(); ++el)
	{
		signature_db.insert(std::make_pair(crn::Char(el.GetAttribute<int>("char")), el.GetAttribute<crn::StringUTF8>("sig")));
	}
}

const std::vector<TextSignature> Project::getSignature(const Line &l) const
{
	std::vector<TextSignature> sig;
	for (size_t wnum = 0; wnum < l.GetWords().size(); ++wnum)
	{
		crn::String t(l.GetWords()[wnum].GetText());
		if (t.IsEmpty())
		{
			CRNWarning(_("Empty word."));
			continue;
		}
		crn::StringUTF8 f;
		for (size_t tmp = 0; tmp < t.Size(); ++tmp)
		{
			std::map<crn::Char, crn::StringUTF8>::const_iterator it(signature_db.find(t[tmp]));
			if (it != signature_db.end())
				f += it->second;
		}
		if (f.IsEmpty())
		{
			CRNWarning(_("Word has no signature: <") + t + ">");
			continue;
		}
		sig.push_back(TextSignature(true, f[0]));
		for (size_t tmp = 1; tmp < f.Size(); ++tmp)
			sig.push_back(TextSignature(false, f[tmp]));

		// add space
		/* TODO uncomment when the spaces will be added to the image signature!
		if (wnum != words.size() - 1)
			sig.push_back(TextSignature(false, feat[' '][0]));
			*/
	}
	return sig;
}

const std::vector<TextSignature> Project::getSignature(const Word &w) const
{
	std::vector<TextSignature> sig;
	crn::String t(w.GetText());
	if (t.IsEmpty())
	{
		CRNWarning(_("Empty word."));
		return sig;
	}
	for (size_t tmp = 0; tmp < t.Size(); ++tmp)
	{
		std::map<crn::Char, crn::StringUTF8>::const_iterator it(signature_db.find(t[tmp]));
		if (it != signature_db.end())
		{
			if (it->second.IsEmpty())
			{
				CRNWarning(_("Character has no signature: <") + crn::String(t[tmp]) + ">"); // XXX is this really a warning…?
				continue;
			}
			sig.push_back(TextSignature(true, it->second[0]));
			for (size_t tmp = 1; tmp < it->second.Size(); ++tmp)
				sig.push_back(TextSignature(false, it->second[tmp]));
		}
	}
	return sig;
}
