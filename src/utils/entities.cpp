#include <CRNUtils/CRNXml.h>
#include <map>

int main(void)
{
	std::map<crn::StringUTF8, crn::StringUTF8> conv;
	// BFM MSS additions to MENOTA
	//
	conv.insert(std::make_pair("stigma", "ς"));
	conv.insert(std::make_pair("eol", "")); // ???
	conv.insert(std::make_pair("dblbar", "͞"));
	conv.insert(std::make_pair("combinvbreve", "̑̑")); // ???
	conv.insert(std::make_pair("combdblinvbreve", "͡")); // ???
	conv.insert(std::make_pair("dbloblhyphen", "⸗"));
	conv.insert(std::make_pair("vdbldot", "v.."));
	// superscript letters in abbreviations
	conv.insert(std::make_pair("aeligsupmod", "")); // ???
	conv.insert(std::make_pair("anligsupmod", "")); // ???
	conv.insert(std::make_pair("anscapligsupmod", "")); // ???
	conv.insert(std::make_pair("aoligsupmod", "")); // ???
	conv.insert(std::make_pair("arligsupmod", "")); // ???
	conv.insert(std::make_pair("asupmod", "ͣ")); // ???
	conv.insert(std::make_pair("avligsupmod", "")); // ???
	conv.insert(std::make_pair("bsupmod", "")); // ???
	conv.insert(std::make_pair("bscapsupmod", "")); // ???
	conv.insert(std::make_pair("ccedilsupmod", "")); // ???
	conv.insert(std::make_pair("ethsupmod", "")); // ???
	conv.insert(std::make_pair("drotsupmod", "")); // ???
	conv.insert(std::make_pair("dscapsupmod", "")); // ???
	conv.insert(std::make_pair("esupmod", "ͤ")); // ???
	conv.insert(std::make_pair("fsupmod", "")); // ???
	conv.insert(std::make_pair("gsupmod", "")); // ???
	conv.insert(std::make_pair("gscapsupmod", "")); // ???
	conv.insert(std::make_pair("inodotsupmod", "")); // ???
	conv.insert(std::make_pair("isupmod", "ͥ")); // ???
	conv.insert(std::make_pair("jsupmod", "")); // ???
	conv.insert(std::make_pair("jnodotsupmod", "")); // ???
	conv.insert(std::make_pair("ksupmod", "")); // ???
	conv.insert(std::make_pair("kscapsupmod", "")); // ???
	conv.insert(std::make_pair("lsupmod", "")); // ???
	conv.insert(std::make_pair("lscapsupmod", "")); // ???
	conv.insert(std::make_pair("mscapsupmod", "")); // ???
	conv.insert(std::make_pair("nsupmod", "")); // ???
	conv.insert(std::make_pair("nscapsupmod", "")); // ???
	conv.insert(std::make_pair("msupmod", "ͫ")); // ???
	conv.insert(std::make_pair("oslashsupmod", "")); // ???
	conv.insert(std::make_pair("orrotsupmod", "")); // ???
	conv.insert(std::make_pair("orumsupmod", "")); // ???
	conv.insert(std::make_pair("osupmod", "ͦ")); // ???
	conv.insert(std::make_pair("psupmod", "")); // ???
	conv.insert(std::make_pair("qsupmod", "")); // ???
	conv.insert(std::make_pair("rrotsupmod", "")); // ???
	conv.insert(std::make_pair("rumsupmod", "")); // ???
	conv.insert(std::make_pair("rscapsupmod", "")); // ???
	conv.insert(std::make_pair("ssupmod", "")); // ???
	conv.insert(std::make_pair("slongsupmod", "")); // ???
	conv.insert(std::make_pair("tscapsupmod", "")); // ???
	conv.insert(std::make_pair("trotsupmod", "")); // ???
	conv.insert(std::make_pair("usupmod", "ͧ")); // ???
	conv.insert(std::make_pair("wsupmod", "")); // ???
	conv.insert(std::make_pair("ysupmod", "")); // ???
	conv.insert(std::make_pair("zsupmod", "")); // ???
	conv.insert(std::make_pair("thornsupmod", "")); // ???
	//
	conv.insert(std::make_pair("arbarmod", "")); // ???
	conv.insert(std::make_pair("erangmod", "")); // ???
	conv.insert(std::make_pair("ercurlmod", "")); // ???
	conv.insert(std::make_pair("ersubmod", "")); // ???
	conv.insert(std::make_pair("ramod", "")); // ???
	conv.insert(std::make_pair("rabarmod", "")); // ???
	conv.insert(std::make_pair("urrotmod", "")); // ???
	conv.insert(std::make_pair("urlemnmod", "")); // ???
	conv.insert(std::make_pair("urmod", "")); // ???

	// MENOTA
	// Basic Latin
	conv.insert(std::make_pair("quot", "\""));
	conv.insert(std::make_pair("amp", "&"));
	conv.insert(std::make_pair("apos", "'"));
	conv.insert(std::make_pair("lt", "<"));
	conv.insert(std::make_pair("gt", ">"));
	// 
	conv.insert(std::make_pair("excl", "!"));
	conv.insert(std::make_pair("dollar", "$"));
	conv.insert(std::make_pair("percnt", "%"));
	conv.insert(std::make_pair("lpar", "("));
	conv.insert(std::make_pair("rpar", ")"));
	conv.insert(std::make_pair("ast", "*"));
	conv.insert(std::make_pair("plus", "+"));
	conv.insert(std::make_pair("comma", ","));
	conv.insert(std::make_pair("hypen", "-"));
	conv.insert(std::make_pair("period", "."));
	conv.insert(std::make_pair("sol", "/"));
	conv.insert(std::make_pair("colon", ":"));
	conv.insert(std::make_pair("semi", ";"));
	conv.insert(std::make_pair("equals", "="));
	conv.insert(std::make_pair("quest", "?"));
	conv.insert(std::make_pair("commat", "@"));
	conv.insert(std::make_pair("lsqb", "["));
	conv.insert(std::make_pair("bsol", "\\"));
	conv.insert(std::make_pair("rsqb", "]"));
	conv.insert(std::make_pair("circ", "^"));
	conv.insert(std::make_pair("lowbar", "_"));
	conv.insert(std::make_pair("grave", "`"));
	conv.insert(std::make_pair("lcub", "{"));
	conv.insert(std::make_pair("verbar", "|"));
	conv.insert(std::make_pair("rcub", "}"));
	conv.insert(std::make_pair("tld", "~"));
	// Latin-1 Supplement
	conv.insert(std::make_pair("nbsp", " ")); // I put a regular space
	conv.insert(std::make_pair("iexcl", "¡"));
	conv.insert(std::make_pair("cent", "¢"));
	conv.insert(std::make_pair("pound", "£"));
	conv.insert(std::make_pair("curren", "¤"));
	conv.insert(std::make_pair("yen", "¥"));
	conv.insert(std::make_pair("brvbar", "¦"));
	conv.insert(std::make_pair("sect", "§"));
	conv.insert(std::make_pair("uml", "¨"));
	conv.insert(std::make_pair("copy", "©"));
	conv.insert(std::make_pair("ordf", "ª"));
	conv.insert(std::make_pair("laquo", "«"));
	conv.insert(std::make_pair("not", "¬"));
	conv.insert(std::make_pair("shy", "­")); // soft hypen???
	conv.insert(std::make_pair("reg", "®"));
	conv.insert(std::make_pair("macr", "¯"));
	conv.insert(std::make_pair("deg", "°"));
	conv.insert(std::make_pair("plusmn", "±"));
	conv.insert(std::make_pair("sup2", "²"));
	conv.insert(std::make_pair("sup3", "³"));
	conv.insert(std::make_pair("acute", "´"));
	conv.insert(std::make_pair("micro", "µ"));
	conv.insert(std::make_pair("para", "¶"));
	conv.insert(std::make_pair("middot", "·"));
	conv.insert(std::make_pair("cedil", "¸"));
	conv.insert(std::make_pair("sup1", "¹"));
	conv.insert(std::make_pair("ordm", "º"));
	conv.insert(std::make_pair("raquo", "»"));
	conv.insert(std::make_pair("frac14", "¼"));
	conv.insert(std::make_pair("frac12", "½"));
	conv.insert(std::make_pair("frac34", "¾"));
	conv.insert(std::make_pair("iquest", "¿"));
	conv.insert(std::make_pair("Agrave", "À"));
	conv.insert(std::make_pair("Aacute", "Á"));
	conv.insert(std::make_pair("Acirc", "Â"));
	conv.insert(std::make_pair("Atilde", "Ã"));
	conv.insert(std::make_pair("Auml", "Ä"));
	conv.insert(std::make_pair("Aring", "Å"));
	conv.insert(std::make_pair("AElig", "Æ"));
	conv.insert(std::make_pair("Ccedil", "Ç"));
	conv.insert(std::make_pair("Egrave", "È"));
	conv.insert(std::make_pair("Eacute", "É"));
	conv.insert(std::make_pair("Ecirc", "Ê"));
	conv.insert(std::make_pair("Euml", "Ë"));
	conv.insert(std::make_pair("Igrave", "Ì"));
	conv.insert(std::make_pair("Iacute", "Í"));
	conv.insert(std::make_pair("Icirc", "Î"));
	conv.insert(std::make_pair("Iuml", "Ï"));
	conv.insert(std::make_pair("ETH", "Ð"));
	conv.insert(std::make_pair("Ntilde", "Ñ"));
	conv.insert(std::make_pair("Ograve", "Ò"));
	conv.insert(std::make_pair("Oacute", "Ó"));
	conv.insert(std::make_pair("Ocirc", "Ô"));
	conv.insert(std::make_pair("Otilde", "Õ"));
	conv.insert(std::make_pair("Ouml", "Ö"));
	conv.insert(std::make_pair("times", "×"));
	conv.insert(std::make_pair("Oslash", "Ø"));
	conv.insert(std::make_pair("Ugrave", "Ù"));
	conv.insert(std::make_pair("Uacute", "Ú"));
	conv.insert(std::make_pair("Ucirc", "Û"));
	conv.insert(std::make_pair("Uuml", "Ü"));
	conv.insert(std::make_pair("Yacute", "Ý"));
	conv.insert(std::make_pair("THORN", "Þ"));
	conv.insert(std::make_pair("szlig", "ß"));
	conv.insert(std::make_pair("agrave", "à"));
	conv.insert(std::make_pair("aacute", "á"));
	conv.insert(std::make_pair("acirc", "â"));
	conv.insert(std::make_pair("atilde", "ã"));
	conv.insert(std::make_pair("auml", "ä"));
	conv.insert(std::make_pair("aring", "å"));
	conv.insert(std::make_pair("aelig", "æ"));
	conv.insert(std::make_pair("ccedil", "ç"));
	conv.insert(std::make_pair("egrave", "è"));
	conv.insert(std::make_pair("eacute", "é"));
	conv.insert(std::make_pair("ecirc", "ê"));
	conv.insert(std::make_pair("euml", "ë"));
	conv.insert(std::make_pair("igrave", "ì"));
	conv.insert(std::make_pair("iacute", "í"));
	conv.insert(std::make_pair("icirc", "î"));
	conv.insert(std::make_pair("iuml", "ï"));
	conv.insert(std::make_pair("eth", "ð"));
	conv.insert(std::make_pair("ntilde", "ñ"));
	conv.insert(std::make_pair("ograve", "ò"));
	conv.insert(std::make_pair("oacute", "ó"));
	conv.insert(std::make_pair("ocirc", "ô"));
	conv.insert(std::make_pair("otilde", "õ"));
	conv.insert(std::make_pair("ouml", "ö"));
	conv.insert(std::make_pair("divide", "÷"));
	conv.insert(std::make_pair("oslash", "ø"));
	conv.insert(std::make_pair("ugrave", "ù"));
	conv.insert(std::make_pair("uacute", "ú"));
	conv.insert(std::make_pair("ucirc", "û"));
	conv.insert(std::make_pair("uuml", "ü"));
	conv.insert(std::make_pair("yacute", "ý"));
	conv.insert(std::make_pair("thorn", "þ"));
	conv.insert(std::make_pair("yuml", "ÿ"));
	// Latin Extended A
	conv.insert(std::make_pair("Amacr", "Ā"));
	conv.insert(std::make_pair("amacr", "ā"));
	conv.insert(std::make_pair("Abreve", "Ă"));
	conv.insert(std::make_pair("abreve", "ă"));
	conv.insert(std::make_pair("Aogon", "Ą"));
	conv.insert(std::make_pair("aogon", "ą"));
	conv.insert(std::make_pair("Cacute", "Ć"));
	conv.insert(std::make_pair("cacute", "ć"));
	conv.insert(std::make_pair("Cdot", "Ċ"));
	conv.insert(std::make_pair("cdot", "ċ"));
	conv.insert(std::make_pair("Dstrok", "Đ"));
	conv.insert(std::make_pair("dstrok", "đ"));
	conv.insert(std::make_pair("Emacr", "Ē"));
	conv.insert(std::make_pair("emacr", "ē"));
	conv.insert(std::make_pair("Ebreve", "Ĕ"));
	conv.insert(std::make_pair("ebreve", "ĕ"));
	conv.insert(std::make_pair("Edot", "Ė"));
	conv.insert(std::make_pair("edot", "ė"));
	conv.insert(std::make_pair("Eogon", "Ę"));
	conv.insert(std::make_pair("eogon", "ę"));
	conv.insert(std::make_pair("Gdot", "Ġ"));
	conv.insert(std::make_pair("gdot", "ġ"));
	conv.insert(std::make_pair("hstrok", "ħ"));
	conv.insert(std::make_pair("Imacr", "Ī"));
	conv.insert(std::make_pair("imacr", "ī"));
	conv.insert(std::make_pair("Ibreve", "Ĭ"));
	conv.insert(std::make_pair("ibreve", "ĭ"));
	conv.insert(std::make_pair("Iogon", "Į"));
	conv.insert(std::make_pair("iogon", "į"));
	conv.insert(std::make_pair("Idot", "İ"));
	conv.insert(std::make_pair("inodot", "ı"));
	conv.insert(std::make_pair("IJlig", "Ĳ"));
	conv.insert(std::make_pair("ijlig", "ĳ"));
	conv.insert(std::make_pair("Lacute", "Ĺ"));
	conv.insert(std::make_pair("lacute", "ĺ"));
	conv.insert(std::make_pair("Lstrok", "Ł"));
	conv.insert(std::make_pair("lstrok", "ł"));
	conv.insert(std::make_pair("Nacute", "Ń"));
	conv.insert(std::make_pair("nacute", "ń"));
	conv.insert(std::make_pair("ENG", "Ŋ"));
	conv.insert(std::make_pair("eng", "ŋ"));
	conv.insert(std::make_pair("Omacr", "Ō"));
	conv.insert(std::make_pair("omacr", "ō"));
	conv.insert(std::make_pair("Obreve", "Ŏ"));
	conv.insert(std::make_pair("obreve", "ŏ"));
	conv.insert(std::make_pair("Odblac", "Ő"));
	conv.insert(std::make_pair("odblac", "ő"));
	conv.insert(std::make_pair("OElig", "Œ"));
	conv.insert(std::make_pair("oelig", "œ"));
	conv.insert(std::make_pair("Racute", "Ŕ"));
	conv.insert(std::make_pair("racute", "ŕ"));
	conv.insert(std::make_pair("Sacute", "Ś"));
	conv.insert(std::make_pair("sacute", "ś"));
	conv.insert(std::make_pair("Umacr", "Ū"));
	conv.insert(std::make_pair("umacr", "ū"));
	conv.insert(std::make_pair("Ubreve", "Ŭ"));
	conv.insert(std::make_pair("ubreve", "ŭ"));
	conv.insert(std::make_pair("Uring", "Ů"));
	conv.insert(std::make_pair("uring", "ů"));
	conv.insert(std::make_pair("Udblac", "Ű"));
	conv.insert(std::make_pair("udblac", "ű"));
	conv.insert(std::make_pair("Uogon", "Ų"));
	conv.insert(std::make_pair("uogon", "ų"));
	conv.insert(std::make_pair("Wcirc", "Ŵ"));
	conv.insert(std::make_pair("wcirc", "ŵ"));
	conv.insert(std::make_pair("Ycirc", "Ŷ"));
	conv.insert(std::make_pair("ycirc", "ŷ"));
	conv.insert(std::make_pair("Yuml", "Ÿ"));
	conv.insert(std::make_pair("Zdot", "Ż"));
	conv.insert(std::make_pair("zdot", "ż"));
	conv.insert(std::make_pair("slong", "ſ"));
	// Latin Extended B
	conv.insert(std::make_pair("bstrok", "ƀ"));
	conv.insert(std::make_pair("hwair", "ƕ"));
	conv.insert(std::make_pair("khook", "ƙ"));
	conv.insert(std::make_pair("lbar", "ƚ"));
	conv.insert(std::make_pair("nlrleg", "ƞ"));
	conv.insert(std::make_pair("YR", "Ʀ"));
	conv.insert(std::make_pair("Zstrok", "Ƶ"));
	conv.insert(std::make_pair("zstrok", "ƶ"));
	conv.insert(std::make_pair("EZH", "Ʒ"));
	conv.insert(std::make_pair("wynn", "ƿ"));
	conv.insert(std::make_pair("Ocar", "Ǒ"));
	conv.insert(std::make_pair("ocar", "ǒ"));
	conv.insert(std::make_pair("Ucar", "Ǔ"));
	conv.insert(std::make_pair("ucar", "ǔ"));
	conv.insert(std::make_pair("Uumlmacr", "Ǖ"));
	conv.insert(std::make_pair("uumlmacr", "ǖ"));
	conv.insert(std::make_pair("AEligmacr", "Ǣ"));
	conv.insert(std::make_pair("aeligmacr", "ǣ"));
	conv.insert(std::make_pair("Gstrok", "Ǥ"));
	conv.insert(std::make_pair("gstrok", "ǥ"));
	conv.insert(std::make_pair("Oogon", "Ǫ"));
	conv.insert(std::make_pair("oogon", "Ǫ"));
	conv.insert(std::make_pair("Oogonmacr", "Ǭ"));
	conv.insert(std::make_pair("oogonmacr", "ǭ"));
	conv.insert(std::make_pair("Gacute", "Ǵ"));
	conv.insert(std::make_pair("gacute", "ǵ"));
	conv.insert(std::make_pair("HWAIR", "Ƕ"));
	conv.insert(std::make_pair("WYNN", "Ƿ"));
	conv.insert(std::make_pair("AEligacute", "Ǽ"));
	conv.insert(std::make_pair("aeligacute", "ǽ"));
	conv.insert(std::make_pair("Oslashacute", "Ǿ"));
	conv.insert(std::make_pair("oslashacute", "ǿ"));
	conv.insert(std::make_pair("YOGH", "Ȝ"));
	conv.insert(std::make_pair("yogh", "ȝ"));
	conv.insert(std::make_pair("Adot", "Ȧ"));
	conv.insert(std::make_pair("adot", "ȧ"));
	conv.insert(std::make_pair("Oumlmacr", "Ȫ"));
	conv.insert(std::make_pair("oumlmacr", "ȫ"));
	conv.insert(std::make_pair("Odot", "Ȯ"));
	conv.insert(std::make_pair("odot", "ȯ"));
	conv.insert(std::make_pair("Ymacr", "Ȳ"));
	conv.insert(std::make_pair("ymacr", "ȳ"));
	conv.insert(std::make_pair("jnodot", "ȷ"));
	conv.insert(std::make_pair("Jbar", "Ɉ"));
	conv.insert(std::make_pair("jbar", "ɉ"));
	// IPA Extensions
	conv.insert(std::make_pair("oopen", "ɔ"));
	conv.insert(std::make_pair("dtail", "ɖ"));
	conv.insert(std::make_pair("schwa", "ə"));
	conv.insert(std::make_pair("jnodotstrok", "ɟ"));
	conv.insert(std::make_pair("gopen", "ɡ"));
	conv.insert(std::make_pair("gscap", "ɢ"));
	conv.insert(std::make_pair("hhook", "ɦ"));
	conv.insert(std::make_pair("istrok", "ɨ"));
	conv.insert(std::make_pair("iscap", "ɪ"));
	conv.insert(std::make_pair("nlfhook", "ɲ"));
	conv.insert(std::make_pair("nscap", "ɴ"));
	conv.insert(std::make_pair("oeligscap", "ɶ"));
	conv.insert(std::make_pair("rdes", "ɼ"));
	conv.insert(std::make_pair("rscap", "ʀ"));
	conv.insert(std::make_pair("ubar", "ʉ"));
	conv.insert(std::make_pair("yscap", "ʏ"));
	conv.insert(std::make_pair("ezh", "ʒ"));
	conv.insert(std::make_pair("bscap", "ʙ"));
	conv.insert(std::make_pair("hscap", "ʜ"));
	conv.insert(std::make_pair("lscap", "ʟ"));
	// Spacing Modifying Letters
	conv.insert(std::make_pair("apomod", "ʼ"));
	conv.insert(std::make_pair("verbarup", "ˈ"));
	conv.insert(std::make_pair("breve", "˘"));
	conv.insert(std::make_pair("dot", "˙"));
	conv.insert(std::make_pair("ring", "˚"));
	conv.insert(std::make_pair("ogon", "˛"));
	conv.insert(std::make_pair("tilde", "˜"));
	conv.insert(std::make_pair("dblac", "˝"));
	conv.insert(std::make_pair("xmod", "ˣ"));
	// Combining Diacritical Marks
	conv.insert(std::make_pair("combgrave", "̀"));
	conv.insert(std::make_pair("combacute", "́"));
	conv.insert(std::make_pair("combcirc", "̂"));
	conv.insert(std::make_pair("combtilde", "̃"));
	conv.insert(std::make_pair("combmacr", "̄"));
	conv.insert(std::make_pair("bar", "̅"));
	conv.insert(std::make_pair("combbreve", "̆"));
	conv.insert(std::make_pair("combdot", "̇"));
	conv.insert(std::make_pair("combuml", "̈"));
	conv.insert(std::make_pair("combhook", "̉"));
	conv.insert(std::make_pair("combring", "̊"));
	conv.insert(std::make_pair("combdblac", "̋"));
	conv.insert(std::make_pair("combsgvertl", "̍"));
	conv.insert(std::make_pair("combdbvertl", "̎"));
	conv.insert(std::make_pair("combcomma", "̕"));
	conv.insert(std::make_pair("combdotbl", "̣"));
	conv.insert(std::make_pair("combced", "̧"));
	conv.insert(std::make_pair("combogon", "̨"));
	conv.insert(std::make_pair("barbl", "̲"));
	conv.insert(std::make_pair("dblbarbl", "̲"));
	conv.insert(std::make_pair("baracr", "̶"));
	conv.insert(std::make_pair("combtildevert", "̾"));
	conv.insert(std::make_pair("dblovl", "̿"));
	conv.insert(std::make_pair("combastbl", "͙"));
	conv.insert(std::make_pair("er", "͛"));
	conv.insert(std::make_pair("combdblbrevebl", "͜"));
	conv.insert(std::make_pair("asup", "ͣ"));
	conv.insert(std::make_pair("esup", "ͤ"));
	conv.insert(std::make_pair("isup", "ͥ"));
	conv.insert(std::make_pair("osup", "ͦ"));
	conv.insert(std::make_pair("usup", "ͧ"));
	conv.insert(std::make_pair("csup", "ͨ"));
	conv.insert(std::make_pair("dsup", "ͩ"));
	conv.insert(std::make_pair("hsup", "ͪ"));
	conv.insert(std::make_pair("msup", "ͫ"));
	conv.insert(std::make_pair("rsup", "ͬ"));
	conv.insert(std::make_pair("tsup", "ͭ"));
	conv.insert(std::make_pair("vsup", "ͮ"));
	conv.insert(std::make_pair("xsup", "ͯ"));
	// Greek and Coptic
	conv.insert(std::make_pair("alpha", "α"));
	conv.insert(std::make_pair("beta", "β"));
	conv.insert(std::make_pair("gamma", "γ"));
	conv.insert(std::make_pair("delta", "δ"));
	conv.insert(std::make_pair("epsilon", "ε"));
	conv.insert(std::make_pair("zeta", "ζ"));
	conv.insert(std::make_pair("eta", "η"));
	conv.insert(std::make_pair("Theta", "Θ"));
	conv.insert(std::make_pair("theta", "θ"));
	conv.insert(std::make_pair("iota", "ι"));
	conv.insert(std::make_pair("kappa", "κ"));
	conv.insert(std::make_pair("lambda", "λ"));
	conv.insert(std::make_pair("mu", "μ"));
	conv.insert(std::make_pair("nu", "ν"));
	conv.insert(std::make_pair("xi", "ξ"));
	conv.insert(std::make_pair("omicron", "ο"));
	conv.insert(std::make_pair("pi", "π"));
	conv.insert(std::make_pair("rho", "ρ"));
	conv.insert(std::make_pair("sigmaf", "ς"));
	conv.insert(std::make_pair("sigma", "σ"));
	conv.insert(std::make_pair("tau", "τ"));
	conv.insert(std::make_pair("upsilon", "υ"));
	conv.insert(std::make_pair("phi", "φ"));
	conv.insert(std::make_pair("chi", "χ"));
	conv.insert(std::make_pair("psi", "ψ"));
	conv.insert(std::make_pair("omega", "ω"));
	// Georgian
	conv.insert(std::make_pair("tridotright", "჻"));
	// Runic
	conv.insert(std::make_pair("fMedrun", "ᚠ"));
	conv.insert(std::make_pair("mMedrun", "ᛘ"));
	// Phonetic Extensions
	conv.insert(std::make_pair("ascap", "ᴀ"));
	conv.insert(std::make_pair("aeligscap", "ᴁ"));
	conv.insert(std::make_pair("cscap", "ᴄ"));
	conv.insert(std::make_pair("dscap", "ᴅ"));
	conv.insert(std::make_pair("ethscap", "ᴆ"));
	conv.insert(std::make_pair("escap", "ᴇ"));
	conv.insert(std::make_pair("jscap", "ᴊ"));
	conv.insert(std::make_pair("kscap", "ᴋ"));
	conv.insert(std::make_pair("mscap", "ᴍ"));
	conv.insert(std::make_pair("oscap", "ᴏ"));
	conv.insert(std::make_pair("pscap", "ᴘ"));
	conv.insert(std::make_pair("tscap", "ᴛ"));
	conv.insert(std::make_pair("uscap", "ᴜ"));
	conv.insert(std::make_pair("vscap", "ᴠ"));
	conv.insert(std::make_pair("wscap", "ᴡ"));
	conv.insert(std::make_pair("zscap", "ᴢ"));
	conv.insert(std::make_pair("Imod", "ᴵ"));
	conv.insert(std::make_pair("gins", "ᵹ"));
	// Combining Diacritical Marks Supplement
	conv.insert(std::make_pair("combcircdbl", "᷍"));
	conv.insert(std::make_pair("combcurl", "᷎"));
	conv.insert(std::make_pair("ersub", "᷏"));
	conv.insert(std::make_pair("combisbelow", "᷐"));
	conv.insert(std::make_pair("ur", "᷑"));
	conv.insert(std::make_pair("us", "᷒"));
	conv.insert(std::make_pair("ra", "ᷓ"));
	conv.insert(std::make_pair("aeligsup", "ᷔ"));
	conv.insert(std::make_pair("aoligsup", "ᷕ"));
	conv.insert(std::make_pair("avligsup", "ᷖ"));
	conv.insert(std::make_pair("ccedilsup", "ᷗ"));
	conv.insert(std::make_pair("drotsup", "ᷘ"));
	conv.insert(std::make_pair("ethsup", "ᷙ"));
	conv.insert(std::make_pair("gsup", "ᷚ"));
	conv.insert(std::make_pair("gscapsup", "ᷛ"));
	conv.insert(std::make_pair("ksup", "ᷜ"));
	conv.insert(std::make_pair("lsup", "ᷝ"));
	conv.insert(std::make_pair("lscapsup", "ᷞ"));
	conv.insert(std::make_pair("mscapsup", "ᷟ"));
	conv.insert(std::make_pair("nsup", "ᷠ"));
	conv.insert(std::make_pair("nscapsup", "ᷡ"));
	conv.insert(std::make_pair("rrotsup", "ᷣ"));
	conv.insert(std::make_pair("ssup", "ᷤ"));
	conv.insert(std::make_pair("slongsup", "ᷥ"));
	conv.insert(std::make_pair("zsup", "ᷦ"));
	// Latin Extended Additional
	conv.insert(std::make_pair("Bdot", "Ḃ"));
	conv.insert(std::make_pair("bdot", "ḃ"));
	conv.insert(std::make_pair("Bdotbl", "Ḅ"));
	conv.insert(std::make_pair("bdotbl", "ḅ"));
	conv.insert(std::make_pair("Ddot", "Ḋ"));
	conv.insert(std::make_pair("ddot", "ḋ"));
	conv.insert(std::make_pair("Ddotbl", "Ḍ"));
	conv.insert(std::make_pair("ddotbl", "ḍ"));
	conv.insert(std::make_pair("Emacracute", "Ḗ"));
	conv.insert(std::make_pair("emacracute", "ḗ"));
	conv.insert(std::make_pair("Fdot", "Ḟ"));
	conv.insert(std::make_pair("fdot", "ḟ"));
	conv.insert(std::make_pair("Hdot", "Ḣ"));
	conv.insert(std::make_pair("hdot", "ḣ"));
	conv.insert(std::make_pair("Hdotbl", "Ḥ"));
	conv.insert(std::make_pair("hdotbl", "ḥ"));
	conv.insert(std::make_pair("Kacute", "Ḱ"));
	conv.insert(std::make_pair("kacute", "ḱ"));
	conv.insert(std::make_pair("Kdotbl", "Ḳ"));
	conv.insert(std::make_pair("kdotbl", "ḳ"));
	conv.insert(std::make_pair("Ldotbl", "Ḷ"));
	conv.insert(std::make_pair("ldotbl", "ḷ"));
	conv.insert(std::make_pair("Macute", "Ḿ"));
	conv.insert(std::make_pair("macute", "ḿ"));
	conv.insert(std::make_pair("Mdot", "Ṁ"));
	conv.insert(std::make_pair("mdot", "ṁ"));
	conv.insert(std::make_pair("Mdotbl", "Ṃ"));
	conv.insert(std::make_pair("mdotbl", "ṃ"));
	conv.insert(std::make_pair("Ndot", "Ṅ"));
	conv.insert(std::make_pair("ndot", "ṅ"));
	conv.insert(std::make_pair("Ndotbl", "Ṇ"));
	conv.insert(std::make_pair("ndotbl", "ṇ"));
	conv.insert(std::make_pair("Omacracute", "Ṓ"));
	conv.insert(std::make_pair("omacracute", "ṓ"));
	conv.insert(std::make_pair("Pacute", "Ṕ"));
	conv.insert(std::make_pair("pacute", "ṕ"));
	conv.insert(std::make_pair("Pdot", "Ṗ"));
	conv.insert(std::make_pair("pdot", "ṗ"));
	conv.insert(std::make_pair("Rdot", "Ṙ"));
	conv.insert(std::make_pair("rdot", "ṙ"));
	conv.insert(std::make_pair("Rdotbl", "Ṛ"));
	conv.insert(std::make_pair("rdotbl", "ṛ"));
	conv.insert(std::make_pair("Sdot", "Ṡ"));
	conv.insert(std::make_pair("sdot", "ṡ"));
	conv.insert(std::make_pair("Sdotbl", "Ṣ"));
	conv.insert(std::make_pair("sdotbl", "ṣ"));
	conv.insert(std::make_pair("Tdot", "Ṫ"));
	conv.insert(std::make_pair("tdot", "ṫ"));
	conv.insert(std::make_pair("Tdotbl", "Ṭ"));
	conv.insert(std::make_pair("tdotbl", "ṭ"));
	conv.insert(std::make_pair("Vdotbl", "Ṿ"));
	conv.insert(std::make_pair("vdotbl", "ṿ"));
	conv.insert(std::make_pair("Wgrave", "Ẁ"));
	conv.insert(std::make_pair("wgrave", "ẁ"));
	conv.insert(std::make_pair("Wacute", "Ẃ"));
	conv.insert(std::make_pair("wacute", "ẃ"));
	conv.insert(std::make_pair("Wuml", "Ẅ"));
	conv.insert(std::make_pair("wuml", "ẅ"));
	conv.insert(std::make_pair("Wdot", "Ẇ"));
	conv.insert(std::make_pair("wdot", "ẇ"));
	conv.insert(std::make_pair("Wdotbl", "Ẉ"));
	conv.insert(std::make_pair("wdotbl", "ẉ"));
	conv.insert(std::make_pair("Ydot", "Ẏ"));
	conv.insert(std::make_pair("ydot", "ẏ"));
	conv.insert(std::make_pair("Zdotbl", "Ẓ"));
	conv.insert(std::make_pair("zdotbl", "ẓ"));
	conv.insert(std::make_pair("wring", "ẘ"));
	conv.insert(std::make_pair("yring", "ẙ"));
	conv.insert(std::make_pair("slongbarslash", "ẜ"));
	conv.insert(std::make_pair("slongbar", "ẝ"));
	conv.insert(std::make_pair("SZlig", "ẞ"));
	conv.insert(std::make_pair("dscript", "ẟ"));
	conv.insert(std::make_pair("Adotbl", "Ạ"));
	conv.insert(std::make_pair("adotbl", "ạ"));
	conv.insert(std::make_pair("Ahook", "Ả"));
	conv.insert(std::make_pair("ahook", "ả"));
	conv.insert(std::make_pair("Abreveacute", "Ắ"));
	conv.insert(std::make_pair("abreveacute", "ắ"));
	conv.insert(std::make_pair("Edotbl", "Ẹ"));
	conv.insert(std::make_pair("edotbl", "ẹ"));
	conv.insert(std::make_pair("Ihook", "Ỉ"));
	conv.insert(std::make_pair("ihook", "ỉ"));
	conv.insert(std::make_pair("Idotbl", "Ị"));
	conv.insert(std::make_pair("idotbl", "ị"));
	conv.insert(std::make_pair("Odotbl", "Ọ"));
	conv.insert(std::make_pair("odotbl", "ọ"));
	conv.insert(std::make_pair("Ohook", "Ỏ"));
	conv.insert(std::make_pair("ohook", "ỏ"));
	conv.insert(std::make_pair("Udotbl", "Ụ"));
	conv.insert(std::make_pair("udotbl", "ụ"));
	conv.insert(std::make_pair("Uhook", "Ủ"));
	conv.insert(std::make_pair("uhook", "ủ"));
	conv.insert(std::make_pair("Ygrave", "Ỳ"));
	conv.insert(std::make_pair("ygrave", "ỳ"));
	conv.insert(std::make_pair("Ydotbl", "Ỵ"));
	conv.insert(std::make_pair("ydotbl", "ỵ"));
	conv.insert(std::make_pair("Yhook", "Ỷ"));
	conv.insert(std::make_pair("yhook", "ỷ"));
	conv.insert(std::make_pair("LLwelsh", "Ỻ"));
	conv.insert(std::make_pair("llwelsh", "ỻ"));
	conv.insert(std::make_pair("Vwelsh", "Ỽ"));
	conv.insert(std::make_pair("vwelsh", "ỽ"));
	conv.insert(std::make_pair("Yloop", "Ỿ"));
	conv.insert(std::make_pair("yloop", "ỿ"));
	// General Punctuation
	conv.insert(std::make_pair("enqd", " ")); // I put a regular space
	conv.insert(std::make_pair("emqd", " ")); // I put a regular space
	conv.insert(std::make_pair("ensp", " ")); // I put a regular space
	conv.insert(std::make_pair("emsp", " ")); // I put a regular space
	conv.insert(std::make_pair("emsp13", " ")); // I put a regular space
	conv.insert(std::make_pair("emsp14", " ")); // I put a regular space
	conv.insert(std::make_pair("emsp16", " ")); // I put a regular space
	conv.insert(std::make_pair("numsp", " ")); // I put a regular space
	conv.insert(std::make_pair("puncsp", " ")); // I put a regular space
	conv.insert(std::make_pair("thinsp", " ")); // I put a regular space
	conv.insert(std::make_pair("hairsp", " ")); // I put a regular space
	conv.insert(std::make_pair("zerosp", " ")); // I put a regular space
	conv.insert(std::make_pair("dash", "‐"));
	conv.insert(std::make_pair("nbhy", "-")); // I put a regular hyphen
	conv.insert(std::make_pair("numdash", "‒"));
	conv.insert(std::make_pair("ndash", "–"));
	conv.insert(std::make_pair("mdash", "—"));
	conv.insert(std::make_pair("horbar", "―"));
	conv.insert(std::make_pair("Verbar", "‖"));
	conv.insert(std::make_pair("lsquo", "‘"));
	conv.insert(std::make_pair("rsquo", "’"));
	conv.insert(std::make_pair("lsquolow", "‚"));
	conv.insert(std::make_pair("rsquorev", "‛"));
	conv.insert(std::make_pair("ldquo", "“"));
	conv.insert(std::make_pair("rdquo", "”"));
	conv.insert(std::make_pair("ldquolow", "„"));
	conv.insert(std::make_pair("rdquorev", "‟"));
	conv.insert(std::make_pair("dagger", "†"));
	conv.insert(std::make_pair("Dagger", "‡"));
	conv.insert(std::make_pair("bull", "•"));
	conv.insert(std::make_pair("tribull", "‣"));
	conv.insert(std::make_pair("sgldr", "․"));
	conv.insert(std::make_pair("dblldr", "‥"));
	conv.insert(std::make_pair("hellip", "…"));
	conv.insert(std::make_pair("hyphpoint", "‧"));
	conv.insert(std::make_pair("nnbsp", " ")); // I put a regular space
	conv.insert(std::make_pair("permil", "‰"));
	conv.insert(std::make_pair("prime", "′"));
	conv.insert(std::make_pair("Prime", "″"));
	conv.insert(std::make_pair("lsaquo", "‹"));
	conv.insert(std::make_pair("rsaquo", "›"));
	conv.insert(std::make_pair("refmark", "※"));
	conv.insert(std::make_pair("triast", "⁂"));
	conv.insert(std::make_pair("fracsol", "⁄"));
	conv.insert(std::make_pair("lsqbqu", "⁅"));
	conv.insert(std::make_pair("rsqbqu", "⁆"));
	conv.insert(std::make_pair("et", "⁊"));
	conv.insert(std::make_pair("revpara", "⁋"));
	conv.insert(std::make_pair("lozengedot", "⁘"));
	conv.insert(std::make_pair("dotcross", "⁜"));
	// Superscripts and subscripts
	conv.insert(std::make_pair("sup0", "⁰"));
	conv.insert(std::make_pair("sup4", "⁴"));
	conv.insert(std::make_pair("sup5", "⁵"));
	conv.insert(std::make_pair("sup6", "⁶"));
	conv.insert(std::make_pair("sup7", "⁷"));
	conv.insert(std::make_pair("sup8", "⁸"));
	conv.insert(std::make_pair("sup9", "⁹"));
	conv.insert(std::make_pair("sub0", "₀"));
	conv.insert(std::make_pair("sub1", "₁"));
	conv.insert(std::make_pair("sub2", "₂"));
	conv.insert(std::make_pair("sub3", "₃"));
	conv.insert(std::make_pair("sub4", "₄"));
	conv.insert(std::make_pair("sub5", "₅"));
	conv.insert(std::make_pair("sub6", "₆"));
	conv.insert(std::make_pair("sub7", "₇"));
	conv.insert(std::make_pair("sub8", "₈"));
	conv.insert(std::make_pair("sub9", "₉"));
	// Currency Symbols
	conv.insert(std::make_pair("pennygerm", "₰"));
	// Letterlike Symbols
	conv.insert(std::make_pair("scruple", "℈"));
	conv.insert(std::make_pair("lbbar", "℔"));
	conv.insert(std::make_pair("Rtailstrok", "℞"));
	conv.insert(std::make_pair("Rslstrok", "℟"));
	conv.insert(std::make_pair("Vslstrok", "℣"));
	conv.insert(std::make_pair("ounce", "℥"));
	conv.insert(std::make_pair("Fturn", "Ⅎ"));
	conv.insert(std::make_pair("fturn", "ⅎ"));
	// Number forms
	conv.insert(std::make_pair("romnumCDlig", "ↀ"));
	conv.insert(std::make_pair("romnumDDlig", "ↁ"));
	conv.insert(std::make_pair("romnumDDdbllig", "ↂ"));
	conv.insert(std::make_pair("CONbase", "Ↄ"));
	conv.insert(std::make_pair("conba", "ↄ"));
	// Arrows
	conv.insert(std::make_pair("arrsgllw", "←"));
	conv.insert(std::make_pair("arrsglupw", "↑"));
	conv.insert(std::make_pair("arrsglrw", "→"));
	conv.insert(std::make_pair("arrsgldw", "↓"));
	// Mathematical Operators
	conv.insert(std::make_pair("minus", "−"));
	conv.insert(std::make_pair("infin", "∞"));
	conv.insert(std::make_pair("logand", "∧"));
	conv.insert(std::make_pair("tridotupw", "∴"));
	conv.insert(std::make_pair("tridotdw", "∵"));
	conv.insert(std::make_pair("quaddot", "∷"));
	conv.insert(std::make_pair("est", "∻"));
	conv.insert(std::make_pair("esse", "≈"));
	conv.insert(std::make_pair("notequals", "≠"));
	conv.insert(std::make_pair("dipledot", "⋗"));
	// Miscellaneous Technical
	conv.insert(std::make_pair("metrshort", "⏑"));
	conv.insert(std::make_pair("metrshortlong", "⏒"));
	conv.insert(std::make_pair("metrlongshort", "⏓"));
	conv.insert(std::make_pair("metrdblshortlong", "⏔"));
	// Geometric Shapes
	conv.insert(std::make_pair("squareblsm", "▪"));
	conv.insert(std::make_pair("squarewhsm", "▫"));
	conv.insert(std::make_pair("trirightwh", "▹"));
	conv.insert(std::make_pair("trileftwh", "◃"));
	conv.insert(std::make_pair("circledot", "◌"));
	// Dingbats
	conv.insert(std::make_pair("cross", "✝"));
	conv.insert(std::make_pair("hedera", "❦"));
	conv.insert(std::make_pair("hederarot", "❧"));
	// Miscellaneous Mathematical Symbols A
	conv.insert(std::make_pair("lwhsqb", "⟦"));
	conv.insert(std::make_pair("rwhsqb", "⟧"));
	conv.insert(std::make_pair("langb", "⟨"));
	conv.insert(std::make_pair("rangb", "⟩"));
	// Supplemental Mathematical Operators
	conv.insert(std::make_pair("dblsol", "⫽"));
	// Latin Extended C
	conv.insert(std::make_pair("Hhalf", "Ⱶ"));
	conv.insert(std::make_pair("hhalf", "ⱶ"));
	// Supplemental Punctuation
	conv.insert(std::make_pair("luhsqbNT", "⸀"));
	conv.insert(std::make_pair("luslst", "⸌"));
	conv.insert(std::make_pair("ruslst", "⸍"));
	conv.insert(std::make_pair("dbloblhyph", "⸗"));
	conv.insert(std::make_pair("rlslst", "⸜"));
	conv.insert(std::make_pair("llslst", "⸝"));
	conv.insert(std::make_pair("verbarql", "⸠"));
	conv.insert(std::make_pair("verbarqr", "⸡"));
	conv.insert(std::make_pair("luhsqb", "⸢"));
	conv.insert(std::make_pair("ruhsqb", "⸣"));
	conv.insert(std::make_pair("llhsqb", "⸤"));
	conv.insert(std::make_pair("rlhsqb", "⸥"));
	conv.insert(std::make_pair("lUbrack", "⸦"));
	conv.insert(std::make_pair("rUbrack", "⸧"));
	conv.insert(std::make_pair("ldblpar", "⸨"));
	conv.insert(std::make_pair("rdblpar", "⸩"));
	conv.insert(std::make_pair("tridotsdownw", "⸪"));
	conv.insert(std::make_pair("tridotsupw", "⸫"));
	conv.insert(std::make_pair("quaddots", "⸬"));
	conv.insert(std::make_pair("fivedots", "⸭"));
	conv.insert(std::make_pair("punctpercont", "⸮"));
	// Latin Extended D
	conv.insert(std::make_pair("fscap", "ꜰ"));
	conv.insert(std::make_pair("sscap", "ꜱ"));
	conv.insert(std::make_pair("AAlig", "Ꜳ"));
	conv.insert(std::make_pair("aalig", "ꜳ"));
	conv.insert(std::make_pair("AOlig", "Ꜵ"));
	conv.insert(std::make_pair("aolig", "ꜵ"));
	conv.insert(std::make_pair("AUlig", "Ꜷ"));
	conv.insert(std::make_pair("aulig", "ꜷ"));
	conv.insert(std::make_pair("AVlig", "Ꜹ"));
	conv.insert(std::make_pair("avlig", "ꜹ"));
	conv.insert(std::make_pair("AVligslash", "Ꜻ"));
	conv.insert(std::make_pair("avligslash", "ꜻ"));
	conv.insert(std::make_pair("AYlig", "Ꜽ"));
	conv.insert(std::make_pair("aylig", "ꜽ"));
	conv.insert(std::make_pair("CONdot", "Ꜿ"));
	conv.insert(std::make_pair("condot", "ꜿ"));
	conv.insert(std::make_pair("Kbar", "Ꝁ"));
	conv.insert(std::make_pair("kbar", "ꝁ"));
	conv.insert(std::make_pair("Kstrleg", "Ꝃ"));
	conv.insert(std::make_pair("kstrleg", "ꝃ"));
	conv.insert(std::make_pair("Kstrascleg", "Ꝅ"));
	conv.insert(std::make_pair("kstrascleg", "ꝅ"));
	conv.insert(std::make_pair("Lbrk", "Ꝇ"));
	conv.insert(std::make_pair("lbrk", "ꝇ"));
	conv.insert(std::make_pair("Lhighstrok", "Ꝉ"));
	conv.insert(std::make_pair("lhighstr", "ꝉ"));
	conv.insert(std::make_pair("OBIIT", "Ꝋ"));
	conv.insert(std::make_pair("obiit", "ꝋ"));
	conv.insert(std::make_pair("Oloop", "Ꝍ"));
	conv.insert(std::make_pair("oloop", "ꝍ"));
	conv.insert(std::make_pair("OOlig", "Ꝏ"));
	conv.insert(std::make_pair("oolig", "ꝏ"));
	conv.insert(std::make_pair("Pbardes", "Ꝑ"));
	conv.insert(std::make_pair("pbardes", "ꝑ"));
	conv.insert(std::make_pair("Pflour", "Ꝓ"));
	conv.insert(std::make_pair("pflour", "ꝓ"));
	conv.insert(std::make_pair("Psquirrel", "Ꝕ"));
	conv.insert(std::make_pair("psquirrel", "ꝕ"));
	conv.insert(std::make_pair("Qbardes", "Ꝗ"));
	conv.insert(std::make_pair("qbardes", "ꝗ"));
	conv.insert(std::make_pair("Qslstrok", "Ꝙ"));
	conv.insert(std::make_pair("qslstrok", "ꝙ"));
	conv.insert(std::make_pair("Rrot", "Ꝛ"));
	conv.insert(std::make_pair("rrot", "ꝛ"));
	conv.insert(std::make_pair("RUM", "ꝛ"));
	conv.insert(std::make_pair("rum", "ꝝ"));
	conv.insert(std::make_pair("Vdiagstrok", "Ꝟ"));
	conv.insert(std::make_pair("vdiagstrok", "ꝟ"));
	conv.insert(std::make_pair("YYlig", "Ꝡ"));
	conv.insert(std::make_pair("yylig", "ꝡ"));
	conv.insert(std::make_pair("Vvisigot", "Ꝣ"));
	conv.insert(std::make_pair("vvisigot", "ꝣ"));
	conv.insert(std::make_pair("THORNbar", "Ꝥ"));
	conv.insert(std::make_pair("thornbar", "ꝥ"));
	conv.insert(std::make_pair("THORNbardes", "Ꝧ"));
	conv.insert(std::make_pair("thornbardes", "ꝧ"));
	conv.insert(std::make_pair("Vins", "Ꝩ"));
	conv.insert(std::make_pair("vins", "ꝩ"));
	conv.insert(std::make_pair("ETfin", "Ꝫ"));
	conv.insert(std::make_pair("etfin", "ꝫ"));
	conv.insert(std::make_pair("IS", "Ꝭ"));
	conv.insert(std::make_pair("is", "ꝭ"));
	conv.insert(std::make_pair("CONdes", "Ꝯ"));
	conv.insert(std::make_pair("condes", "ꝯ"));
	conv.insert(std::make_pair("usmod", "ꝰ"));
	conv.insert(std::make_pair("dtailstrok", "ꝱ"));
	conv.insert(std::make_pair("ltailstrok", "ꝲ"));
	conv.insert(std::make_pair("mtailstrok", "ꝳ"));
	conv.insert(std::make_pair("ntailstrok", "ꝴ"));
	conv.insert(std::make_pair("rtailstrok", "ꝵ"));
	conv.insert(std::make_pair("rscaptailstrok", "ꝶ"));
	conv.insert(std::make_pair("ttailstrok", "ꝷ"));
	conv.insert(std::make_pair("sstrok", "ꝸ"));
	conv.insert(std::make_pair("Drot", "Ꝺ"));
	conv.insert(std::make_pair("drot", "ꝺ"));
	conv.insert(std::make_pair("Fins", "Ꝼ"));
	conv.insert(std::make_pair("fins", "ꝼ"));
	conv.insert(std::make_pair("Gins", "Ᵹ"));
	conv.insert(std::make_pair("Ginsturn", "Ꝿ"));
	conv.insert(std::make_pair("ginsturn", "ꝿ"));
	conv.insert(std::make_pair("Lturn", "Ꞁ"));
	conv.insert(std::make_pair("lturn", "ꞁ"));
	conv.insert(std::make_pair("Rins", "Ꞃ"));
	conv.insert(std::make_pair("rins", "ꞃ"));
	conv.insert(std::make_pair("Sins", "Ꞅ"));
	conv.insert(std::make_pair("sins", "ꞅ"));
	conv.insert(std::make_pair("Trot", "Ꞇ"));
	conv.insert(std::make_pair("trot", "ꞇ"));
	conv.insert(std::make_pair("Frev", "ꟻ"));
	conv.insert(std::make_pair("Prev", "ꟼ"));
	conv.insert(std::make_pair("Minv", "ꟽ"));
	conv.insert(std::make_pair("Ilong", "ꟾ"));
	conv.insert(std::make_pair("M5leg", "ꟿ"));
	// Alphabetic Presentation Forms
	conv.insert(std::make_pair("fflig", "ﬀ"));
	conv.insert(std::make_pair("filig", "ﬁ"));
	conv.insert(std::make_pair("fllig", "ﬂ"));
	conv.insert(std::make_pair("ffilig", "ﬃ"));
	conv.insert(std::make_pair("ffllig", "ﬄ"));
	conv.insert(std::make_pair("slongtlig", "ﬅ"));
	conv.insert(std::make_pair("stlig", "ﬆ"));
	// Ancient Symbols
	// all following symbols are in the private use section of unicode…
	conv.insert(std::make_pair("punctelev", "")); // PUNCTUS ELEVATUS
	conv.insert(std::make_pair("etslash", "")); // LATIN ABBREVIATION SIGN SMALL ET WITH STROKE
	conv.insert(std::make_pair("virgmin", "")); // SHORT VIRGULA

	crn::xml::Document doc;
	doc.PushBackComment("oriflamms entity table");
	crn::xml::Element root(doc.PushBackElement("orient"));
	for (std::map<crn::StringUTF8, crn::StringUTF8>::const_iterator it = conv.begin(); it != conv.end(); ++it)
	{
		crn::xml::Element el(root.PushBackElement("ent"));
		el.SetAttribute("name", it->first);
		el.PushBackText(it->second, true);
	}
	doc.Save("orient.xml");
	return 0;
}

