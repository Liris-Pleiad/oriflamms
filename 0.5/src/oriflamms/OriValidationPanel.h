/* Copyright 2013-2015 INSA-Lyon, IRHT, ZHAO Xiaojuan, Université Paris Descartes
 *
 * file: OriValidationPanel.h
 * \author Yann LEYDIER
 */

#ifndef OriValidationPanel_HEADER
#define OriValidationPanel_HEADER

#include <oriflamms_config.h>
#include <gtkmm.h>
#include <CRNUtils/CRNProgress.h>
#include <OriDocument.h>

namespace ori
{
	/*! \brief A panel with images that can be erased by clicking on them */
	class ValidationPanel: public Gtk::Frame
	{
		public:
			struct ElementId
			{
				ElementId(const ElementPosition &wid, const Id &cid = ""): word_id(wid), char_id(cid) {}
				ElementPosition word_id;
				Id char_id;
				inline bool operator<(const ElementId &other) const noexcept
				{
					if (word_id < other.word_id) return true;
					else if (word_id == other.word_id) return char_id < other.char_id;
					else return false;
				}
			};
			struct Element
			{
				Element(const Glib::RefPtr<Gdk::Pixbuf> &i, const Glib::RefPtr<Gdk::Pixbuf> &ti, const Glib::ustring &t):img(i), context(ti), tip(t) {}
				Glib::RefPtr<Gdk::Pixbuf> img;
				Glib::RefPtr<Gdk::Pixbuf> context;
				Glib::ustring tip;
			};
			using ElementCluster = std::map<ElementId, Element>;
			using ElementList = std::map<crn::StringUTF8, ElementCluster>;

			ValidationPanel(Document &docu, const crn::StringUTF8 &name, bool active_m);
			virtual ~ValidationPanel() override { }

			void add_element(const ElementPosition &pos, const crn::StringUTF8 cluster, const Glib::RefPtr<Gdk::Pixbuf> &pb, const Glib::RefPtr<Gdk::Pixbuf> &tip_pb, const Id &char_id = "");
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

			const ElementList& get_elements() const noexcept { return elements; }

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

			Gtk::DrawingArea da;
			Glib::RefPtr<Gdk::GC> da_gc;
			Gtk::VScrollbar scroll;

			Gtk::Window tipwin;
			Gtk::Label tiplab;
			Gtk::Image tipimg;
			ElementPosition tipword;

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
			sigc::signal<void, ElementList> removed;
			Document &doc;
			bool active_mouse;

	};
}
#endif
