#include <CRNString.h>
#include <CRNUtils/CRNXml.h>
#include <map>

int main(void)
{
	std::map<crn::Char, crn::StringUTF8> sig;
	sig[L' '] = " ";
	sig[L'-'] = ",";
	sig[L'?'] = ".'"; // printed as .° in the manuscript

	sig[L','] = ",";
	sig[L'.'] = ".";
	sig[L':'] = "."; // prints as a .
	sig[L'A'] = ",l";
	sig[L'B'] = "ll";
	sig[L'C'] = "('"; // ll  l,'  'l,  ,l'  ll  ,'  l'  l  ,'  l,'  l',  ll  l'  'l  l'  ll'  l'  'l  l  ll  l'
	sig[L'D'] = "l)"; // l'  ,',  ll  ,','  ,','  l,  ,l'  l'  ,','  ,''  ll  ll  ,'l  l'  ,',  ,',  ,','  ll  ,'',  ,'l  ll  ll
	sig[L'E'] = "l'"; // ','  l'  ll  l'  ','  l',  'l  l'  l'  ll  ,'  ll  'l  l',  l'
	sig[L'F'] = ""; // TODO NOT FOUND
	sig[L'G'] = "(l"; // ll  ll  ll  l'
	sig[L'I'] = "l"; // 'l  ,l  ,l  l  ,l  l  l  l
	sig[L'J'] = "l"; // same as I
	sig[L'K'] = "ll"; // ll  l',  ll  
	sig[L'L'] = "l";
	sig[L'M'] = "lll";
	sig[L'N'] = "ll"; // ll  ll
	sig[L'O'] = "(')"; // l,'l  ll'  l'l  ll'  l,'l  ,,l  l,','  l'l  l'l  l,'l  ll'  l,'l  l,''  lll  lll  ll
	sig[L'P'] = "l)"; // l',  ll  ll  ll
	sig[L'Q'] = "(')"; // ll',  l',l  l,',  ll,'  ll,'  'l,'  l,'l  l',',  l',',  lll
	sig[L'R'] = "ll"; // l,l  ,'l  ll  l'  ll  ll
	sig[L'S'] = "()"; // ,''l   '',  ,''l  ,''l  ',','  ',',l  '',,  l',  l'  ,','l  ,ll  l','l  l','  ,','l  ','',  ,',','
	sig[L'T'] = "l'"; // l',  l'  'l  l'  ll  l',  l'
	sig[L'U'] = "ll"; // ',ll  ll  ll  ll
	sig[L'V'] = "ll";
	sig[L'X'] = ""; // TODO NOT FOUND
	sig[L'Y'] = ",l'"; // NOT FOUND  ,l'  ,ll  ,l'  (3 occurrences)
	sig[L'Z'] = ""; // TODO NOT FOUND
	sig[L'a'] = ",l";
	sig[L'b'] = "l)"; // l,  ll  ll  ll  ll
	sig[L'c'] = "('";
	sig[L'd'] = "(,";
	sig[L'e'] = "('";
	sig[L'f'] = "l'";
	sig[L'g'] = "(l";
	sig[L'h'] = "ll";
	sig[L'i'] = "l";
	sig[L'j'] = ",l"; // only one occurrence
	sig[L'k'] = "l'"; // only one occurrence
	sig[L'l'] = "l";
	sig[L'm'] = "lll";
	sig[L'n'] = "ll";
	sig[L'o'] = "()";
	sig[L'p'] = "l)";
	sig[L'q'] = "(l";
	sig[L'r'] = "l'";
	sig[L's'] = "l"; // ll  l,  'l,  <-- using long s' signature
	sig[L't'] = "(";
	sig[L'u'] = "ll";
	sig[L'v'] = "ll";
	sig[L'x'] = "l'";
	sig[L'y'] = "ll'";
	sig[L'z'] = ")";
	sig[L'í'] = "l";
	sig[L'ſ'] = "l"; // s long
	sig[L'ʼ'] = ""; // nothing
	sig[L'̅'] = ""; // bar
	sig[L'ͣ'] = ""; // a sup <-- nothing
	sig[L'ͤ'] = ""; // e sup <-- nothing
	sig[L'ᷥ'] = ""; // s long sup <-- nothing
	sig[L'ẏ'] = "ll";
	sig[L'ꝛ'] = ")"; // small r rotunda
	sig[L'ꝰ'] = ""; // modified letter us <-- nothing
	sig[L''] = ",'"; // et barré
	sig[L''] = ","; // punctus elevatus
	sig[L''] = ""; // short virgula <-- to small, always cut :(
	sig[L'¶'] = "l"; // para
	sig[L'ꝯ'] = ",)"; // con

	crn::xml::Document doc;
	doc.PushBackComment("oriflamms signature table");
	crn::xml::Element root(doc.PushBackElement("orisig"));
	for (std::map<crn::Char, crn::StringUTF8>::const_iterator it = sig.begin(); it != sig.end(); ++it)
	{
		crn::xml::Element el(root.PushBackElement("ent"));
		el.SetAttribute("char", it->first);
		el.SetAttribute("sig", it->second);
	}
	doc.Save("orisig.xml");
	return 0;
}

