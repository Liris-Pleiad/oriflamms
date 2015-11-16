/* Copyright 2013-2015 INSA-Lyon, IRHT, ZHAO Xiaojuan, Université Paris Descartes
 *
 * file: OriGUI.h
 * \author Yann LEYDIER
 */

#ifndef OriGUI_HEADER
#define OriGUI_HEADER

#include <oriflamms_config.h>
#include <GtkCRNApp.h>
#include <GtkCRNImage.h>
#include <CRNUtils/CRNProgress.h>
#include <OriProject.h>
#include <OriValidation.h>
#include <CRNUtils/CRNOption.h>
#include <OriAlignDialog.h>

namespace ori
{
	class GUI: public GtkCRN::App
	{
		public:
			GUI();
			virtual ~GUI() override {}

		private:
			void new_project();
			void load_project();
			void import_project();
			void setup_window();
			void set_win_title();
			void add_line();
			void rem_line(size_t v, size_t c, size_t l);
			void add_point_to_line(size_t v, size_t c, size_t l, int x, int y);
			void rem_point_from_line(size_t v, size_t c, size_t l, int x, int y);
			virtual void about() override;
			Glib::RefPtr<Gtk::TreeStore> fill_tree(crn::Progress *prog);
			void tree_selection_changed(bool focus);
			void edit_overlays();
			void overlay_changed(crn::String overlay_id, crn::String overlay_item_id, GtkCRN::Image::MouseMode mm);
			void on_rmb_clicked(guint mouse_button, guint32 time, std::vector<std::pair<crn::String, crn::String> > overlay_items_under_mouse, int x_on_image, int y_on_image);
			void reload_tei();
			void align_selection();
			void align_all();
			void validate();
			void set_need_save();
			void save_project();
			void on_close();
			void change_font();
			void manage_entities();
			void stats();
			void display_word(size_t viewid, size_t colid, size_t linid);
			void display_line(size_t viewid, size_t colid, size_t linid);
			void display_characters(size_t viewid, size_t colid, size_t linid);
			void display_update_word(const WordPath &path, const crn::Option<int> &newleft = crn::Option<int>(), const crn::Option<int> &newright = crn::Option<int>());
			void clear_sig();

			void propagate_validation();
			void export_tei_alignment();
			void find_string();
			void display_search(Gtk::Entry* entry, ori::ValidationPanel *panel);

			class model: public Gtk::TreeModelColumnRecord
			{
				public:
					model() { add(name); add(xml); add(image); add(error); add(text); add(index); }
					Gtk::TreeModelColumn<size_t> index;
					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<Glib::ustring> xml;
					Gtk::TreeModelColumn<Glib::ustring> image;
					Gtk::TreeModelColumn<Pango::Style> error;
					Gtk::TreeModelColumn<Glib::ustring> text;
			};
			model columns;
			Glib::RefPtr<Gtk::TreeStore> store;
			Gtk::TreeView tv;
			GtkCRN::Image img;
			size_t current_view_id;
			enum class ViewDepth { None, Page, Column, Line };
			ViewDepth view_depth;
			sigc::connection line_rem_connection;
			sigc::connection line_add_point_connection;
			sigc::connection line_rem_point_connection;
			std::unique_ptr<Validation> validation_win;
			AlignDialog aligndial;

			std::unique_ptr<Project> project;
			bool need_save;

			static const Glib::ustring wintitle;
			static const crn::String linesOverlay;
			static const crn::String wordsOverlay;
			static const crn::String wordsOverlayOk;
			static const crn::String wordsOverlayKo;
			static const crn::String wordsOverlayUn;
			static const crn::String charOverlay;

			static const int minwordwidth;
	};
}

#endif


