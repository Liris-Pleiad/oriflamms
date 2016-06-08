/*! Copyright 2013-2016 A2IA, CNRS, École Nationale des Chartes, ENS Lyon, INSA Lyon, Université Paris Descartes, Université de Poitiers
 *
 * This file is part of Oriflamms.
 *
 * Oriflamms is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Oriflamms is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Oriflamms.  If not, see <http://www.gnu.org/licenses/>.
 *
 * \file OriAlignDialog.cpp
 * \author Yann LEYDIER
 */

#include <oriflamms_config.h>
#include <OriAlignDialog.h>
#include <CRNi18n.h>

using namespace ori;

AlignDialog::AlignDialog(Gtk::Window &parent):
	wbut(_("Align words")),
	wall(_("Align all words")),
	wnok(_("Align non-validated words")),
	wnal(_("Align non-aligned words")),
	wfrontbut(_("Update aligned words' frontiers")),
	charbut(_("Align characters")),
	callw(_("in all aligned words")),
	cokw(_("in validated words")),
	cnkow(_("in non-rejected words")),
	call(_("Align all characters")),
	cnal(_("Align non-aligned characters"))
{
	set_title(_("Alignment"));
	set_transient_for(parent);
	add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	add_button(Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
	std::vector<int> altbut;
	altbut.push_back(Gtk::RESPONSE_ACCEPT);
	altbut.push_back(Gtk::RESPONSE_CANCEL);
	set_alternative_button_order_from_array(altbut);
	set_default_response(Gtk::RESPONSE_ACCEPT);
	
	get_vbox()->pack_start(wbut, true, true, 2);
	wbut.set_active(true);
	wbut.signal_toggled().connect(sigc::mem_fun(this, &AlignDialog::update));

	get_vbox()->pack_start(wall, true, true, 2);
	Gtk::RadioButton::Group g1;
	wall.set_group(g1);
	get_vbox()->pack_start(wnok, true, true, 2);
	wnok.set_group(g1);
	get_vbox()->pack_start(wnal, true, true, 2);
	wnal.set_group(g1);

	get_vbox()->pack_start(wfrontbut, true, true, 2);
	wfrontbut.set_active(false);

	get_vbox()->pack_start(charbut, true, true, 2);
	charbut.set_active(false);
	charbut.signal_toggled().connect(sigc::mem_fun(this, &AlignDialog::update));

	get_vbox()->pack_start(callw, true, true, 2);
	Gtk::RadioButton::Group g2;
	callw.set_group(g2);
	get_vbox()->pack_start(cokw, true, true, 2);
	cokw.set_group(g2);
	get_vbox()->pack_start(cnkow, true, true, 2);
	cnkow.set_group(g2);

	get_vbox()->pack_start(call, true, true, 2);
	Gtk::RadioButton::Group g3;
	call.set_group(g3);
	get_vbox()->pack_start(cnal, true, true, 2);
	cnal.set_group(g3);
	cnal.set_sensitive(false);

	get_vbox()->show_all();
	update();
}

void AlignDialog::update()
{
	wall.set_sensitive(wbut.get_active());
	wnok.set_sensitive(wbut.get_active());
	wnal.set_sensitive(wbut.get_active());

	wfrontbut.set_sensitive(!wbut.get_active());

	callw.set_sensitive(charbut.get_active());
	cokw.set_sensitive(charbut.get_active());
	cnkow.set_sensitive(charbut.get_active());
	call.set_sensitive(charbut.get_active());
	cnal.set_sensitive(charbut.get_active());
}

AlignConfig AlignDialog::get_config() const
{
	auto a = AlignConfig::None;
	if (wbut.get_active())
	{
		if (wall.get_active())
			a |= AlignConfig::AllWords;
		else if (wnok.get_active())
			a |= AlignConfig::NOKWords;
		else
			a |= AlignConfig::NAlWords;
	}
	else if (wfrontbut.get_active())
		a |= AlignConfig::WordFrontiers;

	if (charbut.get_active())
	{
		if (callw.get_active())
			a |= AlignConfig::CharsAllWords;
		else if (cokw.get_active())
			a |= AlignConfig::CharsOKWords;
		else
			a |= AlignConfig::CharsNKOWords;

		if (call.get_active())
			a |= AlignConfig::AllChars;
		else
			a |= AlignConfig::NAlChars;
	}
	return a;
}

