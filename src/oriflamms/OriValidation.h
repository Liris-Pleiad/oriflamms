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
 * \file OriValidation.h
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
			std::map<crn::String, std::vector<Id> > words;
			bool firstrun;
			std::set<ElementPosition> needconfirm;
			bool needsave, batch, clustering;
			std::function<void(void)> saveatclose;
			std::function<void(void)> refreshatclose;
	};
}

#endif


