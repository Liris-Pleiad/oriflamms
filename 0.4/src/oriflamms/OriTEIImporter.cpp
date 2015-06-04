/* Copyright 2014-2015 INSA-Lyon, IRHT, ZHAO Xiaojuan, Université Paris Descartes
 *
 * file: OriTEIImporter.cpp
 * \author ZHAO Xiaojuan, Yann LEYDIER
 */

#include <OriConfig.h>
#include <CRNException.h>
#include <OriTEIImporter.h>
#include <CRNData/CRNDataFactory.h>
#include <OriEntityManager.h>
#include <CRNi18n.h>

#include <iostream>

using namespace ori;

/////////////////////////////////////////////////////////////////////
// TEISelectionNode
/////////////////////////////////////////////////////////////////////

TEISelectionNode& TEISelectionNode::AddChild(crn::StringUTF8 child)
{
	children.emplace_back(child);
	return children.back();
}

/*!
 * Adds an element and its children
 * \param[in]	el	the element to add
 */
void TEISelectionNode::addElement(crn::xml::Element &el)
{
	TEISelectionNode &n = AddChild(el.GetAttribute<crn::StringUTF8>("name"));
	for (crn::xml::Element e = el.BeginElement(); e != el.EndElement(); ++e)
	{
		n.addElement(e);
	}
}

void TEISelectionNode::deserialize(crn::xml::Element &el)
{
	name = el.GetAttribute<crn::StringUTF8>("name");
	for (crn::xml::Element e = el.BeginElement(); e != el.EndElement(); ++e)
	{
		addElement(e);
	}
}

crn::xml::Element TEISelectionNode::serialize(crn::xml::Element &parent) const
{
	crn::xml::Element e(parent.PushBackElement(GetClassName().CStr()));
	e.SetAttribute("name", name);
	for (size_t c = 0; c < children.size(); ++c)
	{
		children[c].serialize(e);
	}
	return e;
}

/////////////////////////////////////////////////////////////////////
// TEIImporter
/////////////////////////////////////////////////////////////////////

/*!
 * \param[in]	path	path to an XML TEI file
 * \param[in]	parent	the parent window
 */
TEIImporter::TEIImporter(const crn::Path &path, Gtk::Window &parent):
	Gtk::Dialog(_("Node selection"), parent, true)
{
	resize(100,600);

	scrollwin.add(view);
	scrollwin.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

	add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	add_button(_("Next"), Gtk::RESPONSE_ACCEPT)->set_image(*Gtk::manage(new Gtk::Image(Gtk::Stock::GO_FORWARD, Gtk::ICON_SIZE_BUTTON)));
	set_alternative_button_order_from_array(std::vector<int>{ Gtk::RESPONSE_ACCEPT, Gtk::RESPONSE_CANCEL });	
	set_default_response(Gtk::RESPONSE_ACCEPT);

	get_vbox()->pack_start(scrollwin);

	treestore = Gtk::TreeStore::create(column);
	view.set_model(treestore);

	view.append_column_editable(_("Selected"), column.choice);
	view.append_column(_("Element"), column.name);
	view.append_column(_("Contains text"), column.text);

	Gtk::TreeViewColumn *tvc = view.get_column(1);
	Gtk::CellRendererText *cr = dynamic_cast<Gtk::CellRendererText*>(view.get_column_cell_renderer(1));
	if (tvc && cr)
	{
		tvc->add_attribute(cr->property_weight(), column.weight);
		tvc->add_attribute(cr->property_foreground(), column.color);
		tvc->add_attribute(cr->property_strikethrough(), column.strike);
	}

	Gtk::CellRendererToggle* renderer = dynamic_cast<Gtk::CellRendererToggle*>(view.get_column_cell_renderer(0));
	renderer->signal_toggled().connect(sigc::mem_fun(this, &TEIImporter::on_row_checked));

	xdoc = EntityManager::ExpandXML(path); // may throw
	auto root = xdoc->GetRoot();

	auto row = *treestore->append();
	row[column.name] = root.GetName().CStr();
	auto toexpand = std::set<Gtk::TreePath>{};
	auto tocollapse = std::set<Gtk::TreePath>{};
	for (auto n = root.BeginNode(); n != root.EndNode(); ++n)
		fill_tree(n, row, toexpand, tocollapse);
	// expand to selected rows
	for (const auto &p : toexpand)
	{
		view.expand_to_path(p);
		check_upward(*treestore->get_iter(p));
	}
	// collapse unwanted tree parts
	for (const auto &p : tocollapse)
	{
		view.collapse_row(p);
		uncheck_downward(*treestore->get_iter(p));
	}

	show_all_children();
}

void TEIImporter::fill_tree(crn::xml::Node &nd, Gtk::TreeModel::Row &row, std::set<Gtk::TreePath> &toexpand, std::set<Gtk::TreePath> &tocollapse)
{
	static const auto keyelems = std::set<Glib::ustring>{ "pb", "cb", "lb", "w", "pc" };
	static const auto okelems = std::set<Glib::ustring>{ "abbr", "sic", "hi", "orig", "lem" };
	static const auto koelems = std::set<Glib::ustring>{ "expan", "corr", "supplied", "reg", "rdg", "note", "teiHeader" };

	if (nd.IsText())
	{ // previous element contains text
		row[column.text] = true;
	}
	else if (nd.IsElement())
	{ // element
		auto el = nd.AsElement();
		const auto name = Glib::ustring{nd.GetValue().CStr()};

		// add node or reuse existing one?
		auto newrow = Gtk::TreeModel::Row{};
		bool found = false;
		for (auto &c : row.children())
			if (c[column.name] == name)
			{ // reuse node
				newrow = c;
				found = true;
				break;
			}

		if (!found)
		{ // add node
			newrow = *treestore->append(row.children());
			newrow[column.name] = name;

			if (keyelems.find(name) != keyelems.end())
			{ // relevant element
				newrow[column.weight] = Pango::WEIGHT_HEAVY;
				toexpand.insert(treestore->get_path(newrow));

				bool error = false;
				if (name == "w" || name == "pc" || name == "cb" || name == "pb")
					if (el.GetAttribute<crn::StringUTF8>("xml:id",true).IsEmpty())
						error = true;
				if (name == "pb")
					if (el.GetAttribute<crn::StringUTF8 > ("facs",true).IsEmpty())
						error = true;
				if (name == "lb")
					if (el.GetAttribute<int>("n",true) == 0)
						error = true;
				if (error)
				{
					newrow[column.color] = "red";
					set_response_sensitive(Gtk::RESPONSE_ACCEPT, false);
				}
			}
			else if (okelems.find(name) != okelems.end())
			{
				toexpand.insert(treestore->get_path(newrow));
			}
			else if (koelems.find(name) != koelems.end())
			{
				tocollapse.insert(treestore->get_path(newrow));
			}
			else
			{ // other elements
				newrow[column.weight] = Pango::WEIGHT_NORMAL;
				newrow[column.choice] = false;
				newrow[column.strike] = true;
			}
		}

		// add children
		for (auto n = el.BeginNode(); n != el.EndNode(); ++n)
			fill_tree(n, newrow, toexpand, tocollapse);

	} // element
}

void TEIImporter::on_row_checked(const Glib::ustring& path)
{
	auto row = *treestore->get_iter(path);
	if (row[column.choice])
		check_upward(row);
	else
		uncheck_downward(row);
}

void TEIImporter::check_upward(Gtk::TreeModel::Row row)
{
	if (row)
	{
		row[column.choice] = true;
		row[column.strike] = false;
		check_upward(*row.parent());
	}
}

void TEIImporter::uncheck_downward(Gtk::TreeModel::Row row)
{
	if (row)
	{
		row[column.choice] = false;
		row[column.strike] = true;
		for (auto &c : row.children())
			uncheck_downward(c);
	}
}

//use the class TEISelectionNode for saving the path of the selected node
void TEIImporter::do_export_selected_elements(const Gtk::TreeModel::Row &row, TEISelectionNode &nde) const
{
	for (Gtk::TreeModel::iterator it_child = row.children().begin(); it_child!=row.children().end(); ++it_child)
	{
		if ((*it_child)[column.choice] == true)
		{
			Glib::ustring p = (*it_child)[column.name];
			TEISelectionNode &nd = nde.AddChild(p.c_str());
			do_export_selected_elements(*it_child, nd);
		}
	}
}

TEISelectionNode TEIImporter::export_selected_elements() const
{
	Gtk::TreeModel::Children children = treestore->children();
	Gtk::TreeModel::iterator it = children.begin();
	Gtk::TreeModel::Row row = *it;
	Glib::ustring p = row[column.name];
	TEISelectionNode nd(p.c_str());
	do_export_selected_elements(*it, nd);
	return nd;
}







CRN_BEGIN_CLASS_CONSTRUCTOR(TEISelectionNode)
	CRN_DATA_FACTORY_REGISTER(U"TEISelectionNode", TEISelectionNode)
CRN_END_CLASS_CONSTRUCTOR(TEISelectionNode)

