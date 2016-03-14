/* Copyright 2015-2016 Université Paris Descartes, ENS-Lyon
 * 
 * file: OriAlignDialog.h
 * \author Yann LEYDIER
 */

#ifndef OriAlignDialog_HEADER
#define OriAlignDialog_HEADER
#include <gtkmm.h>
#include <OriAlignConfig.h>

namespace ori
{
	class AlignDialog: public Gtk::Dialog
	{
		public:
			AlignDialog(Gtk::Window &parent);
			virtual ~AlignDialog() {}
			AlignConfig get_config() const;

		private:
			void update();

			Gtk::CheckButton wbut;
			Gtk::RadioButton wall, wnok, wnal;
			Gtk::CheckButton wfrontbut;
			Gtk::CheckButton charbut;
			Gtk::RadioButton callw, cokw, cnkow;
			Gtk::RadioButton call, cnal;
	};
}

#endif


