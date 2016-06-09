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
 * \file OriAlignDialog.h
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


