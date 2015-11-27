/* Copyright 2013-2015 INSA-Lyon & IRHT
 * 
 * file: OriFeatures.cpp
 * \author Yann LEYDIER
 */

#include <OriFeatures.h>
#include <CRNUtils/CRNXml.h>
#include <CRNi18n.h>
#include <iostream>

using namespace ori;

static inline int add_elem(char c)
{
	switch (c)
	{
		case ' ':
			return 0;
		case ',':
			return 1;
		case '\'':
			return 1;
		case 'l':
			return 2;
		case '.':
			return 1;
		case '(':
			return 1;
		case ')':
			return 1;
	}
	throw crn::ExceptionDomain();
}
static inline int suppr_elem(char c)
{
	switch (c)
	{
		case ' ':
			return 3;
		case ',':
			return 1;
		case '\'':
			return 1;
		case 'l':
			return 2;
		case '.':
			return 3;
		case '(':
			return 1;
		case ')':
			return 1;
	}
	throw crn::ExceptionDomain();
}
static inline int change_elem(char c, char n)
{
	if (c == n)
		return 0;
	switch (c)
	{
		case ' ':
			if (n == 'l')
				return 3;
			else
				return 2;
		case ',':
			if (n == '\'')
				return 2;
			else
				return 1;
		case '\'':
			if (n == ',')
				return 2;
			else
				return 1;
		case 'l':
			if (n == '.')
				return 2;
			else
				return 1;
		case '.':
			if (n == 'l')
				return 3;
			else
				return 2;
		case '(':
			return 1;
		case ')':
			return 1;
	}
	throw crn::ExceptionDomain();
}

/*! Aligns two signature strings
 * \param[in]	isig	image signature string
 * \param[in]	tsig	text signature string
 * \return	a list of bounding boxes and the image signature of their content
 */
std::vector<std::pair<crn::Rect, crn::StringUTF8>> ori::Align(const std::vector<ImageSignature> &isig, const std::vector<TextSignature> &tsig)
{
	// edit distance
	size_t s1 = isig.size(), s2 = tsig.size();
	if (!s1 || !s2)
		return std::vector<std::pair<crn::Rect, crn::StringUTF8>>{};
	std::vector<std::vector<int> > d(s1 + 1, std::vector<int>(s2 + 1, 0));
	for (size_t tmp = 1; tmp <= s1; ++tmp) d[tmp][0] = int(tmp);
	for (size_t tmp = 1; tmp <= s2; ++tmp) d[0][tmp] = int(tmp);
	for (size_t i = 1; i <= s1; ++i)
	{
		for (size_t j = 1; j <= s2; ++j)
		{
			int add = d[i][j - 1] + add_elem(tsig[j - 1].code);
			int suppr = d[i - 1][j] + suppr_elem(isig[i - 1].code);
			int change = d[i- 1][j - 1] + change_elem(isig[i - 1].code, tsig[j - 1].code);
			int min = change;
			if (add < min)
				min = add;
			if (suppr < min) 
				min = suppr;
			d[i][j] = min;
			//std::cout << d[i][j] << " ";
		}
		//std::cout << std::endl;
	}
	//std::cout << d[s1][s2] << std::endl;

	/*
	// find segments that contain only one dot signature element
	std::vector<bool> puncdot(s2 + 1, false);
	for (size_t tmp = 0; tmp < s2 - 1; ++tmp)
		if (txt[tmp].start && txt[tmp + 1].start && (txt[tmp].code == '.'))
			puncdot[tmp + 1] = true;
	if (txt[s2 - 1].start && (txt[s2 - 1].code == '.'))
		puncdot[s2] = true;
	*/
	// look for an optimal path
	std::vector<size_t> path(s2 + 1);
	size_t i = s1;
	size_t t = s2;
	path[t] = i;
	while (t > 0)
	{
		auto start = tsig[t - 1].start;
		auto nextstart = false;
		if (t == s2)
			nextstart = true;
		else if (tsig[t].start)
			nextstart = true;

		int dcost = std::numeric_limits<int>::max();
		int leftcost = std::numeric_limits<int>::max();
		if ((i > 0) /*&& !puncdot[t]*/) // when a segment is just a punctuation dot, we can only go up, or the dot might eat part of the neighbouring segments
		{
			dcost = d[i - 1][t - 1];
			leftcost = d[i- 1][t];
		}
		int upcost = d[i][t - 1];
		if (dcost <= leftcost)
		{
			if ((dcost == upcost) && start && (i > 1))
			{ // if the signature element is the first of a entity, then choose the best cut
				t -= 1;
				if (isig[i - 2].cutproba >= isig[i - 1].cutproba)
					i -= 1;
			}
			else if ((dcost <= upcost) || (start && nextstart && (i > 0)))
			{ // diagonal
				t -= 1;
				i -= 1;
			}
			else
			{ // up
				t -= 1;
			}
		}
		else
		{ // if the signature element is the first of a entity, then choose the best cut
			if ((leftcost == upcost) && start && (i > 1))
			{
				if (isig[i - 2].cutproba >= isig[i - 1].cutproba)
					i -= 1;
				else
					t -= 1;
			}
			else if ((leftcost <= upcost) || (start && nextstart && (i > 0)))
			{ // left
				i -= 1;
			}
			else
			{ // up
				t -= 1;
			}
		}
		path[t] = i;
	}
	// compute bounding boxes
	std::vector<std::pair<crn::Rect, crn::StringUTF8>> align;
	size_t istart = 0, iend = 0;
	for (size_t t = 1; t <= s2; ++t)
	{
		//if (tsig[t - 1].start)
			//std::cout << "s " << tsig[t - 1].code << " ";
		//std::cout << path[t] << " " << t << std::endl;
		size_t imgnum = path[t]; if (imgnum > 0) imgnum -= 1;
		if (tsig[t - 1].start)
		{ // start a new segment

			// end previous segment
			if (!align.empty())
			{
				crn::StringUTF8 sig;
				if ((iend == imgnum) && (iend > 0)) iend -= 1;
				for (size_t is = istart; is <= iend; ++is)
					sig += isig[is].code;
				align.back().second = sig;
			}

			if (!align.empty())
			{ // make sure the previous segment ends just before this segment begins
				if (imgnum > 0)
					if (isig[imgnum - 1].bbox.IsValid())
						align.back().first |= isig[imgnum - 1].bbox;
			}

			// create segment (with no signature)
			align.emplace_back(isig[imgnum].bbox, crn::StringUTF8{});
			//std::cout << isig[imgnum].bbox.ToString() << std::endl;
			istart = iend = imgnum;
		}
		else
		{ // add to existing segment
			iend = imgnum;
			if (!align.back().first.IsValid())
			{ // the first bbox of the segment was invalid
				// TODO is it possible???
				align.back().first = isig[imgnum].bbox;
			}
			else if (isig[imgnum].bbox.IsValid())
			{ // append
				bool ok = true;
				// check if the same image elements repeats until (and including) a new segment
				for (size_t nextt = t + 1; nextt < s2 + 1; ++ nextt)
				{
					size_t nextimgnum = path[nextt]; if (nextimgnum > 0) nextimgnum -= 1;
					if (nextimgnum != imgnum)
						break;
					if (tsig[nextt - 1].start)
					{
						ok = false;
						break;
					}
				}
				if (ok)
					align.back().first |= isig[imgnum].bbox;
			}
		}
	}
	if (align.empty())
		return align; // XXX
	align.front().first |= isig.front().bbox; // not sure if it's necessary
	align.back().first |= isig.back().bbox; // not sure if it's necessary
	// end last segment
	crn::StringUTF8 sig;
	for (size_t is = istart; is < isig.size(); ++is)
		sig += isig[is].code;
	align.back().second = sig;
	
	return align;
}


