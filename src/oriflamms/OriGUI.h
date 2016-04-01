/* Copyright 2013-2016 INSA-Lyon, IRHT, ZHAO Xiaojuan, Université Paris Descartes, ENS-Lyon
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
#include <OriDocument.h>
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
			void load_project();
			void setup_window();
			void set_win_title();
			void show_tei();
			void add_line();
			void rem_lines();
			void rem_line(const Id &l);
			void add_point_to_line(const Id &l, int x, int y);
			void rem_point_from_line(const Id &l, int x, int y);
			void rem_superline(const Id &colid, size_t num);
			virtual void about() override;
			Glib::RefPtr<Gtk::TreeStore> fill_tree(crn::Progress *prog);
			void tree_selection_changed(bool focus);
			void edit_overlays(bool refresh);
			void overlay_changed(crn::String overlay_id, crn::String overlay_item_id, GtkCRN::Image::MouseMode mm);
			void on_rmb_clicked(guint mouse_button, guint32 time, std::vector<std::pair<crn::String, crn::String> > overlay_items_under_mouse, int x_on_image, int y_on_image);
			void align_selection();
			void align_all();
			void validate();
			void set_need_save();
			void save_project();
			void on_close();
			void change_font();
			void stats();
			void display_words(const Id &linid);
			void display_line(const Id &linid);
			void display_supernumerarylines(const Id &colid);
			void display_characters(const Id &linid);
			void display_update_word(const Id &wordid, const crn::Option<int> &newleft = crn::Option<int>(), const crn::Option<int> &newright = crn::Option<int>());
			void clear_sig();
			void show_chars();
			void go_to();

			void propagate_validation();
			void find_string();
			void display_search(Gtk::Entry* entry, ori::ValidationPanel *panel);

			class model: public Gtk::TreeModelColumnRecord
			{
				public:
					model() { add(name); add(xml); add(image); add(error); add(text); add(id); }
					Gtk::TreeModelColumn<Glib::ustring> id;
					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<Glib::ustring> xml; // number of sub-elements in XML
					Gtk::TreeModelColumn<Glib::ustring> image; // number of sub-elements in image
					Gtk::TreeModelColumn<Pango::Style> error;
					Gtk::TreeModelColumn<Glib::ustring> text; // transcription
			};
			model columns;
			Glib::RefPtr<Gtk::TreeStore> store;
			Gtk::TreeView tv;
			GtkCRN::Image img;
			Id current_view_id;
			View current_view;
			enum class ViewDepth { None, View, Page, Column, Line };
			ViewDepth view_depth;
			sigc::connection line_rem_connection;
			sigc::connection line_add_point_connection;
			sigc::connection line_rem_point_connection;
			sigc::connection superline_rem_connection;
			std::unique_ptr<Validation> validation_win;
			AlignDialog aligndial;

			std::unique_ptr<Document> doc;
			bool need_save;

			static const Glib::ustring wintitle;
			static const crn::String linesOverlay;
			static const crn::String superlinesOverlay;
			static const crn::String wordsOverlay;
			static const crn::String wordsOverlayOk;
			static const crn::String wordsOverlayKo;
			static const crn::String wordsOverlayUn;
			static const crn::String charOverlay;

			static const int minwordwidth;
			static const int mincharwidth;
	};
}

#endif


