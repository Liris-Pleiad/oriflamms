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
 * \file OriTEIDisplay.cpp
 * \author ZHAO Xiaojuan, Yann LEYDIER
 */

#include <OriConfig.h>
#include <CRNException.h>
#include <OriTEIDisplay.h>
#include <CRNData/CRNDataFactory.h>
#include <CRNi18n.h>

#include <iostream>

using namespace ori;

/////////////////////////////////////////////////////////////////////
// TEIDisplay
/////////////////////////////////////////////////////////////////////

/*!
 * \param[in]	path1	path to an XML TEI file
 * \param[in]	path2	path to an XML TEI file
 * \param[in]	parent	the parent window
 */
TEIDisplay::TEIDisplay(const crn::Path &path1, const crn::Path &path2, Gtk::Window &parent):
	Gtk::Dialog(_("Node selection"), parent, true)
{
	resize(100,600);

	scrollwin.add(view);
	scrollwin.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

	add_button(Gtk::Stock::CLOSE, Gtk::RESPONSE_ACCEPT);
	set_default_response(Gtk::RESPONSE_ACCEPT);

	get_vbox()->pack_start(scrollwin);

	treestore = Gtk::TreeStore::create(column);
	view.set_model(treestore);

	view.append_column(_("Parsed"), column.choice);
	view.append_column(_("Element name"), column.name);
	view.append_column(_("Contains text"), column.text);

	Gtk::TreeViewColumn *tvc = view.get_column(1);
	Gtk::CellRendererText *cr = dynamic_cast<Gtk::CellRendererText*>(view.get_column_cell_renderer(1));
	if (tvc && cr)
	{
		tvc->add_attribute(cr->property_weight(), column.weight);
		tvc->add_attribute(cr->property_foreground(), column.color);
		tvc->add_attribute(cr->property_strikethrough(), column.strike);
	}

	auto xdoc = crn::xml::Document(path1); // may throw
	auto root = xdoc.GetRoot();

	auto row = *treestore->append();
	row[column.name] = root.GetName().CStr();
	auto toexpand = std::set<Gtk::TreePath>{};
	auto tocollapse = std::set<Gtk::TreePath>{};
	for (auto n = root.BeginNode(); n != root.EndNode(); ++n)
		fill_tree(n, row, toexpand, tocollapse);

	xdoc = crn::xml::Document(path2); // may throw
	root = xdoc.GetRoot();
	for (auto n = root.BeginNode(); n != root.EndNode(); ++n)
		fill_tree(n, row, toexpand, tocollapse);

	// expand to selected rows
	for (const auto &p : toexpand)
	{
		view.expand_to_path(p);
		check_upward(*treestore->get_iter(p));
	}
	// collapse unwanted tree parts
	for (const auto &p : tocollapse)
	{
		view.collapse_row(p);
		uncheck_downward(*treestore->get_iter(p));
	}

	show_all_children();
}

void TEIDisplay::fill_tree(crn::xml::Node &nd, Gtk::TreeModel::Row &row, std::set<Gtk::TreePath> &toexpand, std::set<Gtk::TreePath> &tocollapse)
{
	static const auto keyelems = std::set<Glib::ustring>{ "pb", "cb", "lb", "w", "pc", "seg", "milestone", "c", "g" };

	if (nd.IsText())
	{ // previous element contains text
		row[column.text] = true;
	}
	else if (nd.IsElement())
	{ // element
		auto el = nd.AsElement();
		const auto name = Glib::ustring{nd.GetValue().CStr()};

		// add node or reuse existing one?
		auto newrow = Gtk::TreeModel::Row{};
		bool found = false;
		for (auto &c : row.children())
			if (c[column.name] == name)
			{ // reuse node
				newrow = c;
				found = true;
				break;
			}

		if (!found)
		{ // add node
			newrow = *treestore->append(row.children());
			newrow[column.name] = name;

			if (keyelems.find(name) != keyelems.end())
			{
				newrow[column.weight] = Pango::WEIGHT_HEAVY;
				toexpand.insert(treestore->get_path(newrow));
			}
			if (el.GetAttribute<crn::StringUTF8>("ana", true) != "ori:align-no")
			{ // relevant element
				newrow[column.choice] = true;
				newrow[column.strike] = false;
			}
			else
			{
				tocollapse.insert(treestore->get_path(newrow));
				newrow[column.weight] = Pango::WEIGHT_NORMAL;
				newrow[column.choice] = false;
				newrow[column.strike] = true;
			}
		}

		// add children
		for (auto n = el.BeginNode(); n != el.EndNode(); ++n)
			fill_tree(n, newrow, toexpand, tocollapse);

	} // element
}

void TEIDisplay::check_upward(Gtk::TreeModel::Row row)
{
	if (row)
	{
		row[column.choice] = true;
		row[column.strike] = false;
		check_upward(*row.parent());
	}
}

void TEIDisplay::uncheck_downward(Gtk::TreeModel::Row row)
{
	if (row)
	{
		row[column.choice] = false;
		row[column.strike] = true;
		for (auto &c : row.children())
			uncheck_downward(c);
	}
}

