/* Copyright 2015 Université Paris Descartes
 * 
 * file: OriTextSignature.cpp
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
	sig.emplace_back(true, f[0]);
	for (auto tmp = size_t(1); tmp < f.Size(); ++tmp)
		sig.emplace_back(false, f[tmp]);
	return sig;
}

