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
 * \file OriCharacter.h
 * \author Yann LEYDIER
 */

#ifndef OriCharacter_HEADER
#define OriCharacter_HEADER

#include <oriflamms_config.h>
#include <gtkmm.h>
#include <OriDocument.h>
#include <OriValidationPanel.h>

namespace ori
{
	class CharacterDialog: public Gtk::Dialog
	{
		public:
			CharacterDialog(Document &docu, Gtk::Window &parent);

		private:
			class model: public Gtk::TreeModel::ColumnRecord
			{
				public:
					model() { add(value); add(count); }
					Gtk::TreeModelColumn<Glib::ustring> value;
					Gtk::TreeModelColumn<size_t> count;
			};

			void update_buttons();
			void compute_distmat();
			void update_distmat();
			void compute_gm(const crn::String &character, const std::vector<Id> &ids, crn::SquareMatrixDouble &dm, crn::Progress *prog);
			void compute_gsc(const crn::String &character, const std::vector<Id> &ids, crn::SquareMatrixDouble &dm, crn::Progress *prog);
			void delete_dm();
			void show_clust();

			Document &doc;
			std::map<crn::String, std::unordered_map<Id, std::vector<Id>>> characters;

			Gtk::TreeView tv;
			Glib::RefPtr<Gtk::ListStore> store;
			model columns;
			Gtk::Label dm_ok;
			Gtk::Button compute_dm;
			Gtk::Button update_dm;
			Gtk::Button clear_dm;
			Gtk::Button show_clusters;
	};

	class CharacterTree: public Gtk::Dialog
	{
		public:
			CharacterTree(const crn::String &c, Document &docu, Gtk::Window &parent);
		private:
			class model: public Gtk::TreeModel::ColumnRecord
			{
				public:
					model() { add(value); add(count); }
					Gtk::TreeModelColumn<Glib::ustring> value;
					Gtk::TreeModelColumn<size_t> count;
			};

			void init(crn::Progress *prog);
			void refresh_tv();
			void add_children(Gtk::TreeIter &it, const Id &gid, const std::map<Id, std::set<Id>> &children);
			void sel_changed();
			void on_remove_chars(ValidationPanel::ElementList words);
			void on_unremove_chars(ValidationPanel::ElementList words);
			void change_label(ValidationPanel &p);
			void remove_from_cluster(ValidationPanel &p);
			std::pair<Id, Id> cut(const Id &gid, const std::vector<Id> &chars, crn::Progress *prog);
			void cut_cluster(ValidationPanel &p);
			void clear_clustering();
			void auto_clustering();
			void auto_cut(crn::Progress *prog);

			crn::String character;
			Document &doc;
			std::unordered_map<Id, std::set<Id>> clusters; // glyph id -> { character id }
			Id current_glyph;

			Gtk::TreeView tv;
			Glib::RefPtr<Gtk::TreeStore> store;
			model columns;
			ValidationPanel panel;
			ValidationPanel kopanel;
			std::unordered_map<Id, Glib::RefPtr<Gdk::Pixbuf>> images;
			std::unordered_map<Id, Glib::RefPtr<Gdk::Pixbuf>> tipimages;
			Gtk::Button clear_clusters;
			Gtk::Button auto_clusters;
			Gtk::Button label_ok;
			Gtk::Button label_ko;
			Gtk::Button cut_ok;
			Gtk::Button cut_ko;
			Gtk::Button remove_ok;
			Gtk::Button remove_ko;
	};

	class GlyphSelection: public Gtk::TreeView
	{
		public:
			GlyphSelection(Document &docu);
			Id get_selected_id();
			void add_glyph(const Id &gid, const Glyph &g);
			void add_glyph_dialog(const Id &parent_id, Gtk::Window *parent);
			void add_subglyph_dialog(Gtk::Window *parent);

		private:
			class model: public Gtk::TreeModel::ColumnRecord
			{
				public:
					model() { add(id); add(global); add(desc); add(automatic); }
					Gtk::TreeModelColumn<Glib::ustring> id;
					Gtk::TreeModelColumn<bool> global;
					Gtk::TreeModelColumn<Glib::ustring> desc;
					Gtk::TreeModelColumn<bool> automatic;
			};
			model columns;
			Glib::RefPtr<Gtk::ListStore> store;
			Document &doc;
	};
}

#endif


