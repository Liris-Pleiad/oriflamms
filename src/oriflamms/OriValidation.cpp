/* Copyright 2013-2015 INSA-Lyon, IRHT, ZHAO Xiaojuan
 *
 * file: OriValidation.h
 * \author Yann LEYDIER
 */

#include <OriValidation.h>
#include <GtkCRNProgressWindow.h>
#ifdef CRN_PF_MSVC
#	include <GdkCRNPixbuf.h>
#endif
#include <CRNAI/CRNIterativeClustering.h>
#include <CRNi18n.h>
#include <fstream>
#include <iostream>
using namespace ori;



static bool nullkey(GdkEventKey*)
{
	return true;
}
Validation::Validation(Gtk::Window &parent, Project &proj, bool batch_valid, bool use_clustering, const std::function<void(void)> &savefunc, const std::function<void(void)> &refreshfunc):
	project(proj),
	okwords(proj,_("Accepted"), proj.GetDoc()->GetFilenames(),true),
	kowords(proj,_("Rejected"), proj.GetDoc()->GetFilenames(),true),
	firstrun(true),
	needsave(false),
	batch(batch_valid),
	clustering(use_clustering),
	saveatclose(savefunc),
	refreshatclose(refreshfunc)
{
	set_title(_("Validation"));
	set_transient_for(parent);
	set_default_size(900, 700);
	maximize();

	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);
	vbox->show();
	add(*vbox);

	Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox);
	vbox->pack_start(*hbox, true, true, 2);
	hbox->show();

	store = Gtk::ListStore::create(columns);
	tv.append_column(_("Word"), columns.name);
	tv.append_column(_("Count"), columns.pop);
	tv.append_column(_("Length"), columns.size);
	Gtk::TreeViewColumn *tvc = tv.get_column(0);
	Gtk::CellRendererText *cr = dynamic_cast<Gtk::CellRendererText*>(tv.get_column_cell_renderer(0));
	if (tvc && cr)
	{
		tvc->set_sort_column(columns.name);
		tvc->add_attribute(cr->property_weight(), columns.weight);
	}
	tvc = tv.get_column(1);
	if (tvc)
	{
		tvc->set_sort_column(columns.pop);
	}
	tvc = tv.get_column(2);
	if (tvc)
	{
		tvc->set_sort_column(columns.size);
	}
	Gtk::ScrolledWindow *sw = Gtk::manage(new Gtk::ScrolledWindow);
	sw->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
	hbox->pack_start(*sw, false, true, 0);
	sw->show();
	sw->add(tv);
	tv.show();
	tv.signal_key_press_event().connect(sigc::ptr_fun(nullkey), false);
	tv.signal_key_release_event().connect(sigc::ptr_fun(nullkey), false);

	Gtk::HPaned *hp(Gtk::manage(new Gtk::HPaned));
	hbox->pack_start(*hp, true, true, 0);
	hp->show();
	hp->add1(okwords);
	okwords.signal_removed().connect(sigc::mem_fun(this, &Validation::on_remove_words));
	hp->add2(kowords);
	kowords.signal_removed().connect(sigc::mem_fun(this, &Validation::on_unremove_words));
	hp->set_position(500);

	Gtk::Alignment *al(Gtk::manage(new Gtk::Alignment(1.0)));
	vbox->pack_start(*al, false, false, 2);
	al->show();
	Gtk::Button *but(Gtk::manage(new Gtk::Button(Gtk::Stock::CLOSE)));
	al->add(*but);
	but->show();
	but->signal_clicked().connect(sigc::mem_fun(this, &Validation::on_close));
	signal_delete_event().connect(sigc::bind_return(sigc::hide<0>(sigc::mem_fun(this, &Validation::on_close)), false));

	GtkCRN::ProgressWindow prog(_("Gathering the words"), &parent, true);
	size_t id = prog.add_progress_bar("");
	prog.run(sigc::bind(sigc::mem_fun(this, &Validation::update_words), prog.get_crn_progress(id)));
	tv.get_selection()->set_mode(Gtk::SELECTION_SINGLE);
	tv.set_model(store);
	//tv.get_selection()->unselect_all();
	tv.get_selection()->signal_changed().connect(sigc::mem_fun(this, &Validation::tree_selection_changed));
}

void Validation::update_words(crn::Progress *prog)
{
	prog->SetMaxCount((int)project.GetNbViews());
	std::set<crn::String> kowords;
	for (size_t v = 0; v < project.GetNbViews(); ++v)
	{
		View &page(project.GetStructure().GetViews()[v]);
		for (size_t c = 0; c < page.GetColumns().size(); ++c)
		{
			Column &col(page.GetColumns()[c]);
			for (size_t l = 0; l < col.GetLines().size(); ++l)
			{
				Line &line(col.GetLines()[l]);
				for (size_t w = 0; w < line.GetWords().size(); ++w)
				{
					Word &word(line.GetWords()[w]);
					words[word.GetText()].push_back(WordPath(v, c, l, w));
					if (word.GetValid().IsUnknown())
					{
						kowords.insert(word.GetText());
					}
				}
			}
		}
		prog->Advance();
	}

	store->clear();
	for (std::map<crn::String, std::vector<WordPath> >::const_iterator it = words.begin(); it != words.end(); ++it)
	{
		Gtk::TreeIter lit = store->append();
		lit->set_value(columns.name, Glib::ustring(it->first.CStr()));
		lit->set_value(columns.pop, it->second.size());
		lit->set_value(columns.size, it->first.Length());
		int w = Pango::WEIGHT_NORMAL;
		if (kowords.find(it->first) != kowords.end())
			w = Pango::WEIGHT_HEAVY;
		lit->set_value(columns.weight, w);
	}
}

void Validation::tree_selection_changed()
{
	/* TODO check if needed on WIN32
#ifdef CRN_PF_MSVC
if (firstrun)
{
firstrun = false;
return;
}
#endif
*/
	// concludes with what was displayed
	conclude_word();

	// update
	Gtk::TreeIter it(tv.get_selection()->get_selected());
	if (!it)
		return;
	Glib::ustring sname(it->get_value(columns.name));

	GtkCRN::ProgressWindow prog(_("Loading words"), this, true);
	size_t id = prog.add_progress_bar("Occurrence");
	prog.get_crn_progress(id)->SetType(crn::Progress::Type::ABSOLUTE);
	prog.run(sigc::bind(sigc::mem_fun(this, &Validation::read_word), sname, prog.get_crn_progress(id)));

	// set the word as visited
	it->set_value(columns.weight, int(Pango::WEIGHT_NORMAL));

	okwords.full_refresh();
	kowords.full_refresh();
}

void Validation::read_word(const Glib::ustring &wname, crn::Progress *prog)
{
	okwords.lock();
	kowords.lock();
	const std::vector<WordPath> &elems(words[wname.c_str()]);
	if (prog)
		prog->SetMaxCount(int(elems.size()));
	size_t lastview = std::numeric_limits<size_t>::max();
	Glib::RefPtr<Gdk::Pixbuf> pb;
	std::vector<WordPath> clustpath;
	std::vector<Glib::RefPtr<Gdk::Pixbuf> > clustimg;
	std::vector<std::vector<int> > clustproj;
	std::vector<crn::String> clustsig;
	for (const WordPath &wp : elems)
	{
		Word &oriword(project.GetStructure().GetWord(wp));
		if (oriword.GetBBox().IsValid())
		{

			if (lastview != wp.view)
			{
				pb = Gdk::Pixbuf::create_from_file(project.GetDoc()->GetFilenames()[wp.view].CStr());
				lastview = wp.view;
			}

			std::vector<crn::Point2DInt> fronfrontier = oriword.GetFrontFrontier();
			std::vector<crn::Point2DInt> backfrontier = oriword.GetBackFrontier();

			int max_x = backfrontier[0].X;
			int min_x = fronfrontier[0].X;

			int min_y = fronfrontier[0].Y;
			int max_y = fronfrontier.back().Y;

			for (size_t i = 0; i < backfrontier.size(); ++i)
			{
				if(backfrontier[i].X > max_x)
					max_x = backfrontier[i].X;
			}

			for (size_t i = 0; i < fronfrontier.size(); ++i)
			{
				if(fronfrontier[i].X < min_x)
					min_x = fronfrontier[i].X;
			}

			Glib::RefPtr<Gdk::Pixbuf> wpb(Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, max_x - min_x, max_y - min_y));;
			pb->copy_area(min_x, min_y, max_x - min_x, max_y - min_y, wpb, 0, 0);

			// add word
			if(!wpb->get_has_alpha())
				wpb = wpb->add_alpha(true,255,255,255);
			guint8* pixs = wpb->get_pixels();
			int rowstrides = wpb->get_rowstride();
			int channels = wpb->get_n_channels();

			for(size_t j = 0; j < wpb->get_height(); ++ j)
			{
				int x = 0;
				for(size_t i = 0; i < fronfrontier.size() - 1; ++ i)
				{
					double x1 = double(fronfrontier[i].X - min_x);
					double y1 = double(fronfrontier[i].Y - min_y);
					double x2 = double(fronfrontier[i + 1].X - min_x);
					double y2 = double(fronfrontier[i + 1].Y - min_y);

					if (double(j) == y2)
					{
						x = int(x2);
						//   pixs[(int(x) * channels + j * rowstrides )+3] = 128;
					}
					if (double(j) == y1)
					{
						x = int(x1);
						//  pixs[(int(x) * channels + j * rowstrides )+3] = 128;
					}
					if ((double(j) < y2) && (double(j) > y1))
					{
						x = int(x1 + (x2 - x1) * ((double(j) - y1)/(y2 - y1)));
					}

				}
				for(int k = 0; k <= x; ++k)
					pixs[(k * channels + j * rowstrides )+3] = 0;
			}
			for(size_t j = 0; j < wpb->get_height(); ++ j)
			{
				int x;
				for(size_t i = 0; i < backfrontier.size() - 1; ++ i)
				{
					double x1 = double(backfrontier[i].X - oriword.GetBBox().GetLeft());
					double y1 = double(backfrontier[i].Y - oriword.GetBBox().GetTop());
					double x2 = double(backfrontier[i + 1].X - oriword.GetBBox().GetLeft());
					double y2 = double(backfrontier[i + 1].Y - oriword.GetBBox().GetTop());
					if (double(j) == y2)
					{
						x = int(x2);

					}
					if (double(j) == y1)
					{
						x = int(x1);

					}
					if ((double(j) < y2) && (double(j) > y1))
					{
						x = int(x1 + (x2 - x1) * ((double(j) - y1)/(y2 - y1)));
					}
				}
				for(int k = x; k < wpb->get_width(); ++k)
					pixs[(k * channels + j * rowstrides )+3] = 0;
			}
			if (oriword.GetValid().IsFalse())
			{
				// add to reject list
				kowords.add_element(wpb, kowords.label_ko, wp);
			}
			else
			{
				if (oriword.GetValid().IsTrue())
					okwords.add_element(wpb, okwords.label_ok, wp);

				else
				{
					if (clustering)
					{
						clustpath.push_back(wp);
						clustimg.push_back(wpb);
						clustsig.push_back(oriword.GetImageSignature());
						/*
							 clustproj.push_back(std::vector<int>(wpb->get_width()));
							 for (int x = 0; x < wpb->get_width(); ++x)
							 {
							 int sum = 0;
							 for (int y = 0; y < wpb->get_height(); ++y)
							 {
							 int offset = x * wpb->get_n_channels() + y * wpb->get_rowstride();
							 sum += wpb->get_pixels()[offset] + wpb->get_pixels()[offset + 1] + wpb->get_pixels()[offset + 2];
							 }
							 clustproj.back()[x] = sum;
							 }
							 */
					}
					else
					{
						okwords.add_element(wpb, okwords.label_unknown, wp);
					}
				}
			}
		}
		if (prog)
			prog->Advance();
	}
	if (clustering)
	{
		// cluster the unvalidated words
		/*
			 if (prog)
			 {
			 prog->SetMaxCount(int(crn::Sqr(clustpath.size()) - clustpath.size()) / 2);
			 prog->SetName(_("Clustering…"));
			 prog->SetType(crn::Progress::PERCENT);
			 }
		// distance matrix
		std::vector<std::vector<int> > distmat(clustpath.size(), std::vector<int>(clustpath.size(), 0));
		for (size_t i = 0; i < clustpath.size(); ++i)
		{
		for (size_t j = i + 1; j < clustpath.size(); ++j)
		{
		size_t minw = crn::Min(clustproj[i].size(), clustproj[j].size());
		int dist = crn::Max(clustimg[i]->get_height(), clustimg[j]->get_height()) * int(crn::Max(clustproj[i].size(), clustproj[j].size()) - minw);
		for (size_t tmp = 0; tmp < minw; ++tmp)
		dist += crn::Abs(clustproj[i][tmp] - clustproj[j][tmp]);
		distmat[i][j] = distmat[j][i] = dist;
		if (prog)
		prog->Advance();
		}
		}
		// threshold
		std::ofstream dm("distmat.csv");
		std::multiset<double> meandists;
		for (size_t i = 0; i < distmat.size(); ++i)
		{
		for (size_t j = 0; j < distmat.size(); ++j)
		dm << distmat[i][j] << " ";
		dm << std::endl;

		std::vector<int> li(distmat[i]);
		std::sort(li.begin(), li.end());
		double md = 0;
		md = li[crn::Min(li.size() - 1, size_t(5))];
		meandists.insert(md);
		}
		std::multiset<double>::iterator meandistsit(meandists.begin());
		std::advance(meandistsit, meandists.size() / 5);
		int s = int(*meandistsit);
		std::cout << s << std::endl;
		// clustering
		crn::IterativeClustering<size_t> clust;
		for (size_t i = 0; i < distmat.size(); ++i)
		for (size_t j = i; j < distmat.size(); ++j)
		if (distmat[i][j] < s)
		clust.Associate(i, j);
		// add to panel
		int cnt = 1;
		CRN_FOREACH(const std::set<size_t> &c, clust.GetClusters())
		{
		crn::StringUTF8 lab(cnt++);
		CRN_FOREACH(size_t i, c)
		okwords.add_element(lab, clustpath[i], clustimg[i]);
		}
		*/
		crn::IterativeClustering<size_t> clust;
		for (size_t i = 0; i < clustsig.size(); ++i)
			for (size_t j = i; j < clustsig.size(); ++j)
				if (clustsig[i].Distance(clustsig[j]) < 2)
					clust.Associate(i, j);

		std::vector<std::set<size_t>> cl = clust.GetClusters();
		std::set<size_t> newclust;

		std::cout<<clust.GetClusters().size()<<std::endl;
		if(cl.size() > 1)
		{
			for( size_t i = 0; i < cl.size(); ++i)
			{
				if(cl[i].size() == 1)
				{
					newclust.insert(cl[i].begin(),cl[i].end());
					cl.erase(cl.begin() + i);
					i = i - 1;
				}
			}

			cl.push_back(newclust);

		}


		// add to panel
		int cnt = 1;
		for (const std::set<size_t> &c : cl)
		{
			crn::StringUTF8 lab(cnt++);

			for (size_t i : c)
			{
				okwords.add_element(clustimg[i], lab, clustpath[i]);
			}

		}
	}
	okwords.unlock();
	kowords.unlock();
}

void Validation::on_remove_words(ValidationPanel::ElementList words)
{
	for (const ValidationPanel::ElementList::value_type &el : words)
	{
		for (const ValidationPanel::ElementCluster::value_type &w : el.second)
		{
			project.GetStructure().GetWord(w.first.path).SetValid(FALSE);
			kowords.add_element(w.second, kowords.label_ko, w.first.path);
			// kowords.add_element("ddd", wp, wpb);
		}
	}
	needsave = true;
	kowords.full_refresh();
}

void Validation::on_unremove_words(ValidationPanel::ElementList words)
{
	for (const ValidationPanel::ElementList::value_type &el : words)
	{
		for (const ValidationPanel::ElementCluster::value_type &w : el.second)
		{
			project.GetStructure().GetWord(w.first.path).SetValid(TRUE);
			okwords.add_element(w.second, okwords.label_revalidated, w.first.path);
		}
	}
	needsave = true;
	okwords.full_refresh();
}

void Validation::on_close()
{
	// concludes with what was displayed
	conclude_word();

	if (!needconfirm.empty())
	{
		// some words were visited but not modified
		Gtk::MessageDialog msg(*this, _("You visited some words without performing any modification.\nDo you wish to validate them?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
		if (msg.run() == Gtk::RESPONSE_YES)
		{
			// validate
			for (const WordPath &w : needconfirm)
			{
				project.GetStructure().GetWord(w).SetValid(TRUE);
			}
			needsave = true;
		}
	}
	okwords.hide_tooltip();
	kowords.hide_tooltip();
	hide();

	// wait for the window to be hidden
	while (Gtk::Main::events_pending())
		Gtk::Main::iteration(false);
	if (needsave)
	{
		saveatclose();
		refreshatclose();
	}
}

void Validation::conclude_word()
{
	const std::set<ValidationPanel::ElementId> validcontent(okwords.GetContent());
	if (okwords.IsModified())
	{
		// validate
		for (const auto &w : validcontent)
		{
			project.GetStructure().GetWord(w.path).SetValid(TRUE);
		}
		needsave = true;
	}
	else
	{
		if (batch)
		{
			// store for later
			for (const auto &w : validcontent)
				needconfirm.insert(w.path);
		}
		else
		{
			// check is some words need to be validated
			bool need_ask = false;
			for (const auto &w : validcontent)
			{
				if (project.GetStructure().GetWord(w.path).GetValid().IsUnknown())
				{
					need_ask = true;
					break;
				}
			}
			if (need_ask)
			{
				// ask and validate if needed
				Gtk::MessageDialog msg(*this, _("No modification was performed on this word.\nDo you wish to validate current state?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
				if (msg.run() == Gtk::RESPONSE_YES)
				{
					// validate
					for (const auto &w : validcontent)
					{
						project.GetStructure().GetWord(w.path).SetValid(TRUE);
					}
				} // if response = yes
			} // if needed to ask for validation
		} // not in batch mode (= ask every time)
	} // no modification was made

	project.PropagateValidation();

	okwords.clear();
	kowords.clear();
}

