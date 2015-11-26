/* Copyright 2014 INSA-Lyon, IRHT, ZHAO Xiaojuan
 *
 * file: OriEntityDialog.h
 * \author ZHAO Xiaojuan, Yann LEYDIER
 */
#ifndef ENTITY_DIALOG_H
#define ENTITY_DIALOG_H

#include <OriEntityManager.h>
#include <gtkmm.h>

namespace ori
{
	class EntityDialog:public Gtk::Dialog
	{
		public:
			EntityDialog(Gtk::Window &parent);

		private:
			void dialogAddFile();
			void dialogAddEntity();
			void dialogDelEntity();
			void canDelete();
			void refresh_entities();
			void modifyValue(const Glib::ustring& path, const Glib::ustring& new_text);
			class recolumn :public Gtk::TreeModel::ColumnRecord
			{
				public:
					recolumn(void) { add(key); add(value_xml); add(value_uni); }
					Gtk::TreeModelColumn<Glib::ustring> key;
					Gtk::TreeModelColumn<Glib::ustring> value_xml;
					Gtk::TreeModelColumn<Glib::ustring> value_uni;
			};

			Gtk::ScrolledWindow m_ScrolledWindow;

			Gtk::TreeView view;
			Glib::RefPtr<Gtk::TreeStore> m_reftree;
			recolumn column;

			Gtk::Button butt_add_file;
			Gtk::Button butt_add_word;
			Gtk::Button butt_delete_entity;
	};
}
#endif

