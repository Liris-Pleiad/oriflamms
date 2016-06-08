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
 * \file OriViewImpl.h
 * \author Yann LEYDIER
 */

#ifndef OriViewImpl_HEADER
#define OriViewImpl_HEADER

namespace ori
{
	struct View::Impl
	{
		Impl(const Id &surfid, Document::ViewStructure &s, const crn::Path &base, const crn::StringUTF8 &projname, bool clean);
		~Impl();
		void readZoneElements(crn::xml::Element &el);
		void readLinkElements(crn::xml::Element &el, bool clean);
		void load();
		void save();

		Id id;
		mutable crn::SBlock img;
		crn::Path imagename;
		mutable crn::SImageGray weight;

		Document::ViewStructure &struc;
		std::unordered_map<Id, Zone> zones;

		crn::xml::Document zonesdoc;
		crn::xml::Document linksdoc;
		std::unordered_map<crn::StringUTF8, crn::xml::Element> link_groups;
		crn::xml::Document ontolinksdoc;
		std::unique_ptr<crn::xml::Element> ontolinkgroup;

		crn::Path datapath;
		struct WordValidation
		{
			WordValidation(crn::Prop3 val): ok(val) {}
			crn::Prop3 ok;
			int left_corr = 0, right_corr = 0;
			crn::StringUTF8 imgsig;
		};
		std::unordered_map<Id, WordValidation> validation; // word Id
		std::unordered_map<Id, std::vector<GraphicalLine>> medlines; // column Id
		std::unordered_map<Id, std::pair<Id, size_t>> line_links; // line Id -> column Id + index
		std::unordered_map<Id, std::vector<Id>> onto_links; // word Id -> { glyph Ids }
		crn::StringUTF8 logmsg;
	};
}

#endif


