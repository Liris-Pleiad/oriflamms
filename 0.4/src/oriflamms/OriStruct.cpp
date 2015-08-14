/* Copyright 2013-2014 INSA-Lyon, IRHT, ZHAO Xiaojuan, Université Paris Descartes
 *
 * file: OriStruct.cpp
 * \author Yann LEYDIER
 */

#include <OriStruct.h>
#include <OriConfig.h>
#include <CRNUtils/CRNXml.h>
#include <OriTEIImporter.h>
#include <OriEntityManager.h>
#include <CRNi18n.h>
#include <iostream>

using namespace ori;

WordPath::WordPath(const crn::String &s)
{
	std::vector<crn::String> str(s.Split(U" "));
	if (str.size() != 4)
		throw crn::ExceptionInvalidArgument(_("Malformed word path."));
	view = str[0].ToInt();
	col = str[1].ToInt();
	line = str[2].ToInt();
	word = str[3].ToInt();
}

crn::String WordPath::ToString() const
{
	crn::String s;
	s += view;
	s += U' ';
	s += col;
	s += U' ';
	s += line;
	s += U' ';
	s += word;
	return s;
}

/*! Removes all word and character alignment info */
void Word::ClearAlignment()
{
	bbox = crn::Rect{};
	ClearFrontiers();
	leftcorr = rightcorr = 0;
}

const std::vector<crn::Point2DInt>& Word::GetCharacterFront(size_t i) const
{
	if (!CharactersAligned())
		throw crn::ExceptionUninitialized(crn::StringUTF8("const std::vector<crn::Point2DInt>& Word::GetCharacterFront(size_t i) const: ") + _("character frontiers were not computed."));
	if (i >= text.Size())
		throw crn::ExceptionDomain(crn::StringUTF8("const std::vector<crn::Point2DInt>& Word::GetCharacterFront(size_t i) const: ") + _("index out of bounds."));
	size_t realindex = 0;
	for (size_t tmp = 0; tmp <= i; ++tmp)
		if (ignore_list.find(tmp) == ignore_list.end())
			realindex += 1;
	if (realindex < 2)
		return frontfrontier;
	else
		return frontiers[realindex - 2];
}

const std::vector<crn::Point2DInt>& Word::GetCharacterBack(size_t i) const
{
	if (!CharactersAligned())
		throw crn::ExceptionUninitialized(crn::StringUTF8("const std::vector<crn::Point2DInt>& Word::GetCharacterBack(size_t i) const: ") + _("character frontiers were not computed."));
	if (i >= text.Size())
		throw crn::ExceptionDomain(crn::StringUTF8("const std::vector<crn::Point2DInt>& Word::GetCharacterBack(size_t i) const: ") + _("index out of bounds."));
	size_t realindex = 0;
	for (size_t tmp = 0; tmp <= i; ++tmp)
		if (ignore_list.find(tmp) == ignore_list.end())
			realindex += 1;
	if (realindex == 0)
	{
		if (frontiers.empty())
			return backfrontier; 
		else
			return frontiers.front();
	}
	else
		realindex -= 1;
	if (realindex >= frontiers.size())
		return backfrontier;
	else
		return frontiers[realindex];
}

std::vector<Word::Character> Word::GetCharacters() const
{
	if (!CharactersAligned())
		throw crn::ExceptionUninitialized(crn::StringUTF8("std::vector<Character> Word::GetCharacters() const: ") + _("character frontiers were not computed."));

	std::vector<Character> chars;
	for (size_t tmp = 0; tmp < text.Size(); ++tmp)
	{
		if (ignore_list.find(tmp) != ignore_list.end())
			continue;
		size_t e = tmp;
		for (; e < text.Size() - 1; ++e)
			if (ignore_list.find(e + 1) == ignore_list.end())
				break;
		chars.emplace_back(text.SubString(tmp, e - tmp + 1), GetCharacterFront(tmp), GetCharacterBack(e));
		tmp = e;
	}
	return chars;
}

bool Word::IsAligned() const
{
	return GetBBox().IsValid();
}

bool Word::CharactersAligned() const
{
	if (!IsAligned())
		return false; // word not aligned
	if (!frontiers.empty())
		return true; // there are character frontiers
	// no characters frontiers, count characters
	auto cnt = 0;
	for (auto tmp : crn::Range(text))
		if (ignore_list.find(tmp) == ignore_list.end())
			cnt += 1;
	if (cnt <= 1)
		return true; // one non-ignored character => no need of characters frontiers
	else
		return false; // more than one character => they were not aligned
}

void Line::Append(Line &other)
{
	std::move(other.words.begin(), other.words.end(), std::back_inserter(words));
}

void Column::Cleanup()
{
	std::vector<Line> oklines;
	std::vector<Line> wrap;
	for (Line &l : lines)
	{
		if (!l.IsEmpty())
		{
			if (l.GetNumber().IsEmpty())
			{
				// no line number
				oklines.emplace_back(std::move(l));
			}
			else
			{
				// check if it need to be appended
				bool found = false;
				for (Line &ol : oklines)
				{
					if (ol.GetNumber() == l.GetNumber())
					{
						// found --> append
						found = true;
						ol.Append(l);
						break;
					}
				}
				if (!found)
				{
					if (l.IsWrapping())
						wrap.emplace_back(std::move(l));
					else
						oklines.emplace_back(std::move(l));
				}
			}
		}
	}

	// append wrapping line fragments to the real lines
	for (Line &wl : wrap)
	{
		bool found = false;
		for (Line &l : oklines)
		{
			if (l.GetNumber() == wl.GetNumber())
			{
				found = true;
				l.Append(wl);
				break;
			}
		}
		if (!found)
		{
			// assume it's a overnumerous line
			oklines.emplace_back(std::move(wl));
		}
	}
	oklines.swap(lines);

	lines.erase(std::remove_if(lines.begin(), lines.end(), std::mem_fun_ref(&Line::IsEmpty)), lines.end());
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
	if (fname.IsEmpty())
	{
		AppendColumn(int(views.back().columns.size()) + 1, int(views.back().columns.size()) + 1);
		return;
	}
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

void Document::read_tei(crn::xml::Element &el, const TEISelectionNode &sel, crn::StringUTF8 &lastwordid, crn::StringUTF8 &txt)
{
	for (crn::xml::Node n = el.BeginNode(); n != el.EndNode(); ++n)
	{
		if (n.IsText())
			txt += n.AsText().GetValue();
		else if (n.IsElement())
		{
			crn::xml::Element cel = n.AsElement();
			const crn::StringUTF8 elname(cel.GetName());
			for (const TEISelectionNode &cnode : sel.GetChildren())
			{
				if (cnode.GetName() == elname)
				{
					if (elname == "pb")
					{
						if (txt.IsNotEmpty())
							AppendWord(lastwordid, txt);
						txt = "";
						AppendView(cel.GetAttribute<crn::StringUTF8>("facs", true), cel.GetAttribute<crn::StringUTF8>("xml:id", true));
					}
					else if ((elname == "cb") && (cel.GetAttribute<crn::StringUTF8>("rend", true) != "hidden"))
					{
						if (txt.IsNotEmpty())
							AppendWord(lastwordid, txt);
						txt = "";
						AppendColumn(cel.GetAttribute<crn::StringUTF8>("xml:id", true), cel.GetAttribute<int>("n", true));
					}
					else if (elname == "lb")
					{
						if (txt.IsNotEmpty())
							AppendWord(lastwordid, txt);
						txt = "";
						AppendLine(cel.GetAttribute<int>("n", true), cel.GetAttribute<crn::StringUTF8>("type", true) == "rejet");
					}
					else if ((elname == "w") || (elname == "pc"))
					{
						if (txt.IsNotEmpty())
							AppendWord(lastwordid, txt);
						txt = "";
						lastwordid = cel.GetAttribute<crn::StringUTF8>("xml:id", true);
					}
					read_tei(cel, cnode, lastwordid, txt);
					break;
				} // found element in selection
			} // for each selection sub-node
		}
	} // for each xml sub-node
}

void Document::Import(const crn::Path &fname, const TEISelectionNode &sel)
{
	std::unique_ptr<crn::xml::Document> xdoc(EntityManager::ExpandXML(fname)); // may throw
	crn::xml::Element root = xdoc->GetRoot();
	if ((root.GetName() != "TEI") && (root.GetName() != "teiCorpus"))
		throw crn::ExceptionInvalidArgument(_("Not a TEI document."));
	views.clear();
	crn::StringUTF8 lastwordid, txt;
	read_tei(root, sel, lastwordid, txt);
	AppendWord(lastwordid, txt);
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
					crn::xml::Element trans(wel.GetFirstChildElement("Transcription"));
					if (!trans)
						throw crn::ExceptionNotFound(_("Malformed Oriflamms project file."));
					AppendWord(wel.GetAttribute<crn::StringUTF8>("id"),trans.GetFirstChildText());

					try
					{
						int x1 = wel.GetAttribute<int>("x1", false);
						int x2 = wel.GetAttribute<int>("x2", false);
						int y1 = wel.GetAttribute<int>("y1", false);
						int y2 = wel.GetAttribute<int>("y2", false);
						views.back().columns.back().lines.back().words.back().SetBBox(crn::Rect(x1, y1, x2, y2));
					}
					catch (...) { }

					int v = wel.GetAttribute<int>("valid", false); // might throw
					views.back().columns.back().lines.back().words.back().SetValid(v);

					views.back().columns.back().lines.back().words.back().SetImageSignature(wel.GetAttribute<crn::StringUTF8>("imgsig", true));

					views.back().columns.back().lines.back().words.back().setLeftCorrection(wel.GetAttribute<int>("leftcorr", true));
					views.back().columns.back().lines.back().words.back().setRightCorrection(wel.GetAttribute<int>("rightcorr", true));

					std::vector<crn::StringUTF8> chars(wel.GetAttribute<crn::StringUTF8>("ignore", true).Split(" "));
					std::set<size_t> cil;
					for (const auto &c : chars)
						cil.insert(c.ToInt());
					views.back().columns.back().lines.back().words.back().SetIgnoreList(std::move(cil));

					crn::xml::Element border(wel.GetFirstChildElement("Borders"));
					if (border)
					{
						crn::xml::Element front(border.GetFirstChildElement("Front"));
						std::map<int, crn::Point2DInt> frontier;
						for(crn::xml::Element onep = front.BeginElement(); onep != front.EndElement() ; ++onep)
							frontier.emplace(onep.GetAttribute<int>("n", false), crn::Point2DInt{onep.GetAttribute<int>("x", false), onep.GetAttribute<int>("y", false)});
						std::vector<crn::Point2DInt> ff;
						for (auto &p : frontier)
							ff.emplace_back(std::move(p.second));
						views.back().columns.back().lines.back().words.back().SetFrontFrontier(std::move(ff));
						frontier.clear();

						front = border.GetFirstChildElement("Back");
						for(crn::xml::Element onep = front.BeginElement(); onep != front.EndElement() ; ++onep)
							frontier.emplace(onep.GetAttribute<int>("n", false), crn::Point2DInt{onep.GetAttribute<int>("x", false), onep.GetAttribute<int>("y", false)});
						std::vector<crn::Point2DInt> bf;
						for (auto &p : frontier)
							bf.emplace_back(std::move(p.second));
						views.back().columns.back().lines.back().words.back().SetBackFrontier(std::move(bf));
					}

					crn::xml::Element fron = wel.GetFirstChildElement("Characters");
					if (fron)
					{
						std::map<int, std::vector<crn::Point2DInt>> frontiers;
						for(crn::xml::Element onef = fron.BeginElement(); onef != fron.EndElement(); ++onef)
						{
							std::map<int, crn::Point2DInt> frontier;
							for(crn::xml::Element onep = onef.BeginElement();onep != onef.EndElement() ; ++onep)
							{
								frontier.emplace(onep.GetAttribute<int>("n", false), crn::Point2DInt{onep.GetAttribute<int>("x", false), onep.GetAttribute<int>("y", false)});
							}
							std::vector<crn::Point2DInt> f;
							for (auto &p : frontier)
								f.emplace_back(std::move(p.second));
							frontiers.emplace(onef.GetAttribute<int>("n", false), std::move(f));
						}
						for (auto &f : frontiers)
							views.back().columns.back().lines.back().words.back().AddCharacterFrontier(std::move(f.second));
					}

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

	for (const View &v : views)
	{
		crn::xml::Element vel(root.PushBackElement("View"));
		vel.SetAttribute("imagename", v.imagename);
		vel.SetAttribute("id", v.GetId());

		for (const Column &c : v.GetColumns())
		{
			crn::xml::Element cel(vel.PushBackElement("Column"));
			cel.SetAttribute("id", c.GetId());
			cel.SetAttribute("num", c.GetNumber());

			for (const Line &l : c.GetLines())
			{
				crn::xml::Element lel(cel.PushBackElement("Line"));
				lel.SetAttribute("num", l.GetNumber());
				lel.SetAttribute("corrected", l.GetCorrected());

				for (const Word &w : l.GetWords())
				{
					crn::xml::Element wel(lel.PushBackElement("Word"));
					wel.SetAttribute("id", w.GetId());
					crn::xml::Element wtrans(wel.PushBackElement("Transcription"));
					wtrans.PushBackText(crn::StringUTF8(w.GetText()), true);

					if (w.GetBBox().IsValid())
					{
						wel.SetAttribute("x1", w.GetBBox().GetLeft());
						wel.SetAttribute("y1", w.GetBBox().GetTop());
						wel.SetAttribute("x2", w.GetBBox().GetRight());
						wel.SetAttribute("y2", w.GetBBox().GetBottom());
					} // word bounding box
					wel.SetAttribute("valid", w.GetValid().GetValue());
					if (w.GetImageSignature().IsNotEmpty())
						wel.SetAttribute("imgsig", w.GetImageSignature());
					if (w.GetLeftCorrection())
						wel.SetAttribute("leftcorr", w.GetLeftCorrection());
					if (w.GetRightCorrection())
						wel.SetAttribute("rightcorr", w.GetRightCorrection());

					if (!w.GetFrontFrontier().empty())
					{
						crn::xml::Element border(wel.PushBackElement("Borders"));
						crn::xml::Element frontfrontier(border.PushBackElement("Front"));
						for (size_t tmp = 0; tmp < w.GetFrontFrontier().size(); ++tmp)
						{
							crn::xml::Element point(frontfrontier.PushBackElement("Point"));
							point.SetAttribute("n", int(tmp));
							point.SetAttribute("x", w.GetFrontFrontier()[tmp].X);
							point.SetAttribute("y", w.GetFrontFrontier()[tmp].Y);
						}
						crn::xml::Element backfrontier(border.PushBackElement("Back"));
						for (size_t tmp = 0; tmp < w.GetBackFrontier().size(); ++tmp)
						{
							crn::xml::Element point(backfrontier.PushBackElement("Point"));
							point.SetAttribute("n", int(tmp));
							point.SetAttribute("x", w.GetBackFrontier()[tmp].X);
							point.SetAttribute("y", w.GetBackFrontier()[tmp].Y);
						}
					} // word frontiers

					if (!w.GetCharacterFrontiers().empty())
					{
						crn::xml::Element fron(wel.PushBackElement("Characters"));
						for (size_t i = 0; i < w.GetCharacterFrontiers().size(); ++i)
						{
							crn::xml::Element onefron(fron.PushBackElement("Frontier"));
							onefron.SetAttribute("n", int(i));
							for (size_t j = 0; j < w.GetCharacterFrontiers()[i].size(); ++j)
							{
								crn::xml::Element point(onefron.PushBackElement("Point"));
								point.SetAttribute("n", int(j));
								point.SetAttribute("x", w.GetCharacterFrontiers()[i][j].X);
								point.SetAttribute("y", w.GetCharacterFrontiers()[i][j].Y);
							}
						}
					} // characters frontiers
					
					if (!w.GetIgnoreList().empty())
					{
						auto it = w.GetIgnoreList().begin();
						crn::StringUTF8 iltxt{int(*it)};
						while (++it != w.GetIgnoreList().end())
						{
							iltxt += " ";
							iltxt += int(*it);
						}
						wel.SetAttribute("ignore", iltxt);
					}

				} // for each word
			} // for each line
		} // for each column
		if (prog)
			prog->Advance();
	} // for each view
	doc.Save(fname);
	if (prog)
		prog->Advance();
}



