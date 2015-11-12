/* Copyright 2013-2015 INSA-Lyon, IRHT, ZHAO Xiaojuan, Université Paris Descartes
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

Validation::Validation(Gtk::Window &parent, Document &docu, bool batch_valid, bool use_clustering, const std::function<void(void)> &savefunc, const std::function<void(void)> &refreshfunc):
	doc(docu),
	okwords(docu,_("Accepted"), true),
	kowords(docu,_("Rejected"), true),
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
	prog->SetMaxCount(int(doc.GetViews().size()));
	auto kowords = std::set<crn::StringUTF8>{};
	for (const auto &vid : doc.GetViews())
	{
		auto view = doc.GetView(vid);
		for (const auto &w : view.GetWords())
		{
			words[w.second.GetText()].push_back(w.first);
			if (view.IsValid(w.first).IsUnknown())
				kowords.insert(w.second.GetText());
		}
		prog->Advance();
	}

	store->clear();
	for (const auto &w : words)
	{
		Gtk::TreeIter lit = store->append();
		lit->set_value(columns.name, Glib::ustring(w.first.CStr()));
		lit->set_value(columns.pop, w.second.size());
		lit->set_value(columns.size, w.first.Length());
		int we = Pango::WEIGHT_NORMAL;
		if (kowords.find(w.first) != kowords.end())
			we = Pango::WEIGHT_HEAVY;
		lit->set_value(columns.weight, we);
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
	auto id = prog.add_progress_bar("Occurrence");
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
	const auto &elems = words[wname.c_str()];
	if (prog)
		prog->SetMaxCount(int(elems.size()));
	auto lastview = Id{};
	auto view = View{};
	Glib::RefPtr<Gdk::Pixbuf> pb;
	std::vector<Id> clustpath;
	std::vector<Glib::RefPtr<Gdk::Pixbuf>> clustimg;
	std::vector<std::vector<int>> clustproj;
	std::vector<crn::String> clustsig;
	for (const auto &wp : elems)
	{
		const auto vid = doc.GetPosition(wp).view;
		if (vid != lastview)
		{
			view = doc.GetView(vid);
			lastview = vid;
			pb = Gdk::Pixbuf::create_from_file(view.GetImageName().CStr());
		}

		const auto &oriword = view.GetWord(wp);
		const auto &wzone = view.GetZone(oriword.GetZone());
		const auto &bbox = wzone.GetPosition();
		if (bbox.IsValid())
		{
			auto contour = wzone.GetContour();
			auto frontier = size_t(0);
			if (contour.empty())
			{
				contour.emplace_back(bbox.GetLeft(), bbox.GetTop());
				contour.emplace_back(bbox.GetLeft(), bbox.GetBottom());
				frontier = 2;
				contour.emplace_back(bbox.GetRight(), bbox.GetTop());
				contour.emplace_back(bbox.GetRight(), bbox.GetBottom());
			}
			else
			{
				for (; frontier < contour.size() - 1; ++frontier)
					if (contour[frontier + 1].Y < contour[frontier].Y)
						break;
			}

			auto max_x = contour[frontier].X;
			for (auto i = frontier; i < contour.size(); ++i)
				if (contour[i].X > max_x)
					max_x = contour[i].X;

			auto min_x = contour.front().X;
			for (auto i = size_t(0); i < frontier; ++i)
				if (contour[i].X < min_x)
					min_x = contour[i].X;

			const auto min_y = contour.front().Y;
			const auto max_y = contour[frontier].Y;

			Glib::RefPtr<Gdk::Pixbuf> wpb(Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, max_x - min_x, max_y - min_y));;
			pb->copy_area(min_x, min_y, max_x - min_x, max_y - min_y, wpb, 0, 0);

			// add word
			if (!wpb->get_has_alpha())
				wpb = wpb->add_alpha(true,255,255,255);
			auto* pixs = wpb->get_pixels();
			const auto rowstrides = wpb->get_rowstride();
			const auto channels = wpb->get_n_channels();

			for (auto j = size_t(0); j < wpb->get_height(); ++j)
			{
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
					pixs[k * channels + j * rowstrides + 3] = 0;
			}
			for (auto j = size_t (0); j < wpb->get_height(); ++j)
			{
				int x;
				for(auto i = frontier; i < contour.size() - 1; ++i)
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
				for(int k = x; k < wpb->get_width(); ++k)
					pixs[k * channels + j * rowstrides + 3] = 0;
			}
			if (view.IsValid(wp).IsFalse())
			{
				// add to reject list
				kowords.add_element(wpb, kowords.label_ko, wp);
			}
			else
			{
				if (view.IsValid(wp).IsTrue())
					okwords.add_element(wpb, okwords.label_ok, wp);

				else
				{
					if (clustering)
					{
						clustpath.push_back(wp);
						clustimg.push_back(wpb);
						clustsig.push_back(""/*oriword.GetImageSignature()*/); // TODO
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
		crn::IterativeClustering<size_t> clust;
		for (size_t i = 0; i < clustsig.size(); ++i)
			for (size_t j = i; j < clustsig.size(); ++j)
				if (clustsig[i].Distance(clustsig[j]) < 2)
					clust.Associate(i, j);

		std::vector<std::set<size_t>> cl = clust.GetClusters();
		std::set<size_t> newclust;

		//std::cout<<clust.GetClusters().size()<<std::endl;
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
	auto work = std::unordered_map<Id, std::vector<ValidationPanel::ElementCluster::value_type>>{};
	for (const auto &el : words)
		for (const auto &w : el.second)
			work[doc.GetPosition(w.first.id).view].push_back(w);

	for (const auto &v : work)
	{
		auto view = doc.GetView(v.first);
		for (const auto &w : v.second)
		{
			view.SetValid(w.first.id, false);
			kowords.add_element(w.second, kowords.label_ko, w.first.id);
		}
	}

	needsave = true;
	kowords.full_refresh();
}

void Validation::on_unremove_words(ValidationPanel::ElementList words)
{
	auto work = std::unordered_map<Id, std::vector<ValidationPanel::ElementCluster::value_type>>{};
	for (const auto &el : words)
		for (const auto &w : el.second)
			work[doc.GetPosition(w.first.id).view].push_back(w);

	for (const auto &v : work)
	{
		auto view = doc.GetView(v.first);
		for (const auto &w : v.second)
		{
			view.SetValid(w.first.id, true);
			okwords.add_element(w.second, kowords.label_ko, w.first.id);
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
			auto work = std::unordered_map<Id, std::vector<Id>>{};
			for (const auto &w : needconfirm)
				work[doc.GetPosition(w).view].push_back(w);
			for (const auto &v : work)
			{
				auto view = doc.GetView(v.first);
				for (const auto &w : v.second)
					view.SetValid(w, true);
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
	auto work = std::unordered_map<Id, std::vector<Id>>{};
	for (const auto &w : validcontent)
		work[doc.GetPosition(w.id).view].push_back(w.id);

	if (okwords.IsModified())
	{
		// validate
		for (const auto &v : work)
		{
			auto view = doc.GetView(v.first);
			for (const auto &w : v.second)
				view.SetValid(w, true);
		}
		needsave = true;
	}
	else
	{
		if (batch)
		{
			// store for later
			for (const auto &w : validcontent)
				needconfirm.insert(w.id);
		}
		else
		{
			// check is some words need to be validated
			bool need_ask = false;
			for (const auto &v : work)
			{
				auto view = doc.GetView(v.first);
				for (const auto &w : v.second)
					if (view.IsValid(w).IsUnknown())
					{
						need_ask = true;
						break;
					}
				if (need_ask)
					break;
			}
			if (need_ask)
			{
				// ask and validate if needed
				Gtk::MessageDialog msg(*this, _("No modification was performed on this word.\nDo you wish to validate current state?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
				if (msg.run() == Gtk::RESPONSE_YES)
				{
					// validate
					for (const auto &v : work)
					{
						auto view = doc.GetView(v.first);
						for (const auto &w : v.second)
							view.SetValid(w, true);
					}
				} // if response = yes
			} // if needed to ask for validation
		} // not in batch mode (= ask every time)
	} // no modification was made

	//project.PropagateValidation(); TODO

	okwords.clear();
	kowords.clear();
}


