/* Copyright 2013 INSA-Lyon & IRHT
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

using namespace ori;

const crn::StringUTF8 Validation::label_ok("zzz1");
const crn::StringUTF8 Validation::label_ko("ko");
const crn::StringUTF8 Validation::label_unknown("0");
const crn::StringUTF8 Validation::label_revalidated("zzz0");

static bool nullkey(GdkEventKey*) { return true; }
Validation::Validation(Gtk::Window &parent, Project &proj, bool batch_valid, bool use_clustering, const crn::Functor<void> &savefunc, const crn::Functor<void> &refreshfunc):
	//Gtk::Window(_("Validation"), parent, true),
	project(proj),
	okwords(_("Accepted"), proj.GetDoc()->GetFilenames()),
	kowords(_("Rejected"), proj.GetDoc()->GetFilenames()),
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
#ifdef CRN_PF_MSVC
	if (firstrun)
	{
		firstrun = false;
		return;
	}
#endif
	// concludes with what was displayed
	conclude_word();

	// update
	Gtk::TreeIter it(tv.get_selection()->get_selected());
	if (!it)
		return;
	Glib::ustring sname(it->get_value(columns.name));
	
	GtkCRN::ProgressWindow prog(_("Loading words"), this, true);
	size_t id = prog.add_progress_bar("Occurrence");
	prog.get_crn_progress(id)->SetType(crn::Progress::ABSOLUTE);
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
#ifdef CRN_PF_MSVC
	crn::ImageRGB img;
#else
	Glib::RefPtr<Gdk::Pixbuf> pb;
#endif
	std::vector<WordPath> clustpath;
	std::vector<Glib::RefPtr<Gdk::Pixbuf> > clustimg;
	std::vector<std::vector<int> > clustproj;
	std::vector<crn::String> clustsig;
	CRN_FOREACH(const WordPath &wp, elems)
	{
		Word &oriword(project.GetStructure().GetWord(wp));
		if (oriword.GetBBox().IsValid())
		{ // if the word was aligned
			if (lastview != wp.view)
			{ // lazy load
#ifdef CRN_PF_MSVC
				img.InitFromImage(*crn::Image::NewFromFile(project.GetDoc()->GetFilenames()[wp.view]));
				//pb = GdkCRN::PixbufFromCRNImage(crn::Image::NewFromFile(project.GetDoc()->GetFilenames()[wp.view]));
#else
				pb = Gdk::Pixbuf::create_from_file(project.GetDoc()->GetFilenames()[wp.view].CStr());
#endif
				lastview = wp.view;
			}
			crn::Rect bbox(oriword.GetBBox());
			// create small image (not using subpixbuf since it keeps a reference to the original image)
#ifdef CRN_PF_MSVC
			crn::ImageRGB tiny;
			tiny.InitFromImage(img, bbox);
			Glib::RefPtr<Gdk::Pixbuf> wpb = GdkCRN::PixbufFromCRNImage(&tiny);
#else
			Glib::RefPtr<Gdk::Pixbuf> wpb(Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, bbox.GetWidth(), bbox.GetHeight()));;
			pb->copy_area(bbox.GetLeft(), bbox.GetTop(), bbox.GetWidth(), bbox.GetHeight(), wpb, 0, 0);
#endif
			// add word
			if (oriword.GetValid().IsFalse())
			{ // add to reject list
				kowords.add_element(label_ko, wp, wpb);
			}
			else
			{
				if (oriword.GetValid().IsTrue())
					okwords.add_element(label_ok, wp, wpb);
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
						okwords.add_element(label_unknown, wp, wpb);
					}
				}
			}
		}
		if (prog)
			prog->Advance();
	}
	if (clustering)
	{ // cluster the unvalidated words
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
		// add to panel
		int cnt = 1;
		CRN_FOREACH(const std::set<size_t> &c, clust.GetClusters())
		{
			crn::StringUTF8 lab(cnt++);
			CRN_FOREACH(size_t i, c)
				okwords.add_element(lab, clustpath[i], clustimg[i]);
		}
	}
	okwords.unlock();
	kowords.unlock();
}

void Validation::on_remove_words(Validation::ValidationPanel::WordList words)
{
	CRN_FOREACH(const ValidationPanel::WordList::value_type &el, words)
	{
		CRN_FOREACH(const ValidationPanel::WordCluster::value_type &w, el.second)
		{
			project.GetStructure().GetWord(w.first).SetValid(FALSE);
			kowords.add_element(label_ko, w.first, w.second);
		}
	}
	needsave = true;
	kowords.full_refresh();
}

void Validation::on_unremove_words(Validation::ValidationPanel::WordList words)
{
	CRN_FOREACH(const ValidationPanel::WordList::value_type &el, words)
	{
		CRN_FOREACH(const ValidationPanel::WordCluster::value_type &w, el.second)
		{
			project.GetStructure().GetWord(w.first).SetValid(TRUE);
			okwords.add_element(label_revalidated, w.first, w.second);
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
	{ // some words were visited but not modified
		Gtk::MessageDialog msg(*this, _("You visited some words without performing any modification.\nDo you wish to validate them?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
		if (msg.run() == Gtk::RESPONSE_YES)
		{ // validate
			CRN_FOREACH(const WordPath &w, needconfirm)
			{
				project.GetStructure().GetWord(w).SetValid(TRUE);
			}
			needsave = true;
		}
	}
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
	const std::set<WordPath> validcontent(okwords.GetContent());
	if (okwords.IsModified())
	{ // validate
		CRN_FOREACH(const WordPath &w, validcontent)
		{
			project.GetStructure().GetWord(w).SetValid(TRUE);
		}
		needsave = true;
	}
	else
	{ 
		if (batch)
		{ // store for later
			needconfirm.insert(validcontent.begin(), validcontent.end());
		}
		else
		{
			// check is some words need to be validated
			bool need_ask = false;
			CRN_FOREACH(const WordPath &w, validcontent)
			{
				if (project.GetStructure().GetWord(w).GetValid().IsUnknown())
				{
					need_ask = true;
					break;
				}
			}
			if (need_ask)
			{ // ask and validate if needed
				Gtk::MessageDialog msg(*this, _("No modification was performed on this word.\nDo you wish to validate current state?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
				if (msg.run() == Gtk::RESPONSE_YES)
				{ // validate
					CRN_FOREACH(const WordPath &w, validcontent)
					{
						project.GetStructure().GetWord(w).SetValid(TRUE);
					}
				} // if response = yes
			} // if needed to ask for validation
		} // not in batch mode (= ask every time)
	} // no modification was made
	
	project.PropagateValidation();

	okwords.clear();
	kowords.clear();
}

void Validation::set_color(Cairo::RefPtr<Cairo::Context> &cc, const crn::StringUTF8 &label)
{
	if (label == label_ok)
		cc->set_source_rgb(0.9, 1.0, 0.9);
	else if (label == label_ko)
		cc->set_source_rgb(1.0, 0.9, 0.9);
	else if (label == label_unknown)
		cc->set_source_rgb(1.0, 0.95, 0.9);
	else if (label == label_revalidated)
		cc->set_source_rgb(1.0, 1.0, 0.9);
	else
	{
		int c = 0;
		for (size_t tmp = 0; tmp < label.Size(); ++tmp)
			c += label[tmp];
		c %= 3;
		cc->set_source_rgb(c == 1 ? 0.95 : 0.9, c == 2 ? 0.95 : 0.9, 1.0);
	}

}

Validation::ValidationPanel::ValidationPanel(const crn::StringUTF8 &name, const std::vector<crn::Path> &imagenames):
	title(name),
	nelem(0),
	Gtk::Frame(name.CStr()),
	dispw(0),
	disph(0),
	marking(false),
	locked(false),
	modified(false)
{
	Gtk::HBox *hbox(Gtk::manage(new Gtk::HBox));
	add(*hbox);
	hbox->pack_start(da, true, true, 0);
	da.add_events(Gdk::EXPOSURE_MASK | Gdk::STRUCTURE_MASK | Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::SCROLL_MASK);
	da.signal_expose_event().connect(sigc::mem_fun(this, &ValidationPanel::expose)); // redraw
	da.signal_configure_event().connect(sigc::mem_fun(this, &ValidationPanel::configure)); // size changed
	da.signal_motion_notify_event().connect(sigc::mem_fun(this, &ValidationPanel::mouse_motion)); // mouse motion
	da.signal_button_press_event().connect(sigc::mem_fun(this, &ValidationPanel::button_clicked));
	da.signal_button_release_event().connect(sigc::mem_fun(this, &ValidationPanel::button_clicked));
	da.signal_scroll_event().connect(sigc::mem_fun(this, &ValidationPanel::on_scroll));

	hbox->pack_start(scroll, false, true, 0);
	scroll.get_adjustment()->set_lower(0);
	scroll.get_adjustment()->set_step_increment(10);
	scroll.get_adjustment()->set_page_increment(100);
	scroll.signal_value_changed().connect(sigc::mem_fun(this, &ValidationPanel::refresh));
	set_has_tooltip();

	set_has_tooltip();	
	signal_query_tooltip().connect(sigc::mem_fun(this, &ValidationPanel::tooltip));	

	std::transform(imagenames.begin(), imagenames.end(), std::back_inserter(images), std::mem_fun_ref(&crn::Path::GetFilename));

	show_all();
}

void Validation::ValidationPanel::add_element(const crn::StringUTF8 cluster, const WordPath &p, const Glib::RefPtr<Gdk::Pixbuf> &pb)
{
	elements[cluster].insert(std::make_pair(p, pb)); 
	nelem += 1;
}

const std::set<WordPath> Validation::ValidationPanel::GetContent() const
{
	std::set<WordPath> cont;
	CRN_FOREACH(const ValidationPanel::WordList::value_type &el, elements)
	{
		CRN_FOREACH(const ValidationPanel::WordCluster::value_type &w, el.second)
		{
			cont.insert(w.first);
		}
	}
	return cont;
}

bool Validation::ValidationPanel::expose(GdkEventExpose *ev)
{
	refresh();
	return false;
}

bool Validation::ValidationPanel::configure(GdkEventConfigure *ev)
{
	dispw = ev->width;
	disph = ev->height;
	scroll.get_adjustment()->set_page_size(disph);
	full_refresh();
	return false;
}

bool Validation::ValidationPanel::mouse_motion(GdkEventMotion *ev)
{
	if (marking)
	{
		mark.push_back(crn::Point2DInt(int(ev->x), int(ev->y + scroll.get_adjustment()->get_value())));
		refresh();
	}
	return false;
}

bool Validation::ValidationPanel::button_clicked(GdkEventButton *ev)
{
	if (ev->button == 1)
	{ // left click
		if (ev->type == GDK_BUTTON_PRESS)
		{
			mark.push_back(crn::Point2DInt(int(ev->x), int(ev->y + scroll.get_adjustment()->get_value())));
			marking = true;
			refresh();
		}
		else if (ev->type == GDK_BUTTON_RELEASE)
		{
			marking = false;
			WordList keep, rem;
			int nrem = 0;
			int offset = -int(scroll.get_adjustment()->get_value());
			CRN_FOREACH(const ValidationPanel::WordList::value_type &el, elements)
			{
				CRN_FOREACH(const ValidationPanel::WordCluster::value_type &w, el.second)
				{
					const crn::Point2DInt &pos(positions[w.first]);
					crn::Rect bbox(pos.X, pos.Y + offset,
							pos.X + w.second->get_width(), pos.Y + offset + w.second->get_height());
					bool found = false;
					CRN_FOREACH(const crn::Point2DInt &p, mark)
					{
						if (bbox.Contains(p.X, p.Y + offset))
						{
							found = true;
							break;
						}
					}
					if (found)
					{
						rem[el.first].insert(w);
						nrem += 1;
					}
					else
						keep[el.first].insert(w);
				}
			}
			if (!rem.empty())
			{
				elements.swap(keep);
				nelem -= nrem;
				removed.emit(rem);
				modified = true;
			}
			mark.clear();
			/*full_*/refresh();
		}
	}
	return false;
}

bool Validation::ValidationPanel::on_scroll(GdkEventScroll *ev)
{
	double p = scroll.get_value();
	switch (ev->direction)
	{
		case GDK_SCROLL_UP:
			p -= scroll.get_adjustment()->get_page_increment();
			if (p < 0)
				p = 0;
			break;
		case GDK_SCROLL_DOWN:
			p += scroll.get_adjustment()->get_page_increment();
			if (p + disph > scroll.get_adjustment()->get_upper())
				p = crn::Max(0.0, scroll.get_adjustment()->get_upper() - disph);
			break;
	}
	scroll.set_value(p);
	return false;
}

void Validation::ValidationPanel::refresh()
{
	// update the label
	crn::StringUTF8 lab(title);
	lab += " ("; lab += nelem; lab += ")";
	set_label(lab.CStr());
	
	if (/*(positions.size() != elements.size()) ||*/ (dispw < 0) || (disph < 0) || locked)
		return;

	Glib::RefPtr<Gdk::Window> win = da.get_window();
	if (win)
	{ // if the drawing area is fully created
		// create a buffer
		Glib::RefPtr<Gdk::Pixmap> pm = Gdk::Pixmap::create(win, dispw, disph);
		// clear the buffer
		pm->draw_rectangle(da_gc, true, 0, 0, dispw, disph);
		int offset = -int(scroll.get_value());

		Cairo::RefPtr<Cairo::Context> cc = pm->create_cairo_context();

		// draw the words
		CRN_FOREACH(const ValidationPanel::WordList::value_type &el, elements)
		{
			if (!el.second.empty())
			{
				int by = positions[el.second.begin()->first].Y;
				int ey = by;
				CRN_FOREACH(const ValidationPanel::WordCluster::value_type &w, el.second)
				{
					int y = positions[w.first].Y + w.second->get_height();
					if (y > ey) ey = y;
				}
				if ((el.first == elements.rbegin()->first) && (ey < disph))
				{ // fill till the end of the display
					ey = disph;
				}
				Validation::set_color(cc, el.first);
				cc->rectangle(0, by + offset, dispw, ey - by);
				cc->fill();
			}
			CRN_FOREACH(const ValidationPanel::WordCluster::value_type &w, el.second)
			{
				pm->draw_pixbuf(da_gc, w.second, 0, 0, positions[w.first].X, positions[w.first].Y + offset, w.second->get_width(), w.second->get_height(), Gdk::RGB_DITHER_NONE, 0, 0);
			}
		}
		
		// draw the mark
		if (mark.size() > 1)
		{
			cc->set_source_rgb(1.0, 0, 0);
			cc->move_to(mark.front().X, mark.front().Y + offset);
			for (size_t tmp = 1; tmp < mark.size(); ++tmp)
			{
				cc->line_to(mark[tmp].X, mark[tmp].Y + offset);
			}
			cc->stroke();
		}

		// copy pixmap to drawing area
		win->draw_drawable(da_gc, pm, 0, 0, 0, 0);
	}
}

void Validation::ValidationPanel::full_refresh()
{
	if (locked)
		return;

	positions.clear();
	nelem = 0;
	
	Glib::RefPtr<Gdk::Window> win = da.get_window();
	if (win)
	{ // if the drawing area is fully created
		// update the GC
		da_gc = Gdk::GC::create(win);
		da_gc->set_rgb_fg_color(Gdk::Color("white"));
	}

	int x = margin, y = 0;
	int h = 0;
	for (std::map<crn::StringUTF8, WordCluster>::iterator cit = elements.begin(); cit != elements.end(); ++cit)
	{
		if ((y != 0) || (x != margin))
			y += h + margin;
		x = margin;
		h = 0;
		nelem += int(cit->second.size());
		for (WordCluster::iterator wit = cit->second.begin(); wit != cit->second.end(); ++wit)
		{
			if ((x + wit->second->get_width() >= dispw) && (x > 0) && (h > 0))
			{ // wont fit
				x = margin;
				y += h + margin;
				h = 0;
			}
			positions.insert(std::make_pair(wit->first, crn::Point2DInt(x, y)));
			x += wit->second->get_width() + margin;
			h = crn::Max(h, wit->second->get_height());
			if (x >= dispw)
			{ // the margin made us go off bounds
				x = margin;
				y += h + margin;
				h = 0;
			}
		}
	}
	// compute full height and scrollbar
	h += y;
	scroll.get_adjustment()->set_upper(h);
	if (scroll.get_value() + disph > h)
		scroll.set_value(crn::Max(0, h - disph));
	
	refresh();
}

bool Validation::ValidationPanel::tooltip(int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip)
{
	int offset = -int(scroll.get_adjustment()->get_value());
	CRN_FOREACH(const ValidationPanel::WordList::value_type &el, elements)
	{
		CRN_FOREACH(const ValidationPanel::WordCluster::value_type &w, el.second)
		{
			const crn::Point2DInt &pos(positions[w.first]);
			crn::Rect bbox(pos.X, pos.Y + offset,
					pos.X + w.second->get_width(), pos.Y + offset + w.second->get_height());
				if (bbox.Contains(x, y + offset))
				{
					crn::StringUTF8 msg;
					msg += _("Image");
					msg += " ";
					//msg += w.first.view + 1;
					if (w.first.view < images.size())
						msg += images[w.first.view];
					else
						msg += w.first.view + 1;
					msg += "\n";
					msg += _("Column");
					msg += " ";
					msg += w.first.col + 1;
					msg += "\n";
					msg += _("Line");
					msg += " ";
					msg += w.first.line + 1;
					msg += "\n";
					msg += _("Word");
					msg += " ";
					msg += w.first.word + 1;
					tooltip->set_text(msg.CStr());
					return true;
				}
		}
	}
	return false;
}

