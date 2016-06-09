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
 * \author	Yann LEYDIER
 * \date	May 2013
 */

#include <oriflamms_config.h>
#include <OriGUI.h>
#include <GtkCRNMain.h>
#include <GdkCRNPixbuf.h>
#include <OriConfig.h>
#include <CRNIO/CRNIO.h>
#include <CRNi18n.h>

#include <iostream>

int main(int argc, char *argv[])
{
	GtkCRN::Main kit(argc, argv);
			
	Glib::RefPtr<Gtk::IconFactory> iconfac = Gtk::IconFactory::create();
	std::vector<Gtk::IconSet> icons;
	struct {const char *file; const char *name;} iconlist[] =
	{
		{"C.png", "ori-C"},
		{"I.png", "ori-I"},
		{"L.png", "ori-L"},
		{"W.png", "ori-W"},
	};
	for (size_t tmp = 0; tmp < sizeof(iconlist) / (2 * sizeof(const char*)); tmp++)
	{
		try
		{
			Glib::RefPtr<Gdk::Pixbuf> pb = GdkCRN::PixbufFromFile(ori::Config::GetStaticDataDir() / iconlist[tmp].file);
			if (pb)
			{
				icons.push_back(Gtk::IconSet(pb));
				iconfac->add(Gtk::StockID(iconlist[tmp].name), icons.back());
			}
		}
		catch (...)
		{
			CRNError("Oriflamms: Missing file: " + ori::Config::GetStaticDataDir() / iconlist[tmp].file);
		}
	}
	iconfac->add_default();

	crn::Exception::TraceStack() = false;
	GtkCRN::Main::SetDefaultExceptionHandler();
	ori::GUI app;
	app.show();
	kit.run_thread_safe();
	return 0;
}

