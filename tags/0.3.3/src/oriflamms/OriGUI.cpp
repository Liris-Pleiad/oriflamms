/* Copyright 2013-2014 INSA-Lyon & IRHT
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
#include <CRNi18n.h>

using namespace ori;

const Glib::ustring GUI::wintitle(Glib::ustring("Oriflamms ") + ORIFLAMMS_PACKAGE_VERSION + Glib::ustring(" – LIRIS & IRHT"));
const crn::String GUI::linesOverlay("lines");
const crn::String GUI::wordsOverlay("zwords");
const crn::String GUI::wordsOverlayOk("wordsok");
const crn::String GUI::wordsOverlayKo("wordsko");
const crn::String GUI::wordsOverlayUn("wordsun");
const int GUI::minwordwidth(5);

GUI::GUI():
	view_depth(None),
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

	actions->add(Gtk::Action::create("new-project", Gtk::Stock::NEW, _("_New project"), _("New project")), sigc::mem_fun(this, &GUI::new_project));
	actions->add(Gtk::Action::create("load-project", Gtk::Stock::OPEN, _("_Open project"), _("Open project")), sigc::mem_fun(this, &GUI::load_project));
	actions->add(Gtk::Action::create("import-project", Gtk::Stock::CONVERT, _("_Import project"), _("Import project")), sigc::mem_fun(this, &GUI::import_project));
	actions->add(Gtk::Action::create("save-project", Gtk::Stock::SAVE, _("_Save project"), _("Save project")), sigc::mem_fun(this, &GUI::save_project));
	actions->add(Gtk::Action::create("reload-tei", Gtk::Stock::REFRESH, _("Reload _TEI"), _("Reload TEI")), sigc::mem_fun(this, &GUI::reload_tei));
	
	actions->add(Gtk::Action::create("add-line", Gtk::Stock::INDENT, _("Add _line"), _("Add line")), sigc::mem_fun(this, &GUI::add_line));
	actions->get_action("add-line")->set_sensitive(false);
	
	actions->add(Gtk::Action::create("align-menu", _("_Alignment"), _("Alignment")));
	actions->add(Gtk::Action::create("align-all", Gtk::StockID("gtk-crn-two-pages"), _("Align _all"), _("Align all")), sigc::mem_fun(this, &GUI::align_all));
	actions->add(Gtk::Action::create("align-selection", Gtk::StockID("gtk-crn-block"), _("Align _selection"), _("Align selection")), sigc::mem_fun(this, &GUI::align_selection));
	actions->add(Gtk::Action::create("align-validation", Gtk::Stock::SPELL_CHECK, _("_Validate alignment"), _("Validate alignment")), sigc::mem_fun(this, &GUI::validate));
	actions->add(Gtk::Action::create("align-propagate-validation", Gtk::Stock::REFRESH, _("_Propagate validation"), _("Propagate validation")), sigc::mem_fun(this, &GUI::propagate_validation));
	actions->add(Gtk::Action::create("align-export-chars", Gtk::Stock::ITALIC, _("_Export validated characters"), _("Export validated characters")), sigc::mem_fun(this, &GUI::export_chars));
	actions->add(Gtk::Action::create("align-clear-sig", Gtk::Stock::CLEAR, _("_Clear signatures"), _("Clear signatures")), sigc::mem_fun(this, &GUI::clear_sig));
	actions->add(Gtk::Action::create("align-stats", Gtk::Stock::PRINT, _("_Statistics"), _("Statistics")), sigc::mem_fun(this, &GUI::stats));

	actions->add(Gtk::Action::create("option-menu", _("_Options"), _("Options")));
	Gtk::RadioAction::Group g;
	actions->add(Gtk::RadioAction::create(g, "validation-unit", _("_Unit validation"), "Unit validation"));
	actions->add(Gtk::RadioAction::create(g, "validation-batch", _("_Batch validation"), "Batch validation"));
	actions->add(Gtk::Action::create("change-font", Gtk::Stock::SELECT_FONT, _("Change _font"), _("Change font")), sigc::mem_fun(this, &GUI::change_font));

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
		"		<menu action='align-menu'>"
		"			<menuitem action='align-all'/>"
		"			<menuitem action='align-selection'/>"
		"			<separator/>"
		"			<menuitem action='align-clear-sig'/>"
		"			<separator/>"
		"			<menuitem action='align-validation'/>"
		"			<menuitem action='align-propagate-validation'/>"
		"			<menuitem action='align-export-chars'/>"
		"			<menuitem action='align-stats'/>"
		"		</menu>"
		"		<menu action='option-menu'>"
		"			<menuitem action='validation-batch'/>"
		"			<menuitem action='validation-unit'/>"
		"			<menuitem action='change-font'/>"
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
		"	</toolbar>"
		"</ui>";

	ui_manager->add_ui_from_string(ui_info);
	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox());
	vbox->show();
	add(*vbox);

	vbox->pack_start(*ui_manager->get_widget("/MenuBar"), false, true, 0);
	vbox->pack_start(*ui_manager->get_widget("/ToolBar"), false, true, 0);

	//Gtk::HBox *hb = Gtk::manage(new Gtk::HBox);
	//hb->show();
	//vbox->pack_start(*hb, true, true, 0);
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
	//hb->pack_start(*sw, false, true, 0);
	hb->add1(*sw);
	sw->show();
	sw->add(tv);
	tv.show();
	tv.get_selection()->signal_changed().connect(sigc::mem_fun(this, &GUI::tree_selection_changed));

	//hb->pack_start(img, true, true, 0);
	hb->add2(img);
	hb->set_position(300);
	img.show();
	GtkCRN::Image::OverlayConfig &lc(img.get_overlay_config(linesOverlay));
	lc.color1 = Gdk::Color("#009900");
	lc.color2 = Gdk::Color("#FFFFFF");
	lc.editable = true;
	lc.moveable = false;
	lc.can_jut_out = false;
	lc.draw_arrows = false;
	img.signal_overlay_changed().connect(sigc::mem_fun(this, &GUI::overlay_changed));
	img.signal_rmb_clicked().connect(sigc::mem_fun(this, &GUI::on_rmb_clicked));
	GtkCRN::Image::OverlayConfig &wc(img.get_overlay_config(wordsOverlay));
	wc.color1 = Gdk::Color("#000000");
	wc.color2 = Gdk::Color("#FFFFFF");
	wc.fill_rectangles = true;
	wc.fill_alpha = 0.0;
	wc.editable = true;
	wc.moveable = false;
	wc.can_jut_out = false;
	wc.show_labels = false;
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
	dial.set_copyright("© LIRIS / INSA Lyon & IRHT 2013");
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
			okbut = add_button(Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
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

		const crn::Path GetName() const { return pname.get_text().c_str(); }
		const crn::Path GetXML() const { return xfile.get_filename().c_str(); }
		const crn::Path GetImages() const { return imagedir.get_current_folder().c_str(); }
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
			//project = Project::New(dial.GetName(), dial.GetXML(), dial.GetImages());
			GtkCRN::ProgressWindow pw(_("Analyzing images…"), this, true);
			size_t i = pw.add_progress_bar(_("Image"));
			pw.get_crn_progress(i)->SetType(crn::Progress::ABSOLUTE);
			project = pw.run<crn::SharedPointer<Project> >(sigc::bind(sigc::ptr_fun(&Project::New), dial.GetName(), dial.GetXML(), dial.GetImages(), pw.get_crn_progress(i)));
		}
		catch (crn::Exception &ex)
		{
			project = NULL;
			GtkCRN::App::show_exception(ex, false);
		}
		GtkCRN::ProgressWindow pw(_("Loading…"), this, true);
		size_t i = pw.add_progress_bar(_("Page"));
		pw.get_crn_progress(i)->SetType(crn::Progress::ABSOLUTE);
		store = pw.run<Glib::RefPtr<Gtk::TreeStore> >(sigc::bind(sigc::mem_fun(this, &GUI::fill_tree), pw.get_crn_progress(i)));
		tv.set_model(store);
		setup_window();
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
			project = new Project(dial.get_selection());
		}
		catch (crn::Exception &ex)
		{
			project = NULL;
			GtkCRN::App::show_exception(ex, false);
		}
		GtkCRN::ProgressWindow pw(_("Loading…"), this, true);
		size_t i = pw.add_progress_bar(_("Page"));
		pw.get_crn_progress(i)->SetType(crn::Progress::ABSOLUTE);
		store = pw.run<Glib::RefPtr<Gtk::TreeStore> >(sigc::bind(sigc::mem_fun(this, &GUI::fill_tree), pw.get_crn_progress(i)));
		tv.set_model(store);
		setup_window();
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
		crn::xml::Document oxfile(datadir + crn::Path::Separator() + crn::Path("tei.xml")); // may throw
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
		oxfile.Save(newdatadir + crn::Path::Separator() + "tei.xml");
		// copy view files
		crn::IO::Directory viewdir(datadir);
		CRN_FOREACH(const crn::Path &viewfile, viewdir.GetFiles())
		{
			crn::Path fname(viewfile.GetFilename());
			if (fname == "tei.xml")
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
	actions->get_action("align-menu")->set_sensitive(project != NULL);
	actions->get_action("save-project")->set_sensitive(project != NULL);
	actions->get_action("reload-tei")->set_sensitive(project != NULL);

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
		CRNVector ilines(v.image->GetUserData(Project::LinesKey));
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
			crn::StringUTF8 coltext;
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
					lit->set_value(columns.text, Glib::ustring(s.CStr()));
					coltext += s + "⏎\n";
				}
				cit->set_value(columns.text, Glib::ustring(coltext.CStr()));
			}
			else
				cit->set_value(columns.error, Pango::STYLE_ITALIC);
			if (col < icol)
			{
				CRNVector c(ilines->At(col));
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

void GUI::tree_selection_changed()
{
	img.clear_overlay(linesOverlay);
	img.clear_overlay(wordsOverlay);
	img.clear_overlay(wordsOverlayOk);
	img.clear_overlay(wordsOverlayKo);
	img.clear_overlay(wordsOverlayUn);

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
			view_depth = Page;
			break;
		case 1:
			{ // column
				it = tv.get_selection()->get_selected();
				size_t colid = it->get_value(columns.index);
				const CRNBlock b(project->GetDoc()->GetView(viewid));
				const CRNVector cols(b->GetUserData(ori::Project::LinesKey));
				const CRNVector lines(cols[colid]);
				if (!lines->IsEmpty())
				{
					const OriGraphicalLine l(lines->Front());
					img.focus_on((l->GetFront().X + l->GetBack().X) / 2, l->GetFront().Y);
				}
				for (size_t tmpl = 0; tmpl < lines->Size(); ++tmpl)
				{
					display_line(viewid, colid, tmpl);
				}
				img.set_selection_type(GtkCRN::Image::Overlay::Line);
				view_depth = Column;
			}
			break;
		case 2:
			{ // line
				it = tv.get_selection()->get_selected();
				size_t linid = it->get_value(columns.index);
				size_t colid = it->parent()->get_value(columns.index);
				const CRNBlock b(project->GetDoc()->GetView(viewid));
				const CRNVector cols(b->GetUserData(ori::Project::LinesKey));
				const CRNVector lines(cols[colid]);
				const OriGraphicalLine l(lines[linid]);
				img.focus_on((l->GetFront().X + l->GetBack().X) / 2, l->GetFront().Y);
				display_line(viewid, colid, linid);
				view_depth = Line;
			}
			break;
	}

}

void GUI::display_line(size_t viewid, size_t colid, size_t linid)
{
	const CRNBlock b(project->GetDoc()->GetView(viewid));
	const CRNVector cols(b->GetUserData(ori::Project::LinesKey));
	const CRNVector lines(cols[colid]);
	const OriGraphicalLine l(lines[linid]);
	img.add_overlay_item(linesOverlay, linid, l->GetFront(), l->GetBack());
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
		img.add_overlay_item(wordsOverlay, itemid, w.GetBBox(), w.GetText().CStr());
		crn::String ov = wordsOverlayUn;
		if (w.GetValid().IsTrue()) ov = wordsOverlayOk;
		else if (w.GetValid().IsFalse()) ov = wordsOverlayKo;
		crn::Rect topbox(w.GetBBox());
		topbox.SetHeight(topbox.GetHeight() / 4);
		img.add_overlay_item(ov, itemid, topbox, w.GetText().CStr());
	}
}

void GUI::overlay_changed(crn::String overlay_id, crn::String overlay_item_id, GtkCRN::Image::MouseMode mm)
{
	if (overlay_id == GtkCRN::Image::selection_overlay)
	{ // can add line?
		actions->get_action("add-line")->set_sensitive(img.has_selection());
	}
	else if (overlay_id == linesOverlay)
	{ // line modified
		if (overlay_item_id.IsEmpty())
			return;
		// get path
		const int linid = overlay_item_id.ToInt();
		Gtk::TreeIter it(tv.get_selection()->get_selected());
		if (view_depth == Line)
		{ // iterator on line
			it = it->parent();
		}
		else if (view_depth != Column) // iterator not on column
			return;
		const size_t colid = it->get_value(columns.index);
		//std::cout << current_view_id << ", " << colid << ", " << linid << std::endl;
		// set line bounds
		CRNBlock b(project->GetDoc()->GetView(current_view_id));
		CRNVector cols(b->GetUserData(ori::Project::LinesKey));
		CRNVector lines(cols[colid]);
		OriGraphicalLine l(lines->At(linid));
		GtkCRN::Image::OverlayItem &item(img.get_overlay_item(overlay_id, overlay_item_id));
		l->SetBounds(item.x1, item.x2);
		// realign
		GtkCRN::ProgressWindow pw(_("Aligning…"), this, true);
		size_t i = pw.add_progress_bar("");
		pw.get_crn_progress(i)->SetType(crn::Progress::PERCENT);
		pw.run(sigc::bind(sigc::mem_fun(*project, &Project::AlignLine), current_view_id, colid, linid, pw.get_crn_progress(i))); // TODO XXX what if the line is not associated to a line in the XML?
		project->GetStructure().GetViews()[current_view_id].GetColumns()[colid].GetLines()[linid].SetCorrected();
		set_need_save();
		display_line(current_view_id, colid, linid); // refresh
	}
	else if (overlay_id == wordsOverlay)
	{ // word modified
		if (overlay_item_id.IsEmpty())
			return;
		// get path
		WordPath path(overlay_item_id);

		GtkCRN::Image::OverlayItem &item(img.get_overlay_item(overlay_id, overlay_item_id));
		int nleft = item.x1, nright = item.x2;
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
		pw.get_crn_progress(i)->SetType(crn::Progress::ABSOLUTE);
		store = pw.run<Glib::RefPtr<Gtk::TreeStore> >(sigc::bind(sigc::mem_fun(this, &GUI::fill_tree), pw.get_crn_progress(i)));
		tv.set_model(store);
	}
}

void GUI::align_selection()
{
	GtkCRN::ProgressWindow pw(_("Aligning…"), this, true);
	switch (view_depth)
	{
		case Page:
			{ // page
				size_t ic = pw.add_progress_bar(_("Column"));
				pw.get_crn_progress(ic)->SetType(crn::Progress::ABSOLUTE);
				size_t il = pw.add_progress_bar(_("Line"));
				pw.get_crn_progress(il)->SetType(crn::Progress::ABSOLUTE);
				pw.run(sigc::bind(sigc::mem_fun(*project, &Project::AlignView), current_view_id, pw.get_crn_progress(ic), pw.get_crn_progress(il), (crn::Progress*)NULL));
				set_need_save();
			}
			break;
		case Column:
			{ // column
				Gtk::TreeIter it = tv.get_selection()->get_selected();
				size_t colid = it->get_value(columns.index);

				size_t i = pw.add_progress_bar(_("Line"));
				pw.get_crn_progress(i)->SetType(crn::Progress::ABSOLUTE);
				pw.run(sigc::bind(sigc::mem_fun(*project, &Project::AlignColumn), current_view_id, colid, pw.get_crn_progress(i), (crn::Progress*)NULL));
				set_need_save();
			}
			break;
		case Line:
			{ // line
				Gtk::TreeIter it = tv.get_selection()->get_selected();
				size_t linid = it->get_value(columns.index);
				size_t colid = it->parent()->get_value(columns.index);

				size_t i = pw.add_progress_bar("");
				pw.get_crn_progress(i)->SetType(crn::Progress::PERCENT);
				pw.run(sigc::bind(sigc::mem_fun(*project, &Project::AlignLine), current_view_id, colid, linid, pw.get_crn_progress(i)));
				set_need_save();
			}
			break;
	}
	tree_selection_changed(); // update display
}

void GUI::align_all()
{
	GtkCRN::ProgressWindow pw(_("Aligning…"), this, true);
	size_t iv = pw.add_progress_bar(_("View"));
	pw.get_crn_progress(iv)->SetType(crn::Progress::ABSOLUTE);
	size_t ic = pw.add_progress_bar(_("Column"));
	pw.get_crn_progress(ic)->SetType(crn::Progress::ABSOLUTE);
	size_t il = pw.add_progress_bar(_("Line"));
	pw.get_crn_progress(il)->SetType(crn::Progress::ABSOLUTE);
	pw.run(sigc::bind(sigc::mem_fun(*project, &Project::AlignAll), pw.get_crn_progress(iv), pw.get_crn_progress(ic), pw.get_crn_progress(il), (crn::Progress*)NULL));
	set_need_save();
	tree_selection_changed(); // update display
}

void GUI::validate()
{
	validation_win = new Validation(*this, *project, Glib::RefPtr<Gtk::RadioAction>::cast_dynamic(actions->get_action("validation-batch"))->get_active(), true, crn::Bind(crn::Method(&GUI::set_need_save), *this), crn::Bind(crn::Method(&GUI::tree_selection_changed), *this));
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
		pw.get_crn_progress(i)->SetType(crn::Progress::PERCENT);
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
		pwin.get_crn_progress(id)->SetType(crn::Progress::PERCENT);
		pwin.run(sigc::bind(sigc::mem_fun(*project, &Project::ClearSignatures), pwin.get_crn_progress(id)));
	}
}

void GUI::export_chars()
{
	Gtk::FileChooserDialog dial(*this, _("Please select a folder where the characters will be saved"), Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	dial.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dial.add_button(Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
	std::vector<int> altbut;
	altbut.push_back(Gtk::RESPONSE_ACCEPT);
	altbut.push_back(Gtk::RESPONSE_CANCEL);
	dial.set_alternative_button_order_from_array(altbut);	
	dial.set_default_response(Gtk::RESPONSE_ACCEPT);
	if (dial.run() != Gtk::RESPONSE_ACCEPT)
		return;
	dial.hide();
	GtkCRN::ProgressWindow pwin(_("Exporting characters…"), this, true);
	size_t id = pwin.add_progress_bar("");
	pwin.get_crn_progress(id)->SetType(crn::Progress::PERCENT);
	pwin.run(sigc::bind(sigc::mem_fun(*project, &Project::ExportCharacters), dial.get_current_folder().c_str(), true, pwin.get_crn_progress(id)));
}

void GUI::propagate_validation()	 
{
	GtkCRN::ProgressWindow pwin(_("Propagating validation…"), this, true);
	size_t id = pwin.add_progress_bar("");
	pwin.get_crn_progress(id)->SetType(crn::Progress::PERCENT);
	pwin.run(sigc::bind(sigc::mem_fun(*project, &Project::PropagateValidation), pwin.get_crn_progress(id)));
	set_need_save();
	tree_selection_changed(); // update display
}

