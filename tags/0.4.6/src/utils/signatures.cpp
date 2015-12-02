#include <CRNString.h>
#include <CRNUtils/CRNXml.h>
#include <map>

int main(void)
{
	std::map<char32_t, crn::StringUTF8> sig;
	sig[U' '] = " ";
	sig[U'-'] = ",";
	sig[U'?'] = ".'"; // printed as .° in the manuscript

	sig[U','] = ",";
	sig[U'.'] = ".";
	sig[U':'] = "."; // prints as a .
	sig[U';'] = "."; // ????
	sig[U'A'] = ",l";
	sig[U'B'] = "ll";
	sig[U'C'] = "('"; // ll  l,'  'l,  ,l'  ll  ,'  l'  l  ,'  l,'  l',  ll  l'  'l  l'  ll'  l'  'l  l  ll  l'
	sig[U'D'] = "l)"; // l'  ,',  ll  ,','  ,','  l,  ,l'  l'  ,','  ,''  ll  ll  ,'l  l'  ,',  ,',  ,','  ll  ,'',  ,'l  ll  ll
	sig[U'E'] = "l'"; // ','  l'  ll  l'  ','  l',  'l  l'  l'  ll  ,'  ll  'l  l',  l'
	sig[U'F'] = ""; // TODO NOT FOUND
	sig[U'G'] = "(l"; // ll  ll  ll  l'
	sig[U'I'] = "l"; // 'l  ,l  ,l  l  ,l  l  l  l
	sig[U'J'] = "l"; // same as I
	sig[U'K'] = "ll"; // ll  l',  ll  
	sig[U'L'] = "l";
	sig[U'M'] = "lll";
	sig[U'N'] = "ll"; // ll  ll
	sig[U'O'] = "(')"; // l,'l  ll'  l'l  ll'  l,'l  ,,l  l,','  l'l  l'l  l,'l  ll'  l,'l  l,''  lll  lll  ll
	sig[U'P'] = "l)"; // l',  ll  ll  ll
	sig[U'Q'] = "(')"; // ll',  l',l  l,',  ll,'  ll,'  'l,'  l,'l  l',',  l',',  lll
	sig[U'R'] = "ll"; // l,l  ,'l  ll  l'  ll  ll
	sig[U'S'] = "()"; // ,''l   '',  ,''l  ,''l  ',','  ',',l  '',,  l',  l'  ,','l  ,ll  l','l  l','  ,','l  ','',  ,',','
	sig[U'T'] = "l'"; // l',  l'  'l  l'  ll  l',  l'
	sig[U'U'] = "ll"; // ',ll  ll  ll  ll
	sig[U'V'] = "ll";
	sig[U'X'] = ""; // TODO NOT FOUND
	sig[U'Y'] = ",l'"; // NOT FOUND  ,l'  ,ll  ,l'  (3 occurrences)
	sig[U'Z'] = ""; // TODO NOT FOUND
	sig[U'a'] = ",l";
	sig[U'b'] = "l)"; // l,  ll  ll  ll  ll
	sig[U'c'] = "('";
	sig[U'd'] = "(,";
	sig[U'e'] = "('";
	sig[U'f'] = "l'";
	sig[U'g'] = "(l";
	sig[U'h'] = "ll";
	sig[U'i'] = "l";
	sig[U'j'] = ",l"; // only one occurrence
	sig[U'k'] = "l'"; // only one occurrence
	sig[U'l'] = "l";
	sig[U'm'] = "lll";
	sig[U'n'] = "ll";
	sig[U'o'] = "()";
	sig[U'p'] = "l)";
	sig[U'q'] = "(l";
	sig[U'r'] = "l'";
	sig[U's'] = "l"; // ll  l,  'l,  <-- using long s' signature
	sig[U't'] = "(";
	sig[U'u'] = "ll";
	sig[U'v'] = "ll";
	sig[U'x'] = "l'";
	sig[U'y'] = "ll'";
	sig[U'z'] = ")";
	sig[U'í'] = "l";
	sig[U'ſ'] = "l"; // s long
	sig[U'ʼ'] = ""; // nothing
	sig[L'̅'] = ""; // bar
	sig[L'ͣ'] = ""; // a sup <-- nothing
	sig[L'ͤ'] = ""; // e sup <-- nothing
	sig[L'ᷥ'] = ""; // s long sup <-- nothing
	sig[U'ẏ'] = "ll";
	sig[U'ꝛ'] = ")"; // small r rotunda
	sig[U'ꝰ'] = ""; // modified letter us <-- nothing
	sig[U''] = ",'"; // et barré
	sig[U''] = ","; // punctus elevatus
	sig[U''] = ""; // short virgula <-- to small, always cut :(
	sig[U'¶'] = "l"; // para
	sig[U'ꝯ'] = ",)"; // con

	crn::xml::Document doc;
	doc.PushBackComment("oriflamms signature table");
	crn::xml::Element root(doc.PushBackElement("orisig"));
	for (std::map<char32_t, crn::StringUTF8>::const_iterator it = sig.begin(); it != sig.end(); ++it)
	{
		crn::xml::Element el(root.PushBackElement("ent"));
		el.SetAttribute("char", it->first);
		el.SetAttribute("sig", it->second);
	}
	doc.Save("orisig.xml");
	return 0;
}
