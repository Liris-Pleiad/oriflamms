/*!
 * Oriflamms © INSA-Lyon & IRHT 2013
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

