/* Copyright 2013-2015 INSA-Lyon, IRHT, ZHAO Xiaojuan, Université Paris Descartes
 *
 * file: OriValidation.h
 * \author Yann LEYDIER
 */

#ifndef OriValidation_HEADER
#define OriValidation_HEADER

#include <oriflamms_config.h>
#include <OriValidationPanel.h>
#include <gtkmm.h>
#include <CRNUtils/CRNProgress.h>
#include <functional>

namespace ori
{
	/*! \brief Word tabular validation */
	class Validation: public Gtk::Window
	{
		public:
			Validation(Gtk::Window &parent, Document &docu, bool batch_valid, bool use_clustering, const std::function<void(void)> &savefunc, const std::function<void(void)> &refreshfunc);
			virtual ~Validation() override {}
			const std::map<crn::StringUTF8, std::vector<Id>>& get_word_ids() { return words; }

		private:
			void update_words(crn::Progress *prog);
			void tree_selection_changed();
			void read_word(const Glib::ustring &wname, crn::Progress *prog = nullptr);
			void on_remove_words(ValidationPanel::ElementList words);
			void on_unremove_words(ValidationPanel::ElementList words);
			void on_close();
			void conclude_word();

			// widgets
			class model: public Gtk::TreeModelColumnRecord
			{
				public:
					model() { add(name); add(pop); add(size); add(weight); }
					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<size_t> pop;
					Gtk::TreeModelColumn<size_t> size;
					Gtk::TreeModelColumn<int> weight;
			};
			model columns;
			Glib::RefPtr<Gtk::ListStore> store;
			Gtk::TreeView tv;
			ValidationPanel okwords, kowords;

			// attributes
			Document &doc;
			std::map<crn::StringUTF8, std::vector<Id> > words;
			bool firstrun;
			std::set<Id> needconfirm;
			bool needsave, batch, clustering;
			std::function<void(void)> saveatclose;
			std::function<void(void)> refreshatclose;
	};
}

#endif


