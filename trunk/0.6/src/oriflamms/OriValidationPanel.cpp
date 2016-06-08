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
 * \file OriValidationPanel.h
 * \author Yann LEYDIER
 */

#include <OriValidationPanel.h>
#include <GtkCRNProgressWindow.h>
#include <GdkCRNPixbuf.h>
#include <CRNAI/CRNIterativeClustering.h>
#include <OriConfig.h>
#include <CRNi18n.h>
#include <fstream>
#include <iostream>

using namespace ori;
using namespace crn::literals;

const crn::StringUTF8 ValidationPanel::label_ok("zzz1");
const crn::StringUTF8 ValidationPanel::label_ko("ko");
const crn::StringUTF8 ValidationPanel::label_unknown("0");
const crn::StringUTF8 ValidationPanel::label_revalidated("zzz0");

/*!
 * \param[in]	pro	the oriflamms project
 * \param[in]	name	title of the panel
 * \param[in]	imagenames	the list of the image file name for each view
 * \param[in]	active_m	shall the mouse clicks be processed or ignored?
 */
ValidationPanel::ValidationPanel(Document &docu, const crn::StringUTF8 &name, bool active_m):
	title(name),
	nelem(0),
	dispw(0),
	disph(0),
	marking(false),
	locked(false),
	modified(false),
	doc(docu),
	active_mouse(active_m),
	tipwin(Gtk::WINDOW_POPUP)
{
	Gtk::HBox *hbox(Gtk::manage(new Gtk::HBox));
	add(*hbox);
	hbox->pack_start(da, true, true, 0);

	if (active_mouse)
	{
		da.add_events(Gdk::EXPOSURE_MASK | Gdk::STRUCTURE_MASK | Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::SCROLL_MASK);
		da.signal_motion_notify_event().connect(sigc::mem_fun(this, &ValidationPanel::mouse_motion)); // mouse motion
		da.signal_button_press_event().connect(sigc::mem_fun(this, &ValidationPanel::button_clicked));
		da.signal_button_release_event().connect(sigc::mem_fun(this, &ValidationPanel::button_clicked));
	}
	else
	{
		da.add_events(Gdk::EXPOSURE_MASK | Gdk::STRUCTURE_MASK | Gdk::SCROLL_MASK);
	}
	da.signal_expose_event().connect(sigc::mem_fun(this, &ValidationPanel::expose)); // redraw
	da.signal_configure_event().connect(sigc::mem_fun(this, &ValidationPanel::configure)); // size changed

	da.signal_scroll_event().connect(sigc::mem_fun(this, &ValidationPanel::on_scroll));

	hbox->pack_start(scroll, false, true, 0);
	scroll.get_adjustment()->set_lower(0);
	scroll.get_adjustment()->set_step_increment(10);
	scroll.get_adjustment()->set_page_increment(100);
	scroll.signal_value_changed().connect(sigc::mem_fun(this, &ValidationPanel::refresh));

	da.set_has_tooltip();
	tipwin.set_name("gtk-tooltip");
	hbox = Gtk::manage(new Gtk::HBox);
	tipwin.add(*hbox);
	hbox->pack_start(tipimg, false, true, 2);
	hbox->pack_start(tiplab, false, true, 2);
	tipwin.show_all_children();
	da.signal_query_tooltip().connect(sigc::mem_fun(this, &ValidationPanel::tooltip));
	signal_hide().connect(sigc::mem_fun(tipwin, &Gtk::Widget::hide));

	show_all();
}

/*! Adds an element to the panel
 * \param[in]	pos	the path of the word containing the element
 * \param[in]	cluster	the element's class name
 * \param[in]	pb	the image to be displayed
 * \param[in]	tip_pb	the image to be displayed in tooltip
 * \param[in]	char_id	optional character id
 */
void ValidationPanel::add_element(const ElementPosition &pos, const crn::StringUTF8 cluster, const Glib::RefPtr<Gdk::Pixbuf> &pb, const Glib::RefPtr<Gdk::Pixbuf> &tip_pb, const Id &char_id)
{
	auto msg = crn::StringUTF8{};
	msg += _("View") + " "_s + pos.view + "\n";
	msg += _("Page") + " "_s + pos.page + "\n";
	msg += _("Column") + " "_s += pos.column + "\n";
	msg += _("Line") + " "_s + pos.line + "\n";
	msg += _("Word") + " "_s + pos.word;
	if (char_id.IsNotEmpty())
		msg += "\n"_s + _("Character") + " "_s + char_id;

	elements[cluster].emplace(ElementId{pos, char_id}, Element{pb, tip_pb, msg.CStr()});
	nelem += 1;
}

/*! \return	the list of elements as WordPath/position */
std::set<ValidationPanel::ElementId> ValidationPanel::GetContent() const
{
	std::set<ElementId> cont;
	for (const ValidationPanel::ElementList::value_type &el : elements)
	{
		for (const ValidationPanel::ElementCluster::value_type &w : el.second)
		{
			cont.insert(w.first);
		}
	}
	return cont;
}

bool ValidationPanel::expose(GdkEventExpose *ev)
{
	refresh();
	return false;
}

bool ValidationPanel::configure(GdkEventConfigure *ev)
{
	dispw = ev->width;
	disph = ev->height;
	scroll.get_adjustment()->set_page_size(disph);
	full_refresh();
	return false;
}

bool ValidationPanel::mouse_motion(GdkEventMotion *ev)
{
	if (marking)
	{
		mark.push_back(crn::Point2DInt(int(ev->x), int(ev->y + scroll.get_adjustment()->get_value())));
		refresh();
	}
	return false;
}

bool ValidationPanel::button_clicked(GdkEventButton *ev)
{
	if (ev->button == 1)
	{
		// left click
		if (ev->type == GDK_BUTTON_PRESS)
		{
			mark.emplace_back(crn::Point2DInt(int(ev->x), int(ev->y + scroll.get_adjustment()->get_value())));
			marking = true;
			refresh();
		}
		else if (ev->type == GDK_BUTTON_RELEASE)
		{
			marking = false;
			ElementList keep, rem;
			int nrem = 0;
			int offset = -int(scroll.get_adjustment()->get_value());
			for (const ValidationPanel::ElementList::value_type &el : elements)
			{
				for (const ValidationPanel::ElementCluster::value_type &w : el.second)
				{
					const crn::Point2DInt &pos(positions[w.first]);
					crn::Rect bbox(pos.X, pos.Y + offset,
							pos.X + w.second.img->get_width(), pos.Y + offset + w.second.img->get_height());
					bool found = false;
					for (const crn::Point2DInt &p : mark)
					{
						if (bbox.Contains(p.X, p.Y + offset))
						{
							found = true;
							break;
						}
					}
					if (found)
					{
						rem[el.first].insert(w);
						nrem += 1;
					}
					else
						keep[el.first].insert(w);
				}
			}
			if (!rem.empty())
			{
				elements.swap(keep);
				nelem -= nrem;
				removed.emit(rem);
				modified = true;
			}
			mark.clear();
			/*full_*/refresh();
		}
	}
	return false;
}

bool ValidationPanel::on_scroll(GdkEventScroll *ev)
{
	double p = scroll.get_value();
	switch (ev->direction)
	{
		case GDK_SCROLL_UP:
			p -= scroll.get_adjustment()->get_page_increment();
			if (p < 0)
				p = 0;
			break;
		case GDK_SCROLL_DOWN:
			p += scroll.get_adjustment()->get_page_increment();
			if (p + disph > scroll.get_adjustment()->get_upper())
				p = crn::Max(0.0, scroll.get_adjustment()->get_upper() - disph);
			break;
	}
	scroll.set_value(p);
	return false;
}

void ValidationPanel::refresh()
{
	// update the label
	crn::StringUTF8 lab = "<span font_desc=\"" + Config::GetFont() + "\">" + title + "</span>";
	lab += " (";
	lab += nelem;
	lab += ")";
	auto *labw = get_label_widget();
	auto *llab = dynamic_cast<Gtk::Label*>(labw);
	if (llab)
		llab->set_markup(lab.CStr());
	else
		set_label(lab.CStr());
	

	if (/*(positions.size() != elements.size()) ||*/ (dispw <= 0) || (disph <= 0) || locked)
		return;

	Glib::RefPtr<Gdk::Window> win = da.get_window();
	if (win)
	{
		// if the drawing area is fully created
		// create a buffer
		Glib::RefPtr<Gdk::Pixmap> pm = Gdk::Pixmap::create(win, dispw, disph);
		// clear the buffer
		pm->draw_rectangle(da_gc, true, 0, 0, dispw, disph);
		int offset = -int(scroll.get_value());

		Cairo::RefPtr<Cairo::Context> cc = pm->create_cairo_context();


		// draw the words
		for (const ValidationPanel::ElementList::value_type &el : elements)
		{
			if (!el.second.empty())
			{
				int by = positions[el.second.begin()->first].Y;
				int ey = by;
				for (const ValidationPanel::ElementCluster::value_type &w : el.second)
				{
					int y = positions[w.first].Y + w.second.img->get_height();
					if (y > ey) ey = y;
				}
				if ((el.first == elements.rbegin()->first) && (ey < disph))
				{
					// fill till the end of the display
					ey = disph;
				}
				ValidationPanel::set_color(cc, el.first);
				cc->rectangle(0, by + offset, dispw, ey - by);
				cc->fill();
			}
			for (const ValidationPanel::ElementCluster::value_type &w : el.second)
			{
				Glib::RefPtr<Gdk::Pixbuf> wpb = w.second.img;

				pm->draw_pixbuf(da_gc, wpb, 0, 0, positions[w.first].X, positions[w.first].Y + offset, w.second.img->get_width(), w.second.img->get_height(), Gdk::RGB_DITHER_NONE, 0, 0);
			}
		}

		// draw the mark
		if (mark.size() > 1)
		{
			cc->set_source_rgb(1.0, 0, 0);
			cc->move_to(mark.front().X, mark.front().Y + offset);
			for (size_t tmp = 1; tmp < mark.size(); ++tmp)
			{
				cc->line_to(mark[tmp].X, mark[tmp].Y + offset);
			}
			cc->stroke();
		}

		// copy pixmap to drawing area
		win->draw_drawable(da_gc, pm, 0, 0, 0, 0);
	}

}

void ValidationPanel::full_refresh()
{
	if (locked)
		return;

	positions.clear();
	nelem = 0;

	Glib::RefPtr<Gdk::Window> win = da.get_window();
	if (win)
	{
		// if the drawing area is fully created
		// update the GC
		da_gc = Gdk::GC::create(win);
		da_gc->set_rgb_fg_color(Gdk::Color("white"));
	}

	int x = margin, y = 0;
	int h = 0;
	for (std::map<crn::StringUTF8, ElementCluster>::iterator cit = elements.begin(); cit != elements.end(); ++cit)
	{
		if ((y != 0) || (x != margin))
			y += h + margin;
		x = margin;
		h = 0;
		nelem += int(cit->second.size());
		for (ElementCluster::iterator wit = cit->second.begin(); wit != cit->second.end(); ++wit)
		{
			if ((x + wit->second.img->get_width() >= dispw) && (x > 0) && (h > 0))
			{
				// wont fit
				x = margin;
				y += h + margin;
				h = 0;
			}
			positions.insert(std::make_pair(wit->first, crn::Point2DInt(x, y)));
			x += wit->second.img->get_width() + margin;
			h = crn::Max(h, wit->second.img->get_height());
			if (x >= dispw)
			{
				// the margin made us go off bounds
				x = margin;
				y += h + margin;
				h = 0;
			}
		}
	}
	// compute full height and scrollbar
	h += y;
	scroll.get_adjustment()->set_upper(h);
	if (scroll.get_value() + disph > h)
		scroll.set_value(crn::Max(0, h - disph));

	refresh();

}

bool ValidationPanel::tooltip(int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip)
{
	int offset = -int(scroll.get_adjustment()->get_value());
	for (const ValidationPanel::ElementList::value_type &el : elements)
	{

		for (const ValidationPanel::ElementCluster::value_type &w : el.second)
		{
			const crn::Point2DInt &pos(positions[w.first]);
			crn::Rect bbox(pos.X, pos.Y + offset,
					pos.X + w.second.img->get_width(), pos.Y + offset + w.second.img->get_height());
			if (bbox.Contains(x, y))
			{
				if (w.first.word_id != tipword)
				{
					tiplab.set_text(w.second.tip);
					tipimg.set(w.second.context);
				}

				Glib::RefPtr<Gdk::Window> win(da.get_window());
				if (!win)
					return false;
				tipwin.resize(1, 1);
				int rx, ry;
				win->get_root_coords(x, y, rx, ry);
				tipwin.move(rx + 4, ry + 4);
				tipwin.show();
				return false;
			}
		}
	}
	tipwin.hide();
	return false;
}

/*
bool ValidationPanel::load_tooltip_img()
{
	std::lock_guard<std::mutex> tiplock{tipmutex};
	if (tipword.IsEmpty() || tipword == loadedtip)
		return true;
	loadedtip.Swap(tipword);
	tipword = "";

	crn::Timer::Start();
	const auto oriview = doc.GetView(doc.GetPosition(loadedtip).view);
	std::cout << "load view: " << crn::Timer::Stop() << std::endl;
	const auto &oriword = oriview.GetWord(loadedtip);
	const auto &orizone = oriview.GetZone(oriword.GetZone());
	const auto &bbox = orizone.GetPosition();
	if (!bbox.IsValid())
		return true;

	crn::Timer::Start();
	auto pb = Gdk::Pixbuf::create_from_file(oriview.GetImageName().CStr());
	std::cout << "load image (gdkpixbuf): " << crn::Timer::Stop() << std::endl;
	crn::Timer::Start();
	auto test = crn::NewImageFromFile(oriview.GetImageName());
	std::cout << "load image (libcrn): " << crn::Timer::Stop() << std::endl;
	crn::Timer::Start();
	tippb = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, bbox.GetWidth()+40, bbox.GetHeight());
	pb->copy_area(bbox.GetLeft()-20, bbox.GetTop(), bbox.GetWidth()+40, bbox.GetHeight(), tippb, 0, 0);
	//if (!tippb->get_has_alpha())
	//tippb = tippb->add_alpha(true, 255, 255, 255);
	guint8* pixs = tippb->get_pixels();
	const auto &contour = orizone.GetContour();
	auto frontier = size_t(2);
	for (; frontier < contour.size() - 1; ++frontier)
	{
		const auto y = contour[frontier].Y;
		if ((contour[frontier - 1].Y == y) && (contour[frontier - 2].Y < y) && (contour[frontier + 1].Y < y))
			break;
	}
	const auto rowstride = tippb->get_rowstride();
	const auto channels = tippb->get_n_channels();

	for (auto j = size_t(0); j < tippb->get_height(); ++j)
		for (auto i = size_t(0); i < frontier - 1; ++i)
		{
			auto x1 = contour[i].X - bbox.GetLeft() + 20;
			auto y1 = contour[i].Y - bbox.GetTop();
			auto x2 = contour[i + 1].X - bbox.GetLeft() + 20;
			auto y2 = contour[i + 1].Y - bbox.GetTop();
			if (j == y2)
			{
				//pixs[(int(x2) * channels + j * rowstride) + 3] = 128;
				pixs[x2 * channels + j * rowstride + 0] = 255;
				pixs[x2 * channels + j * rowstride + 1] = 0;
				pixs[x2 * channels + j * rowstride + 2] = 0;
				break;
			}
			if (j == y1)
			{
				//pixs[(int(x1) * channels + j * rowstride) + 3] = 128;
				pixs[x1 * channels + j * rowstride + 0] = 255;
				pixs[x1 * channels + j * rowstride + 1] = 0;
				pixs[x1 * channels + j * rowstride + 2] = 0;
				break;
			}
			if (j < y2 && j > y1)
			{
				auto x = int(x1 + (x2 - x1) * ((double(j) - y1)/(y2 - y1)));
				//pixs[(x * channels + j * rowstride) + 3] = 128;
				pixs[x * channels + j * rowstride + 0] = 255;
				pixs[x * channels + j * rowstride + 1] = 0;
				pixs[x * channels + j * rowstride + 2] = 0;
				break;
			}

		}
	for (auto j = size_t(0); j < tippb->get_height(); ++j)
		for (auto i = frontier + 1; i < contour.size(); ++i)
		{
			auto x1 = contour[i].X - bbox.GetLeft() + 20;
			auto y1 = contour[i].Y - bbox.GetTop();
			auto x2 = contour[i - 1].X - bbox.GetLeft() + 20;
			auto y2 = contour[i - 1].Y - bbox.GetTop();
			if (j == y2)
			{
				//pixs[(int(x2) * channels + j * rowstride) + 3] = 128;
				pixs[x2 * channels + j * rowstride + 0] = 255;
				pixs[x2 * channels + j * rowstride + 1] = 0;
				pixs[x2 * channels + j * rowstride + 2] = 0;
				break;
			}
			if (j == y1)
			{
				//pixs[(int(x1) * channels + j * rowstride) + 3] = 128;
				pixs[x1 * channels + j * rowstride + 0] = 255;
				pixs[x1 * channels + j * rowstride + 1] = 0;
				pixs[x1 * channels + j * rowstride + 2] = 0;
				break;
			}
			if( j < y2 && j > y1)
			{
				auto x = int(x1 + (x2 - x1) * ((double(j) - y1)/(y2 - y1)));
				//pixs[(x * channels + j * rowstride) + 3] = 128;
				pixs[x * channels + j * rowstride + 0] = 255;
				pixs[x * channels + j * rowstride + 1] = 0;
				pixs[x * channels + j * rowstride + 2] = 0;
				break;
			}
		}
	std::cout << "create thumbnail: " << crn::Timer::Stop() << std::endl;
	tipsig.emit();
	return true;
}
*/

void ValidationPanel::set_color(Cairo::RefPtr<Cairo::Context> &cc, const crn::StringUTF8 &label)
{
	if (label == label_ok)
		cc->set_source_rgb(0.9, 1.0, 0.9);
	else if (label == label_ko)
		cc->set_source_rgb(1.0, 0.9, 0.9);
	else if (label == label_unknown)
		cc->set_source_rgb(1.0, 0.95, 0.9);
	else if (label == label_revalidated)
		cc->set_source_rgb(1.0, 1.0, 0.9);
	else
	{
		int c = 0;
		for (size_t tmp = 0; tmp < label.Size(); ++tmp)
			c += label[tmp];
		c %= 3;
		cc->set_source_rgb(c == 1 ? 0.95 : 0.9, c == 2 ? 0.95 : 0.9, 1.0);
	}

}

