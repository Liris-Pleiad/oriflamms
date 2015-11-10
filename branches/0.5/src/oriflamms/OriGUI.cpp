/* Copyright 2013-2015 INSA-Lyon, IRHT, ZHAO Xiaojuan, Université Paris Descartes
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

const Glib::ustring GUI::wintitle(Glib::ustring("Oriflamms ") + ORIFLAMMS_PACKAGE_VERSION + Glib::ustring(" – LIRIS, IRHT & LIPADE"));
const crn::String GUI::linesOverlay("lines");
const crn::String GUI::wordsOverlay("zwords");
const crn::String GUI::wordsOverlayOk("wordsok");
const crn::String GUI::wordsOverlayKo("wordsko");
const crn::String GUI::wordsOverlayUn("wordsun");
const crn::String GUI::charOverlay("chars");
const int GUI::minwordwidth(5);


GUI::GUI():
	view_depth(ViewDepth::None),
	aligndial(*this),
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
	actions->add(Gtk::Action::create("load-project", Gtk::Stock::OPEN, _("_Open project"), _("Open project")), sigc::mem_fun(this, &GUI::load_project));
	actions->add(Gtk::Action::create("save-project", Gtk::Stock::SAVE, _("_Save project"), _("Save project")), sigc::mem_fun(this, &GUI::save_project));

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

	// Structure menu
	actions->add(Gtk::Action::create("structure-menu", _("_Structure"), _("Structure")));
	actions->add(Gtk::Action::create("add-line", Gtk::Stock::INDENT, _("Add _line"), _("Add line")), sigc::mem_fun(this, &GUI::add_line));
	actions->get_action("add-line")->set_sensitive(false);

	// Option menu
	actions->add(Gtk::Action::create("option-menu", _("_Options"), _("Options")));
	Gtk::RadioAction::Group g;
	actions->add(Gtk::RadioAction::create(g, "validation-unit", _("_Unit validation"), "Unit validation"));
	actions->add(Gtk::RadioAction::create(g, "validation-batch", _("_Batch validation"), "Batch validation"));
	actions->add(Gtk::Action::create("change-font", Gtk::Stock::SELECT_FONT, _("Change _font"), _("Change font")), sigc::mem_fun(this, &GUI::change_font));
	actions->add(Gtk::Action::create("manage-entities", Gtk::Stock::EDIT, _("Manage _entities"), _("Manage entities")), sigc::mem_fun(this,&GUI::manage_entities));

	// Line menu
	actions->add(Gtk::Action::create("add-point-to-line", Gtk::Stock::ADD, _("_Add point"), _("Add point")));
	actions->add(Gtk::Action::create("rem-point-from-line", Gtk::Stock::REMOVE, _("_Remove point"), _("Remove point")));
	actions->add(Gtk::Action::create("remove-line", Gtk::Stock::DELETE, _("_Delete line"), _("Delete line")));

	ui_manager->insert_action_group(img.get_actions());

	add_accel_group(ui_manager->get_accel_group());

	Glib::ustring ui_info =
		"<ui>"
		"	<menubar name='MenuBar'>"
		"		<menu action='app-file-menu'>"
		"			<menuitem action='load-project'/>"
		"			<menuitem action='save-project'/>"
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
		"			<separator/>"
		"			<menuitem action='find-string'/>"
		"		</menu>"
		"		<menu action='structure-menu'>"
		"			<menuitem action='edit'/>"
		"			<separator/>"
		"			<menuitem action='add-line'/>"
		"		</menu>"
		"		<menu action='align-menu'>"
		"			<menuitem action='align-clear-sig'/>"
		"			<menuitem action='align-all'/>"
		"			<menuitem action='align-selection'/>"
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
		"	<popup name='LinePopup'>"
		"		<menuitem action='add-point-to-line'/>"
		"		<menuitem action='rem-point-from-line'/>"
		"		<separator/>"
		"		<menuitem action='remove-line'/>"
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
	dial.set_copyright("© LIRIS / INSA Lyon & IRHT & LIPADE / Université Paris Descartes 2013-2015");
	dial.set_website("http://oriflamms.hypotheses.org/");
	try
	{
		dial.set_logo(Gdk::Pixbuf::create_from_file((ori::Config::GetStaticDataDir() + crn::Path::Separator() + "icon.png").CStr()));
	}
	catch (...) { }
	dial.show();
	dial.run();
}

void GUI::load_project()
{
	Gtk::FileChooserDialog dial(*this, _("Open project"), Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	dial.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dial.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);
	std::vector<int> altbut;
	altbut.push_back(Gtk::RESPONSE_ACCEPT);
	altbut.push_back(Gtk::RESPONSE_CANCEL);
	dial.set_alternative_button_order_from_array(altbut);
	dial.set_default_response(Gtk::RESPONSE_ACCEPT);
	if (dial.run() == Gtk::RESPONSE_ACCEPT)
	{
		dial.hide();
		try
		{
			doc = std::make_unique<Document>(dial.get_current_folder().c_str());
			const auto &error = doc->ErrorReport();
			if (error.IsNotEmpty())
			{
				GtkCRN::App::show_message(error.CStr(), Gtk::MESSAGE_WARNING);
			}
		}
		catch (ExceptionTEISelection &ex)
		{
			auto dir = crn::IO::Directory(dial.get_current_folder().c_str() / "texts"_p);
			for (const auto fname : dir.GetFiles())
				if (fname.EndsWith("-c.xml"))
				{
					TEIImporter impdial{fname, *this};
					if (impdial.run() == Gtk::RESPONSE_ACCEPT)
					{
						impdial.hide();
						impdial.export_selected_elements().Save(dial.get_current_folder().c_str() / "oriflamms"_p / "tei_selection.xml"_p);
						try
						{
							doc = std::make_unique<Document>(dial.get_current_folder().c_str());
							const auto &error = doc->ErrorReport();
							if (error.IsNotEmpty())
							{
								GtkCRN::App::show_message(error.CStr(), Gtk::MESSAGE_WARNING);
							}
						}
						catch (crn::Exception &ex)
						{
							GtkCRN::App::show_exception(ex, false);
						}
					}
					else
					{
						GtkCRN::App::show_exception(ex, false);
					}
				}
		}
		catch (crn::Exception &ex)
		{
			doc.reset();
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

void GUI::setup_window()
{
	actions->get_action("align-menu")->set_sensitive(doc != nullptr);
	actions->get_action("valid-menu")->set_sensitive(doc != nullptr);
	actions->get_action("display-menu")->set_sensitive(doc != nullptr);
	actions->get_action("structure-menu")->set_sensitive(doc != nullptr);
	actions->get_action("save-project")->set_sensitive(doc != nullptr);

	set_win_title();
}

void GUI::set_win_title()
{
	Glib::ustring t(wintitle);
	if (need_save)
		t += " *";
	if (doc)
	{
		t += " (";
		t += doc->GetName().CStr();
		t += ")";
	}
	set_title(t);
}

void GUI::add_line()
{
	try
	{
		// convert selection to line
		auto pts = img.get_selection_as_line();
		if (crn::Abs(pts.first.X - pts.second.X) < crn::Abs(pts.first.Y - pts.second.Y))
		{
			GtkCRN::App::show_message(_("The line was not added because its skew is too high."), Gtk::MESSAGE_ERROR);
			return;
		}
		auto plist = std::vector<crn::Point2DInt>{};
		if (pts.first.X < pts.second.X)
		{
			plist.push_back(pts.first);
			plist.push_back(pts.second);
		}
		else
		{
			plist.push_back(pts.second);
			plist.push_back(pts.first);
		}
		// TODO
#if 0
		// get column data
		auto b = doc->GetView(current_view_id);
		auto cols = std::static_pointer_cast<crn::Vector>(b->GetUserData(ori::Project::LinesKey));
		auto it = tv.get_selection()->get_selected();
		size_t colid = it->get_value(columns.index);
		auto lines = std::static_pointer_cast<crn::Vector>(cols->At(colid));
		// find where to place the new line
		auto lh = size_t(0);
		auto pos = std::numeric_limits<size_t>::max();
		for (auto lnum : crn::Range(*lines))
		{
			auto l = std::static_pointer_cast<GraphicalLine>(lines->At(lnum));
			lh = l->GetLineHeight();
			if (l->GetFront().Y > plist.front().Y)
			{
				pos = lnum;
				break;
			}
		}
		// add the line
		if (lh == 0)
		{
			// TODO ask for line height
			lh = 10; // XXX
		}
		if (pos == std::numeric_limits<size_t>::max())
			lines->PushBack(std::make_shared<GraphicalLine>(std::make_shared<crn::LinearInterpolation>(plist.begin(), plist.end()), lh));
		else
			lines->Insert(std::make_shared<GraphicalLine>(std::make_shared<crn::LinearInterpolation>(plist.begin(), plist.end()), lh), pos);
		// clear column alignment
		project->ClearAlignment(current_view_id, colid);
		// update display
		auto s = int(lines->Size()) + " "_s;
		s += _("line(s)");
		it->set_value(columns.image, Glib::ustring(s.CStr()));
		img.clear_selection();
		tree_selection_changed(false);
#endif
	}
	catch (...) { }
}

void GUI::rem_line(const Id &l)
{
	// TODO
#if 0
	// remove line
	auto b = project->GetDoc()->GetView(v);
	auto cols = std::static_pointer_cast<crn::Vector>(b->GetUserData(ori::Project::LinesKey));
	auto lines = std::static_pointer_cast<crn::Vector>(cols->At(c));
	lines->Remove(l);
	// clear column alignment
	project->ClearAlignment(v, c);
	// update display
	auto s = int(lines->Size()) + " "_s;
	s += _("line(s)");
	auto it = tv.get_selection()->get_selected(); // XXX this expects the removed line to be on the selected view!
	it->set_value(columns.image, Glib::ustring(s.CStr()));
	img.clear_selection();
	tree_selection_changed(false);
#endif
}

void GUI::add_point_to_line(const Id &l, int x, int y)
{
	// TODO
#if 0
	auto b = project->GetDoc()->GetView(v);
	auto cols = std::static_pointer_cast<crn::Vector>(b->GetUserData(ori::Project::LinesKey));
	auto lines = std::static_pointer_cast<crn::Vector>(cols->At(c));
	auto line = std::static_pointer_cast<GraphicalLine>(lines->At(l));
	auto pts = std::vector<crn::Point2DInt>{};
	for (const auto &p : line->GetMidline()) // get a copy of the points
		pts.emplace_back(int(p.X), int(p.Y));
	auto newpt = crn::Point2DInt(x, y);
	auto it = std::upper_bound(pts.begin(), pts.end(), newpt, [](const crn::Point2DInt &p1, const crn::Point2DInt &p2){ return p1.X < p2.X; });
	pts.insert(it, newpt);
	line->SetMidline(pts);
	set_need_save();
	display_line(v, c, l); // refresh
#endif
}

void GUI::rem_point_from_line(const Id &l, int x, int y)
{
	// TODO
#if 0
	auto b = project->GetDoc()->GetView(v);
	auto cols = std::static_pointer_cast<crn::Vector>(b->GetUserData(ori::Project::LinesKey));
	auto lines = std::static_pointer_cast<crn::Vector>(cols->At(c));
	auto line = std::static_pointer_cast<GraphicalLine>(lines->At(l));
	auto pts = std::vector<crn::Point2DInt>{};
	for (const auto &p : line->GetMidline()) // get a copy of the points
		pts.emplace_back(int(p.X), int(p.Y));
	if (pts.size() <= 2)
	{
		return;
	}
	auto rit = pts.begin();
	auto dist = std::numeric_limits<int>::max();
	for (auto it = pts.begin(); it != pts.end(); ++it)
	{
		auto d = crn::Max(crn::Abs(it->X - x), crn::Abs(it->Y - y));
		if (d < dist)
		{
			dist = d;
			rit = it;
		}
	}
	pts.erase(rit);
	line->SetMidline(pts);
	set_need_save();
	display_line(v, c, l); // refresh
#endif
}

Glib::RefPtr<Gtk::TreeStore> GUI::fill_tree(crn::Progress *prog)
{
	Glib::RefPtr<Gtk::TreeStore> newstore = Gtk::TreeStore::create(columns);
	current_view_id = "";
	current_view = View{};
	if (!doc)
		return newstore;

	prog->SetMaxCount(int(doc->GetViews().size()));
	for (const auto &vid : doc->GetViews())
	{ // display view
		auto view = doc->GetView(vid);
		auto vit = newstore->append();
		vit->set_value(columns.id, Glib::ustring{vid.CStr()});
		vit->set_value(columns.name, Glib::ustring(view.GetImageName().GetBase().CStr()));
		auto xnum = crn::StringUTF8(int(view.GetPages().size())).CStr() + Glib::ustring(" ") + _("page(s)");
		vit->set_value(columns.xml, xnum);

		for (const auto &pid : view.GetPages())
		{ // display page
			auto pit = newstore->append(vit->children());
			pit->set_value(columns.id, Glib::ustring{pid.CStr()});
			auto name = Glib::ustring(_("Page"));
			name += " ";
			name += pid.CStr();
			pit->set_value(columns.name, name);
			const auto &p = view.GetPage(pid);
			xnum = crn::StringUTF8(int(p.GetColumns().size())).CStr() + Glib::ustring(" ") + _("column(s)");
			pit->set_value(columns.xml, xnum);

			for (const auto &cid : p.GetColumns())
			{ // display column
				auto cit = newstore->append(pit->children());
				cit->set_value(columns.id, Glib::ustring{cid.CStr()});
				name = _("Column");
				name += " ";
				name += cid.CStr();
				cit->set_value(columns.name, name);
				const auto &c = view.GetColumn(cid);
				const auto xlines = c.GetLines().size();
				xnum = crn::StringUTF8(int(xlines)).CStr() + Glib::ustring(" ") + _("line(s)");
				cit->set_value(columns.xml, xnum);
				const auto ilines = view.GetGraphicalLines(cid).size();
				xnum = crn::StringUTF8(int(ilines)).CStr() + Glib::ustring(" ") + _("line(s)");
				cit->set_value(columns.image, xnum);
				if (xlines != ilines)
					cit->set_value(columns.error, Pango::STYLE_ITALIC);

				auto ctxt = Glib::ustring("");
				for (const auto &lid : c.GetLines())
				{ // display line
					auto lit = newstore->append(cit->children());
					lit->set_value(columns.id, Glib::ustring{lid.CStr()});
					name = _("Line");
					name += " ";
					name += lid.CStr();
					lit->set_value(columns.name, name);
					const auto &l = view.GetLine(lid);
					xnum = crn::StringUTF8(int(l.GetWords().size())).CStr() + Glib::ustring(" ") + _("word(s)");
					lit->set_value(columns.xml, xnum);

					// get text and remove html elements
					auto s = ""_s;
					for (const auto &wid : l.GetWords())
						s += view.GetWord(wid).GetText() + " ";
					auto ltxt = Glib::ustring{};
					ltxt.reserve(s.Size() * 2);
					for (size_t tmp = 0; tmp < s.Size(); ++tmp)
					{
						if (s[tmp] == '&')
							ltxt += "&amp;";
						else if (s[tmp] == '<')
							ltxt += "&lt;";
						else if (s[tmp] == '>')
							ltxt += "&gt;";
						else
							ltxt += s[tmp];
					}
					lit->set_value(columns.text, ltxt);

					ctxt += ltxt + "\n";
				} // lines
				cit->set_value(columns.text, ctxt);
			} // columns
		} // pages
		prog->Advance();
	} // views
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
	auto viewid = Id{it->get_value(columns.id).c_str()};
	if (viewid != current_view_id)
	{
		current_view_id = viewid;
		current_view = doc->GetView(current_view_id);
		img.set_pixbuf(Gdk::Pixbuf::create_from_file(current_view.GetImageName().CStr()));
	}
	img.set_selection_type(GtkCRN::Image::Overlay::None);
	img.clear_selection();
	actions->get_action("add-line")->set_sensitive(false);

	switch (level)
	{
		case 0:
			// view
			tv.expand_row(Gtk::TreePath(it), false);
			for (const auto &row : it->children())
				tv.expand_row(Gtk::TreePath(row), false);
			view_depth = ViewDepth::View;
			break;
		case 1:
			// page
			tv.expand_row(Gtk::TreePath(tv.get_selection()->get_selected()), false);
			view_depth = ViewDepth::Page;
			break;
		case 2:
			{
				// column
				it = tv.get_selection()->get_selected();
				auto colid = Id{it->get_value(columns.id).c_str()};
				const auto &column = current_view.GetColumn(colid);
				auto fx = 0;
				auto fy = 0;
				for (const auto &lid : column.GetLines())
				{
					if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("show-lines"))->get_active())
						display_line(lid);
					if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("show-words"))->get_active())
						display_words(lid);
					if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("show-characters"))->get_active())
						display_characters(lid);
					if (!fx && !fy)
					{
						try
						{
							const auto lzoneid = current_view.GetLine(lid).GetZone();
							const auto &r = current_view.GetZone(lzoneid).GetPosition();
							fx = r.GetCenterX();
							fy = r.GetTop();
						}
						catch (...) {}
					}
				}
				// set focus
				if (!fx || !fy)
				{
					const auto &glines = current_view.GetGraphicalLines(colid);
					if (!glines.empty())
					{
						fx = (glines.front().GetFront().X + glines.front().GetBack().X) / 2;
						fy = glines.front().GetFront().Y;
					}
				}
				img.focus_on(fx, fy);

				if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("edit"))->get_active())
					img.set_selection_type(GtkCRN::Image::Overlay::Line);

				view_depth = ViewDepth::Column;
			}
			break;
		case 3:
			{
				// line
				it = tv.get_selection()->get_selected();
				auto linid = Id{it->get_value(columns.id).c_str()};
				// set focus
				auto ok = true;
				try
				{
					const auto lzoneid = current_view.GetLine(linid).GetZone();
					const auto &r = current_view.GetZone(lzoneid).GetPosition();
					img.focus_on(r.GetCenterX(), r.GetTop());
				}
				catch (...) { ok = false; }
				if (!ok)
				{
					try
					{
						const auto &gl = current_view.GetGraphicalLine(linid);
						img.focus_on((gl.GetFront().X + gl.GetBack().X) / 2, gl.GetFront().Y);
					}
					catch (...) { }
				}

				if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("show-lines"))->get_active())
					display_line(linid);
				if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("show-words"))->get_active())
					display_words(linid);
				if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("show-characters"))->get_active())
					display_characters(linid);

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
	
	if (view_depth == ViewDepth::Column)
	{
		if (mod)
			img.set_selection_type(GtkCRN::Image::Overlay::Line);
		else
		{
			img.clear_selection();
			img.set_selection_type(GtkCRN::Image::Overlay::None);
		}
	}

	if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("show-words"))->get_active())
		tree_selection_changed(false);
}

void GUI::display_line(const Id &linid)
{
	try
	{
		const auto &gl = current_view.GetGraphicalLine(linid);
		auto pts = std::vector<crn::Point2DInt>{};
		for (const auto &p : gl.GetMidline())
			pts.emplace_back(int(p.X), int(p.Y));
		img.add_overlay_item(linesOverlay, linid, pts);
	}
	catch (...) { }
}

void GUI::display_words(const Id &linid)
{
	const auto &line = current_view.GetLine(linid);
	for (const auto &wid : line.GetWords())
	{
		display_update_word(wid);
	}
}

void GUI::display_characters(const Id &linid)
{
	const auto &line = current_view.GetLine(linid);
	for (const auto &wid : line.GetWords())
	{
		const auto &word = current_view.GetWord(wid);
		if (!current_view.GetZone(word.GetZone()).GetPosition().IsValid())
			continue;
		for (const auto &cid : word.GetCharacters())
		{
			const auto &character = current_view.GetCharacter(cid);
			const auto &zid = character.GetZone();
			auto ov = wordsOverlayUn;
			// TODO change color if validated? wordsOverlayOk wordsOverlayKo
			const auto &zone = current_view.GetZone(zid);
			auto topbox = zone.GetPosition();
			if (!topbox.IsValid())
				continue;
			topbox.SetHeight(topbox.GetHeight() / 4);
			img.add_overlay_item(ov, cid, topbox, character.GetText().CStr());
			img.add_overlay_item(charOverlay, cid, zone.GetContour());
		}
	}
}

void GUI::display_update_word(const Id &wordid, const crn::Option<int> &newleft, const crn::Option<int> &newright)
{
	// compute id
	auto itemid = crn::String{wordid};
	// clean old overlay items
	try { img.remove_overlay_item(wordsOverlay, itemid); } catch (...) { }
	try { img.remove_overlay_item(wordsOverlayUn, itemid); } catch (...) { }
	try { img.remove_overlay_item(wordsOverlayOk, itemid); } catch (...) { } 
	try { img.remove_overlay_item(wordsOverlayKo, itemid); } catch (...) { }

	const auto &word = current_view.GetWord(wordid);
	if (word.GetZone().IsEmpty())
		return;
	auto &zone = current_view.GetZone(word.GetZone());
	if (!zone.GetPosition().IsValid())
		return;
	if (newleft)
	{
		current_view.UpdateLeftFrontier(wordid, newleft.Get());
	}
	if (newright)
	{
		current_view.UpdateRightFrontier(wordid, newright.Get());
	}
	if (newright || newleft)
	{
		current_view.ComputeContour(word.GetZone());
		// TODO
		//project->AlignWordCharacters(AlignConfig::AllChars, wordid);
	}

	if (Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(actions->get_action("edit"))->get_active())
	{ // edit mode, display boxes
		img.add_overlay_item(wordsOverlay, wordid, zone.GetPosition(), word.GetText().CStr());
	}
	else
	{ // not in edit mode, display polygons
		img.add_overlay_item(wordsOverlay, wordid, zone.GetContour(), word.GetText().CStr());
	}

	auto ov = wordsOverlayUn;
	const auto &valid = current_view.IsValid(wordid);
	if (valid.IsTrue()) ov = wordsOverlayOk;
	if (valid.IsFalse()) ov = wordsOverlayKo;
	auto topbox = zone.GetPosition();
	topbox.SetHeight(topbox.GetHeight() / 4);
	img.add_overlay_item(ov, wordid, topbox, word.GetText().CStr());
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
		// get id
		const auto linid = overlay_item_id.ToInt();
		Gtk::TreeIter it(tv.get_selection()->get_selected());
		if (view_depth == ViewDepth::Line)
		{
			// iterator on line
			it = it->parent();
		}
		else if (view_depth != ViewDepth::Column) // iterator not on column
			return;
		const auto colid = Id{it->get_value(columns.id).c_str()};
		// set line bounds
		// TODO
#if 0
		crn::SBlock b(project->GetDoc()->GetView(current_view_id));
		crn::SVector cols(std::static_pointer_cast<crn::Vector>(b->GetUserData(ori::Project::LinesKey)));
		crn::SVector lines(std::static_pointer_cast<crn::Vector>(cols->At(colid)));
		SGraphicalLine l(std::static_pointer_cast<GraphicalLine>(lines->At(linid)));
		GtkCRN::Image::OverlayItem &item(img.get_overlay_item(overlay_id, overlay_item_id));

		GtkCRN::Image::Polygon &po = static_cast<GtkCRN::Image::Polygon&>(item);
		l->SetMidline(po.points);

		/*
		// realign
		GtkCRN::ProgressWindow pw(_("Aligning…"), this, true);
		size_t i = pw.add_progress_bar("");
		pw.get_crn_progress(i)->SetType(crn::Progress::Type::PERCENT);
		pw.run(sigc::bind(sigc::mem_fun(*project, &Project::AlignLine), current_view_id, colid, linid, pw.get_crn_progress(i))); // TODO XXX what if the line is not associated to a line in the XML?
		project->GetStructure().GetViews()[current_view_id].GetColumns()[colid].GetLines()[linid].SetCorrected();
		*/
#endif
		set_need_save();
		display_line(linid); // refresh
	}
	else if (overlay_id == wordsOverlay)
	{
		// word modified
		if (overlay_item_id.IsEmpty())
			return;
		// get id
		const auto id = Id{overlay_item_id};

		const auto &item = img.get_overlay_item(overlay_id, overlay_item_id);
		const auto &rect = static_cast<const GtkCRN::Image::Rectangle&>(item);
		auto nleft = rect.rect.GetLeft() ,nright = rect.rect.GetRight();

		if (nright - nleft < minwordwidth)
			nright = nleft + minwordwidth;

		const auto &path = doc->GetPosition(id);
		const auto &line = current_view.GetLine(path.line).GetWords();
		auto it = std::find(line.begin(), line.end(), id);
		// need to update previous word?
		if (it != line.begin());
		{
			const auto &pp = *(it - 1);
			const auto &pzone = current_view.GetZone(current_view.GetWord(pp).GetZone());
			if (pzone.GetPosition().IsValid())
			{
				if (nleft < pzone.GetPosition().GetLeft() + minwordwidth)
					nleft = pzone.GetPosition().GetLeft() + minwordwidth;
			}
			display_update_word(pp, crn::Option<int>(), nleft - 1);
		}
		// need to update next word?
		++it;
		if (it != line.end())
		{
			const auto &np = *it;
			const auto &nzone = current_view.GetZone(current_view.GetWord(np).GetZone());
			if (nzone.GetPosition().IsValid())
			{
				if (nright > nzone.GetPosition().GetRight() - minwordwidth)
					nright = nzone.GetPosition().GetRight() - minwordwidth;
			}
			display_update_word(np, nright + 1, crn::Option<int>());
		}

		// update bbox
		display_update_word(id, nleft, nright);

		set_need_save();
	}
}

void GUI::on_rmb_clicked(guint mouse_button, guint32 time, std::vector<std::pair<crn::String, crn::String> > overlay_items_under_mouse, int x_on_image, int y_on_image)
{
	for (std::vector<std::pair<crn::String, crn::String> >::const_iterator it = overlay_items_under_mouse.begin(); it != overlay_items_under_mouse.end(); ++it)
	{
		if (it->first == wordsOverlay)
		{ // if it is a word, cycle through validation
			try
			{
				auto id = Id{it->second};
				auto &w = current_view.GetWord(id);
				const auto &val = current_view.IsValid(id);
				if (val.IsTrue())
					current_view.SetValid(id, false);
				else if (val.IsFalse())
					current_view.SetValid(id, crn::Prop3::Unknown);
				else
					current_view.SetValid(id, true);
				display_update_word(id);
				set_need_save();
				return; // modify just one
			}
			catch (...) { }
		}
		if (it->first == linesOverlay)
		{ // if it is a line, pop a menu up
			auto sit = tv.get_selection()->get_selected();
			auto colid = sit->get_value(columns.id);
			auto lineid = Id{it->second};
			line_rem_connection.disconnect();
			line_rem_connection = actions->get_action("remove-line")->signal_activate().connect(sigc::bind(sigc::mem_fun(this, &GUI::rem_line), lineid));
			line_add_point_connection.disconnect();
			line_add_point_connection = actions->get_action("add-point-to-line")->signal_activate().connect(sigc::bind(sigc::mem_fun(this, &GUI::add_point_to_line), lineid, x_on_image, y_on_image));
			line_rem_point_connection.disconnect();
			line_rem_point_connection = actions->get_action("rem-point-from-line")->signal_activate().connect(sigc::bind(sigc::mem_fun(this, &GUI::rem_point_from_line), lineid, x_on_image, y_on_image));
			dynamic_cast<Gtk::Menu*>(ui_manager->get_widget("/LinePopup"))->popup(mouse_button, time);
		}
	}
}

void GUI::align_selection()
{
	if (aligndial.run() == Gtk::RESPONSE_ACCEPT)
	{
		aligndial.hide();
		GtkCRN::ProgressWindow pw(_("Aligning…"), this, true);
		// TODO
#if 0
		switch (view_depth)
		{
			case ViewDepth::Page:
				{
					// page
					size_t ic = pw.add_progress_bar(_("Column"));
					pw.get_crn_progress(ic)->SetType(crn::Progress::Type::ABSOLUTE);
					size_t il = pw.add_progress_bar(_("Line"));
					pw.get_crn_progress(il)->SetType(crn::Progress::Type::ABSOLUTE);
					pw.run(sigc::bind(sigc::mem_fun(*project, &Project::AlignView), aligndial.get_config(), current_view_id, pw.get_crn_progress(ic), pw.get_crn_progress(il), (crn::Progress*)nullptr));
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
					pw.run(sigc::bind(sigc::mem_fun(*project, &Project::AlignColumn), aligndial.get_config(), current_view_id, colid, pw.get_crn_progress(i), (crn::Progress*)nullptr));
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
					pw.run(sigc::bind(sigc::mem_fun(*project, &Project::AlignLine), aligndial.get_config(), current_view_id, colid, linid, pw.get_crn_progress(i)));
					set_need_save();
				}
				break;
		}
#endif
		tree_selection_changed(false); // update display
	}
	else
		aligndial.hide();
}

void GUI::align_all()
{
	if (aligndial.run() == Gtk::RESPONSE_ACCEPT)
	{
		aligndial.hide();
		// TODO
#if 0
		GtkCRN::ProgressWindow pw(_("Aligning…"), this, true);
		size_t iv = pw.add_progress_bar(_("View"));
		pw.get_crn_progress(iv)->SetType(crn::Progress::Type::ABSOLUTE);
		size_t ic = pw.add_progress_bar(_("Column"));
		pw.get_crn_progress(ic)->SetType(crn::Progress::Type::ABSOLUTE);
		size_t il = pw.add_progress_bar(_("Line"));
		pw.get_crn_progress(il)->SetType(crn::Progress::Type::ABSOLUTE);
		pw.run(sigc::bind(sigc::mem_fun(*project, &Project::AlignAll), aligndial.get_config(), pw.get_crn_progress(iv), pw.get_crn_progress(ic), pw.get_crn_progress(il), (crn::Progress*)nullptr));
#endif
		set_need_save();
		tree_selection_changed(false); // update display
	}
	else
		aligndial.hide();
}

void GUI::validate()
{
	// TODO
#if 0
	validation_win = std::make_unique<Validation>(*this, *project, Glib::RefPtr<Gtk::RadioAction>::cast_dynamic(actions->get_action("validation-batch"))->get_active(), true, std::bind(std::mem_fn(&GUI::set_need_save), this), std::bind(std::mem_fn(&GUI::tree_selection_changed), this, false));
	validation_win->show();
#endif
}

void GUI::set_need_save()
{
	if (doc)
	{
		need_save = true;
		set_win_title();
	}
}

void GUI::save_project()
{
	if (doc)
	{
		// TODO
#if 0
		GtkCRN::ProgressWindow pw(_("Saving…"), this, true);
		size_t i = pw.add_progress_bar("");
		pw.get_crn_progress(i)->SetType(crn::Progress::Type::PERCENT);
		pw.run(sigc::bind(sigc::mem_fun(*project, &Project::Save), pw.get_crn_progress(i)));
#endif
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
	// TODO
#if 0
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
#endif
}

void GUI::clear_sig()
{
	// TODO
#if 0
	Gtk::MessageDialog msg(*this, _("Are you sure you wish to flush the images' signatures?"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
	if (msg.run() == Gtk::RESPONSE_YES)
	{
		msg.hide();
		GtkCRN::ProgressWindow pwin(_("Flushing signatures…"), this, true);
		size_t id = pwin.add_progress_bar("");
		pwin.get_crn_progress(id)->SetType(crn::Progress::Type::PERCENT);
		pwin.run(sigc::bind(sigc::mem_fun(*project, &Project::ClearSignatures), pwin.get_crn_progress(id)));
	}
#endif
}


void GUI::propagate_validation()
{
	// TODO
#if 0
	GtkCRN::ProgressWindow pwin(_("Propagating validation…"), this, true);
	size_t id = pwin.add_progress_bar("");
	pwin.get_crn_progress(id)->SetType(crn::Progress::Type::PERCENT);
	pwin.run(sigc::bind(sigc::mem_fun(*project, &Project::PropagateValidation), pwin.get_crn_progress(id)));
	set_need_save();
	tree_selection_changed(false); // update display
#endif
}

void GUI::manage_entities()
{
	ori::EntityDialog mapwindow(*this);
	if (mapwindow.run() == Gtk::RESPONSE_ACCEPT)
		EntityManager::Save();
	else
		EntityManager::Reload();
}

void GUI::display_search(Gtk::Entry *entry, ori::ValidationPanel *panel)
{
	panel->clear();
	const auto str = crn::StringUTF8(entry->get_text().c_str());
	if (str.IsNotEmpty())
		for (const auto &vid : doc->GetViews())
		{ // for each view
			const auto view = doc->GetView(vid);
			// load image
			auto pb = Gdk::Pixbuf::create_from_file(view.GetImageName().CStr());
			for (const auto &w : view.GetWords())
			{ // for each character
				const auto &wzone = view.GetZone(w.second.GetZone());
				if (!wzone.GetPosition().IsValid())
					continue;
				// look for searched string
				const auto &text = w.second.GetText();
				auto pos = text.Find(str);
				while (pos != text.NPos())
				{ // found
					const auto &fczone = view.GetZone(w.second.GetCharacters()[pos]);
					if (!fczone.GetPosition().IsValid())
						continue;
					const auto &bczone = view.GetZone(w.second.GetCharacters()[pos + str.Size() - 1]);
					if (!bczone.GetPosition().IsValid())
						continue;
					// retrieve frontiers
					auto fronfrontier = std::vector<crn::Point2DInt>{};
					const auto &fcont = fczone.GetContour();
					if (fcont.empty())
					{
						const auto &box = fczone.GetPosition();
						fronfrontier.emplace_back(box.GetLeft(), box.GetTop());
						fronfrontier.emplace_back(box.GetLeft(), box.GetBottom());
					}
					else
					{
						for (auto tmp : crn::Range(fcont))
						{
							if (fcont[tmp].Y > fcont[tmp + 1].Y)
								break;
							fronfrontier.push_back(fcont[tmp]);
						}
					}
					auto backfrontier = std::vector<crn::Point2DInt>{};
					const auto &bcont = bczone.GetContour();
					if (bcont.empty())
					{
						const auto &box = bczone.GetPosition();
						backfrontier.emplace_back(box.GetRight(), box.GetTop());
						backfrontier.emplace_back(box.GetRight(), box.GetBottom());
					}
					else
					{
						auto tmp = size_t(0);
						for (; tmp < bcont.size(); ++tmp)
						{
							if (bcont[tmp].Y > bcont[tmp + 1].Y)
								break;
						}
						for (; tmp < bcont.size(); ++tmp)
							backfrontier.push_back(bcont[tmp]);
					}
				}
			}
			/*
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
							if (!oriword.IsAligned())
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
							crn::Rect bbox(oriword.GetBBox());
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
								panel->add_element(wpb, panel->label_ko, Id(v, c, l, w), pos);
							}
							else if (oriword.GetValid().IsTrue())
								panel->add_element(wpb, panel->label_ok, Id(v, c, l, w), pos);
							else
								panel->add_element(wpb, panel->label_unknown, Id(v, c, l, w), pos);

							if (pos + wstring.Length() < text.Size())
								pos = text.Find(wstring, pos + wstring.Length());
							else
								pos = crn::String::NPos();
						} // found text

					} // for each word
				} // for each line
			} // for each column
		*/
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

	// TODO
	ori::ValidationPanel *panel = Gtk::manage(new ori::ValidationPanel(*doc, _("Found"), false));
	dialog.get_vbox()->pack_start(*panel, true, true, 2);

	bt->signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &GUI::display_search), entry, panel));
	entry->signal_activate().connect(sigc::bind(sigc::mem_fun(this, &GUI::display_search), entry, panel));

	dialog.run();
}

