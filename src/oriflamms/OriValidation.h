/* Copyright 2013 INSA-Lyon & IRHT
 * 
 * file: OriValidation.h
 * \author Yann LEYDIER
 */

#ifndef OriValidation_HEADER
#define OriValidation_HEADER

#include <oriflamms_config.h>
#include <gtkmm.h>
#include <CRNUtils/CRNProgress.h>
#include <OriStruct.h>
#include <OriProject.h>
#include <CRNUtils/CRNFunctor.h>

namespace ori
{
	class Validation: public Gtk::Window
	{
		private:
			/*! \brief A panel with images that can be erased by clicking on them */
			class ValidationPanel: public Gtk::Frame
			{
				public:
					typedef std::map<WordPath, Glib::RefPtr<Gdk::Pixbuf> > WordCluster;
					typedef std::map<crn::StringUTF8, WordCluster> WordList;

					ValidationPanel(const crn::StringUTF8 &name, const std::vector<crn::Path> &imagenames);
					void add_element(const crn::StringUTF8 cluster, const WordPath &p, const Glib::RefPtr<Gdk::Pixbuf> &pb);
					void clear() { elements.clear(); positions.clear(); scroll.set_value(0); modified = false; }
					void refresh();
					void full_refresh();
					bool IsModified() const { return modified; }
					const std::set<WordPath> GetContent() const;

					void lock() { locked = true; };
					void unlock() { locked = false; }

					sigc::signal<void, WordList> signal_removed() { return removed; }

				private:
					/*! \brief Asks for a refresh */
					bool expose(GdkEventExpose *ev);
					/*! \brief Resized */
					bool configure(GdkEventConfigure *ev);
					/*! \brief Handles mouse motion */
					bool mouse_motion(GdkEventMotion *ev);
					/*! \brief Handles mouse click */
					bool button_clicked(GdkEventButton *ev);
					/*! \brief Scrolling */
					bool on_scroll(GdkEventScroll *ev);
					/*! \brief Init tooltip */
					bool tooltip(int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip);

					Gtk::DrawingArea da;
					Glib::RefPtr<Gdk::GC> da_gc;
					Gtk::VScrollbar scroll;

					WordList elements;
					std::map<WordPath, crn::Point2DInt> positions;
					int dispw, disph;
					static const int margin = 10;
					std::vector<crn::Point2DInt> mark;
					bool marking;
					bool locked;
					crn::StringUTF8 title;
					int nelem;
					bool modified;
					std::vector<crn::StringUTF8> images;
					sigc::signal<void, WordList> removed;
			};

		public:
			Validation(Gtk::Window &parent, Project &proj, bool batch_valid, bool use_clustering, const crn::Functor<void> &savefunc, const crn::Functor<void> &refreshfunc);

			static void set_color(Cairo::RefPtr<Cairo::Context> &cc, const crn::StringUTF8 &label);
		private:
			void update_words(crn::Progress *prog);
			void tree_selection_changed();
			void read_word(const Glib::ustring &wname, crn::Progress *prog = NULL);
			void on_remove_words(ValidationPanel::WordList words);
			void on_unremove_words(ValidationPanel::WordList words);
			void on_close();
			void conclude_word();

			// widgets
			class model: public Gtk::TreeModelColumnRecord
			{
				public:
					model() { add(name); add(pop); add(size); add(weight); }
					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<size_t> pop;
					Gtk::TreeModelColumn<size_t> size;
					Gtk::TreeModelColumn<int> weight;
			};
			model columns;
			Glib::RefPtr<Gtk::ListStore> store;
			Gtk::TreeView tv;
			ValidationPanel okwords, kowords;

			// attributes
			Project &project;
			std::map<crn::String, std::vector<WordPath> > words;
			bool firstrun;
			std::set<WordPath> needconfirm;
			bool needsave, batch, clustering;
			crn::Functor<void> saveatclose; // XXX why cannot I keep a const ref???
			crn::Functor<void> refreshatclose; // XXX why cannot I keep a const ref???
			static const crn::StringUTF8 label_ok;
			static const crn::StringUTF8 label_ko;
			static const crn::StringUTF8 label_unknown;
			static const crn::StringUTF8 label_revalidated;
	};
}

#endif


