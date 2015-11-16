/*!
 * Oriflamms © INSA-Lyon & IRHT 2013
 * \author	Yann LEYDIER
 * \date	May 2013
 */

#include <oriflamms_config.h>
#include <OriLines.h>
#include <OriFeatures.h>
#include <OriProject.h>
#include <OriGUI.h>
#include <GtkCRNMain.h>
#include <CRNi18n.h>

int main(int argc, char *argv[])
{
	GtkCRN::Main kit(argc, argv);
	crn::Exception::TraceStack() = false;
	GtkCRN::Main::SetDefaultExceptionHandler();
	ori::GUI app;
	app.show();
	kit.run_thread_safe();
	return 0;
}

