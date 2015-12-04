/* Copyright 2015 Université Paris Descartes
 *
 * file: OriCharacter.cpp
 * \author Yann LEYDIER
 */

#include <OriCharacter.h>
#include <iostream>
#include <CRNFeature/CRNGradientMatching.h>
#include <CRNFeature/CRNGradientShapeContext.h>
#include <GtkCRNProgressWindow.h>
#include <GdkCRNPixbuf.h>
#include <GtkCRNApp.h>
#include "genetic.hpp"
#include <unordered_set>
#include <CRNAI/CRN2Means.h>
#include <CRNi18n.h>

using namespace ori;
using namespace crn::literals;

/////////////////////////////////////////////////////////////////////////////////
// CharacterDialog
/////////////////////////////////////////////////////////////////////////////////
CharacterDialog::CharacterDialog(Document &docu, Gtk::Window &parent):
	Gtk::Dialog(_("Characters"), parent, true),
	doc(docu),
	characters(docu.CollectCharacters()),
	dm_ok(_(("OK"))),
	compute_dm(_("Compute")),
	clear_dm(Gtk::Stock::CLEAR),
	show_clusters(_("Show and edit clusters"))
{
	set_default_size(0, 600);

	auto *hbox = Gtk::manage(new Gtk::HBox);
	get_vbox()->pack_start(*hbox, true, true, 4);

	// left panel
	auto *sw = Gtk::manage(new Gtk::ScrolledWindow);
	hbox->pack_start(*sw, false, true, 4);
	sw->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

	store = Gtk::ListStore::create(columns);
	tv.set_model(store);
	tv.append_column(_("Character"), columns.value);
	tv.append_column(_("Count"), columns.count);
	sw->add(tv);

	for (const auto &c : characters)
	{
		auto row = *store->append();
		row[columns.value] = c.first.CStr();
		auto cnt = size_t(0);
		for (const auto &v : c.second)
			cnt += v.second.size();
		row[columns.count] = cnt;
	}

	tv.get_selection()->signal_changed().connect(sigc::mem_fun(this, &CharacterDialog::update_buttons));

	// right panel
	auto *tab = Gtk::manage(new Gtk::Table(2, 3));
	hbox->pack_start(*tab, false, true, 4);
	tab->set_spacings(4);
	tab->attach(*Gtk::manage(new Gtk::Label(_("Distance matrix"))), 0, 1, 0, 1, Gtk::FILL, Gtk::FILL);
	auto *hbox2 = Gtk::manage(new Gtk::HBox);
	tab->attach(*hbox2, 1, 2, 0, 1, Gtk::FILL, Gtk::FILL);
	hbox2->pack_start(dm_ok, true, false, 0);
	hbox2->pack_start(compute_dm, true, false, 0);
	compute_dm.signal_clicked().connect(sigc::mem_fun(this, &CharacterDialog::compute_distmat));
	tab->attach(clear_dm, 2, 3, 0, 1, Gtk::FILL, Gtk::FILL);
	clear_dm.signal_clicked().connect(sigc::mem_fun(this, &CharacterDialog::delete_dm));

	tab->attach(*Gtk::manage(new Gtk::Label(_("Clustering"))), 0, 1, 1, 2, Gtk::FILL, Gtk::FILL);
	auto *hbox3 = Gtk::manage(new Gtk::HBox);
	tab->attach(*hbox3, 1, 2, 1, 2, Gtk::FILL, Gtk::FILL);
	hbox3->pack_start(show_clusters, false, false, 0);
	show_clusters.signal_clicked().connect(sigc::mem_fun(this, &CharacterDialog::show_clust));

	add_button(Gtk::Stock::CLOSE, Gtk::RESPONSE_CANCEL);
	//add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
	std::vector<int> altbut;
	//altbut.push_back(Gtk::RESPONSE_ACCEPT);
	altbut.push_back(Gtk::RESPONSE_CANCEL);
	set_alternative_button_order_from_array(altbut);
	//set_default_response(Gtk::RESPONSE_ACCEPT);

	show_all_children();
	update_buttons();
}

void CharacterDialog::update_buttons()
{
	auto it = tv.get_selection()->get_selected();
	if (!it)
	{
		dm_ok.hide();
		compute_dm.hide();
		clear_dm.set_sensitive(false);
		show_clusters.set_sensitive(false);
	}
	else
	{
		const auto character = crn::String{Glib::ustring{(*it)[columns.value]}.c_str()};
		try
		{
			doc.GetDistanceMatrix(character);
			compute_dm.hide();
			dm_ok.show();
			clear_dm.set_sensitive(true);
			show_clusters.set_sensitive(true);
		}
		catch (crn::ExceptionNotFound&)
		{
			compute_dm.show();
			dm_ok.hide();
			clear_dm.set_sensitive(false);
			show_clusters.set_sensitive(false);
		}
	}
}

void CharacterDialog::compute_distmat()
{
	auto row = *tv.get_selection()->get_selected();
	const auto character = crn::String{Glib::ustring{row[columns.value]}.c_str()};
	auto ids = std::vector<Id>{};
	for (const auto &v : characters[character])
		ids.insert(ids.end(), v.second.begin(), v.second.end());
	auto dm = crn::SquareMatrixDouble{ids.size(), 0.0};

	const auto res = Gtk::MessageDialog{*this,
			_("Are all the occurrences of this character approximately the same size?"),
			false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO}.run();

	GtkCRN::ProgressWindow pw(_("Computing distance matrix..."), this, true);
	const auto pid = pw.add_progress_bar("");

	if (res == Gtk::RESPONSE_YES)
	{ // gradient matching
		pw.run(sigc::bind(sigc::mem_fun(this, &CharacterDialog::compute_gm), character, ids, std::ref(dm), pw.get_crn_progress(pid)));
	}
	else
	{ // gradient shape context
		pw.run(sigc::bind(sigc::mem_fun(this, &CharacterDialog::compute_gsc), character, ids, std::ref(dm), pw.get_crn_progress(pid)));
	}
	doc.SetDistanceMatrix(character, std::move(ids), std::move(dm));
	update_buttons();
}

void CharacterDialog::compute_gm(const crn::String &character, const std::vector<Id> &ids, crn::SquareMatrixDouble &dm, crn::Progress *prog)
{
	prog->SetMaxCount(2 * ids.size());
	auto grad = std::vector<crn::GradientModel>{};
	for (const auto &v : characters[character])
	{
		auto view = doc.GetView(v.first);
		for (const auto &c : v.second)
		{
			grad.emplace_back(*view.GetZoneImage(view.GetCharacter(c).GetZone())->GetGradient());
			prog->Advance();
		}
	}
	for (auto i : crn::Range(ids))
	{
		for (auto j = i + 1; j < ids.size(); ++j)
		{
			dm[i][j] = dm[j][i] = crn::Distance(grad[i], grad[j], 0);
		}
		prog->Advance();
	}
}

void CharacterDialog::compute_gsc(const crn::String &character, const std::vector<Id> &ids, crn::SquareMatrixDouble &dm, crn::Progress *prog)
{
	prog->SetMaxCount(2 * ids.size());
	using GSCF = crn::GradientShapeContext<8, 3, 8>;
	using GSC = std::vector<GSCF::SC>;
	auto grad = std::vector<GSC>{};
	for (const auto &v : characters[character])
	{
		auto view = doc.GetView(v.first);
		for (const auto &c : v.second)
		{
			grad.push_back(GSCF::CreateRatio(*view.GetZoneImage(view.GetCharacter(c).GetZone())->GetGradient(), 6, 1));
			prog->Advance();
		}
	}
	for (auto i : crn::Range(ids))
	{
		for (auto j = i + 1; j < ids.size(); ++j)
		{
			dm[i][j] = dm[j][i] = GSCF::Distance(grad[i], grad[j]);
		}
		prog->Advance();
	}
}

void CharacterDialog::delete_dm()
{
	auto row = *tv.get_selection()->get_selected();
	const auto character = crn::String{Glib::ustring{row[columns.value]}.c_str()};
	doc.EraseDistanceMatrix(character);
	update_buttons();
}

void CharacterDialog::show_clust()
{
	auto row = *tv.get_selection()->get_selected();
	const auto character = crn::String{Glib::ustring{row[columns.value]}.c_str()};
	CharacterTree{character, doc, *this}.run();
}

/////////////////////////////////////////////////////////////////////////////////
// CharacterTree
/////////////////////////////////////////////////////////////////////////////////
static const auto UNLABELLED = _("Unlabelled");

CharacterTree::CharacterTree(const crn::String &c, Document &docu, Gtk::Window &parent):
	Gtk::Dialog(_("Character tree"), parent, true),
	character(c),
	doc(docu),
	panel(docu, c.CStr(), true),
	kopanel(docu, _("Put aside"), true),
	clear_clusters(_("Delete automatic clusters")),
	auto_clusters(_("Create automatic clusters")),
	label_ok(_("Add glyph")),
	label_ko(_("Add glyph")),
	cut_ok(_("Cut in two")),
	cut_ko(_("Cut in two")),
	remove_ok(_("Remove from class")),
	remove_ko(_("Remove from class"))
{
	set_default_size(900, 700);
	maximize();

	auto *hbox = Gtk::manage(new Gtk::HBox);
	get_vbox()->pack_start(*hbox, false, true, 4);
	hbox->pack_start(auto_clusters, false, true, 4);
	auto_clusters.signal_clicked().connect(sigc::mem_fun(this, &CharacterTree::auto_clustering));
	hbox->pack_start(clear_clusters, false, true, 4);
	clear_clusters.signal_clicked().connect(sigc::mem_fun(this, &CharacterTree::clear_clustering));

	hbox = Gtk::manage(new Gtk::HBox);
	get_vbox()->pack_start(*hbox, true, true, 4);

	// left panel
	auto *sw = Gtk::manage(new Gtk::ScrolledWindow);
	hbox->pack_start(*sw, false, true, 4);
	sw->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

	store = Gtk::TreeStore::create(columns);
	tv.set_model(store);
	tv.append_column(_("Cluster"), columns.value);
	tv.append_column(_("Count"), columns.count);
	sw->add(tv);

	// right panel
	auto *hpan = Gtk::manage(new Gtk::HPaned);
	hbox->pack_start(*hpan, true, true, 4);
	auto *vbox = Gtk::manage(new Gtk::VBox);
	hpan->add1(*vbox);
	auto *hbox2 = Gtk::manage(new Gtk::HBox);
	vbox->pack_start(*hbox2, false, true, 4);
	hbox2->pack_start(label_ok, false, false, 4);
	label_ok.signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &CharacterTree::change_label), std::ref(panel)));
	hbox2->pack_start(cut_ok, false, false, 4);
	cut_ok.signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &CharacterTree::cut_cluster), std::ref(panel)));
	hbox2->pack_start(remove_ok, false, false, 4);
	remove_ok.signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &CharacterTree::remove_from_cluster), std::ref(panel)));
	vbox->pack_start(panel, true, true, 4);
	panel.signal_removed().connect(sigc::mem_fun(this, &CharacterTree::on_remove_chars));

	vbox = Gtk::manage(new Gtk::VBox);
	hpan->add2(*vbox);
	hbox2 = Gtk::manage(new Gtk::HBox);
	vbox->pack_start(*hbox2, false, true, 4);
	hbox2->pack_start(label_ko, false, false, 4);
	label_ko.signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &CharacterTree::change_label), std::ref(kopanel)));
	hbox2->pack_start(cut_ko, false, false, 4);
	cut_ko.signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &CharacterTree::cut_cluster), std::ref(kopanel)));
	hbox2->pack_start(remove_ko, false, false, 4);
	remove_ko.signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &CharacterTree::remove_from_cluster), std::ref(kopanel)));
	vbox->pack_start(kopanel, true, true, 4);
	kopanel.signal_removed().connect(sigc::mem_fun(this, &CharacterTree::on_unremove_chars));
	hpan->set_position(600);

	add_button(Gtk::Stock::CLOSE, Gtk::RESPONSE_CANCEL);
	//add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
	std::vector<int> altbut;
	//altbut.push_back(Gtk::RESPONSE_ACCEPT);
	altbut.push_back(Gtk::RESPONSE_CANCEL);
	set_alternative_button_order_from_array(altbut);
	//set_default_response(Gtk::RESPONSE_ACCEPT);

	// create images
	GtkCRN::ProgressWindow pw(_("Collecting data..."), this, true);
	const auto pid = pw.add_progress_bar("");
	pw.run(sigc::bind(sigc::mem_fun(this, &CharacterTree::init), pw.get_crn_progress(pid)));

	refresh_tv();
	tv.get_selection()->signal_changed().connect(sigc::mem_fun(this, &CharacterTree::sel_changed));

	show_all_children();
}

void CharacterTree::init(crn::Progress *prog)
{
	const auto &dm = doc.GetDistanceMatrix(character);
	auto charperview = std::unordered_map<Id, std::vector<Id>>{};
	for (const auto &cid : dm.first)
		charperview[doc.GetPosition(cid).view].push_back(cid);
	if (prog)
		prog->SetMaxCount(dm.first.size());
	for (const auto &v : charperview)
	{
		auto view = doc.GetView(v.first);
		// load image
		auto pb = GdkCRN::PixbufFromFile(view.GetImageName());

		for (const auto &cid : v.second)
		{
			// get contour
			const auto &czone = view.GetZone(view.GetCharacter(cid).GetZone());
			auto contour = czone.GetContour();
			auto frontier = size_t(0);
			if (contour.empty())
			{
				const auto &bbox = czone.GetPosition();
				contour.emplace_back(bbox.GetLeft(), bbox.GetTop());
				contour.emplace_back(bbox.GetLeft(), bbox.GetBottom());
				frontier = 2;
				contour.emplace_back(bbox.GetRight(), bbox.GetBottom());
				contour.emplace_back(bbox.GetRight(), bbox.GetTop());
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

			// create image
			Glib::RefPtr<Gdk::Pixbuf> cpb(Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, max_x - min_x, max_y - min_y));;
			pb->copy_area(min_x, min_y, max_x - min_x, max_y - min_y, cpb, 0, 0);

			if (!cpb->get_has_alpha())
				cpb = cpb->add_alpha(true, 255, 255, 255);
			auto* pixs = cpb->get_pixels();
			const auto rowstrides = cpb->get_rowstride();
			const auto channels = cpb->get_n_channels();

			auto wbbox = view.GetZone(view.GetWord(doc.GetPosition(cid).word).GetZone()).GetPosition() | crn::Rect{min_x, min_y, max_x, max_y};
			wbbox.SetLeft(wbbox.GetLeft() - wbbox.GetHeight());
			if (wbbox.GetLeft() < 0)
				wbbox.SetLeft(0);
			wbbox.SetRight(wbbox.GetRight() + wbbox.GetHeight());
			if (wbbox.GetRight() >= pb->get_width())
				wbbox.SetRight(pb->get_width() - 1);
			auto tippb = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, wbbox.GetWidth(), wbbox.GetHeight());
			pb->copy_area(wbbox.GetLeft(), wbbox.GetTop(), wbbox.GetWidth(), wbbox.GetHeight(), tippb, 0, 0);
			auto* tippixs = tippb->get_pixels();
			const auto tiprowstrides = tippb->get_rowstride();
			const auto tipchannels = tippb->get_n_channels();
			const auto dx = min_x - wbbox.GetLeft();
			const auto dy = min_y - wbbox.GetTop();

			for (auto j = size_t(0); j < cpb->get_height(); ++j)
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
				for (auto k = 0; k < x; ++k)
					pixs[k * channels + j * rowstrides + 3] = 0;
				tippixs[(x + dx) * tipchannels + (j + dy) * tiprowstrides] = 255;
				tippixs[(x + dx) * tipchannels + (j + dy) * tiprowstrides + 1] = 0;
				tippixs[(x + dx) * tipchannels + (j + dy) * tiprowstrides + 2] = 0;
			}
			for (auto j = size_t (0); j < cpb->get_height(); ++j)
			{
				auto x = max_x - min_x;
				for(auto i = frontier; i < contour.size() - 1; ++i)
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
				for (auto k = x + 1; k < cpb->get_width(); ++k)
					pixs[k * channels + j * rowstrides + 3] = 0;
				tippixs[(x + dx) * tipchannels + (j + dy) * tiprowstrides] = 255;
				tippixs[(x + dx) * tipchannels + (j + dy) * tiprowstrides + 1] = 0;
				tippixs[(x + dx) * tipchannels + (j + dy) * tiprowstrides + 2] = 0;
			}

			images.emplace(cid, cpb);
			tipimages.emplace(cid, tippb);

			if (prog)
				prog->Advance();
		}
	}
}

void CharacterTree::refresh_tv()
{
	const auto &dm = doc.GetDistanceMatrix(character);
	auto charperview = std::unordered_map<Id, std::vector<Id>>{};
	for (const auto &cid : dm.first)
		charperview[doc.GetPosition(cid).view].push_back(cid);

	// fill clusters
	clusters.clear();
	for (const auto &v : charperview)
	{
		auto view = doc.GetView(v.first);
		for (const auto &cid : v.second)
		{
			const auto &glyphs = view.GetClusters(cid);
			if (glyphs.empty())
				clusters[UNLABELLED].insert(cid);
			else
				for (const auto &gid : glyphs)
				{
					clusters[gid].insert(cid);
					auto pgid = doc.GetGlyph(gid).GetParent();
					while (pgid.IsNotEmpty())
					{
						clusters[pgid].insert(cid);
						pgid = doc.GetGlyph(pgid).GetParent();
					}
				}
		}
	}

	// compute tree structure
	auto children = std::map<Id, std::set<Id>>{};
	auto topmost = std::set<Id>{};
	for (const auto &c : clusters)
	{
		auto gid = c.first;
		if (gid != UNLABELLED)
		{
			auto pgid = doc.GetGlyph(gid).GetParent();
			while (pgid.IsNotEmpty())
			{
				children[pgid].insert(gid);
				gid = pgid;
				pgid = doc.GetGlyph(pgid).GetParent();
			}
		}
		topmost.emplace(gid);
	}

	// create tree
	store->clear();
	for (const auto gid : topmost)
	{
		auto topit = store->append();
		(*topit)[columns.value] = gid.CStr();
		(*topit)[columns.count] = clusters[gid].size();
		add_children(topit, gid, children);
	}

	tv.expand_all();
}

void CharacterTree::add_children(Gtk::TreeIter &it, const Id &gid, const std::map<Id, std::set<Id>> &children)
{
	auto cit = children.find(gid);
	if (cit == children.end())
		return;
	for (const auto &id : cit->second)
	{
		auto newit = store->append(it->children());
		(*newit)[columns.value] = id.CStr();
		(*newit)[columns.count] = clusters[id].size();
		add_children(newit, id, children);
	}
}

void CharacterTree::sel_changed()
{
	panel.lock();
	panel.clear();
	auto it = tv.get_selection()->get_selected();
	if (it)
	{
		current_glyph = Id(Glib::ustring((*it)[columns.value]).c_str());
		const auto &dm = doc.GetDistanceMatrix(character);
		for (const auto &cid : clusters[current_glyph])
			panel.add_element(doc.GetPosition(cid), ""/*doc.GetPosition(cid).view*/, images[cid], tipimages[cid], cid);
	}
	else
		current_glyph = "";
	panel.unlock();
	panel.full_refresh();
	kopanel.clear();
	kopanel.full_refresh();
}

void CharacterTree::on_remove_chars(ValidationPanel::ElementList words)
{
	for (const auto &el : words)
		for (const auto &w : el.second)
			kopanel.add_element(w.first.word_id, ValidationPanel::label_ko, w.second.img, w.second.context, w.first.char_id);
	kopanel.full_refresh();
}

void CharacterTree::on_unremove_chars(ValidationPanel::ElementList words)
{
	for (const auto &el : words)
		for (const auto &w : el.second)
			panel.add_element(w.first.word_id, ""/*doc.GetPosition(cid).view*/, w.second.img, w.second.context, w.first.char_id);
	panel.full_refresh();
}

void CharacterTree::change_label(ValidationPanel &p)
{
	Gtk::Dialog dial(_("Add glyph"), *this, true);
	dial.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dial.add_button(Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
	std::vector<int> altbut;
	altbut.push_back(Gtk::RESPONSE_ACCEPT);
	altbut.push_back(Gtk::RESPONSE_CANCEL);
	dial.set_alternative_button_order_from_array(altbut);
	dial.set_default_response(Gtk::RESPONSE_ACCEPT);
	Gtk::HBox hbox;
	dial.get_vbox()->pack_start(hbox, false, true, 4);
	Gtk::Button addbut(Gtk::Stock::ADD);
	hbox.pack_start(addbut, false, true, 4);
	Gtk::Button addbut2(_("Add sub-class"));
	hbox.pack_start(addbut2, false, true, 4);
	Gtk::ScrolledWindow sw;
	sw.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	dial.get_vbox()->pack_start(sw, true, true, 4);
	GlyphSelection gsel{doc};
	addbut.signal_clicked().connect(sigc::bind(sigc::mem_fun(gsel, &GlyphSelection::add_glyph_dialog), Id{}, &dial));
	addbut2.signal_clicked().connect(sigc::bind(sigc::mem_fun(gsel, &GlyphSelection::add_subglyph_dialog), &dial));
	sw.add(gsel);
	dial.get_vbox()->show_all_children();
	dial.set_default_size(300, 500);
	if (dial.run() == Gtk::RESPONSE_ACCEPT)
	{
		dial.hide();
		const auto gid = gsel.get_selected_id();
		if (gid.IsEmpty())
		{ // TODO make the ok button insensitive if nothing is selected
			GtkCRN::App::show_message(_("No glyph selected."), Gtk::MESSAGE_ERROR);
			return;
		}

		auto charperview = std::unordered_map<Id, std::vector<Id>>{};
		for (const auto &el : p.get_elements())
			for (const auto &w : el.second)
				charperview[doc.GetPosition(w.first.char_id).view].push_back(w.first.char_id);

		for (const auto &v : charperview)
		{
			auto view = doc.GetView(v.first);
			for (const auto &cid : v.second)
				view.GetClusters(cid).push_back(gid);
		}

		panel.clear();
		kopanel.clear();
		refresh_tv();
	}
}

void CharacterTree::remove_from_cluster(ValidationPanel &p)
{
	auto charperview = std::unordered_map<Id, std::vector<Id>>{};
	for (const auto &el : p.get_elements())
		for (const auto &w : el.second)
			charperview[doc.GetPosition(w.first.char_id).view].push_back(w.first.char_id);

	for (const auto &v : charperview)
	{
		auto view = doc.GetView(v.first);
		for (const auto &cid : v.second)
		{ // remove current
			auto &glyphs = view.GetClusters(cid);
			glyphs.erase(std::remove(glyphs.begin(), glyphs.end(), current_glyph), glyphs.end());
		}
	}
	panel.clear();
	kopanel.clear();
	refresh_tv();
}

static std::multimap<double, std::vector<size_t>> run_genetic(const crn::SquareMatrixDouble &dm)
{
	auto rng = std::default_random_engine{};
	// create cluster population
	auto population = std::vector<std::vector<size_t>>(100, std::vector<size_t>(dm.GetRows()));
	auto ran = std::uniform_int_distribution<size_t>{0, 1};
	for (auto &idv : population)
		for (auto &gene : idv)
			gene = ran(rng);

	// optimize the clusters!
	auto bestclustering = Genetic(population.begin(), population.end(),
			[](const std::vector<size_t> &idv1, const std::vector<size_t> &idv2, std::default_random_engine &rng)
			{
				auto ran = std::uniform_real_distribution<double>{0, 1};
				auto children = CrossOver{}(idv1, idv2, rng);
				if (ran(rng) < 0.1)
				{ // mutate
					children.first[size_t(ran(rng) * double(children.first.size() - 1))] ^= 1;
				}
				if (ran(rng) < 0.1)
				{ // mutate
					children.second[size_t(ran(rng) * double(children.second.size() - 1))] ^= 1;
				}
				return children;
			},
			[&dm](const std::vector<size_t> &idv)
			{
				// gather cluster indices
				auto c0 = std::vector<size_t>();
				auto c1 = std::vector<size_t>();
				for (auto i : crn::Range(idv))
					if (idv[i] == 0)
						c0.push_back(i);
					else
						c1.push_back(i);

				if (c0.size() < 3)
					return std::numeric_limits<double>::max();

				/*
				auto m0 = std::vector<double>(c0.size(), 0.0);
				for (auto i : crn::Range(c0))
					for (auto j : crn::Range(c0))
						if (i != j)
							m0[i] += dm[c0[i]][c0[j]];
				auto disp0 = std::accumulate(m0.begin(), m0.end(), 0.0) / double(m0.size() - 1) / double(m0.size()); // mean mean distance to others
				return disp0 / log(double(c0.size()));
*/
				//if (c0.empty() || c1.empty()) // no empty class allowed (for now)
				if ((c0.size() < 2) || (c1.size() < 2)) // at least 2 individuals per class (to allow computation of distance to neighbours)
					return std::numeric_limits<double>::max();

				auto m0 = std::vector<double>(c0.size(), 0.0);
				for (auto i : crn::Range(c0))
					for (auto j : crn::Range(c0))
						if (i != j)
							m0[i] += dm[c0[i]][c0[j]];
				auto center0 = c0[std::min_element(m0.begin(), m0.end()) - m0.begin()]; // centroid
				//auto disp0 = *std::max_element(m0.begin(), m0.end()) / double(m0.size() - 1); // max mean distance to others
				auto disp0 = std::accumulate(m0.begin(), m0.end(), 0.0) / double(m0.size() - 1) / double(m0.size()); // mean mean distance to others
				auto m1 = std::vector<double>(c1.size(), 0.0);
				for (auto i : crn::Range(c1))
					for (auto j : crn::Range(c1))
						if (i != j)
							m1[i] += dm[c1[i]][c1[j]];
				//auto disp1 = *std::min_element(m1.begin(), m1.end()) / double(m1.size() - 1); // min mean distance to others
				auto disp1 = std::accumulate(m1.begin(), m1.end(), 0.0) / double(m1.size() - 1) / double(m1.size()); // mean mean distance to others
				//std::sort(m1.begin(), m1.end());
				//auto disp1 = m1[m1.size() / 2] / double(m1.size() - 1); // median mean distance to others
				//auto disp10 = std::numeric_limits<double>::max(); // min distance to centroid0
				auto disp10 = 0.0; // mean distance to centroid0
				for (auto i : c1)
				{
					//disp10 = crn::Min(disp10, dm[i][center0]);
					disp10 += dm[i][center0];
				}
				disp10 /= double(c1.size());
				/*
				if (!disp1 || !disp10)
					return std::numeric_limits<double>::max();

				// disp0 must be low (ie its most outlying element is not too far from the others)
				// disp1 must be high (ie the elements in cluster 1 are far from each other)
				// disp10 must be high (ie cluster 1 is far from cluster 0)
				return disp0 / disp1 / disp10;
*/
				auto center1 = c1[std::min_element(m1.begin(), m1.end()) - m1.begin()];
				// Davies-Bouldin
				auto d0 = 0.0;
				for (auto i : c0)
					d0 += dm[center0][i];
				if (!c0.empty())
					d0 /= double(c0.size());
				auto d1 = 0.0;
				for (auto i : c1)
					d1 += dm[center1][i];
				if (!c1.empty())
					d1 /= double(c1.size());
				return (d0 + d1) / dm[center0][center1];
				//return (d0 + d1) / dm[center0][center1] * disp0;
				//return (d0 + d1) / dm[center0][center1] / disp1;
				/*
				// Dunn
				auto dmin = std::numeric_limits<double>::max();
				auto dmax = 0.0;
				for (auto i : crn::Range(idv))
				for (auto j = i + 1; j < idv.size(); ++j)
				if (idv[i] == idv[j])
				{ // same class
				dmax = crn::Max(dmax, dm.At(i, j));
				}
				else
				{ // different class
				dmin = crn::Min(dmin, dm.At(i, j));
				}
				//return dmin / dmax / disp1;
				return dmin / dmax;
				*/
			},
		GenerationCounter{1000},
		rng);
	return bestclustering;
}

std::pair<Id, Id> CharacterTree::cut(const Id &gid, const std::vector<Id> &chars, crn::Progress *prog)
{
	if (prog)
		prog->SetMaxCount(4);

	// compute distance matrix for selected characters
	const auto nelem = chars.size();
	const auto &dm = doc.GetDistanceMatrix(character);
	auto indices = std::vector<size_t>{};
	indices.reserve(nelem);
	for (const auto &cid : chars)
		indices.push_back(std::find(dm.first.begin(), dm.first.end(), cid) - dm.first.begin());
	auto distmat = crn::SquareMatrixDouble{nelem};
	for (auto row : crn::Range(size_t(0), nelem))
		for (auto col : crn::Range(size_t(0), nelem))
			distmat[row][col] = dm.second[indices[row]][indices[col]];
	if (prog)
		prog->Advance();

	// cut in two
	auto res = run_genetic(distmat);
	const auto bestclustering = res.begin()->second;
	if (prog)
		prog->Advance();

	// check if there are really two classes
	auto class0 = size_t(0);
	for (auto c : bestclustering)
		if (c == bestclustering.front())
			class0 += 1;
	if (class0 == 0 || class0 == bestclustering.size())
	{
		if (prog)
		{
			prog->Advance();
			prog->Advance();
		}
		return std::make_pair(Id{}, Id{});
	}

	// create new glyphs
	auto newglyphs = std::array<Id, 2>{};
	auto cnt = 0;
	auto gidbase = Glyph::BaseId(gid);
	auto parent = gid;
	if (gidbase == UNLABELLED)
	{
		gidbase = character.CStr();
		parent = "";
	}
	for (auto &ngid : newglyphs)
	{
		auto ok = false;
		while (!ok)
		{
			ngid = gidbase + "-"_s + cnt++;
			try
			{
				doc.AddGlyph(ngid, _("Subclass for ") + gidbase, parent, true);
				ngid = Glyph::LocalId(ngid);
				ok = true;
			}
			catch (...) { }
		}
	}
	if (prog)
		prog->Advance();

	// add new glyphs to characters
	auto work = std::unordered_map<Id, std::unordered_map<Id, Id>>{};
	auto tmp = size_t(0);
	for (const auto & cid : chars)
	{
		work[doc.GetPosition(cid).view][cid] = newglyphs[bestclustering[tmp++]];
	}
	for (const auto &v : work)
	{
		auto view = doc.GetView(v.first);
		for (const auto &c : v.second)
		{
			view.GetClusters(c.first).push_back(c.second);
			clusters[c.second].insert(c.first);
		}
	}
	if (prog)
		prog->Advance();
	return std::make_pair(newglyphs[0], newglyphs[1]);
}

void CharacterTree::cut_cluster(ValidationPanel &p)
{
	auto it = tv.get_selection()->get_selected();
	if (it)
	{
		auto ids = std::vector<Id>{};
		for (const auto &el : p.get_elements())
			for (const auto &w : el.second)
				ids.push_back(w.first.char_id);

		GtkCRN::ProgressWindow pw(_("Clustering..."), this, true);
		const auto pid = pw.add_progress_bar("");
		pw.run(sigc::hide_return(sigc::bind(sigc::mem_fun(this, &CharacterTree::cut), Id(Glib::ustring((*it)[columns.value]).c_str()), ids, pw.get_crn_progress(pid))));

		refresh_tv();
	}
}

void CharacterTree::clear_clustering()
{
	const auto &dm = doc.GetDistanceMatrix(character);
	auto charperview = std::unordered_map<Id, std::vector<Id>>{};
	for (const auto &cid : dm.first)
		charperview[doc.GetPosition(cid).view].push_back(cid);
	for (const auto &v : charperview)
	{
		auto view = doc.GetView(v.first);
		for (const auto &cid : v.second)
		{
			auto &glyphs = view.GetClusters(cid);
			glyphs.erase(
					std::remove_if(glyphs.begin(), glyphs.end(),
						[this](const Id &id){ return doc.GetGlyph(id).IsAuto(); }),
					glyphs.end());
		}
	}
	refresh_tv();
}

void CharacterTree::auto_clustering()
{
	GtkCRN::ProgressWindow pw(_("Clustering..."), this, true);
	const auto pid = pw.add_progress_bar("");
	pw.run(sigc::bind(sigc::mem_fun(this, &CharacterTree::auto_cut), pw.get_crn_progress(pid)));

	doc.Save();
	refresh_tv();
}

static double split_score(const std::vector<Id> &sel, std::unordered_map<Id, size_t> &indices, const crn::SquareMatrixDouble &distmat)
{
	if (sel.size() <= 1)
		return 0.0;
	auto d = std::vector<double>{};
	for (auto i : crn::Range(sel))
		for (auto j = i + 1; j < sel.size(); ++j)
			d.push_back(distmat[indices[sel[i]]][indices[sel[j]]]);
	if (d.size() == 1)
		return d.front();
	return crn::TwoMeans(d.begin(), d.end()).second;
}
void CharacterTree::auto_cut(crn::Progress *prog)
{
	static constexpr auto NCLUST = size_t(37);

	if (prog)
		prog->SetMaxCount(NCLUST);

	const auto &distmat = doc.GetDistanceMatrix(character);
	auto indices = std::unordered_map<Id, size_t>{};
	for (auto tmp = size_t(0); tmp < distmat.first.size(); ++tmp)
		indices.emplace(distmat.first[tmp], tmp);

	auto baseid = "auto_"_s + character.CStr();
	try { doc.AddGlyph(baseid, _("Base class for automatic clustering of ") + crn::StringUTF8(character), "", true); } catch (...) { }
	baseid = Glyph::LocalId(baseid);

	auto selectionset = std::unordered_map<crn::StringUTF8, std::vector<Id>>{};
	selectionset[baseid] = distmat.first;
	auto done = std::unordered_map<crn::StringUTF8, std::vector<Id>>{};
	auto split = std::multimap<double, crn::StringUTF8, std::greater<double>>{};
	split.emplace(split_score(selectionset[baseid], indices, distmat.second), baseid);

	while (selectionset.size() + done.size() < NCLUST)
	{
		// find cluster to cut
		const auto bestit = split.begin();
		if (bestit->first == 0.0)
			break;
		const auto bestsel = bestit->second;

		const auto &selection = selectionset[bestsel];

		const auto newglyphs = cut(bestsel, selection, nullptr);

		if (newglyphs.first.IsNotEmpty() && newglyphs.second.IsNotEmpty())
		{
			auto newsel = std::vector<Id>{};
			newsel.insert(newsel.end(), clusters[newglyphs.first].begin(), clusters[newglyphs.first].end());
			split.emplace(split_score(newsel, indices, distmat.second), newglyphs.first);
			selectionset[newglyphs.first] = std::move(newsel);

			newsel = std::vector<Id>{};
			newsel.insert(newsel.end(), clusters[newglyphs.second].begin(), clusters[newglyphs.second].end());
			split.emplace(split_score(newsel, indices, distmat.second), newglyphs.second);
			selectionset[newglyphs.second] = std::move(newsel);
		}
		done[bestsel] = std::move(selectionset[bestsel]);
		selectionset.erase(bestsel);
		split.erase(bestit);

		if (prog)
			prog->Advance();

		if (split.empty())
			break;
	}

}

/////////////////////////////////////////////////////////////////////////////////
// GlyphSelection
/////////////////////////////////////////////////////////////////////////////////
GlyphSelection::GlyphSelection(Document &docu):
	doc(docu)
{
	store = Gtk::ListStore::create(columns);
	const auto &glyphs = doc.GetGlyphs();
	for (const auto &g : glyphs)
		add_glyph(g.first, g.second);

	set_model(store);
	append_column(_("Global"), columns.global);
	append_column(_("Id"), columns.id);
	append_column(_("Auto"), columns.automatic);
	append_column(_("Description"), columns.desc);
	get_column(0)->set_sort_column(columns.global);
	get_column(1)->set_sort_column(columns.id);
	get_column(2)->set_sort_column(columns.automatic);
}

Id GlyphSelection::get_selected_id()
{
	auto it = get_selection()->get_selected();
	if (!it)
		return "";
	const auto id = Id(Glib::ustring((*it)[columns.id]).c_str());
	if ((*it)[columns.global])
		return Glyph::GlobalId(id);
	else
		return Glyph::LocalId(id);
}

void GlyphSelection::add_glyph(const Id &gid, const Glyph &g)
{
	auto row = *store->append();
	row[columns.id] = Glyph::BaseId(gid).CStr();
	row[columns.global] = Glyph::IsGlobal(gid);
	row[columns.desc] = g.GetDescription().CStr();
	row[columns.automatic] = g.IsAuto();
}

void GlyphSelection::add_glyph_dialog(const Id &parent_id, Gtk::Window *parent)
{
	Gtk::Dialog dial(_("Add glyph"), true);
	if (parent)
		dial.set_transient_for(*parent);
	dial.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dial.add_button(Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
	std::vector<int> altbut;
	altbut.push_back(Gtk::RESPONSE_ACCEPT);
	altbut.push_back(Gtk::RESPONSE_CANCEL);
	dial.set_alternative_button_order_from_array(altbut);
	dial.set_default_response(Gtk::RESPONSE_ACCEPT);
	Gtk::Table tab(3, 2);
	dial.get_vbox()->pack_start(tab, false, true, 4);
	tab.set_col_spacings(4);
	tab.attach(*Gtk::manage(new Gtk::Label(_("Id"))), 0, 1, 0, 1, Gtk::FILL, Gtk::FILL);
	Gtk::Entry ident;
	tab.attach(ident, 1, 2, 0, 1, Gtk::FILL|Gtk::EXPAND, Gtk::FILL);
	tab.attach(*Gtk::manage(new Gtk::Label(_("Description"))), 0, 1, 1, 2, Gtk::FILL, Gtk::FILL);
	Gtk::Entry descent;
	tab.attach(descent, 1, 2, 1, 2, Gtk::FILL|Gtk::EXPAND, Gtk::FILL);
	tab.attach(*Gtk::manage(new Gtk::Label(_("Parent"))), 0, 1, 2, 3, Gtk::FILL, Gtk::FILL);
	if (parent_id.IsNotEmpty())
		tab.attach(*Gtk::manage(new Gtk::Label(parent_id.CStr())), 1, 2, 2, 3, Gtk::FILL, Gtk::FILL);

	dial.get_vbox()->show_all_children();
	if (dial.run() == Gtk::RESPONSE_ACCEPT)
	{
		dial.hide();
		const auto id = Id(ident.get_text().c_str());
		if (id.IsEmpty())
		{
			GtkCRN::App::show_message(_("Invalid id."), Gtk::MESSAGE_ERROR);
			return;
		}
		try
		{
			doc.GetGlyph(Glyph::LocalId(id));
			GtkCRN::App::show_message(_("The id is already used in local database."), Gtk::MESSAGE_ERROR);
			return;
		}
		catch (...) { }
		try
		{
			doc.GetGlyph(Glyph::GlobalId(id));
			GtkCRN::App::show_message(_("The id is already used in global database."), Gtk::MESSAGE_ERROR);
			return;
		}
		catch (...) { }
		const auto &glyph = doc.AddGlyph(id, descent.get_text().c_str(), parent_id, false);
		add_glyph(Glyph::LocalId(id) , glyph);
	}
}

void GlyphSelection::add_subglyph_dialog(Gtk::Window *parent)
{
	add_glyph_dialog(get_selected_id(), parent);
}

