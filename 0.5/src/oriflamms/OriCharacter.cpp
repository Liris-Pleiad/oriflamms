/* Copyright 2015 Université Paris Descartes
 * 
 * file: OriCharacter.cpp
 * \author Yann LEYDIER
 */

#include <OriCharacter.h>
#include <CRNi18n.h>

using namespace ori;

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

	tab->attach(*Gtk::manage(new Gtk::Label(_("Clusters"))), 0, 1, 1, 2, Gtk::FILL, Gtk::FILL);
	auto *hbox3 = Gtk::manage(new Gtk::HBox);
	tab->attach(*hbox3, 1, 2, 1, 2, Gtk::FILL, Gtk::FILL);
	hbox3->pack_start(compute_clusters, false, false, 0);
	hbox3->pack_start(show_clusters, false, false, 0);
	tab->attach(clear_clusters, 2, 3, 1, 2, Gtk::FILL, Gtk::FILL);

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
}

void CharacterDialog::compute_distmat()
{
	if (Gtk::MessageDialog{*this, 
			_("Are all the occurrences of this character approximately the same size?"), 
			false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO}.run() == Gtk::RESPONSE_YES)
	{ // gradient matching
	}
	else
	{ // gradient shape context
	}
}

