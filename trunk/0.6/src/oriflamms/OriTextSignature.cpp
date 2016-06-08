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
 * \file OriTextSignature.cpp
 * \author Yann LEYDIER
 */

#include <OriTextSignature.h>
#include <OriFeatures.h>
#include <CRNUtils/CRNXml.h>
#include <CRNi18n.h>

using namespace ori;

TextSignatureDB::TextSignatureDB() = default;

TextSignatureDB& TextSignatureDB::getInstance()
{
	static auto instance = TextSignatureDB{};
	return instance;
}

void TextSignatureDB::Load(const crn::Path &fname)
{
	auto &inst = getInstance();
	inst.signature_db.clear();
	auto doc = crn::xml::Document(fname); // may throw
	auto root = doc.GetRoot();
	for (auto el = root.BeginElement(); el != root.EndElement(); ++el)
	{
		inst.signature_db.emplace(char32_t(el.GetAttribute<int>("char")), el.GetAttribute<crn::StringUTF8>("sig"));
	}
}

crn::StringUTF8 TextSignatureDB::Convert(char32_t c)
{
	return getInstance().signature_db[c];
}

crn::StringUTF8 TextSignatureDB::Convert(const crn::String &str)
{
	auto &inst = getInstance();
	auto res = crn::StringUTF8{};
	for (auto tmp = size_t(0); tmp < str.Size(); ++tmp)
		res += inst.signature_db[str[tmp]];
	return res;
}

std::vector<TextSignature> TextSignatureDB::Sign(const crn::String &str)
{
	const auto f = Convert(str);
	auto sig = std::vector<TextSignature>{};
	if (f.IsEmpty())
		return sig;
	sig.emplace_back(true, f[0]);
	for (auto tmp = size_t(1); tmp < f.Size(); ++tmp)
		sig.emplace_back(false, f[tmp]);
	return sig;
}
