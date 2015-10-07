/* Copyright 2013-2015 INSA-Lyon, IRHT, ZHAO Xiaojuan
 *
 * file: OriValidationPanel.h
 * \author Yann LEYDIER
 */

#ifndef OriValidationPanel_HEADER
#define OriValidationPanel_HEADER

#include <oriflamms_config.h>
#include <gtkmm.h>
#include <CRNUtils/CRNProgress.h>
#include <OriStruct.h>
#include <OriProject.h>

namespace ori
{
	/*! \brief A panel with images that can be erased by clicking on them */
	class ValidationPanel: public Gtk::Frame
	{
		public:
			struct ElementId
			{
				ElementId(const WordPath &wp, size_t p = 0): path(wp), pos(p) {}
				WordPath path;
				size_t pos;
				inline bool operator<(const ElementId &other) const
				{
					if (path < other.path) return true;
					else if (path == other.path) return pos < other.pos;
					else return false;
				}
			};
			using ElementCluster = std::map<ElementId, Glib::RefPtr<Gdk::Pixbuf>>;
			using ElementList = std::map<crn::StringUTF8, ElementCluster>;

			ValidationPanel(Project &pro, const crn::StringUTF8 &name, const std::vector<crn::Path> &imagenames, bool active_m);
			virtual ~ValidationPanel() override { if (tipthread) tipthread->join(); }

			void add_element(const Glib::RefPtr<Gdk::Pixbuf> &pb, const crn::StringUTF8 cluster, const WordPath &p, size_t pos = 0);
			/*! \brief Erases all elements */
			void clear()
			{
				elements.clear();
				positions.clear();
				scroll.set_value(0);
				modified = false;
			}
			void refresh();
			void full_refresh();
			bool IsModified() const noexcept { return modified; }
			std::set<ElementId> GetContent() const;

			void hide_tooltip() { tipwin.hide(); }

			/*! \brief Prevents the refreshing of the panel (use while adding a batch of elements) */
			void lock() noexcept { locked = true; };
			/*! \brief Reactivates the refreshing of the panel */
			void unlock() noexcept { locked = false; }

			/*! \brief Signal called when an Element (or a bunch of Elements) was clicked on by the user */
			sigc::signal<void, ElementList> signal_removed() { return removed; }
			static const crn::StringUTF8 label_ok; /*!< class name for good elements */
			static const crn::StringUTF8 label_ko; /*!< class name for bad elements */
			static const crn::StringUTF8 label_unknown; /*!< class name for unvalidated elements */
			static const crn::StringUTF8 label_revalidated; /*!< class name for revalidated elements */

		private:
			/*! \brief Sets the pen color of a Cairo Context from a class name */
			static void set_color(Cairo::RefPtr<Cairo::Context> &cc, const crn::StringUTF8 &label);

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
			/*! \brief Loads tooltip image */
			void load_tooltip_img();
			/*! \brief Displays tooltip image */
			void set_tooltip_img();

			Gtk::DrawingArea da;
			Glib::RefPtr<Gdk::GC> da_gc;
			Gtk::VScrollbar scroll;

			Gtk::Window tipwin;
			Gtk::Label tiplab;
			Gtk::Image tipimg;
			WordPath tipword;
			Glib::RefPtr<Gdk::Pixbuf> tippb;
			Glib::Dispatcher tipsig;
			Glib::Thread *tipthread;

			ElementList elements;
			std::map<ElementId, crn::Point2DInt> positions;
			int dispw, disph;
			static const int margin = 10;
			std::vector<crn::Point2DInt> mark;
			bool marking;
			bool locked;
			crn::StringUTF8 title;
			int nelem;
			bool modified;
			std::vector<crn::StringUTF8> images;
			sigc::signal<void, ElementList> removed;
			Project &project;
			bool active_mouse;

	};
}
#endif
