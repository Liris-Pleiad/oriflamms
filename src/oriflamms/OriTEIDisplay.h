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
 * \file OriTEIDisplay.h
 * \author ZHAO Xiaojuan, Yann LEYDIER
 */

#ifndef TEIIMPORTER_H
#define TEIIMPORTER_H
#include <CRNXml/CRNXml.h>
#include <set>
#include <gtkmm.h>

namespace ori
{
	/*! \brief Displays the structure of a TEI document and allows the selection of paths */
	class TEIDisplay: public Gtk::Dialog
	{
		public:
			TEIDisplay(const crn::Path &path1, const crn::Path &path2, Gtk::Window &parent);
			virtual ~TEIDisplay(void) override {}

		private:
			class recolumn :public Gtk::TreeModel::ColumnRecord
			{
				public:
					recolumn(void) { add(name); add(text); add(weight); add(color); add(choice); add(strike); }

					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<bool> text;
					Gtk::TreeModelColumn<int> weight;
					Gtk::TreeModelColumn<Glib::ustring> color;
					Gtk::TreeModelColumn<bool> choice;
					Gtk::TreeModelColumn<bool> strike;
			};

			recolumn column;

			Gtk::ScrolledWindow scrollwin;
			Gtk::TreeView view;
			Glib::RefPtr<Gtk::TreeStore> treestore;

			void fill_tree(crn::xml::Node &nd, Gtk::TreeModel::Row &row, std::set<Gtk::TreePath> &toexpand, std::set<Gtk::TreePath> &tocollapse);
			void check_upward(Gtk::TreeModel::Row row);
			void uncheck_downward(Gtk::TreeModel::Row row);
	};

}
#endif

