/* Copyright 2014-2015 INSA-Lyon, IRHT, ZHAO Xiaojuan, Université Paris Descartes
 *
 * file: OriTEIImporter.h
 * \author ZHAO Xiaojuan, Yann LEYDIER
 */

#ifndef TEIIMPORTER_H
#define TEIIMPORTER_H
#include <CRNUtils/CRNXml.h>
#include <set>
#include <gtkmm.h>

namespace ori
{
	class ExceptionTEISelection: public crn::Exception
	{
		public:
			explicit ExceptionTEISelection() noexcept {}
			explicit ExceptionTEISelection(const crn::StringUTF8 &msg) noexcept: crn::Exception(msg) {}
			explicit ExceptionTEISelection(const char *msg) noexcept: crn::Exception(msg) {}
	};
	
	/*! \brief Node to create a tree of XML element paths */
	class TEISelectionNode: public crn::ComplexObject
	{
		public:
			TEISelectionNode(crn::StringUTF8 n): name(n) { }

			TEISelectionNode(const TEISelectionNode&) = default;
			TEISelectionNode(TEISelectionNode&&) = default;
			TEISelectionNode& operator = (const TEISelectionNode&) = default;
			TEISelectionNode& operator = (TEISelectionNode&&) = default;

			virtual ~TEISelectionNode() override {}

			virtual const crn::String& GetClassName() const override { static crn::String cn(U"TEISelectionNode"); return cn; }
			virtual crn::Protocol GetClassProtocols() const noexcept override { return crn::Protocol::Serializable; }

			TEISelectionNode& AddChild(crn::StringUTF8 child);

			const crn::StringUTF8& GetName() const noexcept { return name; }
			const std::vector<TEISelectionNode>& GetChildren() const noexcept { return children; }

		private:
			void addElement(crn::xml::Element &el);
			virtual void deserialize(crn::xml::Element &el) override;
			virtual crn::xml::Element serialize(crn::xml::Element &parent) const override;
			std::vector<TEISelectionNode> children;
			crn::StringUTF8 name;

			CRN_DECLARE_CLASS_CONSTRUCTOR(TEISelectionNode)
			CRN_SERIALIZATION_CONSTRUCTOR(TEISelectionNode)
	};
	CRN_ALIAS_SMART_PTR(TEISelectionNode)

	/*! \brief Displays the structure of a TEI document and allows the selection of paths */
	class TEIImporter: public Gtk::Dialog
	{
		public:
			TEIImporter(const crn::Path &path1, const crn::Path &path2, Gtk::Window &parent);
			virtual ~TEIImporter(void) override {}

			TEISelectionNode export_selected_elements() const;

		private:
			class recolumn :public Gtk::TreeModel::ColumnRecord
			{
				public:
					recolumn(void) { add(name); add(text); add(weight); add(color); add(choice); add(strike); }

					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<bool> text;
					Gtk::TreeModelColumn<int> weight;
					Gtk::TreeModelColumn<Glib::ustring> color;
					Gtk::TreeModelColumn<bool> choice;
					Gtk::TreeModelColumn<bool> strike;
			};

			recolumn column;

			Gtk::ScrolledWindow scrollwin;
			Gtk::TreeView view;
			Glib::RefPtr<Gtk::TreeStore> treestore;

			void fill_tree(crn::xml::Node &nd, Gtk::TreeModel::Row &row, std::set<Gtk::TreePath> &toexpand, std::set<Gtk::TreePath> &tocollapse);
			void on_row_checked(const Glib::ustring& path);
			void check_upward(Gtk::TreeModel::Row row);
			void uncheck_downward(Gtk::TreeModel::Row row);
			void do_export_selected_elements(const Gtk::TreeModel::Row &row, TEISelectionNode &node) const;
	};

}
#endif

