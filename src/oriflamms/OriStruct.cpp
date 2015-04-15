/* Copyright 2013 INSA-Lyon & IRHT
 * 
 * file: OriStruct.cpp
 * \author Yann LEYDIER
 */

#include <OriStruct.h>
#include <OriConfig.h>
#include <CRNUtils/CRNXml.h>
#include <CRNi18n.h>
#include <iostream>

using namespace ori;

WordPath::WordPath(const crn::String &s)
{
	std::vector<crn::String> str(s.Split(L" "));
	if (str.size() != 4)
		throw crn::ExceptionInvalidArgument(_("Malformed word path."));
	view = str[0].ToInt();
	col = str[1].ToInt();
	line = str[2].ToInt();
	word = str[3].ToInt();
}

const crn::String WordPath::ToString() const
{
	crn::String s;
	s += view;
	s += L' ';
	s += col;
	s += L' ';
	s += line;
	s += L' ';
	s += word;
	return s;
}


void Line::Append(const Line &other)
{
	std::copy(other.words.begin(), other.words.end(), std::back_inserter(words));
}

#if 0
static inline int add_elem(char c)
{
	switch (c)
	{
		case ' ':
			return 0;
		case ',':
			return 1;
		case '\'':
			return 1;
		case 'l':
			return 2;
		case '.':
			return 1;
		case '(':
			return 1;
		case ')':
			return 1;
	}
	throw crn::ExceptionDomain();
}
static inline int suppr_elem(char c)
{
	switch (c)
	{
		case ' ':
			return 3;
		case ',':
			return 1;
		case '\'':
			return 1;
		case 'l':
			return 2;
		case '.':
			return 3;
		case '(':
			return 1;
		case ')':
			return 1;
	}
	throw crn::ExceptionDomain();
}
static inline int change_elem(char c, char n)
{
	if (c == n)
		return 0;
	switch (c)
	{
		case ' ':
			if (n == 'l')
				return 3;
			else
				return 2;
		case ',':
			if (n == '\'')
				return 2;
			else
				return 1;
		case '\'':
			if (n == ',')
				return 2;
			else
				return 1;
		case 'l':
			if (n == '.')
				return 2;
			else
				return 1;
		case '.':
			if (n == 'l')
				return 3;
			else
				return 2;
		case '(':
			return 1;
		case ')':
			return 1;
	}
	throw crn::ExceptionDomain();
}
void Line::Align(const std::vector<ImageSignature> &img)
{
	const int addcost = 1;
	const int supprcost = 1;
	const int changecost = 1;

	const std::vector<TextSignature> txt(featurize());
	// edit distance
	size_t s1 = img.size(), s2 = txt.size();
	std::vector<std::vector<int> > d(s1 + 1, std::vector<int>(s2 + 1, 0));
	for (size_t tmp = 1; tmp < s1; ++tmp) d[tmp][0] = int(tmp) * supprcost;
	for (size_t tmp = 1; tmp < s2; ++tmp) d[0][tmp] = int(tmp) * addcost;
	for (size_t i = 1; i <= s1; ++i)
	{
		for (size_t j = 1; j <= s2; ++j)
		{
			int add = d[i][j - 1] + add_elem(txt[j - 1].code);//addcost;
			int suppr = d[i - 1][j] + suppr_elem(img[i - 1].code);//supprcost; //int(img[i - 1].bbox.GetWidth() * img[i - 1].weight);
			int change = d[i- 1][j - 1] + change_elem(img[i - 1].code, txt[j - 1].code);//(img[i - 1].code == txt[j - 1].code ? 0 : changecost);
			int min = change;
			if (add < min)
				min = add;
			if (suppr < min) 
				min = suppr;
			d[i][j] = min;
			//std::cout << d[i][j] << " ";
		}
		//std::cout << std::endl;
	}
	//std::cout << d[s1][s2] << std::endl;

	/*
	// find words that contain only one dot signature element
	std::vector<bool> puncdot(s2 + 1, false);
	for (size_t tmp = 0; tmp < s2 - 1; ++tmp)
		if (txt[tmp].start && txt[tmp + 1].start && (txt[tmp].code == '.'))
			puncdot[tmp + 1] = true;
	if (txt[s2 - 1].start && (txt[s2 - 1].code == '.'))
		puncdot[s2] = true;
	*/
	// look for an optimal path
	std::vector<size_t> path(s2 + 1);
	size_t i = s1;
	size_t t = s2;
	path[t] = i;
	while (t > 0)
	{
		int dcost = std::numeric_limits<int>::max();
		int leftcost = std::numeric_limits<int>::max();
		if ((i > 0) /*&& !puncdot[t]*/) // when a word is just a punctuation dot, we can only go up, or the dot might eat part of the neighbouring words
		{
			dcost = d[i - 1][t - 1];
			leftcost = d[i- 1][t];
		}
		int upcost = d[i][t - 1];
		if (dcost <= leftcost)
		{
			if (dcost <= upcost)
			{ // diagonal
				t -= 1;
				i -= 1;
			}
			else
			{ // up
				t -= 1;
			}
		}
		else
		{
			if (leftcost <= upcost)
			{ // left
				i -= 1;
			}
			else
			{ // up
				t -= 1;
			}
		}
		path[t] = i;
	}
	// compute bounding boxes
	std::vector<crn::Rect> wordsbb;
	std::vector<crn::StringUTF8> wordimgsig;
	size_t istart = 0, iend = 0;
	for (size_t t = 1; t <= s2; ++t)
	{
		//std::cout << path[t] << " " << t << std::endl;
		size_t imgnum = path[t]; if (imgnum > 0) imgnum -= 1;
		if (txt[t - 1].start)
		{ // start a new word

			// end previous word
			if (!wordsbb.empty())
			{
				if ((iend == imgnum) && (iend > 0)) iend -= 1;
				crn::StringUTF8 sig;
				for (size_t is = istart; is <= iend; ++is)
					sig += img[is].code;
				wordimgsig.push_back(sig);
				std::cout << "<" << sig << ">" << " ";
			}

			if (!wordsbb.empty())
			{ // make sure the previous word ends just before this words begins
				if (imgnum > 0)
					if (img[imgnum - 1].bbox.IsValid())
						wordsbb.back() |= img[imgnum - 1].bbox;
			}
			wordsbb.push_back(img[imgnum].bbox);
			istart = iend = imgnum;
		}
		else
		{ // add to existing word
			iend = imgnum;
			if (!wordsbb.back().IsValid())
			{ // the first bbox of the word was invalid
				// TODO is it possible???
				wordsbb.back() = img[imgnum].bbox;
			}
			else if (img[imgnum].bbox.IsValid())
			{ // append
				bool ok = true;
				// check if the same image elements repeats until (and including) a new word
				for (size_t nextt = t + 1; nextt < s2 + 1; ++ nextt)
				{
					size_t nextimgnum = path[nextt]; if (nextimgnum > 0) nextimgnum -= 1;
					if (nextimgnum != imgnum)
						break;
					if (txt[nextt - 1].start)
					{
						ok = false;
						break;
					}
				}
				if (ok)
					wordsbb.back() |= img[imgnum].bbox;
			}
		}
	}
	wordsbb.back() |= img.back().bbox; // not sure if it's necessary
	// end last word
	crn::StringUTF8 sig;
	for (size_t is = istart; is < img.size(); ++is)
		sig += img[is].code;
	wordimgsig.push_back(sig);
	std::cout << "<" << sig << ">" << std::endl;

	// set bounding boxes
	size_t bb = 0;
	for (size_t tmp = 0; tmp < words.size(); ++tmp)
	{
		if (bb >= wordsbb.size())
		{
			std::cout << "not enough boxes! " << words[tmp].GetText() << std::endl;
			std::cout << "\t" << words.size() << " got " << wordsbb.size() << std::endl;
			break;
		}
		if (words[tmp].GetText().IsNotEmpty()) // XXX TODO not enough, a non-empty word might have no signature
		{
			words[tmp].SetImageSignature(wordimgsig[bb]);
			if (words[tmp].GetBBox() != wordsbb[bb])
			{ // if there is an update, reset validation
				words[tmp].SetValid(crn::Prop3::Unknown);
			}
			words[tmp].SetBBox(wordsbb[bb]); // call even if the bbox is the same in order to reset the left/right corrections
			bb += 1;
		}
	}
}
#endif

/*
void Line::Align(const std::vector<std::pair<crn::Rect, crn::StringUTF8> > &sig)
{
	// set bounding boxes
	size_t bb = 0;
	for (size_t tmp = 0; tmp < words.size(); ++tmp)
	{
		if (bb >= sig.size())
		{
			std::cout << "not enough boxes! " << words[tmp].GetText() << std::endl;
			std::cout << "\t" << words.size() << " got " << sig.size() << std::endl;
			break;
		}
		if (words[tmp].GetText().IsNotEmpty()) // XXX TODO not enough, a non-empty word might have no signature
		{
			words[tmp].SetImageSignature(sig[bb].second);
			if (words[tmp].GetBBox() != sig[bb].first)
			{ // if there is an update, reset validation
				words[tmp].SetValid(crn::Prop3::Unknown);
			}
			words[tmp].SetBBox(sig[bb].first); // call even if the bbox is the same in order to reset the left/right corrections
			bb += 1;
		}
	}
}
*/

#if 0
const std::vector<TextSignature> Line::featurize() const
{
	static std::map<crn::Char, crn::StringUTF8> feat;
	if (feat.empty())
	{ // load feature set
		crn::xml::Document doc(ori::Config::GetInstance().GetStaticDataDir() + crn::Path::Separator() + "orisig.xml");
		crn::xml::Element root(doc.GetRoot());
		for (crn::xml::Element el = root.BeginElement(); el != root.EndElement(); ++el)
		{
			feat.insert(std::make_pair(crn::Char(el.GetAttribute<int>("char")), el.GetAttribute<crn::StringUTF8>("sig")));
		}
	}

	std::vector<TextSignature> sig;
	for (size_t wnum = 0; wnum < words.size(); ++wnum)
	{
		crn::String t(words[wnum].GetText());
		if (t.IsEmpty())
		{
			CRNWarning(_("Empty word."));
			continue;
		}
		crn::StringUTF8 f;
		for (size_t tmp = 0; tmp < t.Size(); ++tmp)
			f += feat[t[tmp]];
		if (f.IsEmpty())
		{
			CRNWarning(_("Word has no signature: <") + t + ">");
			continue;
		}
		sig.push_back(TextSignature(true, f[0]));
		for (size_t tmp = 1; tmp < f.Size(); ++tmp)
			sig.push_back(TextSignature(false, f[tmp]));
		/*
		// first character with a non-null signature
		size_t c = 0;
		crn::StringUTF8 f;
		while (f.IsEmpty() && (c < t.Size()))
			f = feat[t[c++]];
		if (c >= t.Size())
			continue;
		sig.push_back(TextSignature(true, f[0]));
		for (size_t tmp = 1; tmp < f.Size(); ++tmp)
			sig.push_back(TextSignature(false, f[tmp]));
		// other characters
		for (; c < t.Size(); ++c)
		{
			crn::StringUTF8 f(feat[t[c]]);
			for (size_t tmp = 0; tmp < f.Size(); ++tmp)
				sig.push_back(TextSignature(false, f[tmp]));
		}
		*/
		// add space
		/* TODO uncomment when the spaces will be added to the image signature!
		if (wnum != words.size() - 1)
			sig.push_back(TextSignature(false, feat[' '][0]));
			*/
	}
	/*
	bool start = true;
	for (size_t tmp = 0; tmp < line.Length(); ++tmp)
	{
		crn::StringUTF8 f(feat[line[tmp]]);
		for (size_t c = 0; c < f.Size(); ++c)
		{
			sig.push_back(TextSignature(start, f[c]));
			start = false;
		}
		if (line[tmp] == L' ')
			start = true;
	}
	*/
	return sig;
	
}
#endif

void Column::Cleanup()
{
	//lines.erase(std::remove_if(lines.begin(), lines.end(), std::mem_fun_ref(&Line::IsEmpty)), lines.end());

	std::vector<Line> oklines;
	std::vector<Line> wrap;
	CRN_FOREACH(const Line &l, lines)
	{
		if (!l.IsEmpty())
		{
			if (l.GetNum().IsEmpty())
			{ // no line number
				oklines.push_back(l);
			}
			else
			{ // check if it need to be appended
				bool found = false;
				CRN_FOREACH(Line &ol, oklines)
				{
					if (ol.GetNum() == l.GetNum())
					{ // found --> append
						found = true;
						ol.Append(l);
						break;
					}
				}
				if (!found)
				{
					if (l.IsWrapping())
						wrap.push_back(l);
					else
						oklines.push_back(l);
				}
			}
		}
	}

	// append wrapping line fragments to the real lines
	CRN_FOREACH(const Line &wl, wrap)
	{
		bool found = false;
		CRN_FOREACH(Line &l, oklines)
		{
			if (l.GetNum() == wl.GetNum())
			{
				found = true;
				l.Append(wl);
				break;
			}
		}
		if (!found)
		{ // assume it's a overnumerous line
			oklines.push_back(wl);
		}
	}
	oklines.swap(lines);
}

void View::Cleanup()
{
	std::for_each(columns.begin(), columns.end(), std::mem_fun_ref(&Column::Cleanup));
	columns.erase(std::remove_if(columns.begin(), columns.end(), std::mem_fun_ref(&Column::IsEmpty)), columns.end());
}

void Document::Cleanup()
{
	std::for_each(views.begin(), views.end(), std::mem_fun_ref(&View::Cleanup));
	views.erase(std::remove_if(views.begin(), views.end(), std::mem_fun_ref(&View::IsEmpty)), views.end());
}

void Document::AppendView(const crn::StringUTF8 &fname, const crn::StringUTF8 &i) 
{ 
	views.push_back(View(fname, i));
}

void Document::AppendColumn(const crn::StringUTF8 &i, const crn::StringUTF8 &n) 
{ 
	if (views.empty())
		throw crn::ExceptionRuntime(_("No view to add column id:") + i + " #" + n);
	views.back().columns.push_back(Column(i, n)); 
}

void Document::AppendLine(const crn::StringUTF8 &n, bool wrap)
{ 
	if (views.empty())
		throw crn::ExceptionRuntime(_("No view to add line #") + n);
	if (views.back().columns.empty())
		AppendColumn("", "");
	views.back().columns.back().lines.push_back(Line(n, wrap));
}

void Document::AppendWord(const crn::StringUTF8 &word_id, const crn::String &content) 
{ 
	if (views.empty())
		throw crn::ExceptionRuntime(_("No view to add word id:") + word_id + " (" + content.CStr() + ")");
	if (views.back().columns.empty())
		AppendColumn("", "");
	if (views.back().columns.back().lines.empty())
		AppendLine("");
	views.back().columns.back().lines.back().words.push_back(Word(word_id, content));
}

static const crn::StringUTF8 remove_ent(const crn::StringUTF8 &s)
{
	//std::cout << "remove ent :" << s << std::flush;
	static std::map<crn::StringUTF8, crn::StringUTF8> conv;
	if (conv.empty())
	{
		crn::xml::Document doc(ori::Config::GetInstance().GetStaticDataDir() + crn::Path::Separator() + "orient.xml");
		crn::xml::Element root(doc.GetRoot());
		for (crn::xml::Element el = root.BeginElement(); el != root.EndElement(); ++el)
		{
			conv.insert(std::make_pair(el.GetAttribute<crn::StringUTF8>("name"), el.GetFirstChildText()));
		}
	}

	//crn::StringUTF8 ns(s);
	crn::StringUTF8 ns;
	for (size_t tmp = 0; tmp < s.Size(); ++tmp)
	{
		if ((s[tmp] != ' ') && (s[tmp] != '\t') && (s[tmp] != '\n'))
			ns += s[tmp];
	}
	size_t b = 0;
	while ((b = ns.Find("&")) != crn::StringUTF8::NPos())
	{
		size_t e = ns.Find(";", b + 1);
		if (e != crn::StringUTF8::NPos())
		{
			crn::StringUTF8 ent(ns.SubString(b + 1, e - b - 1)); // trim & and ;
			if (conv.find(ent) == conv.end()) // XXX DEBUG
				std::cout << "missing entity " << ent << std::endl;
			ns.Replace(conv[ent], b, e - b + 1);
		}
		else // there is a & but no ;
			return ns;
	}
	//std::cout << " -> " << ns << std::endl;
	return ns;
}

static void read_tei(crn::xml::Element el, ori::Document &doc, const crn::StringUTF8 &last_id)
{
	crn::StringUTF8 lastid(last_id);
	crn::StringUTF8 name(el.GetName());
	if ((name == "front") || 
			(name == "note") || 
			(name == "teiHeader") || 
			(name == "listWit") || 
			(name == "listBibl") || 
			(name == "supplied"))
		return;
	if (name == "pb")
	{ // page break
		// XXX let's hope there is only one page per view!
		try
		{
			doc.AppendView(el.GetAttribute<crn::StringUTF8>("facs", false), el.GetAttribute<crn::StringUTF8>("xml:id", true)); // may throw
		}
		catch (crn::ExceptionNotFound&)
		{
			throw crn::ExceptionNotFound(_("A <pb> element does not contain a facs attribute."));
		}
		return;
	}
	if (name == "cb")
	{ // column break
		doc.AppendColumn(el.GetAttribute<crn::StringUTF8>("xml:id", true), el.GetAttribute<crn::StringUTF8>("n", true));
		return;
	}
	if (name == "lb")
	{ // line break
		if (el.GetAttribute<crn::StringUTF8>("ed", true) != "norm") // XXX should be unnecessary now
			doc.AppendLine(el.GetAttribute<crn::StringUTF8>("n", true), el.GetAttribute<crn::StringUTF8>("type", true) == "rejet");
		return;
	}
	if (name == "choice")
	{ // choice
		crn::xml::Element sic(el.GetFirstChildElement("sic"));
		if (sic)
		{ // inside a <sic>/<corr> choice. the <sic> element should contain a <w>
			read_tei(sic, doc, lastid);
			return;
		}
		crn::xml::Element sel(el.GetFirstChildElement("me:facs"));
		if (sel == el.EndElement())
			sel = el.GetFirstChildElement("me:dipl");
		if (sel == el.EndElement())
			sel = el.GetFirstChildElement("abbr");
		if (sel != el.EndElement())
		{ // found a transcription
			crn::StringUTF8 word;
			for (crn::xml::Node n = sel.BeginNode(); n != sel.EndNode(); ++n)
			{
				try
				{ // plain text
					crn::xml::Text t(n.AsText());
					word += remove_ent(t.GetValue());
					continue;
				}
				catch (crn::ExceptionDomain&) { }
				try
				{ // found an element in the facsimile transcription!
					crn::xml::Element e(n.AsElement());
					if (e.GetName() == "lb")
					{ // its a line break, add the word so far and a new line
						if (word.IsNotEmpty())
						{
							doc.AppendWord(lastid, word);
							word = "";
						}
						doc.AppendLine(el.GetAttribute<crn::StringUTF8>("n", true), el.GetAttribute<crn::StringUTF8>("type", true) == "rejet");
					}
					else if (e.GetName() == "ex")
					{
						// abbrev.
						try
						{
							const crn::StringUTF8 cont = e.GetFirstChildText();
							if (cont == "et")
								word += "";
							else if (cont == "con")
								word += "ꝯ";
							// else: ignore
						}
						catch (...) { }
					}
					else if ((e.GetName() == "hi") && (e.GetAttribute<crn::StringUTF8>("rend", true) == "initiale"))
					{ // it's a capital, do not add it
					}
					else
					{ // the element might contain text
						word += remove_ent(e.GetFirstChildText());
					}
				}
				catch (crn::ExceptionDomain&) { }
				catch (crn::ExceptionNotFound&) { }
			}
			// add the word
			if (word.IsNotEmpty())
				doc.AppendWord(lastid, word);
		}
		return;
	}
	if (name == "w")
	{ // found a word, store its id and continue to process
		lastid = el.GetAttribute<crn::StringUTF8>("xml:id");
	}
	for (crn::xml::Node n = el.BeginNode(); n != el.EndNode(); ++n)
	{
		try
		{ // found plain text, it's a word!
			crn::xml::Text t(n.AsText());
			doc.AppendWord(lastid, remove_ent(t.GetValue()));
			continue;
		}
		catch (crn::ExceptionDomain&) { }
		try
		{ // found an element, let's parse it
			crn::xml::Element sel(n.AsElement());
			read_tei(sel, doc, lastid);
		}
		catch (crn::ExceptionDomain&) { }
	}
}

void Document::Import(const crn::Path &fname)
{
	crn::xml::Document xdoc(fname); // may throw
	crn::xml::Element root = xdoc.GetRoot();
	if (root.GetName() != "TEI")
		throw crn::ExceptionInvalidArgument(_("Not a TEI document."));
	views.clear();
	read_tei(root, *this, "");
	teipath = fname;
	Cleanup();
}

void Document::Load(const crn::Path &fname)
{
	crn::xml::Document doc(fname); // may throw
	crn::xml::Element root(doc.GetRoot());
	teipath = root.GetAttribute<crn::StringUTF8>("teipath", false); // may throw;
	views.clear();
	crn::xml::Element vel = root.GetFirstChildElement("View");
	while (vel)
	{
		AppendView(vel.GetAttribute<crn::StringUTF8>("imagename", false), vel.GetAttribute<crn::StringUTF8>("id", true)); // should not throw
		crn::xml::Element cel = vel.GetFirstChildElement("Column");
		while (cel)
		{
			AppendColumn(cel.GetAttribute<crn::StringUTF8>("id", true), cel.GetAttribute<crn::StringUTF8>("num", true));
			crn::xml::Element lel = cel.GetFirstChildElement("Line");
			while (lel)
			{
				AppendLine(lel.GetAttribute<crn::StringUTF8>("num", true));
				bool corrected = false;
				try
				{
					corrected = lel.GetAttribute<bool>("corrected", false);
				}
				catch (...) { }
				views.back().columns.back().lines.back().SetCorrected(corrected);
				crn::xml::Element wel = lel.GetFirstChildElement("Word");
				while (wel)
				{
					AppendWord(wel.GetAttribute<crn::StringUTF8>("id"), wel.GetFirstChildText());
					try
					{
						int x1 = wel.GetAttribute<int>("x1", false);
						int x2 = wel.GetAttribute<int>("x2", false);
						int y1 = wel.GetAttribute<int>("y1", false);
						int y2 = wel.GetAttribute<int>("y2", false);
						views.back().columns.back().lines.back().words.back().SetBBox(crn::Rect(x1, y1, x2, y2));
					}
					catch (...) { }
					try
					{
						int v = wel.GetAttribute<int>("valid", false);
						views.back().columns.back().lines.back().words.back().SetValid(v);
					}
					catch (...) { } // default is already Unknwon
					views.back().columns.back().lines.back().words.back().SetImageSignature(wel.GetAttribute<crn::StringUTF8>("imgsig", true));
					int corr = 0;
					try
					{
						corr = wel.GetAttribute<int>("leftcorr", false);
					}
					catch (...) { }
					if (corr)
						views.back().columns.back().lines.back().words.back().setLeftCorrection(corr);
					corr = 0;
					try
					{
						corr = wel.GetAttribute<int>("rightcorr", false);
					}
					catch (...) { }
					if (corr)
						views.back().columns.back().lines.back().words.back().setRightCorrection(corr);
					wel = wel.GetNextSiblingElement("Word");
				}
				lel = lel.GetNextSiblingElement("Line");
			}
			cel = cel.GetNextSiblingElement("Column");
		}
		vel = vel.GetNextSiblingElement("View");
	}
}

void Document::Save(const crn::Path &fname, crn::Progress *prog) const
{
	if (prog)
		prog->SetMaxCount(int(views.size()) + 1);
	crn::xml::Document doc;
	crn::xml::Element root(doc.PushBackElement("oridoc"));
	root.SetAttribute("teipath", teipath);
	CRN_FOREACH(const View &v, views)
	{
		crn::xml::Element vel(root.PushBackElement("View"));
		vel.SetAttribute("imagename", v.imagename);
		vel.SetAttribute("id", v.GetId());
		CRN_FOREACH(const Column &c, v.GetColumns())
		{
			crn::xml::Element cel(vel.PushBackElement("Column"));
			cel.SetAttribute("id", c.GetId());
			cel.SetAttribute("num", c.GetNumber());
			CRN_FOREACH(const Line &l, c.GetLines())
			{
				crn::xml::Element lel(cel.PushBackElement("Line"));
				lel.SetAttribute("num", l.GetNumber());
				lel.SetAttribute("corrected", l.GetCorrected());
				CRN_FOREACH(const Word &w, l.GetWords())
				{
					crn::xml::Element wel(lel.PushBackElement("Word"));
					wel.SetAttribute("id", w.GetId());
					if (w.GetBBox().IsValid())
					{
						wel.SetAttribute("x1", w.GetBBox().GetLeft());
						wel.SetAttribute("y1", w.GetBBox().GetTop());
						wel.SetAttribute("x2", w.GetBBox().GetRight());
						wel.SetAttribute("y2", w.GetBBox().GetBottom());
					}
					wel.SetAttribute("valid", w.GetValid().GetValue());
					if (w.GetImageSignature().IsNotEmpty())
						wel.SetAttribute("imgsig", w.GetImageSignature());
					wel.SetAttribute("leftcorr", w.GetLeftCorrection());
					wel.SetAttribute("rightcorr", w.GetRightCorrection());
					wel.PushBackText(crn::StringUTF8(w.GetText()), true);
				}
			}
		}
		if (prog)
			prog->Advance();
	}
	doc.Save(fname);
	if (prog)
		prog->Advance();
}

