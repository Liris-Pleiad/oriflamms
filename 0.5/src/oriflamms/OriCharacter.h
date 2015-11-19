/* Copyright 2015 Université Paris Descartes
 * 
 * file: OriCharacter.h
 * \author Yann LEYDIER
 */

#ifndef OriCharacter_HEADER
#define OriCharacter_HEADER

#include <oriflamms_config.h>
#include <gtkmm.h>
#include <OriDocument.h>

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

			Document &doc;
			std::map<crn::String, std::unordered_map<Id, std::vector<Id>>> characters;

			Gtk::TreeView tv;
			Glib::RefPtr<Gtk::ListStore> store;
			model columns;
			Gtk::Label dm_ok;
			Gtk::Button compute_dm;
			Gtk::Button clear_dm;
			Gtk::Button compute_clusters;
			Gtk::Button show_clusters;
			Gtk::Button clear_clusters;
	};
}

#endif


