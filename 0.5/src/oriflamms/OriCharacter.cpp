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
#include "genetic.hpp"
#include <unordered_set>
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
	compute_clusters(_("Compute")),
	show_clusters(_("Show")),
	clear_clusters(Gtk::Stock::CLEAR)
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
	hbox2->pack_start(dm_ok, false, false, 0);
	hbox2->pack_start(compute_dm, false, false, 0);
	compute_dm.signal_clicked().connect(sigc::mem_fun(this, &CharacterDialog::compute_distmat));
	tab->attach(clear_dm, 2, 3, 0, 1, Gtk::FILL, Gtk::FILL);
	clear_dm.signal_clicked().connect(sigc::mem_fun(this, &CharacterDialog::delete_dm));

	tab->attach(*Gtk::manage(new Gtk::Label(_("Clusters"))), 0, 1, 1, 2, Gtk::FILL, Gtk::FILL);
	auto *hbox3 = Gtk::manage(new Gtk::HBox);
	tab->attach(*hbox3, 1, 2, 1, 2, Gtk::FILL, Gtk::FILL);
	hbox3->pack_start(compute_clusters, false, false, 0);
	compute_clusters.signal_clicked().connect(sigc::mem_fun(this, &CharacterDialog::clustering));
	hbox3->pack_start(show_clusters, false, false, 0);
	show_clusters.signal_clicked().connect(sigc::mem_fun(this, &CharacterDialog::show_clust));
	tab->attach(clear_clusters, 2, 3, 1, 2, Gtk::FILL, Gtk::FILL);
	clear_clusters.signal_clicked().connect(sigc::mem_fun(this, &CharacterDialog::clear_clust));

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
		compute_clusters.set_sensitive(false);
		clear_clusters.set_sensitive(false);
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
			compute_clusters.set_sensitive(true);
			clear_clusters.set_sensitive(true);
		}
		catch (crn::ExceptionNotFound&)
		{
			compute_dm.show();
			dm_ok.hide();
			clear_dm.set_sensitive(false);
			compute_clusters.set_sensitive(false);
			clear_clusters.set_sensitive(false);
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

void CharacterDialog::clustering()
{
	const auto character = crn::String{Glib::ustring{(*tv.get_selection()->get_selected())[columns.value]}.c_str()};
	// TODO

	try
	{
		doc.AddGlyph("test" + crn::StringUTF8(character), "test", "", true);
	}
	catch (crn::ExceptionDomain&) {}
	for (const auto &v : characters[character])
	{
		auto gid = crn::StringUTF8(character) + v.first;
		try
		{
			doc.AddGlyph(gid, "test " + v.first, Glyph::LocalId("test" + crn::StringUTF8(character)), true);
		}
		catch (crn::ExceptionDomain&) {}
		gid = Glyph::LocalId(gid);
		auto view = doc.GetView(v.first);
		for (const auto &cid : v.second)
			view.GetClusters(cid).push_back(gid);
	}
}

void CharacterDialog::show_clust()
{
	auto row = *tv.get_selection()->get_selected();
	const auto character = crn::String{Glib::ustring{row[columns.value]}.c_str()};
	CharacterTree{character, doc, *this}.run();
}

void CharacterDialog::clear_clust()
{
	const auto character = crn::String{Glib::ustring{(*tv.get_selection()->get_selected())[columns.value]}.c_str()};
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
	// TODO update gui
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
	relabel(_("Set label")),
	cutcluster(_("Cut cluster in two")),
	unlabel(_("Remove from class"))
{
	set_default_size(900, 700);
	maximize();

	auto *hbox = Gtk::manage(new Gtk::HBox);
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
	hbox2->pack_start(relabel, false, false, 4);
	relabel.signal_clicked().connect(sigc::mem_fun(this, &CharacterTree::change_label));
	relabel.set_sensitive(false); // TODO
	hbox2->pack_start(cutcluster, false, false, 4);
	cutcluster.signal_clicked().connect(sigc::mem_fun(this, &CharacterTree::cut_cluster));
	vbox->pack_start(panel, true, true, 4);
	panel.signal_removed().connect(sigc::mem_fun(this, &CharacterTree::on_remove_chars));

	vbox = Gtk::manage(new Gtk::VBox);
	hpan->add2(*vbox);
	hbox2 = Gtk::manage(new Gtk::HBox);
	vbox->pack_start(*hbox2, false, true, 4);
	hbox2->pack_start(unlabel, false, false, 4);
	unlabel.signal_clicked().connect(sigc::mem_fun(this, &CharacterTree::remove_from_cluster));
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
		for (const auto &cid : v.second)
		{
			// create image
			auto b = view.GetZoneImage(view.GetCharacter(cid).GetZone());
			auto pb = GdkCRN::PixbufFromCRNImage(*b->GetRGB());
			if (!pb->get_has_alpha())
				pb = pb->add_alpha(true, 255, 255, 255);
			images.emplace(cid, pb);

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
				clusters[UNLABELLED].push_back(cid);
			else
				for (const auto &gid : glyphs)
				{
					clusters[gid].push_back(cid);
					auto pgid = doc.GetGlyph(gid).GetParent();
					while (pgid.IsNotEmpty())
					{
						clusters[pgid].push_back(cid);
						pgid = doc.GetGlyph(pgid).GetParent();
					}
				}
		}
	}

	// compute tree structure
	auto children = std::unordered_map<Id, std::vector<Id>>{};
	auto topmost = std::unordered_set<Id>{};
	for (const auto &c : clusters)
	{
		auto gid = c.first;
		if (gid != UNLABELLED)
		{
			auto pgid = doc.GetGlyph(gid).GetParent();
			while (pgid.IsNotEmpty())
			{
				children[pgid].push_back(gid);
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

void CharacterTree::add_children(Gtk::TreeIter &it, const Id &gid, const std::unordered_map<Id, std::vector<Id>> &children)
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
			panel.add_element(images[cid], ""/*doc.GetPosition(cid).view*/, doc.GetPosition(cid).word, cid);
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
			kopanel.add_element(w.second, ValidationPanel::label_ko, w.first.word_id, w.first.char_id);
	kopanel.full_refresh();
}

void CharacterTree::on_unremove_chars(ValidationPanel::ElementList words)
{
	for (const auto &el : words)
		for (const auto &w : el.second)
			panel.add_element(w.second, ""/*doc.GetPosition(cid).view*/, w.first.word_id, w.first.char_id);
	panel.full_refresh();
}

void CharacterTree::change_label()
{
}

void CharacterTree::remove_from_cluster()
{
	auto charperview = std::unordered_map<Id, std::vector<Id>>{};
	for (const auto &el : kopanel.get_elements())
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

void CharacterTree::cut(const Id &gid)
{
	// compute distance matrix for selected characters
	const auto nelem = clusters[gid].size();
	const auto &dm = doc.GetDistanceMatrix(character);
	auto indices = std::vector<size_t>{};
	indices.reserve(nelem);
	for (const auto &cid : clusters[gid])
		indices.push_back(std::find(dm.first.begin(), dm.first.end(), cid) - dm.first.begin());
	auto distmat = crn::SquareMatrixDouble{nelem};
	for (auto row : crn::Range(size_t(0), nelem))
		for (auto col : crn::Range(size_t(0), nelem))
			distmat[row][col] = dm.second[indices[row]][indices[col]];

	// cut in two
	auto res = run_genetic(distmat);
	const auto bestclustering = res.begin()->second;

	// create new glyphs
	auto newglyphs = std::array<Id, 2>{};
	auto cnt = 0;
	auto gidbase = gid;
	auto parent = gid;
	if (gidbase == UNLABELLED)
	{
		gidbase = character.CStr();
		parent = "";
	}
	else
	{ // TODO remove prefix
	}
	for (auto &ngid : newglyphs)
	{
		auto ok = false;
		while (!ok)
		{
			ngid = gid + "-"_s + cnt++;
			try
			{
				doc.AddGlyph(ngid, _("Subclass for ") + gidbase, parent, true);
				ngid = Glyph::LocalId(ngid);
				ok = true;
			}
			catch (...) { }
		}
	}

	// add new glyphs to characters
	auto work = std::unordered_map<Id, std::unordered_map<Id, Id>>{};
	for (auto tmp : crn::Range(bestclustering))
	{
		const auto cid = clusters[gid][tmp];
		work[doc.GetPosition(cid).view][cid] = newglyphs[bestclustering[tmp]];
	}
	for (const auto &v : work)
	{
		auto view = doc.GetView(v.first);
		for (const auto &c : v.second)
			view.GetClusters(c.first).push_back(c.second);
	}

	refresh_tv();
}

void CharacterTree::cut_cluster()
{
	auto it = tv.get_selection()->get_selected();
	if (it)
	{
		cut(Id(Glib::ustring((*it)[columns.value]).c_str()));
	}
}

