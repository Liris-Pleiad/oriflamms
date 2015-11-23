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
		compute_clusters.hide();
		show_clusters.hide();
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
		}
		catch (crn::ExceptionNotFound&)
		{
			compute_dm.show();
			dm_ok.hide();
			clear_dm.set_sensitive(false);
			compute_clusters.set_sensitive(false);
		}
		// XXX tmp
		compute_clusters.show();
	}
		show_clusters.show(); // XXX
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
	compute_dm.hide();
	dm_ok.show();
	clear_dm.set_sensitive(true);
	compute_clusters.set_sensitive(true);
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
	compute_dm.show();
	dm_ok.hide();
	clear_dm.set_sensitive(false);
	compute_clusters.set_sensitive(false);
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
	compute_clusters.hide();
	show_clusters.show();
	clear_clusters.set_sensitive(true);
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
	panel(docu, c.CStr(), false)
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
	hbox->pack_start(panel, true, true, 4);

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

	// create tree
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
	for (const auto gid : topmost)
	{
		auto topit = store->append();
		(*topit)[columns.value] = gid.CStr();
		(*topit)[columns.count] = clusters[gid].size();
		add_children(topit, gid, children);
	}

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
			images.push_back(pb);

			// fill clusters
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

			if (prog)
				prog->Advance();
		}
	}
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
	const auto gid = Id(Glib::ustring((*tv.get_selection()->get_selected())[columns.value]).c_str());
	const auto &dm = doc.GetDistanceMatrix(character);
	panel.lock();
	panel.clear();
	auto cnt = 0;
	for (const auto &cid : clusters[gid])
		panel.add_element(images[std::find(dm.first.begin(), dm.first.end(), cid) - dm.first.begin()], "", doc.GetPosition(cid).word, cnt++);
	panel.unlock();
	panel.full_refresh();
}



