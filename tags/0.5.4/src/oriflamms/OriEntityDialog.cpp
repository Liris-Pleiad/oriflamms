/* Copyright 2014 INSA-Lyon, IRHT, ZHAO Xiaojuan
 *
 * file: OriEntityDialog.cpp
 * \author ZHAO Xiaojuan, Yann LEYDIER
 */
#include <OriEntityDialog.h>
#include <CRNString.h>
#include "OriConfig.h"
#include <CRNi18n.h>


using namespace std;
using namespace ori;

EntityDialog::EntityDialog(Gtk::Window &parent):
	Gtk::Dialog(_("Entity manager"), parent, true)
{
	resize(300, 600);
	m_ScrolledWindow.add(view);
	m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

	add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
	std::vector<int> altbut;
	altbut.push_back(Gtk::RESPONSE_ACCEPT);
	altbut.push_back(Gtk::RESPONSE_CANCEL);
	set_alternative_button_order_from_array(altbut);	
	set_default_response(Gtk::RESPONSE_ACCEPT);

	butt_add_file.set_image(*Gtk::manage(new Gtk::Image( Gtk::Stock::DIRECTORY,Gtk::ICON_SIZE_BUTTON)));
	butt_add_file.set_label(_("Add _file"));
	butt_add_file.set_use_underline();

	butt_add_word.set_image(*Gtk::manage(new Gtk::Image(Gtk::Stock::ADD, Gtk::ICON_SIZE_BUTTON)));
	butt_add_word.set_label(_("Add _entity"));
	butt_add_word.set_use_underline();

	butt_delete_entity.set_image(*Gtk::manage(new Gtk::Image(Gtk::Stock::DELETE, Gtk::ICON_SIZE_BUTTON)));
	butt_delete_entity.set_label(_("_Delete entity"));
	butt_delete_entity.set_use_underline();

	Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox);
	hbox->pack_start(butt_add_file, false, true, 4);
	hbox->pack_start(butt_add_word,false,true,4);
	hbox->pack_start(butt_delete_entity,false,true,4);

	hbox->show();
	get_vbox()->pack_start(*hbox, false, true, 4);

	butt_add_file.show();
	butt_add_word.show();
	butt_delete_entity.show();

	butt_add_file.signal_clicked().connect(sigc::mem_fun(this, &EntityDialog::dialogAddFile));
	butt_add_word.signal_clicked().connect(sigc::mem_fun(this, &EntityDialog::dialogAddEntity));
	butt_delete_entity.signal_clicked().connect(sigc::mem_fun(this, &EntityDialog::dialogDelEntity));

	butt_delete_entity.set_sensitive(false);

	get_vbox()->pack_end(m_ScrolledWindow);

	m_reftree = Gtk::TreeStore::create(column);
	view.set_model(m_reftree);

	view.append_column_editable(_("Entity"), column.key);
	view.append_column_editable(_("XML value"), column.value_xml);
	view.append_column(_("Unicode value"), column.value_uni);

	Gtk::CellRendererText *modify = dynamic_cast<Gtk::CellRendererText*>(view.get_column_cell_renderer(1));
	modify->signal_edited().connect(sigc::mem_fun(this, &EntityDialog::modifyValue));

	// choose a column for the sort
	for (int tmp = 0; tmp < 3; ++tmp)
	{
		Gtk::TreeViewColumn *col = view.get_column(tmp);
		if (col)
			col->set_sort_column(tmp);
	}

	view.signal_cursor_changed().connect(sigc::mem_fun(*this,&EntityDialog::canDelete));

	Pango::FontDescription fdesc;
	fdesc.set_family("Palemonas MUFI");
	fdesc.set_absolute_size(12 * Pango::SCALE);
	Gtk::CellRendererText *c = dynamic_cast<Gtk::CellRendererText*>(view.get_column_cell_renderer(2));
	c->property_font_desc() = fdesc;

	refresh_entities();
	show_all();
}

void EntityDialog::dialogAddFile()
{
	Gtk::FileChooserDialog dialog(*this, _("Please choose a entity file to add"), Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.set_modal(true);
	dialog.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	Gtk::CheckButton bt(_("_Overwrite existing entities"));
	dialog.get_vbox()->pack_start(bt, false, true, 4);
	bt.show();
	int result = dialog.run();

	//Handle the response:
	if(result == Gtk::RESPONSE_OK)
	{
		ori::EntityManager::AddEntityFile(crn::StringUTF8{dialog.get_filename()},bt.get_active());
		refresh_entities();
	}
}

class addEntityDialog: public Gtk::Dialog
{
	public:
		addEntityDialog(Gtk::Window &parent):
			Gtk::Dialog(_("Add entity"), parent, true)
		{
			add_button(Gtk::Stock::ADD, Gtk::RESPONSE_ACCEPT);
			add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
			std::vector<int> altbut;
			altbut.push_back(Gtk::RESPONSE_ACCEPT);
			altbut.push_back(Gtk::RESPONSE_CANCEL);
			set_alternative_button_order_from_array(altbut);	
			set_default_response(Gtk::RESPONSE_ACCEPT);

			Gtk::HBox *hb = Gtk::manage(new Gtk::HBox);
			get_vbox()->pack_start(*hb);
			hb->pack_start(*Gtk::manage(new Gtk::Label(_("Name"))), false, true);
			hb->pack_start(ek);
			hb->show_all();

			hb = Gtk::manage(new Gtk::HBox);
			get_vbox()->pack_start(*hb);
			hb->pack_start(*Gtk::manage(new Gtk::Label(_("Value"))), false, true);
			hb->pack_start(ev);
			ev.set_activates_default();
			hb->show_all();

			set_response_sensitive(Gtk::RESPONSE_ACCEPT, false);

			ev.signal_changed().connect(sigc::mem_fun(*this, &addEntityDialog::checkEnable));
			ek.signal_changed().connect(sigc::mem_fun(*this, &addEntityDialog::checkEnable));

		}
		crn::StringUTF8 get_name() const { return ek.get_text().c_str(); }
		crn::StringUTF8 get_value() const { return ev.get_text().c_str(); }
	private:
		void checkEnable()
		{
			set_response_sensitive(Gtk::RESPONSE_ACCEPT, !ev.get_text().empty() && !ek.get_text().empty());
		}
		Gtk::Entry ek;
		Gtk::Entry ev;
};
void EntityDialog::dialogAddEntity()
{
	addEntityDialog dial(*this);
	if (dial.run() == Gtk::RESPONSE_ACCEPT)
	{
		EntityManager::AddEntity(dial.get_name(), dial.get_value());
		refresh_entities();
	}
}

// show all the entity of the file "orient.xml" on the window
void EntityDialog::refresh_entities()
{
	m_reftree->clear();
	for (const auto &p : EntityManager::GetMap())
	{
		Gtk::TreeModel::iterator it = m_reftree->append();
		Gtk::TreeModel::Row row = *it;
		Glib::ustring k(p.first.CStr());
		row[column.key] = k;
		Glib::ustring va_xml(p.second.CStr());
		row[column.value_xml] = va_xml;
		Glib::ustring val(EntityManager::ExpandUnicodeEntities(p.second).CStr());
		row[column.value_uni] = val;
	}
}

void EntityDialog::modifyValue(const Glib::ustring& path, const Glib::ustring& new_text)
{
	Gtk::TreeModel::Row row = *m_reftree->get_iter(path);
	crn::StringUTF8 s(new_text.c_str());
	Glib::ustring val(EntityManager::ExpandUnicodeEntities(s).CStr());
	row[column.value_uni] = val;
	EntityManager::AddEntity(Glib::ustring(row[column.key]).c_str(), s, true);
}

void EntityDialog::dialogDelEntity()
{
	Glib::RefPtr<Gtk::TreeSelection> selection = view.get_selection();
	Gtk::TreeModel::iterator selectedRow = selection->get_selected();
	Gtk::TreeModel::Row row = *selectedRow;
	Glib::ustring k = row[column.key];
	crn::StringUTF8 ke(k.c_str());
	EntityManager::RemoveEntity(ke);
	m_reftree->erase(selectedRow);
	butt_delete_entity.set_sensitive(false);
}

void EntityDialog::canDelete()
{
	Glib::RefPtr<Gtk::TreeSelection> selection = view.get_selection();
	if (!selection)
		butt_delete_entity.set_sensitive(false);
	else
		butt_delete_entity.set_sensitive(selection->get_selected() != m_reftree->children().end());
}

