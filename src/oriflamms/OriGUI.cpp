/* Copyright 2013-2015 INSA-Lyon, IRHT, ZHAO Xiaojuan
 *
 * file: OriGUI.h
 * \author Yann LEYDIER
 */

#include <OriGUI.h>
#include <CRNIO/CRNPath.h>
#include <GtkCRNFileSelecterDialog.h>
#include <GtkCRNProgressWindow.h>
#include <OriConfig.h>
#include <OriLines.h>
#include <CRNUtils/CRNXml.h>
#include <CRNIO/CRNIO.h>
#include <CRNi18n.h>
#include <OriTEIImporter.h>
#include <OriEntityDialog.h>
#include <OriStruct.h>
#include <gtkmm/accelmap.h>

using namespace ori;
using namespace crn::literals;

const Glib::ustring GUI::wintitle(Glib::ustring("Oriflamms ") + ORIFLAMMS_PACKAGE_VERSION + Glib::ustring(" – LIRIS & IRHT"));
const crn::String GUI::linesOverlay("lines");
const crn::String GUI::wordsOverlay("zwords");
const crn::String GUI::wordsOverlayOk("wordsok");
const crn::String GUI::wordsOverlayKo("wordsko");
const crn::String GUI::wordsOverlayUn("wordsun");
const crn::String GUI::charOverlay("chars");
const int GUI::minwordwidth(5);


GUI::GUI():
	view_depth(ViewDepth::None),
	need_save(false)
{
	try
	{
		crn::Path iconfile(ori::Config::GetStaticDataDir() + crn::Path::Separator() + "icon.png");
		set_icon(Gdk::Pixbuf::create_from_file(iconfile.CStr()));
	}
	catch (...) { }
	set_default_size(900, 700);
	maximize();

	guint k;
	Gdk::ModifierType mod;
	Gtk::AccelGroup::parse("<control>space", k, mod);
	Gtk::AccelMap::add_entry("<Oriflamms>/DisplayMenu/Clear", k, mod);
	Gtk::AccelGroup::parse("<control>l", k, mod);
	Gtk::AccelMap::add_entry("<Oriflamms>/DisplayMenu/Lines", k, mod);
	Gtk::AccelGroup::parse("<control>w", k, mod);
	Gtk::AccelMap::add_entry("<Oriflamms>/DisplayMenu/Words", k, mod);
	Gtk::AccelGroup::parse("<control>c", k, mod);
	Gtk::AccelMap::add_entry("<Oriflamms>/DisplayMenu/Characters", k, mod);
	Gtk::AccelGroup::parse("<control>e", k, mod);
	Gtk::AccelMap::add_entry("<Oriflamms>/DisplayMenu/Edit", k, mod);

	// File menu
	actions->add(Gtk::Action::create("new-project", Gtk::Stock::NEW, _("_New project"), _("New project")), sigc::mem_fun(this, &GUI::new_project));
	actions->add(Gtk::Action::create("load-project", Gtk::Stock::OPEN, _("_Open project"), _("Open project")), sigc::mem_fun(this, &GUI::load_project));
	actions->add(Gtk::Action::create("import-project", Gtk::Stock::CONVERT, _("_Import project"), _("Import project")), sigc::mem_fun(this, &GUI::import_project));
	actions->add(Gtk::Action::create("save-project", Gtk::Stock::SAVE, _("_Save project"), _("Save project")), sigc::mem_fun(this, &GUI::save_project));
	actions->add(Gtk::Action::create("reload-tei", Gtk::Stock::REFRESH, _("Reload _TEI"), _("Reload TEI")), sigc::mem_fun(this, &GUI::reload_tei));

	// Display menu
	actions->add(Gtk::Action::create("display-menu", _("_Display"), _("Display")));

	Gtk::RadioAction::Group dispgroup;
	actions->add(Gtk::RadioAction::create(dispgroup, "show-nothing", Gtk::StockID("gtk-crn-inkpen"), _("Show _image"), _("Show image")), sigc::bind(sigc::mem_fun(this,&GUI::tree_selection_changed), false));
	actions->get_action("show-nothing")->set_accel_path("<Oriflamms>/DisplayMenu/Clear");
	actions->add(Gtk::RadioAction::create(dispgroup, "show-lines", Gtk::StockID("gtk-crn-inkpen"), _("Show _lines"), _("Show lines")), sigc::bind(sigc::mem_fun(this,&GUI::tree_selection_changed), false));
	actions->get_action("show-lines")->set_accel_path("<Oriflamms>/DisplayMenu/Lines");
	actions->add(Gtk::RadioAction::create(dispgroup, "show-words", Gtk::Stock::SPELL_CHECK, _("Show _words"), _("Show words")), sigc::bind(sigc::mem_fun(this,&GUI::tree_selection_changed), false));
	actions->get_action("show-words")->set_accel_path("<Oriflamms>/DisplayMenu/Words");
	actions->add(Gtk::RadioAction::create(dispgroup, "show-characters", Gtk::Stock::SELECT_FONT, _("Show _characters"), _("Show characters")), sigc::bind(sigc::mem_fun(this,&GUI::tree_selection_changed), false));
	actions->get_action("show-characters")->set_accel_path("<Oriflamms>/DisplayMenu/Characters");
	actions->add(Gtk::ToggleAction::create("edit", Gtk::Stock::EDIT, _("_Edit"), _("Edit")), sigc::mem_fun(this,&GUI::edit_overlays));
	actions->get_action("edit")->set_accel_path("<Oriflamms>/DisplayMenu/Edit");
	
	// Validation menu
	actions->add(Gtk::Action::create("valid-menu", _("_Validation"), _("Validation")));
	
	actions->add(Gtk::Action::create("valid-words", Gtk::Stock::SPELL_CHECK, _("Validate _words"), _("Validate words")), sigc::mem_fun(this, &GUI::validate));

	actions->add(Gtk::Action::create("valid-propagate", Gtk::Stock::REFRESH, _("_Propagate validation"), _("Propagate validation")), sigc::mem_fun(this, &GUI::propagate_validation));
	actions->add(Gtk::Action::create("valid-stats", Gtk::Stock::PRINT, _("_Statistics"), _("Statistics")), sigc::mem_fun(this, &GUI::stats));

	actions->add(Gtk::Action::create("find-string",Gtk::Stock::FIND,_("_Find string"), _("Find string")), sigc::mem_fun(this,&GUI::find_string));

	// Alignment menu
	actions->add(Gtk::Action::create("align-menu", _("_Alignment"), _("Alignment")));

	actions->add(Gtk::Action::create("align-clear-sig", Gtk::Stock::CLEAR, _("_Clear signatures"), _("Clear signatures")), sigc::mem_fun(this, &GUI::clear_sig));
	actions->add(Gtk::Action::create("align-all", Gtk::StockID("gtk-crn-two-pages"), _("Align _all"), _("Align all")), sigc::mem_fun(this, &GUI::align_all));
	actions->add(Gtk::Action::create("align-selection", Gtk::StockID("gtk-crn-block"), _("Align _selection"), _("Align selection")), sigc::mem_fun(this, &GUI::align_selection));

	actions->add(Gtk::Action::create("align-export-tei",_("_Export TEI"),_("Export TEI")),sigc::mem_fun(this,&GUI::export_tei_alignment));
	
	actions->add(Gtk::Action::create("add-line", Gtk::Stock::INDENT, _("Add _line"), _("Add line")), sigc::mem_fun(this, &GUI::add_line));
	actions->get_action("add-line")->set_sensitive(false);

	// Option menu
	actions->add(Gtk::Action::create("option-menu", _("_Options"), _("Options")));
	Gtk::RadioAction::Group g;
	actions->add(Gtk::RadioAction::create(g, "validation-unit", _("_Unit validation"), "Unit validation"));
	actions->add(Gtk::RadioAction::create(g, "validation-batch", _("_Batch validation"), "Batch validation"));
	actions->add(Gtk::Action::create("change-font", Gtk::Stock::SELECT_FONT, _("Change _font"), _("Change font")), sigc::mem_fun(this, &GUI::change_font));
	actions->add(Gtk::Action::create("manage-entities", Gtk::Stock::EDIT, _("Manage _entities"), _("Manage entities")), sigc::mem_fun(this,&GUI::manage_entities));

	ui_manager->insert_action_group(img.get_actions());

	add_accel_group(ui_manager->get_accel_group());

	Glib::ustring ui_info =
		"<ui>"
		"	<menubar name='MenuBar'>"
		"		<menu action='app-file-menu'>"
		"			<menuitem action='new-project'/>"
		"			<menuitem action='load-project'/>"
		"			<menuitem action='import-project'/>"
		"			<menuitem action='save-project'/>"
		"			<separator/>"
		"			<menuitem action='reload-tei'/>"
		"			<separator/>"
		"			<menuitem action='app-quit'/>"
		"		</menu>"
		"		<menu action='display-menu'>"
		"			<menuitem action='image-zoom-in'/>"
		"			<menuitem action='image-zoom-out'/>"
		"			<menuitem action='image-zoom-100'/>"
		"			<menuitem action='image-zoom-fit'/>"
		"			<separator/>"
		"			<menuitem action='show-nothing'/>"
		"			<menuitem action='show-lines'/>"
		"			<menuitem action='show-words'/>"
		"			<menuitem action='show-characters'/>"
		"			<menuitem action='edit'/>"
		"			<separator/>"
		"			<menuitem action='find-string'/>"
		"		</menu>"
		"		<menu action='align-menu'>"
		"			<menuitem action='align-clear-sig'/>"
		"			<menuitem action='align-all'/>"
		"			<menuitem action='align-selection'/>"
		"			<separator/>"
		"			<menuitem action='align-export-tei'/>"
		"		</menu>"
		"		<menu action='valid-menu'>"
		"			<menuitem action='valid-words'/>"
		"			<separator/>"
		"			<menuitem action='valid-propagate'/>"
		"			<menuitem action='valid-stats'/>"
		"		</menu>"
		"		<menu action='option-menu'>"
		"			<menuitem action='validation-batch'/>"
		"			<menuitem action='validation-unit'/>"
		"			<menuitem action='change-font'/>"
		"			<menuitem action='manage-entities'/>"
		"		</menu>"
		"		<menu action='app-help-menu'>"
		"			<menuitem action='app-about'/>"
		"		</menu>"
		"	</menubar>"
		"	<toolbar name='ToolBar'>"
		"		<toolitem action='add-line'/>"
		"		<separator/>"
		"		<toolitem action='image-zoom-in'/>"
		"		<toolitem action='image-zoom-out'/>"
		"		<toolitem action='image-zoom-100'/>"
		"		<toolitem action='image-zoom-fit'/>"
		"		<separator/>"
		"		<toolitem action='edit'/>"
		"	</toolbar>"
		"	<popup name='ToolBarMenu'>"
		"		<menuitem action='show-nothing'/>"
		"		<menuitem action='show-lines'/>"
		"		<menuitem action='show-words'/>"
		"		<menuitem action='show-characters'/>"
		"	</popup>"
		"</ui>";

	ui_manager->add_ui_from_string(ui_info);
	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox());
	vbox->show();
	add(*vbox);

	vbox->pack_start(*ui_manager->get_widget("/MenuBar"), false, true, 0);
	vbox->pack_start(*ui_manager->get_widget("/ToolBar"), false, true, 0);
	auto *toolmenu = Gtk::manage(new Gtk::MenuToolButton);
	toolmenu->set_menu(*dynamic_cast<Gtk::Menu*>(ui_manager->get_widget("/ToolBarMenu")));
	toolmenu->show();
	dynamic_cast<Gtk::Toolbar*>(ui_manager->get_widget("/ToolBar"))->append(*toolmenu);

	Gtk::HPaned *hb = Gtk::manage(new Gtk::HPaned);
	hb->show();
	vbox->pack_start(*hb, true, true, 0);


	store = Gtk::TreeStore::create(columns);
	tv.set_model(store);
	tv.append_column(_("View"), columns.name);
	tv.append_column(_("TEI"), columns.xml);
	tv.append_column(_("Images"), columns.image);
	for (int tmp = 0; tmp < 3; ++tmp)
	{
		Gtk::TreeViewColumn *tvc = tv.get_column(tmp);
		Gtk::CellRendererText *cr = dynamic_cast<Gtk::CellRendererText*>(tv.get_column_cell_renderer(tmp));
		if (tvc && cr)
		{
			tvc->add_attribute(cr->property_style(), columns.error);
		}
	}
	tv.set_tooltip_column(4);
	Gtk::ScrolledWindow *sw = Gtk::manage(new Gtk::ScrolledWindow);
	sw->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_ALWAYS);
	hb->add1(*sw);
	sw->show();
	sw->add(tv);
	tv.show();
	tv.get_selection()->signal_changed().connect(sigc::bind(sigc::mem_fun(this, &GUI::tree_selection_changed), true));

	hb->add2(img);
	hb->set_position(300);
	img.show();
	img.signal_overlay_changed().connect(sigc::mem_fun(this, &GUI::overlay_changed));
	img.signal_rmb_clicked().connect(sigc::mem_fun(this, &GUI::on_rmb_clicked));
	GtkCRN::Image::OverlayConfig &lc(img.get_overlay_config(linesOverlay));
	lc.color1 = Gdk::Color("#009900");
	lc.color2 = Gdk::Color("#FFFFFF");
	lc.editable = false;
	lc.moveable = false;
	lc.can_jut_out = false;
	lc.draw_arrows = false;
	GtkCRN::Image::OverlayConfig &wc(img.get_overlay_config(wordsOverlay));
	wc.color1 = Gdk::Color("#000099");
	wc.color2 = Gdk::Color("#FFFFFF");
	wc.fill = false;
	wc.fill_alpha = 0.0;
	wc.editable = false;
	wc.moveable = false;
	wc.can_jut_out = false;
	wc.show_labels = false;
	wc.closed_polygons = true;
	GtkCRN::Image::OverlayConfig &wokc(img.get_overlay_config(wordsOverlayOk));
	wokc.color1 = Gdk::Color("#003300");
	wokc.color2 = Gdk::Color("#CCFFCC");
	wokc.fill_alpha = 0.9;
	wokc.editable = false;
	wokc.moveable = false;
	wokc.can_jut_out = false;
	wokc.show_labels = true;
	wokc.absolute_text_size = false;
	wokc.font_family = "Palemonas MUFI";
	GtkCRN::Image::OverlayConfig &wkoc(img.get_overlay_config(wordsOverlayKo));
	wkoc.color1 = Gdk::Color("#330000");
	wkoc.color2 = Gdk::Color("#FFCCCC");
	wkoc.fill_alpha = 0.9;
	wkoc.editable = false;
	wkoc.moveable = false;
	wkoc.can_jut_out = false;
	wkoc.show_labels = true;
	wkoc.absolute_text_size = false;
	wkoc.font_family = "Palemonas MUFI";
	GtkCRN::Image::OverlayConfig &wunc(img.get_overlay_config(wordsOverlayUn));
	wunc.color1 = Gdk::Color("#333300");
	wunc.color2 = Gdk::Color("#FFFFCC");
	wunc.fill_alpha = 0.9;
	wunc.editable = false;
	wunc.moveable = false;
	wunc.can_jut_out = false;
	wunc.show_labels = true;
	wunc.absolute_text_size = false;
	wunc.font_family = "Palemonas MUFI";
	GtkCRN::Image::OverlayConfig &wchar(img.get_overlay_config(charOverlay));
	wchar.color1 = Gdk::Color("#DDEEDD");
	wchar.color2 = Gdk::Color("#DD00DD");
	wchar.fill = false;
	wchar.editable = false;
	wchar.moveable = false;
	wchar.can_jut_out = true;
	wchar.closed_polygons = true;

	signal_delete_event().connect(sigc::bind_return(sigc::hide<0>(sigc::mem_fun(this, &GUI::on_close)), false));
	setup_window();
}

void GUI::about()
{
	Gtk::AboutDialog dial;
	dial.set_transient_for((Gtk::Window&)(*get_ancestor(GTK_TYPE_WINDOW)));
	dial.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
	dial.set_program_name("Oriflamms");
	dial.set_version(ORIFLAMMS_PACKAGE_VERSION);
	dial.set_comments(_("Text alignment between TEI xml and images"));
	dial.set_copyright("© LIRIS / INSA Lyon & IRHT 2013-2015");
	dial.set_website("http://oriflamms.hypotheses.org/");
	try
	{
		dial.set_logo(Gdk::Pixbuf::create_from_file((ori::Config::GetStaticDataDir() + crn::Path::Separator() + "icon.png").CStr()));
	}
	catch (...) { }
	dial.show();
	dial.run();
}

class NewProject: public Gtk::Dialog
{
	public:
		NewProject(Gtk::Window &parent, const Glib::ustring &project_name = "", const Glib::ustring &tei_name = ""):
			Gtk::Dialog(_("New project"), parent, true, false),
			xfile(_("Please select a TEI XML file"), Gtk::FILE_CHOOSER_ACTION_OPEN),
			imagedir(_("Please select the folder containing the images"), Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER)
			{
				set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
				Gtk::Table *tab = Gtk::manage(new Gtk::Table(3, 2));
				get_vbox()->pack_start(*tab, true, true, 4);
				tab->attach(*Gtk::manage(new Gtk::Label(_("Project name"))), 0, 1, 0, 1, Gtk::FILL, Gtk::FILL);
				tab->attach(pname, 1, 2, 0, 1, Gtk::FILL|Gtk::EXPAND, Gtk::FILL);
				tab->attach(*Gtk::manage(new Gtk::Label(_("TEI XML file"))), 0, 1, 1, 2, Gtk::FILL, Gtk::FILL);
				tab->attach(xfile, 1, 2, 1, 2, Gtk::FILL|Gtk::EXPAND, Gtk::FILL);
				tab->attach(*Gtk::manage(new Gtk::Label(_("Image folder"))), 0, 1, 2, 3, Gtk::FILL, Gtk::FILL);
				tab->attach(imagedir, 1, 2, 2, 3, Gtk::FILL|Gtk::EXPAND, Gtk::FILL);
				get_vbox()->show_all();

				add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
				//okbut = add_button(Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
				okbut = add_button(_("Next"), Gtk::RESPONSE_ACCEPT);
				okbut->set_image(*Gtk::manage(new Gtk::Image(Gtk::Stock::GO_FORWARD, Gtk::ICON_SIZE_BUTTON)));
				okbut->set_sensitive(false);
				std::vector<int> altbut;
				altbut.push_back(Gtk::RESPONSE_ACCEPT);
				altbut.push_back(Gtk::RESPONSE_CANCEL);
				set_alternative_button_order_from_array(altbut);
				set_default_response(Gtk::RESPONSE_ACCEPT);

				pname.signal_changed().connect(sigc::mem_fun(this, &NewProject::updatebut));
				xfile.signal_selection_changed().connect(sigc::mem_fun(this, &NewProject::updatebut));
				imagedir.signal_current_folder_changed().connect(sigc::mem_fun(this, &NewProject::updatebut));

				if (!project_name.empty())
					pname.set_text(project_name);
				if (!tei_name.empty())
				{
					Gtk::FileFilter ff;
					ff.set_name(tei_name);
					ff.add_pattern(tei_name);
					xfile.add_filter(ff);
					Gtk::FileFilter ff2;
					ff2.set_name(_("All files"));
					ff2.add_pattern("*");
					xfile.add_filter(ff2);
				}
			}

		crn::Path GetName() const
		{
			return pname.get_text().c_str();
		}
		crn::Path GetXML() const
		{
			return xfile.get_filename().c_str();
		}
		crn::Path GetImages() const
		{
			return imagedir.get_current_folder().c_str();
		}
	private:
		void updatebut()
		{
			bool active = true;
			Glib::ustring name = pname.get_text();
			if (name.empty())
				active = false;
			else if (name.find(crn::Path::Separator()) != Glib::ustring::npos)
				active = false;
			else if (xfile.get_filename().empty() || imagedir.get_current_folder().empty())
				active = false;
			okbut->set_sensitive(active);
		}
		Gtk::Entry pname;
		Gtk::FileChooserButton xfile, imagedir;
		Gtk::Button *okbut;
};

void GUI::new_project()
{
	NewProject dial(*this);
	if (dial.run() == Gtk::RESPONSE_ACCEPT)
	{
		dial.hide();
		try
		{
			ori::TEIImporter p(dial.GetXML(), *this); // may throw
			if(p.run() == Gtk::RESPONSE_ACCEPT)
			{
				STEISelectionNode sel(std::make_shared<TEISelectionNode>(std::move(p.export_selected_elements())));
				GtkCRN::ProgressWindow pw(_("Analyzing images…"), this, true);
				size_t i = pw.add_progress_bar(_("Image"));
				pw.get_crn_progress(i)->SetType(crn::Progress::Type::ABSOLUTE);
				project = pw.run<std::unique_ptr<Project>>(sigc::bind(sigc::ptr_fun(&Project::New), dial.GetName(), dial.GetXML(), sel, dial.GetImages(), pw.get_crn_progress(i)));

				GtkCRN::ProgressWindow pw2(_("Loading…"), this, true);
				i = pw2.add_progress_bar(_("Page"));
				pw2.get_crn_progress(i)->SetType(crn::Progress::Type::ABSOLUTE);
				store = pw2.run<Glib::RefPtr<Gtk::TreeStore>>(sigc::bind(sigc::mem_fun(this, &GUI::fill_tree), pw2.get_crn_progress(i)));
				tv.set_model(store);
				setup_window();
			}
		}
		catch (crn::Exception &ex)
		{
			project.reset();
			GtkCRN::App::show_exception(ex, false);
		}
	}
}

void GUI::load_project()
{
	GtkCRN::FileSelecterDialog dial(crn::Document::GetDefaultDirName(), this);
	if (dial.run() == Gtk::RESPONSE_ACCEPT)
	{
		dial.hide();
		try
		{
			project = std::make_unique<Project>(dial.get_selection());
		}
		catch (crn::Exception &ex)
		{
			project.reset();
			GtkCRN::App::show_exception(ex, false);
		}
		GtkCRN::ProgressWindow pw(_("Loading…"), this, true);
		size_t i = pw.add_progress_bar(_("Page"));
		pw.get_crn_progress(i)->SetType(crn::Progress::Type::ABSOLUTE);
		store = pw.run<Glib::RefPtr<Gtk::TreeStore> >(sigc::bind(sigc::mem_fun(this, &GUI::fill_tree), pw.get_crn_progress(i)));
		tv.set_model(store);
		setup_window();


		/* exportation des caractères
		for (size_t v = 0; v < project->GetNbViews(); ++v)
		{
			Glib::RefPtr<Gdk::Pixbuf> pb = Gdk::Pixbuf::create_from_file(project->GetDoc()->GetFilenames()[v].CStr());
			View &page(project->GetStructure().GetViews()[v]);
			for (size_t c = 0; c < page.GetColumns().size(); ++c)
			{
				Column &col(page.GetColumns()[c]);
				for (size_t l = 0; l < col.GetLines().size(); ++l)
				{
					Line &line(col.GetLines()[l]);
					for (size_t w = 0; w < line.GetWords().size(); ++w)
					{
						Word &oriword(line.GetWords()[w]);
						crn::String text(oriword.GetText().CStr());
						for (auto pos : crn::Range(text))
						{
							if (oriword.GetCharacterFrontiers().empty())
								continue;
							crn::Rect bbox(oriword.GetBBox());
							if (!bbox.IsValid() || (bbox.GetHeight() <= 1))
								continue;

							// create small image (not using subpixbuf since it keeps a reference to the original image)
							std::vector<crn::Point2DInt> fronfrontier(oriword.GetCharacterFront(pos));
							std::vector<crn::Point2DInt> backfrontier(oriword.GetCharacterBack(pos));

							int max_x = backfrontier[0].X;
							int min_x = fronfrontier[0].X;

							int min_y = fronfrontier[0].Y;

							for (size_t i = 0; i < backfrontier.size(); ++i)
							{
								if(backfrontier[i].X > max_x)
									max_x = backfrontier[i].X;
							}
							for (size_t i = 0; i < fronfrontier.size(); ++i)
							{
								if(fronfrontier[i].X < min_x)
									min_x = fronfrontier[i].X;
							}
							if(max_x - min_x <= 1)
								continue;
							if ((min_x < 0) || (max_x >= pb->get_width()) || (bbox.GetTop() < 0) || (bbox.GetBottom() >= pb->get_height()))
								continue;
							Glib::RefPtr<Gdk::Pixbuf> wpb(Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, max_x - min_x, bbox.GetHeight()));

							pb->copy_area(min_x, bbox.GetTop(), max_x - min_x, bbox.GetHeight(), wpb, 0, 0);

							if(!wpb->get_has_alpha())
								wpb = wpb->add_alpha(true, 255, 255, 255);
							guint8* pixs = wpb->get_pixels();
							int rowstrides = wpb->get_rowstride();
							int channels = wpb->get_n_channels();

							for (size_t j = 0; j < wpb->get_height(); ++j)
							{
								int x = 0;
								for (size_t i = 0; i < fronfrontier.size() - 1; ++i)
								{
									double x1 = double(fronfrontier[i].X - min_x);
									double y1 = double(fronfrontier[i].Y - min_y);
									double x2 = double(fronfrontier[i + 1].X - min_x);
									double y2 = double(fronfrontier[i + 1].Y - min_y);

									if (double(j) == y2)
										x = int(x2);
									else if (double(j) == y1)
										x = int(x1);
									else if ((double(j) < y2) && (double(j) > y1))
										x = int(x1 + (x2 - x1) * ((double(j) - y1)/(y2 - y1)));
								}
								for (int k = 0; k <= crn::Min(x, wpb->get_width() - 1); ++k)
									pixs[k * channels + j * rowstrides + 3] = 0; // RGBA

								int xx = 0;
								for (size_t i = 0; i < backfrontier.size() - 1; ++i)
								{
									double x1 = double(backfrontier[i].X - oriword.GetBBox().GetLeft());
									double y1 = double(backfrontier[i].Y - oriword.GetBBox().GetTop());
									double x2 = double(backfrontier[i + 1].X - oriword.GetBBox().GetLeft());
									double y2 = double(backfrontier[i + 1].Y - oriword.GetBBox().GetTop());
									if (double(j) == y2)
										xx = int(x2);

									if (double(j) == y1)
										xx = int(x1);

									if ((double(j) < y2) && (double(j) > y1))
										xx = int(x1 + (x2 - x1) * ((double(j) - y1)/(y2 - y1)));

								}
								for (int k = crn::Max(xx, 0); k < wpb->get_width(); ++k)
									pixs[k * channels + j * rowstrides + 3] = 0; // RGBA
							}
							try
							{
								crn::Path outpath = (U"chars/"_s + text[pos]).CStr();
								if (!crn::IO::Access(outpath, crn::IO::EXISTS))
									crn::IO::Mkdir(outpath);
								outpath /= int(v) + " "_p + int(c) + " "_p + int(l) + " "_p + int(w) + ".png";
								wpb->save(outpath.CStr(), "png");
							}
							catch (...) {}
						} // found text

					} // for each word
				} // for each line
			} // for each column
		} // for each view
 */
	}
}

void GUI::import_project()
{
	// select project file
	Gtk::FileChooserDialog impfile(*this, _("Please select a project file to import"), Gtk::FILE_CHOOSER_ACTION_OPEN);
	impfile.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	impfile.add_button(Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
	std::vector<int> altbut;
	altbut.push_back(Gtk::RESPONSE_ACCEPT);
	altbut.push_back(Gtk::RESPONSE_CANCEL);
	impfile.set_alternative_button_order_from_array(altbut);
	impfile.set_default_response(Gtk::RESPONSE_ACCEPT);
	if (impfile.run() != Gtk::RESPONSE_ACCEPT)
		return;
	impfile.hide();

	// create project
	crn::Path opname(impfile.get_filename().c_str());
	try
	{
		crn::xml::Document opfile(opname); // may throw
		//crn::Path datadir(opfile.GetRoot().GetAttribute<crn::StringUTF8>("basename", false)); // may throw
		crn::Path datadir(opname + "_data");
		crn::xml::Document oxfile(datadir + crn::Path::Separator() + crn::Path("structure.xml")); // may throw
		crn::Path oteipath(oxfile.GetRoot().GetAttribute<crn::StringUTF8>("teipath", false)); // may throw
		NewProject dial(*this, opname.GetFilename().CStr(), oteipath.GetFilename().CStr());
		if (dial.run() != Gtk::RESPONSE_ACCEPT)
			return;
		dial.hide();
		// create data directory
		crn::Path newdatadir = crn::Document::GetDefaultDirName() + crn::Path::Separator() + dial.GetName() + "_data";
		crn::IO::Mkdir(newdatadir); // may throw
		// copy project file
		crn::xml::Element root(opfile.GetRoot());
		root.SetAttribute("basename", newdatadir);
		for (crn::xml::Element el = root.BeginElement(); el != root.EndElement(); ++el)
		{
			if (el.GetName() == "View")
			{
				crn::Path img(el.GetAttribute<crn::StringUTF8>("fname", false)); // may throw
				el.SetAttribute("fname", dial.GetImages() + crn::Path::Separator() + img.GetFilename());
			}
		}
		opfile.Save(crn::Document::GetDefaultDirName() + crn::Path::Separator() + dial.GetName());
		// copy tei description file
		root = oxfile.GetRoot();
		root.SetAttribute("teipath", dial.GetXML());
		oxfile.Save(newdatadir + crn::Path::Separator() + "structure.xml");
		// copy view files
		crn::IO::Directory viewdir(datadir);
		for (const crn::Path &viewfile : viewdir.GetFiles())
		{
			crn::Path fname(viewfile.GetFilename());
			if (fname == "structure.xml")
				continue;
			crn::xml::Document vdoc(viewfile); // should not throw
			root = vdoc.GetRoot(); // should not throw
			crn::Path imgname(root.GetAttribute<crn::StringUTF8>("name", false)); // should not throw
			root.SetAttribute("name", dial.GetImages() + crn::Path::Separator() + imgname.GetFilename());
			vdoc.Save(newdatadir + crn::Path::Separator() + fname);
		}
	}
	catch (crn::Exception &ex)
	{
		show_exception(ex, false);
	}
}

void GUI::setup_window()
{
	actions->get_action("align-menu")->set_sensitive(project != nullptr);
	actions->get_action("valid-menu")->set_sensitive(project != nullptr);
	actions->get_action("display-menu")->set_sensitive(project != nullptr);
	actions->get_action("save-project")->set_sensitive(project != nullptr);
	actions->get_action("reload-tei")->set_sensitive(project != nullptr);

	set_win_title();
}

void GUI::set_win_title()
{
	Glib::ustring t(wintitle);
	if (need_save)
		t += " *";
	if (project)
	{
		t += " (";
		t += project->GetTitle().CStr();
		t += ")";
	}
	set_title(t);
}

void GUI::add_line()
{
	GtkCRN::App::show_message("TODO", Gtk::MESSAGE_WARNING);
}

Glib::RefPtr<Gtk::TreeStore> GUI::fill_tree(crn::Progress *prog)
{
	Glib::RefPtr<Gtk::TreeStore> newstore = Gtk::TreeStore::create(columns);
	current_view_id = std::numeric_limits<size_t>::max();
	if (!project)
		return newstore;

	prog->SetMaxCount(int(project->GetNbViews()));
	for (size_t tmp = 0; tmp < project->GetNbViews(); ++tmp)
	{
		Project::View v(project->GetView(tmp));
		Gtk::TreeIter vit = newstore->append();
		size_t xcol = v.page.GetColumns().size();
		crn::SVector ilines(std::static_pointer_cast<crn::Vector>(v.image->GetUserData(Project::LinesKey)));
		size_t icol = ilines->Size();
		vit->set_value(columns.error, xcol != icol ? Pango::STYLE_ITALIC : Pango::STYLE_NORMAL);
		vit->set_value(columns.name, Glib::ustring(v.page.GetImageName().CStr()));
		vit->set_value(columns.index, tmp);
		crn::StringUTF8 s = xcol;
		s += " ";
		s += _("column(s)");
		vit->set_value(columns.xml, Glib::ustring(s.CStr()));
		s = icol;
		s += " ";
		s += _("column(s)");
		vit->set_value(columns.image, Glib::ustring(s.CStr()));
		for (size_t col = 0; col < crn::Max(xcol, icol); ++col)
		{
			Glib::ustring coltext;
			Gtk::TreeIter cit = newstore->append(vit->children());
			s = _("Column");
			s += " ";
			s += col + 1;
			cit->set_value(columns.name, Glib::ustring(s.CStr()));
			cit->set_value(columns.index, col);
			size_t xnlines = 0, inlines = 0;
			if (col < xcol)
			{
				xnlines = v.page.GetColumns()[col].GetLines().size();
				s = xnlines;
				s += " ";
				s += _("line(s)");
				cit->set_value(columns.xml, Glib::ustring(s.CStr()));
				for (size_t lin = 0; lin < v.page.GetColumns()[col].GetLines().size(); ++lin)
				{
					const ori::Line &l(v.page.GetColumns()[col].GetLines()[lin]);
					Gtk::TreeIter lit = newstore->append(cit->children());
					s = _("Line");
					s += " ";
					s += lin + 1;
					lit->set_value(columns.name, Glib::ustring(s.CStr()));
					lit->set_value(columns.index, lin);
					s = l.GetWords().size();
					s += " ";
					s += _("word(s)");
					lit->set_value(columns.xml, Glib::ustring(s.CStr()));
					s = "";
					for (size_t wor = 0; wor < l.GetWords().size(); ++wor)
					{
						const ori::Word &w(l.GetWords()[wor]);
						s += w.GetText().CStr();
						s += " ";
					}
					auto escape = Glib::ustring{};
					escape.reserve(s.Size() * 2);
					for (size_t tmp = 0; tmp < s.Size(); ++tmp)
					{
						if (s[tmp] == '&')
							escape += "&amp;";
						else if (s[tmp] == '<')
							escape += "&lt;";
						else if (s[tmp] == '>')
							escape += "&gt;";
						else
							escape += s[tmp];
					}
					lit->set_value(columns.text, escape);
					coltext += escape + "⏎\n";
				}
				cit->set_value(columns.text, coltext);
			}
			else
				cit->set_value(columns.error, Pango::STYLE_ITALIC);
			if (col < icol)
			{
				crn::SVector c(std::static_pointer_cast<crn::Vector>(ilines->At(col)));
				inlines = c->Size();
				s = inlines;
				s += " ";
				s += _("line(s)");
				cit->set_value(columns.image, Glib::ustring(s.CStr()));
			}
			else
				cit->set_value(columns.error, Pango::STYLE_ITALIC);
			if (inlines != xnlines)
			{
				cit->set_value(columns.error, Pango::STYLE_ITALIC);
				vit->set_value(columns.error, Pango::STYLE_ITALIC);
			}
		}
		prog->Advance();
	}
	return newstore;
}

void GUI::tree_selection_changed(bool focus)
{
	img.clear_overlay(linesOverlay);
	img.clear_overlay(wordsOverlay);
	img.clear_overlay(wordsOverlayOk);
	img.clear_overlay(wordsOverlayKo);
	img.clear_overlay(wordsOverlayUn);
	img.clear_overlay(charOverlay);

	Gtk::TreeIter it(tv.get_selection()->get_selected());
	if (!it)
		return;

	int level = 0;
	while (it->parent())
	{
		level += 1;
		it = it->parent();
	}
	size_t viewid = it->get_value(columns.index);
	if (viewid != current_view_id)
	{
		img.set_pixbuf(Gdk::Pixbuf::create_from_file(project->GetDoc()->GetFilenames()[viewid].CStr()));
		current_view_id = viewid;
	}
	img.set_selection_type(GtkCRN::Image::Overlay::None);
	img.clear_selection();
	actions->get_action("add-line")->set_sensitive(false);

	switch (level)
	{
		case 0:
			// page
			tv.expand_row(Gtk::TreePath(it), false);
			view_depth = ViewDepth::Page;
			break;
		case 1:
			{
				// column
				it = tv.get_selection()->get_selected();
				size_t colid = it->get_value(columns.index);
				crn::SCBlock b(project->GetDoc()->GetView(viewid));
				crn::SCVector cols(std::static_pointer_cast<const crn::Vector>(b->GetUserData(ori::Project::LinesKey)));
				crn::SCVector lines(std::static_pointer_cast<const crn::Vector>(cols->At(colid)));
				if (!lines->IsEmpty() && focus)
				{
					SCGraphicalLine l(std::static_pointer_cast<const GraphicalLine>(lines->Front()));
					img.focus_on((l->GetFront().X + l->GetBack().X) / 2, l->GetFront().Y);
				}

				for (size_t tmpl = 0; tmpl < lines->Size(); ++tmpl)
				{
					if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("show-lines"))->get_active())
						display_line(viewid, colid, tmpl);
					if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("show-words"))->get_active())
						display_word(viewid, colid, tmpl);
					if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("show-characters"))->get_active())
						display_characters(viewid, colid, tmpl);

				}
				//img.set_selection_type(GtkCRN::Image::Overlay::Line); TODO
				view_depth = ViewDepth::Column;
			}
			break;
		case 2:
			{
				// line
				it = tv.get_selection()->get_selected();
				size_t linid = it->get_value(columns.index);
				size_t colid = it->parent()->get_value(columns.index);
				crn::SCBlock b(project->GetDoc()->GetView(viewid));
				crn::SCVector cols(std::static_pointer_cast<const crn::Vector>(b->GetUserData(ori::Project::LinesKey)));
				crn::SCVector lines(std::static_pointer_cast<const crn::Vector>(cols->At(colid)));
				SCGraphicalLine l(std::static_pointer_cast<const GraphicalLine>(lines->At(linid)));
				if (focus)
					img.focus_on((l->GetFront().X + l->GetBack().X) / 2, l->GetFront().Y);
				if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("show-lines"))->get_active())
					display_line(viewid, colid, linid);
				if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("show-words"))->get_active())
					display_word(viewid, colid, linid);
				if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("show-characters"))->get_active())
					display_characters(viewid, colid, linid);

				view_depth = ViewDepth::Line;
			}
			break;
	}

}

void GUI::edit_overlays()
{
	auto mod = Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("edit"))->get_active();
	GtkCRN::Image::OverlayConfig &lc(img.get_overlay_config(linesOverlay));
	lc.editable = mod;
	GtkCRN::Image::OverlayConfig &wc(img.get_overlay_config(wordsOverlay));
	wc.editable = mod;
	GtkCRN::Image::OverlayConfig &wchar(img.get_overlay_config(charOverlay));
	//wchar.editable = mod; TODO

	if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("show-words"))->get_active())
		tree_selection_changed(false);
}

void GUI::display_line(size_t viewid, size_t colid, size_t linid)
{
	crn::SCBlock b(project->GetDoc()->GetView(viewid));
	crn::SCVector cols(std::static_pointer_cast<const crn::Vector>(b->GetUserData(ori::Project::LinesKey)));
	crn::SCVector lines(std::static_pointer_cast<const crn::Vector>(cols->At(colid)));
	SCGraphicalLine l(std::static_pointer_cast<const GraphicalLine>(lines->At(linid)));
	std::vector<crn::Point2DInt> pts;
	for (const crn::Point2DDouble &p : l->GetMidline())
		pts.push_back(crn::Point2DInt{int(p.X), int(p.Y)});
	img.add_overlay_item(linesOverlay, linid, pts);
}

void GUI::display_word(size_t viewid, size_t colid, size_t linid)
{
	if (colid >= project->GetView(viewid).page.GetColumns().size())
		return; // this column does not exist in the XML file
	if (linid >= project->GetView(viewid).page.GetColumns()[colid].GetLines().size())
		return; // this line does not exist in the XML file
	const std::vector<ori::Word> &words(project->GetView(viewid).page.GetColumns()[colid].GetLines()[linid].GetWords());
	for (size_t tmpw = 0; tmpw < words.size(); ++tmpw)
	{
		display_update_word(WordPath(viewid, colid, linid, tmpw));
	}
}

void GUI::display_characters(size_t viewid, size_t colid, size_t linid)
{
	crn::SCBlock b(project->GetDoc()->GetView(viewid));
	crn::SCVector cols(std::static_pointer_cast<const crn::Vector>(b->GetUserData(ori::Project::LinesKey)));
	crn::SCVector lines(std::static_pointer_cast<const crn::Vector>(cols->At(colid)));
	if (colid >= project->GetView(viewid).page.GetColumns().size())
		return; // this column does not exist in the XML file
	if (linid >= project->GetView(viewid).page.GetColumns()[colid].GetLines().size())
		return; // this line does not exist in the XML file
	std::vector<ori::Word> &words(project->GetView(viewid).page.GetColumns()[colid].GetLines()[linid].GetWords());

	for (int tmpw = 0; tmpw < words.size(); ++tmpw)
	{
		crn::String ov = wordsOverlayUn;
		if (words[tmpw].GetValid().IsTrue()) ov = wordsOverlayOk;
		if (words[tmpw].GetValid().IsFalse()) ov = wordsOverlayKo;

		std::vector<ori::Word::Character> chars;
		try
		{
			chars = words[tmpw].GetCharacters();
		}
		catch	(crn::ExceptionUninitialized&)
		{
			continue;
		}
		int cnt = 0;
		for (const auto &c : chars)
		{
			crn::String s(words[tmpw].GetId() + " " + crn::String(cnt++));

			int topy = crn::Min(c.front.front().Y, c.back.front().Y);
			crn::Rect topbox(c.front.front().X, topy, c.back.front().X, topy + words[tmpw].GetBBox().GetHeight() / 4);
			img.add_overlay_item(ov, s, topbox, c.text.CStr());

			std::vector<crn::Point2DInt> poly(c.front);
			poly.insert(poly.end(), c.back.rbegin(), c.back.rend());
			img.add_overlay_item(charOverlay, s, poly);
		}		
		/*
			 std::vector<std::vector<crn::Point2DInt>> l = words[tmpw].GetCharacterFrontiers();
			 for (size_t fron = 0; fron < l.size(); ++fron)
			 {
			 crn::String s(words[tmpw].GetId() + " " + crn::String(fron));

			 if (!words[tmpw].GetCharacterFrontiers()[fron].empty())
			 {
			 std::vector<crn::Point2DInt> frontiers = l[fron];
			 std::vector<crn::Point2DInt> lis;

			 if (fron == l.size() - 1)
			 {
			 lis = words[tmpw].GetBackFrontier();
			 }
			 else
			 {
			 lis = l[fron + 1];
			 }
			 if (fron == 0)
			 {
			 std::vector<crn::Point2DInt> li = words[tmpw].GetFrontFrontier();
			 for(size_t id = frontiers.size(); id > 0 ; --id)
			 li.push_back(frontiers[id - 1]);
			 crn::String ss (s);
			 ss.Insert(ss.NPos(),crn::String("0"));
			 img.add_overlay_item(charOverlay, ss,li);

			 crn::Rect topbox;
			 int w = li.back().X - li[0].X;
			 topbox.SetLeft(li[0].X);
			 topbox.SetTop(li[0].Y);
			 topbox.SetWidth(w);
			 topbox.SetHeight(words[tmpw].GetBBox().GetHeight() / 4);
			 crn::String ch(words[tmpw].GetText().CStr());
			 crn::String ov = wordsOverlayUn;
			 if (words[tmpw].GetValid().IsTrue()) ov = wordsOverlayOk;
			 if (words[tmpw].GetValid().IsFalse()) ov = wordsOverlayKo;
			 img.add_overlay_item(ov, ss, topbox, ch.SubString(fron, 1).CStr());
			 }

			 for(size_t id = lis.size(); id > 0; --id)
			 frontiers.push_back(lis[id - 1]);

			 img.add_overlay_item(charOverlay,s,frontiers );

			 int w = frontiers.back().X - frontiers[0].X;
			 crn::Rect topbox;
			 topbox.SetLeft(frontiers[0].X);
			 topbox.SetTop(frontiers[0].Y);
			 topbox.SetWidth(w);
			 topbox.SetHeight(words[tmpw].GetBBox().GetHeight() / 4);
			 crn::String ch(words[tmpw].GetText().CStr());
			 crn::String ov = wordsOverlayUn;
			 if (words[tmpw].GetValid().IsTrue()) ov = wordsOverlayOk;
			 if (words[tmpw].GetValid().IsFalse()) ov = wordsOverlayKo;
			 img.add_overlay_item(ov, s, topbox, ch.SubString(fron + 1, 1).CStr());

			 }
			 }
			 */
	} // foreach word
}

void GUI::display_update_word(const WordPath &path, const crn::Option<int> &newleft, const crn::Option<int> &newright)
{
	// compute id
	crn::String itemid = path.ToString();
	// clean old overlay items
	try { img.remove_overlay_item(wordsOverlay, itemid); } catch (...) { }
	try { img.remove_overlay_item(wordsOverlayUn, itemid); } catch (...) { }
	try { img.remove_overlay_item(wordsOverlayOk, itemid); } catch (...) { } 
	try { img.remove_overlay_item(wordsOverlayKo, itemid); } catch (...) { }

	// add overlay items
	ori::Word &w(project->GetStructure().GetWord(path));
	if (w.GetBBox().IsValid())
	{
		if (newleft)
		{
			w.SetLeft(newleft.Get());
			if (w.GetRightCorrection())
				w.SetValid(true);
		}
		if (newright)
		{
			w.SetRight(newright.Get());
			if (w.GetLeftCorrection())
				w.SetValid(true);
		}
		if (newright || newleft)
		{
			project->ComputeWordFrontiers(path);
			project->AlignWordCharacters(path);
		}

		if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("edit"))->get_active())
		{ // edit mode, display boxes
			img.add_overlay_item(wordsOverlay, itemid, w.GetBBox(), w.GetText().CStr());
		}
		else
		{ // not in edit mode, display polygons
			std::vector<crn::Point2DInt> li = w.GetFrontFrontier();
			std::vector<crn::Point2DInt> wordbox = w.GetBackFrontier();
			for(size_t i = li.size(); i > 0; --i)
			{
				wordbox.push_back(li[i-1]);
			}
			img.add_overlay_item(wordsOverlay, itemid, wordbox, w.GetText().CStr());
		}

		crn::String ov = wordsOverlayUn;
		if (w.GetValid().IsTrue()) ov = wordsOverlayOk;
		if (w.GetValid().IsFalse()) ov = wordsOverlayKo;
		crn::Rect topbox(w.GetBBox());
		topbox.SetHeight(topbox.GetHeight() / 4);
		img.add_overlay_item(ov, itemid, topbox, w.GetText().CStr());
	}
}

void GUI::overlay_changed(crn::String overlay_id, crn::String overlay_item_id, GtkCRN::Image::MouseMode mm)
{
	if (overlay_id == GtkCRN::Image::selection_overlay)
	{
		// can add line?
		actions->get_action("add-line")->set_sensitive(img.has_selection());
	}
	else if (overlay_id == linesOverlay)
	{
		// line modified
		if (overlay_item_id.IsEmpty())
			return;
		// get path
		const int linid = overlay_item_id.ToInt();
		Gtk::TreeIter it(tv.get_selection()->get_selected());
		if (view_depth == ViewDepth::Line)
		{
			// iterator on line
			it = it->parent();
		}
		else if (view_depth != ViewDepth::Column) // iterator not on column
			return;
		const size_t colid = it->get_value(columns.index);
		// set line bounds
		crn::SBlock b(project->GetDoc()->GetView(current_view_id));
		crn::SVector cols(std::static_pointer_cast<crn::Vector>(b->GetUserData(ori::Project::LinesKey)));
		crn::SVector lines(std::static_pointer_cast<crn::Vector>(cols->At(colid)));
		SGraphicalLine l(std::static_pointer_cast<GraphicalLine>(lines->At(linid)));
		GtkCRN::Image::OverlayItem &item(img.get_overlay_item(overlay_id, overlay_item_id));

		GtkCRN::Image::Polygon &po = static_cast<GtkCRN::Image::Polygon&>(item);
		l->SetMidline(po.points);

		// realign
		GtkCRN::ProgressWindow pw(_("Aligning…"), this, true);
		size_t i = pw.add_progress_bar("");
		pw.get_crn_progress(i)->SetType(crn::Progress::Type::PERCENT);
		pw.run(sigc::bind(sigc::mem_fun(*project, &Project::AlignLine), current_view_id, colid, linid, pw.get_crn_progress(i))); // TODO XXX what if the line is not associated to a line in the XML?
		project->GetStructure().GetViews()[current_view_id].GetColumns()[colid].GetLines()[linid].SetCorrected();
		set_need_save();
		display_line(current_view_id, colid, linid); // refresh
	}
	else if (overlay_id == wordsOverlay)
	{
		// word modified
		if (overlay_item_id.IsEmpty())
			return;
		// get path
		WordPath path(overlay_item_id);

		GtkCRN::Image::OverlayItem &item(img.get_overlay_item(overlay_id, overlay_item_id));
		GtkCRN::Image::Rectangle &rect = static_cast<GtkCRN::Image::Rectangle&>(item);
		int nleft = rect.rect.GetLeft() ,nright = rect.rect.GetRight();
		//int nleft = item.x1, nright = item.x2;

		if (nright - nleft < minwordwidth)
			nright = nleft + minwordwidth;
		// need to update previous word?
		if (path.word != 0)
		{
			WordPath pp(path);
			pp.word -= 1;
			ori::Word &w(project->GetStructure().GetWord(pp));
			if (w.GetBBox().IsValid())
			{
				if (nleft < w.GetBBox().GetLeft() + minwordwidth)
					nleft = w.GetBBox().GetLeft() + minwordwidth;
			}
			display_update_word(pp, crn::Option<int>(), nleft - 1);
		}
		// need to update next word?
		if (path.word + 1 < project->GetStructure().GetViews()[path.view].GetColumns()[path.col].GetLines()[path.line].GetWords().size())
		{
			WordPath np(path);
			np.word += 1;
			ori::Word &w(project->GetStructure().GetWord(np));
			if (w.GetBBox().IsValid())
			{
				if (nright > w.GetBBox().GetRight() - minwordwidth)
					nright = w.GetBBox().GetRight() - minwordwidth;
			}
			display_update_word(np, nright + 1, crn::Option<int>());
		}

		// update bbox
		display_update_word(path, nleft, nright);

		set_need_save();
	}
}

void GUI::on_rmb_clicked(guint mouse_button, guint32 time, std::vector<std::pair<crn::String, crn::String> > overlay_items_under_mouse, int x_on_image, int y_on_image)
{
	for (std::vector<std::pair<crn::String, crn::String> >::const_iterator it = overlay_items_under_mouse.begin(); it != overlay_items_under_mouse.end(); ++it)
	{
		if (it->first == wordsOverlay)
		{
			try
			{
				WordPath path(it->second); // may throw
				ori::Word &w(project->GetStructure().GetWord(path));
				if (w.GetValid().IsTrue())
					w.SetValid(false);
				else if (w.GetValid().IsFalse())
					w.SetValid(crn::Prop3::Unknown);
				else
					w.SetValid(true);
				display_update_word(path);
				set_need_save();
				return; // modify just one
			}
			catch (...) { }
		}
	}
}

void GUI::reload_tei()
{
	if (project)
	{
		project->ReloadTEI();
		GtkCRN::ProgressWindow pw(_("Loading…"), this, true);
		size_t i = pw.add_progress_bar(_("Page"));
		pw.get_crn_progress(i)->SetType(crn::Progress::Type::ABSOLUTE);
		store = pw.run<Glib::RefPtr<Gtk::TreeStore> >(sigc::bind(sigc::mem_fun(this, &GUI::fill_tree), pw.get_crn_progress(i)));
		tv.set_model(store);
	}
}

void GUI::align_selection()
{
	GtkCRN::ProgressWindow pw(_("Aligning…"), this, true);
	switch (view_depth)
	{
		case ViewDepth::Page:
			{
				// page
				size_t ic = pw.add_progress_bar(_("Column"));
				pw.get_crn_progress(ic)->SetType(crn::Progress::Type::ABSOLUTE);
				size_t il = pw.add_progress_bar(_("Line"));
				pw.get_crn_progress(il)->SetType(crn::Progress::Type::ABSOLUTE);
				pw.run(sigc::bind(sigc::mem_fun(*project, &Project::AlignView), current_view_id, pw.get_crn_progress(ic), pw.get_crn_progress(il), (crn::Progress*)nullptr));
				set_need_save();
			}
			break;
		case ViewDepth::Column:
			{
				// column
				Gtk::TreeIter it = tv.get_selection()->get_selected();
				size_t colid = it->get_value(columns.index);

				size_t i = pw.add_progress_bar(_("Line"));
				pw.get_crn_progress(i)->SetType(crn::Progress::Type::ABSOLUTE);
				pw.run(sigc::bind(sigc::mem_fun(*project, &Project::AlignColumn), current_view_id, colid, pw.get_crn_progress(i), (crn::Progress*)nullptr));
				set_need_save();
			}
			break;
		case ViewDepth::Line:
			{
				// line
				Gtk::TreeIter it = tv.get_selection()->get_selected();
				size_t linid = it->get_value(columns.index);
				size_t colid = it->parent()->get_value(columns.index);

				size_t i = pw.add_progress_bar("");
				pw.get_crn_progress(i)->SetType(crn::Progress::Type::PERCENT);
				pw.run(sigc::bind(sigc::mem_fun(*project, &Project::AlignLine), current_view_id, colid, linid, pw.get_crn_progress(i)));
				set_need_save();
			}
			break;
	}
	tree_selection_changed(false); // update display
}

void GUI::align_all()
{
	GtkCRN::ProgressWindow pw(_("Aligning…"), this, true);
	size_t iv = pw.add_progress_bar(_("View"));
	pw.get_crn_progress(iv)->SetType(crn::Progress::Type::ABSOLUTE);
	size_t ic = pw.add_progress_bar(_("Column"));
	pw.get_crn_progress(ic)->SetType(crn::Progress::Type::ABSOLUTE);
	size_t il = pw.add_progress_bar(_("Line"));
	pw.get_crn_progress(il)->SetType(crn::Progress::Type::ABSOLUTE);
	pw.run(sigc::bind(sigc::mem_fun(*project, &Project::AlignAll), pw.get_crn_progress(iv), pw.get_crn_progress(ic), pw.get_crn_progress(il), (crn::Progress*)nullptr));
	set_need_save();
	tree_selection_changed(false); // update display
}

void GUI::validate()
{
	validation_win = std::make_unique<Validation>(*this, *project, Glib::RefPtr<Gtk::RadioAction>::cast_dynamic(actions->get_action("validation-batch"))->get_active(), true, std::bind(std::mem_fn(&GUI::set_need_save), this), std::bind(std::mem_fn(&GUI::tree_selection_changed), this, false));
	validation_win->show();
}

void GUI::set_need_save()
{
	if (project)
	{
		need_save = true;
		set_win_title();
	}
}

void GUI::save_project()
{
	if (project)
	{
		GtkCRN::ProgressWindow pw(_("Saving…"), this, true);
		size_t i = pw.add_progress_bar("");
		pw.get_crn_progress(i)->SetType(crn::Progress::Type::PERCENT);
		pw.run(sigc::bind(sigc::mem_fun(*project, &Project::Save), pw.get_crn_progress(i)));
		need_save = false;
		set_win_title();
	}
}

void GUI::on_close()
{
	if (need_save)
		save_project();
}

void GUI::change_font()
{
	Gtk::FontSelectionDialog dial;
	dial.set_transient_for(*this);
	dial.set_preview_text(_("The quick bꝛown fox jumpſ over the lazy dog."));
	if (dial.run() == Gtk::RESPONSE_OK)
	{
		Pango::FontDescription fd(dial.get_font_name());
		Glib::ustring ff = fd.get_family();
		GtkCRN::Image::OverlayConfig &wokc(img.get_overlay_config(wordsOverlayOk));
		wokc.font_family = ff;
		GtkCRN::Image::OverlayConfig &wkoc(img.get_overlay_config(wordsOverlayKo));
		wkoc.font_family = ff;
		GtkCRN::Image::OverlayConfig &wunc(img.get_overlay_config(wordsOverlayUn));
		wunc.font_family = ff;
		img.force_redraw();
	}
}



void GUI::stats()
{
	Gtk::FileChooserDialog fdial(*this, _("Export statistics to…"), Gtk::FILE_CHOOSER_ACTION_SAVE);
	fdial.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	fdial.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
	std::vector<int> altbut;
	altbut.push_back(Gtk::RESPONSE_ACCEPT);
	altbut.push_back(Gtk::RESPONSE_CANCEL);
	fdial.set_alternative_button_order_from_array(altbut);
	fdial.set_default_response(Gtk::RESPONSE_ACCEPT);
	Gtk::FileFilter ff;
	ff.set_name(_("ODS files"));
	ff.add_pattern("*.ods");
	ff.add_pattern("*.Ods");
	ff.add_pattern("*.ODS");
	fdial.add_filter(ff);
	fdial.set_filename("stats.ods");
	if (fdial.run() != Gtk::RESPONSE_ACCEPT)
		return;
	fdial.hide();
	crn::Path fname(fdial.get_filename().c_str());
	crn::Path ext = fname.GetExtension();
	if ((ext != "ods") && (ext != "Ods") && (ext != "ODS"))
		fname += ".ods";
	project->ExportStats(fname);
}

void GUI::clear_sig()
{
	Gtk::MessageDialog msg(*this, _("Are you sure you wish to flush the images' signatures?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
	if (msg.run() == Gtk::RESPONSE_YES)
	{
		msg.hide();
		GtkCRN::ProgressWindow pwin(_("Flushing signatures…"), this, true);
		size_t id = pwin.add_progress_bar("");
		pwin.get_crn_progress(id)->SetType(crn::Progress::Type::PERCENT);
		pwin.run(sigc::bind(sigc::mem_fun(*project, &Project::ClearSignatures), pwin.get_crn_progress(id)));
	}
}


void GUI::propagate_validation()
{
	GtkCRN::ProgressWindow pwin(_("Propagating validation…"), this, true);
	size_t id = pwin.add_progress_bar("");
	pwin.get_crn_progress(id)->SetType(crn::Progress::Type::PERCENT);
	pwin.run(sigc::bind(sigc::mem_fun(*project, &Project::PropagateValidation), pwin.get_crn_progress(id)));
	set_need_save();
	tree_selection_changed(false); // update display
}

void GUI::manage_entities()
{
	ori::EntityDialog mapwindow(*this);
	if (mapwindow.run() == Gtk::RESPONSE_ACCEPT)
		EntityManager::Save();
	else
		EntityManager::Reload();
}

void GUI::export_tei_alignment()
{
	Gtk::FileChooserDialog dialog(_("Please choose a file name for the TEI export."), Gtk::FILE_CHOOSER_ACTION_SAVE);
	dialog.set_current_name("alignment.xml");
	dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	std::vector<int> altbut;
	altbut.push_back(Gtk::RESPONSE_ACCEPT);
	altbut.push_back(Gtk::RESPONSE_CANCEL);
	dialog.set_alternative_button_order_from_array(altbut);
	dialog.set_default_response(Gtk::RESPONSE_ACCEPT);

	if (dialog.run() == Gtk::RESPONSE_ACCEPT)
	{
		dialog.hide();
		crn::xml::Document new_doc;
		new_doc.PushBackComment("Oriflamms TEI alignment");
		crn::xml::Element root(new_doc.PushBackElement("TEI"));
		crn::xml::Element root1(root.PushBackElement("facsimile"));
		const std::vector<ori::View> &views = project->GetStructure().GetViews();
		for (size_t i = 0; i < views.size(); ++i)
		{
			crn::xml::Element root2(root1.PushBackElement("surface"));
			crn::xml::Element e(root2.PushBackElement("graphic"));
			e.SetAttribute("url",views[i].GetImageName());
			const std::vector<ori::Column> &columns = views[i].GetColumns();
			for (size_t j=0; j<columns.size(); j++)
			{
				const std::vector<ori::Line> &lines = columns[j].GetLines();
				for (size_t k=0; k<lines.size(); k++)
				{
					const std::vector<ori::Word> &words = lines[k].GetWords();
					for (size_t l=0; l<words.size(); l++)
					{
						crn::xml::Element el(root2.PushBackElement("zone"));
						el.SetAttribute("start", "#" + words[l].GetId());
						const crn::Rect &box = words[l].GetBBox();
						el.SetAttribute("ulx", box.GetLeft());
						el.SetAttribute("uly", box.GetTop());
						el.SetAttribute("lrx", box.GetRight());
						el.SetAttribute("lry", box.GetBottom());
					}
				}
			}
		}
		new_doc.Save(dialog.get_filename().c_str());
	}
}

void GUI::display_search(Gtk::Entry *entry, ori::ValidationPanel *panel)
{
	panel->clear();

	crn::String wstring(entry->get_text().c_str());
	if (wstring.IsNotEmpty())
		for (size_t v = 0; v < project->GetNbViews(); ++v)
		{
			Glib::RefPtr<Gdk::Pixbuf> pb = Gdk::Pixbuf::create_from_file(project->GetDoc()->GetFilenames()[v].CStr());
			View &page(project->GetStructure().GetViews()[v]);
			for (size_t c = 0; c < page.GetColumns().size(); ++c)
			{
				Column &col(page.GetColumns()[c]);
				for (size_t l = 0; l < col.GetLines().size(); ++l)
				{
					Line &line(col.GetLines()[l]);
					for (size_t w = 0; w < line.GetWords().size(); ++w)
					{
						Word &oriword(line.GetWords()[w]);
						crn::String text(oriword.GetText().CStr());
						size_t pos = text.Find(wstring);
						while (pos != text.NPos())
						{
							//if (oriword.GetCharacterFrontiers().empty())
								//continue;
							crn::Rect bbox(oriword.GetBBox());
							if (!bbox.IsValid())
								break;

							// create small image (not using subpixbuf since it keeps a reference to the original image)
							std::vector<crn::Point2DInt> fronfrontier(oriword.GetCharacterFront(pos));
							std::vector<crn::Point2DInt> backfrontier(oriword.GetCharacterBack(pos + wstring.Length() - 1));

							int max_x = backfrontier[0].X;
							int min_x = fronfrontier[0].X;

							int min_y = fronfrontier[0].Y;
							int max_y = fronfrontier.back().Y;

							for (size_t i = 0; i < backfrontier.size(); ++i)
							{
								if(backfrontier[i].X > max_x)
									max_x = backfrontier[i].X;
							}
							for (size_t i = 0; i < fronfrontier.size(); ++i)
							{
								if(fronfrontier[i].X < min_x)
									min_x = fronfrontier[i].X;
							}
							if(max_x -min_x == 0)
								break;
							Glib::RefPtr<Gdk::Pixbuf> wpb(Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, max_x - min_x, bbox.GetHeight()));

							pb->copy_area(min_x, bbox.GetTop(), max_x - min_x, bbox.GetHeight(), wpb, 0, 0);

							if(!wpb->get_has_alpha())
								wpb = wpb->add_alpha(true, 255, 255, 255);
							guint8* pixs = wpb->get_pixels();
							int rowstrides = wpb->get_rowstride();
							int channels = wpb->get_n_channels();

							for(size_t j = 0; j < wpb->get_height(); ++ j)
							{
								int x = 0;
								for(size_t i = 0; i < fronfrontier.size() - 1; ++ i)
								{
									double x1 = double(fronfrontier[i].X - min_x);
									double y1 = double(fronfrontier[i].Y - min_y);
									double x2 = double(fronfrontier[i + 1].X - min_x);
									double y2 = double(fronfrontier[i + 1].Y - min_y);

									if (double(j) == y2)
										x = int(x2);

									if (double(j) == y1)
										x = int(x1);

									if ((double(j) < y2) && (double(j) > y1))
										x = int(x1 + (x2 - x1) * ((double(j) - y1)/(y2 - y1)));
								}
								for (int k = 0; k <= crn::Min(x, wpb->get_width() - 1); ++k)
									pixs[(k * channels + j * rowstrides )+3] = 0;

								int xx = 0;
								for(size_t i = 0; i < backfrontier.size() - 1; ++ i)
								{
									double x1 = double(backfrontier[i].X - oriword.GetBBox().GetLeft());
									double y1 = double(backfrontier[i].Y - oriword.GetBBox().GetTop());
									double x2 = double(backfrontier[i + 1].X - oriword.GetBBox().GetLeft());
									double y2 = double(backfrontier[i + 1].Y - oriword.GetBBox().GetTop());
									if (double(j) == y2)
										xx = int(x2);

									if (double(j) == y1)
										xx = int(x1);

									if ((double(j) < y2) && (double(j) > y1))
										xx = int(x1 + (x2 - x1) * ((double(j) - y1)/(y2 - y1)));

								}
								for (int k = crn::Max(xx, 0); k < wpb->get_width(); ++k)
									pixs[(k * channels + j * rowstrides )+3] = 0;
							}
							if (oriword.GetValid().IsFalse())
							{
								// add to reject list
								panel->add_element(wpb, panel->label_ko, WordPath(v, c, l, w), pos);
							}
							else if (oriword.GetValid().IsTrue())
								panel->add_element(wpb, panel->label_ok, WordPath(v, c, l, w), pos);
							else
								panel->add_element(wpb, panel->label_unknown, WordPath(v, c, l, w), pos);

							if (pos + wstring.Length() < text.Size())
								pos = text.Find(wstring, pos + wstring.Length());
							else
								pos = crn::String::NPos();
						} // found text

					} // for each word
				} // for each line
			} // for each column
		} // for each view
	panel->full_refresh();
}

void GUI::find_string()
{
	Gtk::Dialog dialog(_("Find string"),this);
	dialog.maximize();
	dialog.add_button(Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE);
	Gtk::Entry* entry = Gtk::manage(new Gtk::Entry);
	entry->show();
	Gtk::HBox* hb = Gtk::manage(new Gtk::HBox);
	hb->show();
	Gtk::Button* bt = Gtk::manage(new Gtk::Button(Gtk::Stock::FIND));
	bt->show();

	hb->pack_start(*entry, false, true, 0);
	hb->pack_start(*bt, false, true, 0);
	dialog.get_vbox()->pack_start(*hb, false, true, 0);

	ori::ValidationPanel *panel = Gtk::manage(new ori::ValidationPanel(*project, _("Found"), project->GetDoc()->GetFilenames(), false));
	dialog.get_vbox()->pack_start(*panel, true, true, 2);

	bt->signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &GUI::display_search), entry, panel));
	entry->signal_activate().connect(sigc::bind(sigc::mem_fun(this, &GUI::display_search), entry, panel));
	dialog.run();
}

