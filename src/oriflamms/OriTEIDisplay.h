/* Copyright 2014-2016 INSA-Lyon, IRHT, ZHAO Xiaojuan, Université Paris Descartes, ENS-Lyon
 *
 * file: OriTEIDisplay.h
 * \author ZHAO Xiaojuan, Yann LEYDIER
 */

#ifndef TEIIMPORTER_H
#define TEIIMPORTER_H
#include <CRNUtils/CRNXml.h>
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

