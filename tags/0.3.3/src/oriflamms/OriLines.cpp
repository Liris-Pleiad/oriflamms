/* Copyright 2013 INSA-Lyon & IRHT
 * 
 * file: OriLines.cpp
 * \author Yann LEYDIER
 */

#include <OriLines.h>
#include <CRNMath/CRNLinearInterpolation.h>
#include <CRNImage/CRNColorModel.h>
#include <CRNImage/CRNPixelHSV.h>
#include <CRNMath/CRNMatrixComplex.h>
#include <CRNAI/CRN2Means.h>
#include <CRNUtils/CRNXml.h>
#include <CRNMath/CRNPolynomialRegression.h>
#include <CRNImage/CRNDifferential.h>
#include <numeric>
#include <CRNi18n.h>

#include <iostream>

#ifdef CRN_PF_MSVC
namespace std
{
	template<> void swap<crn::LinearInterpolation, void, void>(CRNLinearInterpolation &l1, CRNLinearInterpolation &l2)
	{
		l1.Swap(l2);
	}
}
#endif

static const std::vector<crn::Rect> detectColumns(const crn::ImageIntGray &ydiff)
{
	CRNImageIntGray ig2(ydiff.Clone());
	ig2->Abs();
	ig2->Negative();
	// projection of line gradients
	CRNHistogram vp(ig2->MakeImageBWFisher()->MakeVerticalProjection());
	static int cnt = 0;
	//vp->MakeImageBW(500)->SavePNG(crn::Path("vproj ") + cnt++ + crn::Path(".png"));

	/*
	// init threshold
	size_t maxval = vp->Max() * 3 / 4;//vp->MakePopulationHistogram()->Fisher();
	// # pikes @ threshold
	size_t nspik = 0;
	for (size_t x = 0; x < ydiff.GetWidth() - 1; ++x)
		if ((vp[x] < maxval) && (vp[x + 1] > maxval))
			nspik += 1;
	// lower the threshold to avoid cutting the beginning and end of lines
	for (; maxval > 0; --maxval)
	{
		size_t n = 0;
		for (size_t x = 0; x < ydiff.GetWidth() - 1; ++x)
			if ((vp[x] < maxval) && (vp[x + 1] >= maxval))
				n += 1;
		if (n != nspik) // # pikes changed, stop
			break;
	}
	maxval += 1;

	// compute text zones
	std::vector<crn::Rect> zones, thumbzones;
	bool intext = false;
	size_t precx = 0;
	for (size_t x = 0; x < ydiff.GetWidth(); ++x)
	{
		if (!intext && (vp[x] > maxval))
		{
			intext = true;
			thumbzones.push_back(crn::Rect(int(x), 0, int(ydiff.GetWidth()) - 1, int(ydiff.GetHeight()) - 1));
		}
		if (intext && (vp[x] < maxval))
		{
			thumbzones.back().SetRight(int(x));
			intext = false;
			precx = x;
		}
	}
	*/

	const unsigned int max = vp->Max();
	const size_t maxmod = 20;
	std::vector<unsigned int> modes(maxmod, 0);
	std::vector<std::vector<unsigned int> > modey(maxmod);
	for (unsigned int y = 0; y < max; ++y)
	{
		int t1 = 0, t2 = 0;
		int b = -1, e = 0;
		int incnt = 0;
		for (size_t x = 0; x < vp->Size() - 1; ++x)
		{
			bool in1 = vp->GetBin(x) > y;
			if (in1) incnt += 1;
			bool in2 = vp->GetBin(x + 1) > y;
			if (!in1 && in2)
			{
				t1 += 1; // entering the data
				if (b == -1) b = int(x);
			}
			else if (in1 && !in2)
			{
				t2 += 1; // leaving the data
				e = int(x);
			}
		}
		int nmodes = crn::Max(t1, t2);
		if (nmodes >= maxmod)
			nmodes = int(maxmod - 1);
		if (nmodes > 0)
		{
			modes[nmodes - 1] += e - b + 1;//incnt;
			modey[nmodes - 1].push_back(y);
		}
	}
	int maxmcnt = 0;
	int nmodes = 0;
	/*
	for (std::map<int, int>::const_iterator it = modes.begin(); it != modes.end(); ++it)
	{
		std::cout << it->first << "(" << it->second << ") ";
		if (it->second > maxmcnt)
		{
			maxmcnt = it->second;
			nmodes = it->first;
		}
	}
	*/
	std::vector<unsigned int>::iterator maxit = std::max_element(modes.begin(), modes.end());
	if (maxit != modes.end())
	{
		std::vector<unsigned int>::iterator nit = maxit + 1;
		while (nit != modes.end())
		{
			if (*nit > *maxit / 2)
			{
				++nit;
				++maxit;
			}
			else
				break;
		}
		nmodes = int(maxit - modes.begin()) + 1;
	}
	else
	{ // XXX white page!!!
		return std::vector<crn::Rect>();
	}

	//std::cout << "************** " << nmodes << std::endl;

	bool in = false;
	unsigned int th = modey[nmodes - 1][modey[nmodes - 1].size() / 2];
	//std::cout << th << std::endl;
	int bx = 0;
	std::vector<crn::Rect> thumbzones;
	for (size_t x = 0; x < vp->Size(); ++x)
	{
		if (in)
		{
			if (vp->GetBin(x) <= th)
			{
				thumbzones.push_back(crn::Rect(bx, 0, int(x) - 1, int(ydiff.GetHeight()) - 1));
				in = false;
			}
		}
		else
		{
			if (vp->GetBin(x) > th)
			{
				bx = int(x);
				in = true;
			}
		}
	}
	if (in)
	{
		thumbzones.push_back(crn::Rect(bx, 0, int(ydiff.GetWidth()) - 1, int(ydiff.GetHeight()) - 1));
	}
	/*
	// estimate the number of columns
	std::map<size_t, size_t> modecnt;
	std::map<size_t, std::vector<size_t> > modemean;
	while (vp->Modes().size() > 1)
	{
		vp->AverageSmoothing(1);
		std::vector<size_t> modes(vp->Modes());
		size_t nmode = modes.size();
		if (modecnt.find(nmode) == modecnt.end())
		{
			modecnt[nmode] = 1;
			modemean[nmode] = modes;
		}
		else
		{
			modecnt[nmode] += 1;
			for (size_t tmp = 0; tmp < nmode; ++tmp)
				modemean[nmode][tmp] += modes[tmp];
		}
	}
	size_t maxcnt = 0, npeak = 0;
	for (std::map<size_t, size_t>::const_iterator it = modecnt.begin(); it != modecnt.end(); ++it)
	{
		if (it->second > maxcnt)
		{
			maxcnt = it->second;
			npeak = it->first;
		}
	}
	//std::cout << npeak << " pics @ ";
	for (size_t tmp = 0; tmp < npeak; ++tmp)
	{
		modemean[npeak][tmp] /= maxcnt;
		//std::cout << modemean[npeak][tmp] << " ";
	}
	//std::cout << std::endl;

	vp = ig2->MakeImageBWFisher()->MakeVerticalProjection();
	ig2 = NULL;

	std::vector<crn::Rect> thumbzones;
	size_t start = 0;
	for (size_t tmp = 0; tmp < npeak - 1; ++tmp)
	{
		size_t cut, cut2;
		unsigned int mini = std::numeric_limits<unsigned int>::max();
		for (size_t x = modemean[npeak][tmp]; x < modemean[npeak][tmp + 1]; ++x)
		{
			if (vp->GetBin(x) < mini)
			{
				mini = vp->GetBin(x);
				cut = x;
			}
			if (vp->GetBin(x) <= mini)
				cut2 = x;
		}
		if (start == 0)
		{
			while (vp->GetBin(start) < mini) ++start;
		}

		thumbzones.push_back(crn::Rect(int(start), 0, int(cut), int(ydiff.GetHeight()) - 1));
		//std::cout << start << " -> " << cut << std::endl;
		start = cut2;
	}
	int end;
	for (end = int(vp->Size()) - 1; vp->GetBin(end) < vp->GetBin(start); --end) { }
	thumbzones.push_back(crn::Rect(int(start), 0, int(end), int(ydiff.GetHeight()) - 1));
	//std::cout << start << " -> " << end << std::endl;

	*/
	return thumbzones;
}

/*
static bool isblack(crn::Block &b, int x, int y, int win, int th, CRNImageGray enmask)
{
	int bx = crn::Max(0, x - 2 * win);
	int ex = crn::Min(b.GetAbsoluteBBox().GetWidth(), x + 2 * win + 1);
	int by = crn::Max(0, y - 3 * win);
	int ey = crn::Min(b.GetAbsoluteBBox().GetHeight(), y + 3 * win + 1);
	unsigned int sum = 0;
	for (int ty = by; ty < ey; ++ty)
		for (int tx = bx; tx < ex; ++tx)
		{
			sum += ((b.GetGray()->GetPixel(tx, ty) < th) && !enmask->GetPixel(tx, ty)) ? 1 : 0;
		}
	return sum > ((ex - bx) * (ey - by) / 3); // more than a 3rd of the pixels are below the threshold
}
*/

static CRNLinearInterpolation cut_line(crn::LinearInterpolation &l, crn::Block &b, size_t sw, int th, CRNImageGray enmask, const crn::Rect &clip)
{
	if (l.GetData().size() <= 2)
		return NULL;

	int bx = int(l.GetData().front().X);
	int by = int(l.GetData().front().Y);
	int ex = int(l.GetData().back().X);
	int ey = int(l.GetData().back().Y);
	int win = int(sw) / 2 + 1;

	bool can_enlarge_b = true, can_enlarge_e = true;
	
	// cut at enlightened marks
	int cx = (bx + ex) / 2;
	for (int x = cx; x > bx; --x)
		if (enmask->GetPixel(x, l[x]) != 0)
		{
			bx = x;
			can_enlarge_b = false;
			break;
		}
	for (int x = cx + 1; x < ex; ++x)
		if (enmask->GetPixel(x, l[x]) != 0)
		{
			ex = x;
			can_enlarge_e = false;
			break;
		}


	/* USELESS
	// count transitions
	int tr = 0;
	for (int x = bx; x < ex; ++x)
	{
		bool bg1 = b.GetGray()->GetPixel(x, l[x]) > th;
		bool bg2 = b.GetGray()->GetPixel(x + 1, l[x + 1]) > th;
		if (bg1 != bg2)
			tr += 1;
	}
	if ((double(tr) / (ex - bx) * double(sw)) < 0.2)
	{
		std::cout << "filtered because too few transitions" << std::endl;
		return NULL; // too few transitions, we might have located a spot or a stain
	}
	*/

	std::vector<int> trans;
	for (int x = clip.GetLeft(); x <= clip.GetRight(); ++x)
	{
		int y1 = l[x];
		int y2 = l[x + 1];
		bool diff = false;
		for (int d = -2 * int(sw); d < 2 * int(sw); ++d)
		{
			if ((y1 + d < clip.GetTop()) || (y1 + d > clip.GetBottom()) || 
					(y2 + d < clip.GetTop()) || (y2 + d > clip.GetBottom()))
				continue;
			bool bg1 = b.GetGray()->GetPixel(x, y1 + d) > th;
			bool bg2 = b.GetGray()->GetPixel(x + 1, y2 + d) > th;
			if ((bg1 != bg2) && !enmask->GetPixel(x, y1 +d) && !enmask->GetPixel(x + 1, y2 +d))
			{
				diff = true;
				break;
			}
		}
		if (diff)
			trans.push_back(1);
		else
			trans.push_back(0);
	}
	std::vector<int> trate;
	for (size_t x = sw; x < int(trans.size()) - sw; ++x)
	{
		trate.push_back(std::accumulate(trans.begin() + x - sw, trans.begin() + x + sw, 0));
		/*
		b.GetRGB()->DrawLine(clip.GetLeft() + x, by, clip.GetLeft() + x, by - (isblack(b, int(clip.GetLeft() + x), l[int(clip.GetLeft() + x)], win, th, enmask) ? 15 : 0), crn::PixelRGB(crn::Byte(0), 0, 255));
		b.GetRGB()->DrawLine(clip.GetLeft() + x, by, clip.GetLeft() + x, by - trate.back(), crn::PixelRGB(crn::Byte(255), 0, 255));
		if (!trate.back() && !isblack(b, int(clip.GetLeft() + x), l[int(clip.GetLeft() + x)], win, th, enmask))
			b.GetRGB()->DrawLine(clip.GetLeft() + x, by, clip.GetLeft() + x, by - 15, crn::PixelRGB(crn::Byte(255), 0, 0));
		*/
	}
	
	if (bx < clip.GetLeft() + int(sw)) bx = clip.GetLeft() + int(sw);
	if (ex > clip.GetRight() - int(sw)) ex = clip.GetRight() - int(sw) - 1;
	if (trate[bx - clip.GetLeft() - sw] > 1)
	{ // grow
		if  (can_enlarge_b)
		{
			int x = bx - 1;
			for (; x >= clip.GetLeft() + sw; --x)
				if (trate[x - clip.GetLeft() - sw] <= 1)
					break;
			bx = x + 1;
		}
	}
	else
	{ // shrink
		int x = bx + 1;
		for (; x < clip.GetRight() - sw; ++x)
			if (trate[x - clip.GetLeft() - sw] > 1)
				break;
		bx = x;
	}

	if (trate[ex - clip.GetLeft() - sw] > 1)
	{ // grow
		if  (can_enlarge_e)
		{
			int x = ex + 1;
			for (; x < clip.GetRight() - sw; ++x)
				if (trate[x - clip.GetLeft() - sw] <= 1)
					break;
			ex = x - 1;
		}
	}
	else
	{ // shrink
		int x = ex - 1;
		for (; x >= clip.GetLeft() +	sw; --x)
			if (trate[x - clip.GetLeft() - sw] > 1)
				break;
		ex = x;
	}

	/*
	// fit start
	if (isblack(b, bx, by, win, th, enmask) && can_enlarge_b)
	{ // try to enlarge
		int x = bx - 1;
		for (; x >= clip.GetLeft(); --x)
			if (!isblack(b, x, by, win, th, enmask) || (enmask->GetPixel(x, l[x]) != 0))
				break;
		bx = x + 1;
	}
	else
	{ // try to shrink
		int x = bx + 1;
		for (; x <= clip.GetRight(); ++x)
			if (isblack(b, x, l[x], win, th, enmask))
				break;
		bx = x;
	}

	// fit end
	if (isblack(b, ex, ey, win, th, enmask) && can_enlarge_e)
	{ // try to enlarge
		int x = ex + 1;
		for (; x <= clip.GetRight(); ++x)
			if (!isblack(b, x, ey, win, th, enmask) || (enmask->GetPixel(x, l[x]) != 0))
				break;
		ex = x - 1;
	}
	else
	{ // try to shrink
		int x = ex - 1;
		for (; x >= clip.GetLeft(); --x)
			if (isblack(b, x, l[x], win, th, enmask))
				break;
		ex = x;
	}
*/
	if (ex <= bx)
		return NULL;

	// XXX display
	//b.GetRGB()->DrawLine(bx, l[bx] - 3*win, bx, l[bx] + 3*win, crn::PixelRGB(crn::Byte(0), 200, 0));
	//b.GetRGB()->DrawLine(ex, l[ex] - 3*win, ex, l[ex] + 3*win, crn::PixelRGB(crn::Byte(255), 0, 0));
	// XXX

	// create new line
	std::vector<crn::Point2DDouble> newline;
	if (bx < l.GetData().front().X) // first point out of the line
		newline.push_back(crn::Point2DDouble(bx, l.GetData().front().Y));
	else // first point in the line
		newline.push_back(crn::Point2DDouble(bx, l[bx]));
	CRN_FOREACH(const crn::Point2DDouble &p, l.GetData()) // middle points
		if ((p.X > bx) && (p.X < ex))
			newline.push_back(p);
	if (newline.empty())
		return NULL;
	if (ex > newline.back().X) // end point out of the line
		newline.push_back(crn::Point2DDouble(ex, newline.back().Y));
	else // end point in the line
		newline.push_back(crn::Point2DDouble(ex, l[ex]));
	if (newline.size() < 2)
		return NULL;
	return new crn::LinearInterpolation(newline.begin(), newline.end());
}

struct LineSorter: public std::binary_function<const CRNLinearInterpolation&, const CRNLinearInterpolation&, bool>
{
	inline bool operator()(const CRNLinearInterpolation &l1, const CRNLinearInterpolation &l2) const
	{
		return l1->GetData().front().Y < l2->GetData().front().Y;
	}
};

const CRNVector DetectLines(crn::Block &b)
{
	//b.SubstituteGray(b.GetRGB()->MakeValue());
	const size_t sw = b.GetGray()->GetStrokesWidth();
	const size_t w = b.GetGray()->GetWidth();
	const size_t h = b.GetGray()->GetHeight();
	//////////////////////////////////////////////////////////////
	// thumbnail
	//////////////////////////////////////////////////////////////
	const size_t xdiv = sw * 2;
	const size_t ydiv = 1;
	const size_t nw = w / xdiv;
	const size_t nh = h / ydiv;
	crn::ImageIntGray ig(nw, nh);
	std::vector<crn::Angle<crn::ByteAngle> > hues; // used compute mean hue
	CRNPixelRGB rgbpix = b.GetRGB()->MakePixel(0);
	for (size_t y = 0; y < nh; ++y)
		for (size_t x = 0; x < nw; ++x)
		{
			int mval = 255;
			size_t okx = x * xdiv, oky = y * ydiv;
			for (size_t ty = y * ydiv; ty < crn::Min(h, (y + 1) * ydiv); ++ty)
				for (size_t tx = x * xdiv; tx < crn::Min(w, (x + 1) * xdiv); ++tx)
				{
					int v = b.GetGray()->GetPixel(tx, ty);
					if (v < mval)
					{
						mval = v;
						okx = tx;
						oky = ty;
					}
				}
			// thumbnail
			ig.SetPixel(x, y, mval);
			// hue
			b.GetRGB()->MovePixel(*rgbpix, okx, oky);
			crn::PixelHSV hsvpix;
			hsvpix.Set(*rgbpix);
			hues.push_back(hsvpix.GetHue());
		}
	// x smoothing
	ig.Convolve(*crn::MatrixDouble::NewGaussianLine(10.0));

	//////////////////////////////////////////////////////////////
	// compute linespace
	//////////////////////////////////////////////////////////////
	bool stop = false;
	crn::UInt64 ps = 0;
	size_t lspace;
	for (lspace = 2; lspace < 1000; ++lspace)
	{
		crn::UInt64 s = 0;
		for (size_t y = 0; y < nh; ++y)
		{
			size_t y2 = (y + lspace) % nh;
			for (size_t x = 0; x < nw; ++x)
				s += crn::Abs(ig.GetPixel(x, y) - int(ig.GetPixel(x, y2)));
		}
		if (s < ps) // difference decreasing
			stop = true; // lines are overlapping
		if (stop && (s > ps)) // difference increasing
			break; // lines stop overlapping
		ps = s;
	}
	size_t lspace1 = (lspace - 1) * ydiv; // scale 1

	//////////////////////////////////////////////////////////////
	// compute enlightened marks' mask
	//////////////////////////////////////////////////////////////
	crn::Angle<crn::ByteAngle> meanhue = crn::AngularMean(hues.begin(), hues.end());
	int stdhue = 4 * int(sqrt(crn::AngularVariance(hues.begin(), hues.end(), meanhue)));
	// create a mask of colored parts
	CRNImageBW colormask(new crn::ImageBW(w, h, crn::PixelBW::WHITE));
	rgbpix = b.GetRGB()->MakePixel(0);
	FOREACHPIXEL(tmp, *b.GetRGB())
	{
		b.GetRGB()->MovePixel(*rgbpix, tmp);
		crn::PixelHSV hsvpix;
		hsvpix.Set(*rgbpix);
		if (((hsvpix.GetSaturation() > 127) && (hsvpix.GetValue() > 127)) || // colorful
			 (crn::AngularDistance(crn::Angle<crn::ByteAngle>(hsvpix.GetHue()), meanhue) > stdhue)) // not the main color
		{
			colormask->SetPixel(tmp, crn::PixelBW::BLACK);
		}
	}
	// filter the color mask
	CRNBlock colorb(crn::Block::New(colormask));
	CRNImageIntGray colormap = colorb->ExtractCC(STR("cc"));
	CRNImageGray enmask(new crn::ImageGray(w, h, crn::PixelBW::BLACK));
	if (colorb->HasTree(STR("cc")))
	{
		for (crn::Block::block_iterator it = colorb->BlockBegin(STR("cc")); it != colorb->BlockEnd(STR("cc")); ++it)
		{
			int num = it->GetName().ToInt();
			if (/*(it->GetAbsoluteBBox().GetWidth() < 2 * sw) || */(it->GetAbsoluteBBox().GetHeight() > 2 * lspace1))
			{
				for (crn::Block::pixel_iterator pit = colorb->PixelBegin(*it); pit != colorb->PixelEnd(*it); ++pit)
				{
					if (colormap->GetPixel(pit->X, pit->Y) == num)
					{
						enmask->SetPixel(pit->X, pit->Y, crn::PixelBW::WHITE);
					}
				}
			}
		}
	}
	colormask = NULL;
	colorb = NULL;
	colormap = NULL;
	// fill the horizontal gaps in the enlightened marks' mask
	for (size_t y = 0; y < h; ++y)
	{
		size_t bx = 0;
		for (size_t x = 0; x < w; ++x)
		{
			if (enmask->GetPixel(x, y)) 
			{
				b.GetRGB()->SetPixel(x, y, 255, 255, 0);
				int diff = int(x) - int(bx);
				if ((diff > 1) && (diff < lspace1/2))
					for (size_t tx = bx + 1; tx < x; ++tx)
					{
						enmask->SetPixel(tx, y, 255);
						b.GetRGB()->SetPixel(tx, y, 255, 255, 0);
					}
				bx = x;
			}
		}
	}

	//////////////////////////////////////////////////////////////
	// vertical differential
	//////////////////////////////////////////////////////////////
	CRNMatrixDouble ydiff(crn::MatrixDouble::NewGaussianLineDerivative(double(lspace) / 6.0));
	ydiff->Transpose();
	ydiff->NormalizeForConvolution();
	ig.Convolve(*ydiff);

	// Columns
	std::vector<crn::Rect> thumbzones(detectColumns(ig));
	CRN_FOREACH(const crn::Rect &r, thumbzones)
	{
		for (size_t x = r.GetLeft() * xdiv; x < r.GetRight() * xdiv; ++x)
			for (size_t y = 0; y < h; ++y)
				b.GetRGB()->GetBluePixels()[x + y * w] = 0;
	}

	//////////////////////////////////////////////////////////////
	// find lines
	//////////////////////////////////////////////////////////////
	// create mask of line guides
	crn::ImageBW mask(nw, nh, crn::PixelBW::BLACK);
	int maxig = 0;
	FOREACHPIXEL(tmp, ig)
		if (crn::Abs(ig.GetPixel(tmp)) > maxig) maxig = crn::Abs(ig.GetPixel(tmp));
	crn::Histogram linehist(maxig + 1);
	FOREACHPIXEL(tmp, ig)
		linehist.IncBin(crn::Abs(ig.GetPixel(tmp)));
	crn::UInt64 linethresh = linehist.Fisher();
	CRN_FOREACH(const crn::Rect &tz, thumbzones)
	{
		for (size_t y = tz.GetTop(); y < tz.GetBottom() - 1; ++y)
			for (size_t x = tz.GetLeft(); x < tz.GetRight(); ++x)
			{
				if ((ig.GetPixel(x, y) >= 0) && (ig.GetPixel(x, y + 1) < 0))
				{
					bool ok = false;
					for (size_t ty = crn::Max((size_t)tz.GetTop(), y - lspace/4); ty < crn::Min((size_t)tz.GetBottom(), y + lspace/4); ++ty)
							if (crn::Abs(ig.GetPixel(x, ty)) > linethresh)
							{
								ok = true;
								break;
							}
					if (ok)
						mask.SetPixel(x, y, crn::PixelBW::WHITE);
				}
			}
	}

	// extract lines
	CRNVector cols(new crn::Vector(crn::protocol::Serializable));
	const int line_x_search = 10;
	const int line_y_search = 10;
	for (size_t tmpz = 0; tmpz < thumbzones.size(); ++tmpz)
	{
		const crn::Rect &tz(thumbzones[tmpz]);
		std::vector<CRNLinearInterpolation> lines;
		for (size_t x = tz.GetLeft(); x < tz.GetRight(); ++x)
			for (size_t y = tz.GetTop(); y < tz.GetBottom(); ++y)
				if (mask.GetPixel(x, y))
				{ // found a line guide
					mask.SetPixel(x, y, 0); // erase from mask
					size_t x1 = x;
					size_t y1 = y;
					bool found = true;
					//int minl, maxl;
					//minl = maxl = b.GetGray()->GetPixel(x1 * xdiv, y1 * ydiv);
					std::vector<crn::Point2DInt> points;
					points.push_back(crn::Point2DInt(int(x1 * xdiv), int(y1 * ydiv)));
					while (found)
					{
						found = false;
						size_t by = crn::Max(0, int(y1) - line_y_search);
						size_t ey = crn::Min(nh, y1 + line_y_search);
						for (size_t x2 = x1; x2 < crn::Min(size_t(tz.GetRight()), x1 + line_x_search); ++x2)
							for (size_t y2 = by; (y2 < ey) && !found; ++y2)
								if (mask.GetPixel(x2, y2))
								{ // found a line guide
									mask.SetPixel(x2, y2, 0); // erase from mask
									y1 = y2;
									x1 = x2;
									points.push_back(crn::Point2DInt(int(x1 * xdiv), int(y1 * ydiv)));
									//minl = crn::Min(minl, (int)b.GetGray()->GetPixel(x1 * xdiv, y1 * ydiv));
									//maxl = crn::Max(maxl, (int)b.GetGray()->GetPixel(x1 * xdiv, y1 * ydiv));
									found = true;
									break;
								}
					}
					if (points.size() > 2)
						lines.push_back(new crn::LinearInterpolation(points.begin(), points.end()));
				}

		// compute thresholds
		std::vector<std::pair<crn::Byte, crn::Byte> > linelum;
		CRN_FOREACH(CRNLinearInterpolation &l, lines)
		{
			std::vector<crn::Byte> values;
			for (int x = int(l->GetData().front().X); x <= int(l->GetData().back().X); ++x)
			{
				int ty = (*l)[x];
				for (int y = ty - int(2*sw); y <= ty + int(2*sw); ++y)
					if ((y > 0) && (y < b.GetAbsoluteBBox().GetHeight()))
						values.push_back(b.GetGray()->GetPixel(x, y));
			}
			try
			{
				std::pair<crn::Byte, crn::Byte> f = crn::TwoMeans(values.begin(), values.end());
				linelum.push_back(f);
			}
			catch (crn::ExceptionDomain&)
			{
				linelum.push_back(std::pair<crn::Byte, crn::Byte>(0, 255));
			}
		}

		// cut lines
		std::vector<CRNLinearInterpolation> filteredlines;
		//std::map<CRNLinearInterpolation, int> filteredthresh;
		//CRNPixelRGB fgcol(colm->GetClass(crn::ColorModel::Ink)->Front());
		//int bgg = (bgcol->GetRed() + bgcol->GetGreen() + bgcol->GetBlue()) / 3;
		//int fgg = (fgcol->GetRed() + fgcol->GetGreen() + fgcol->GetBlue()) / 3;
		//int inkth = (fgg * 3 + bgg) / 4;
		//CRN_FOREACH(CRNLinearInterpolation &l, lines)
		std::vector<crn::Point2DDouble> linestart;
		for (size_t tmp = 0; tmp < lines.size(); ++tmp)
		{
			CRNLinearInterpolation l(lines[tmp]);
			CRNLinearInterpolation fl(NULL);
			{
				int thresh = (linelum[tmp].first + linelum[tmp].second) / 2;
				int left = int(tz.GetLeft() * xdiv);
				if (tmpz == 0) left = 0; // grow first zone to beginning
				else left = int(left + thumbzones[tmpz - 1].GetRight() * xdiv) / 2; // grow to half the distance to previous zone
				int right = int(tz.GetRight() * xdiv);
				if (tmpz == thumbzones.size() - 1) right = b.GetAbsoluteBBox().GetRight(); // grow last zone to end
				else right = int(right + thumbzones[tmpz + 1].GetLeft() * xdiv) / 2; // grow to half the distance to next zone
				fl = cut_line(*l, b, sw, thresh, enmask, crn::Rect(left, int(tz.GetTop() * ydiv), right, int(tz.GetBottom() * ydiv)));
				if (fl)
				{
					filteredlines.push_back(fl);
					//filteredthresh[fl] = thresh;
					linestart.push_back(crn::Point2DDouble(fl->GetData().front().Y, fl->GetData().front().X));
				}
			}
			// XXX DISPLAY
			
			for (int x = int(l->GetData().front().X); x < int(l->GetData().back().X); ++x)
				b.GetRGB()->SetPixel(x, (*l)[x], 0, 0, 255);
			if (fl)
				for (int x = int(fl->GetData().front().X); x < int(fl->GetData().back().X); ++x)
					b.GetRGB()->SetPixel(x, (*fl)[x], 0, 200, 0);
		}
		lines.swap(filteredlines);

		if (!lines.empty())
		{
			if (linestart.size() > 3)
			{
				/////////////////////////////////////////////////////////////
				// Cut the beginning of lines that caught an enlightened mark
				/////////////////////////////////////////////////////////////

				// median line start
				std::vector<double> posx;
				CRN_FOREACH(const crn::Point2DDouble &p, linestart)
				{
					posx.push_back(p.Y);
				}
				std::sort(posx.begin(), posx.end());
				double meanx = posx[posx.size() / 2];
				b.GetRGB()->DrawLine(int(meanx), 0, int(meanx), b.GetAbsoluteBBox().GetBottom(), crn::PixelRGB(crn::Byte(0), 0, 255)); // XXX DISPLAY
				// keep only nearest lines starts
				std::vector<crn::Point2DDouble> linestart2;
				CRN_FOREACH(const crn::Point2DDouble &p, linestart)
				{
					double d = crn::Abs(p.Y - meanx);
					if (d < lspace1 / 2) // heuristic :o(
						linestart2.push_back(p);
				}
				if (linestart2.size() > 3)
				{
					crn::PolynomialRegression reg(linestart2.begin(), linestart2.end(), 1);
					b.GetRGB()->DrawLine(reg[0], 0, reg[b.GetAbsoluteBBox().GetBottom()], b.GetAbsoluteBBox().GetBottom(), crn::PixelRGB(crn::Byte(255), 0, 0)); // XXX DISPLAY
					// cut lines
					CRN_FOREACH(CRNLinearInterpolation &l, lines)
					{
						double margin = reg[l->GetData().front().Y];
						double d = margin - l->GetData().front().X; // distance to regression of the margin
						if (d > 2 * sw) // heuristic :o(
						{ // the line goes too far, we're cutting it at the margin
							std::vector<crn::Point2DDouble> newline;
							newline.push_back(crn::Point2DDouble(margin, (*l)[margin]));
							CRN_FOREACH(const crn::Point2DDouble &p, l->GetData())
							{
								if (p.X > margin)
									newline.push_back(p);
							}
							//int thresh = filteredthresh[l];
							if (newline.size() > 2)
								l = new crn::LinearInterpolation(newline.begin(), newline.end());
							// TODO XXX ELSE WHAT ???
							//filteredthresh[l] = thresh;

							// XXX DISPLAY
							crn::Rect r(int(l->GetData().front().X) - 20, int(l->GetData().front().Y) - 20, int(l->GetData().front().X) + 20, int(l->GetData().front().Y) + 20);
							CRN_FOREACH(const crn::Point2DInt &p, r)
							{
								b.GetRGB()->GetGreenPixels()[p.X + p.Y * w] = 0;
								b.GetRGB()->GetBluePixels()[p.X + p.Y * w] = 0;
							}
						}
					}
				}
			}
			//////////////
			// store lines
			//////////////
			LineSorter sorter;
			std::sort(lines.begin(), lines.end(), sorter);
			CRNVector col(new crn::Vector(crn::protocol::Serializable));
			CRN_FOREACH(CRNLinearInterpolation &l, lines)
				col->PushBack(new ori::GraphicalLine(l, lspace1/*, filteredthresh[l]*/));
			cols->PushBack(col);
		}
	}

	return cols;
}

using namespace ori;
GraphicalLine::GraphicalLine(CRNLinearInterpolation lin, size_t lineheight/*, int thresh*/):
	midline(lin),
	lh(lineheight)/*,
	lumthresh(thresh)*/
{ 
	if (!lin) 
		throw crn::ExceptionInvalidArgument();
	if (lin->GetData().empty())
		throw crn::ExceptionInvalidArgument();
	left = int(lin->GetData().front().X);
	right = int(lin->GetData().front().Y);
}

#define comma ,
const std::vector<ImageSignature> GraphicalLine::ExtractFeatures(crn::Block &b) const
{
	if (!features.empty())
		return features; // do not recompute

	// create subblock
	int bx = GetLeft();
	int ex = GetRight();
	int by = int(crn::Min(At(bx), At(ex))) - int(lh) / 2;
	int ey = int(crn::Max(At(bx), At(ex))) + int(lh) / 2;
	CRNBlock lb = b.AddChildAbsolute(L"lines", crn::Rect(bx, by, ex, ey));

	CRNDifferential diff(crn::Differential::New(*lb->GetRGB(), 0));
	size_t sw = b.GetGray()->GetStrokesWidth();
	diff->Diffuse(sw*1);
	CRNImageGradient igr(diff->MakeImageGradient());

	// compute horizontal strokes (points between left and right gradients)
	crn::ImageBW strokes(igr->GetWidth(), igr->GetHeight(), crn::PixelBW::BLACK);
	const size_t iw = strokes.GetWidth();
	const size_t ih = strokes.GetHeight();
	for (int y = 0; y < ih; ++y)
	{
		int sx = 0;
		bool in = false;
		for (int x = 0; x < iw; ++x)
		{
			if (igr->IsSignificant(x, y))
			{
				int a = (igr->GetTheta(x, y) + crn::Angle<crn::ByteAngle>(32)).value / 64; // 4 angles (add 32 to rotate the frame 45°)
				if (a == 0) // left
				{
					sx = x;
					in = true;
				}
				else if (a == 2) // right
				{
					if (in)
					{ // fill
						for (size_t tx = sx; tx <= x; ++tx)
						{
							strokes.SetPixel(tx, y, crn::PixelBW::WHITE);
							//lb->GetRGB()->SetPixel(tx, y, 255, 255, 255); // XXX display
						}
					}
					in = false;
				}
				else
				{
					in = false;
				}

			} // is significan
		} // x
	} // y
	strokes.SavePNG("art strokes.png");

	// project pixels between opposing horizontal gradients, above an below the midline
	crn::Histogram projt(iw);
	crn::Histogram projb(iw);
	//int precx = 0, precy1 = 0, precy2 = 0; // XXX display
	for (int x = 0; x < iw; ++x)
	{
		int y = At(x + bx) - by;
		for (int dy = -int(lh/2); dy < int(lh/2); ++dy)
		{
			if (strokes.GetPixel(x, crn::Cap(y + dy, 0, int(ih) - 1)))
			{
				if (dy < 0)
					projt.IncBin(x);
				else
					projb.IncBin(x);
			}
		}
		
		/*
		// XXX display
		lb->GetRGB()->SetPixel(x, crn::Max(0, y - int(projt[x])), 255, 0, 255);
		lb->GetRGB()->SetPixel(x, crn::Min(int(ih) - 1, y + int(projb[x])), 255, 0, 255);
		*/

		/*
		// XXX display
		if (precy1 == 0) precy1 = y;
		if (precy2 == 0) precy2 = y;
		lb->GetRGB()->DrawLine(precx, precy1, x, crn::Max(0, y - int(projt[x])), crn::PixelRGB(crn::Byte(255), 255, 255));
		lb->GetRGB()->DrawLine(precx, precy2, x, crn::Min(int(ih) - 1, y + int(projb[x])), crn::PixelRGB(crn::Byte(255), 255, 255));
		precx = x; precy1 = crn::Max(0, y - int(projt[x])); precy2 = crn::Min(int(ih) - 1, y + int(projb[x]));
		*/
	}
	//lb->GetRGB()->SavePNG("art proj.png");

	projt.AverageSmoothing(1);
	projb.AverageSmoothing(1);
	// extract modes
	std::vector<size_t> modest(projt.Modes());
	std::vector<size_t> modesb(projb.Modes());

	/*
	lb->FlushRGB();
	CRN_FOREACH(size_t x, modest) // XXX display
		lb->GetRGB()->DrawLine(x, 0, x, At(bx + int(x)) - by, crn::PixelRGB(crn::Byte(0), 0, 255));
	CRN_FOREACH(size_t x, modesb) // XXX display
		lb->GetRGB()->DrawLine(x, At(bx + int(x)) - by, x, ih - 1, crn::PixelRGB(crn::Byte(0), 0, 255));
	*/
	/*
	// XXX display
	int mint, minb;
	mint = minb = std::numeric_limits<int>::max();
	for (size_t x = 0; x < projt.Size(); ++x)
	{
	if (projt[x] < mint) mint = projt[x];
	if (projb[x] < minb) minb = projb[x];
	}
	for (size_t x = 1; x < projt.Size(); ++x)
	{
	b.GetRGB()->DrawLine(bx + x - 1, At(bx + int(x)) - (projt[x - 1] - mint) / 255, bx + x, At(bx + int(x)) - (projt[x] - mint) / 255, crn::PixelRGB(crn::Byte(255), 0, 255));
	b.GetRGB()->DrawLine(bx + x - 1, At(bx + int(x)) + (projb[x - 1] - minb) / 255, bx + x, At(bx + int(x)) + (projb[x] - minb) / 255, crn::PixelRGB(crn::Byte(255), 0, 255));
	}
	// XXX display
	*/

	// compute center guide
	std::vector<bool> centerguide(iw, false);
	for (size_t x = 0; x < iw; ++x)
	{
		int y = At(int(x) + bx) - by;
		if (strokes.GetPixel(x, y))
		{
			centerguide[x] = true;
			//lb->GetRGB()->SetPixel(x, y, 0, 0, 0); // XXX display
		}
		/*
		else // XXX display
			lb->GetRGB()->SetPixel(x, y, 255, 255, 255);
		*/
	}
	//lb->GetRGB()->SavePNG("art modes.png");

	bool in = false;
	std::vector<char> presig(iw, '\0'); // signature
	std::vector<std::pair<std::vector<size_t>, std::vector<size_t> > > segments; // top and bottom modes that are on a same black stream
	size_t topm = 0, botm = 0; // iterate on the modes
	for (int x = 0; x < iw; ++x)
	{
		int y = At(x + bx) - by;
		if (!in && centerguide[x])
		{ // enter black stream
			in = true;
		}
		if (!centerguide[x])
		{
			if (in)
			{ // enter white stream
				in = false;
				// gather modes that were in the black stream
				std::vector<size_t> topseg;
				for (; (topm < modest.size()) && (modest[topm] <= x); ++ topm)
					topseg.push_back(modest[topm]);
				std::vector<size_t> botseg;
				for (; (botm < modesb.size()) && (modesb[botm] <= x); ++ botm)
					botseg.push_back(modesb[botm]);
				segments.push_back(std::make_pair(topseg, botseg));
			}
			else
			{ // already inside a white stream
				// add modes to signature if there are modes here
				if ((topm < modest.size()) && (modest[topm] == x))
					presig[modest[topm++]] = '\'';
				if ((botm < modesb.size()) && (modesb[botm] == x))
				{
					if (presig[modesb[botm]] == '\'')
					{ // there is already a symbol here, make it a long stroke (not sure I should…)
						presig[modesb[botm]] = 'l';
					}
					else
						presig[modesb[botm]] = ',';
					botm += 1;
				}
			}
		}
	}
	// merge modes that are on the same black stream
	CRN_FOREACH(std::pair<std::vector<size_t> comma std::vector<size_t> > s, segments)
	{
		if (s.first.empty() && s.second.empty())
		{ // nothing
		}
		else if (s.first.empty())
		{ // just copy bottom parts
			CRN_FOREACH(size_t x, s.second)
				presig[x] = ',';
		}
		else if (s.second.empty())
		{ // just copy top parts
			CRN_FOREACH(size_t x, s.first)
				presig[x] = '\'';
		}
		else
		{
			// for each top part find nearest bottom part
			std::vector<size_t> neart;
			CRN_FOREACH(size_t x, s.first)
			{
				int dmin = std::numeric_limits<int>::max();
				size_t p = 0;
				CRN_FOREACH(size_t xx, s.second)
				{
					int d = crn::Abs(int(x) - int(xx));
					if (d < dmin)
					{
						dmin = d;
						p = xx;
					}
				}
				neart.push_back((x + p) / 2);
			}
			// for each bottom part find nearest top part
			std::vector<size_t> nearb;
			CRN_FOREACH(size_t x, s.second)
			{
				int dmin = std::numeric_limits<int>::max();
				size_t p = 0;
				CRN_FOREACH(size_t xx, s.first)
				{
					int d = crn::Abs(int(x) - int(xx));
					if (d < dmin)
					{
						dmin = d;
						p = xx;
					}
				}
				nearb.push_back((x + p) / 2);
			}
			// add top parts and long bars
			for (size_t tmp = 0; tmp < s.first.size(); ++tmp)
			{
				bool found = false;
				for (size_t tmp2 = 0; tmp2 < s.second.size(); ++tmp2)
				{
					if (neart[tmp] == nearb[tmp2])
					{
						found = true;
						presig[neart[tmp]] = 'l';
						break;
					}
				}
				if (!found)
					presig[s.first[tmp]] = '\'';
			}
			// add bottom parts and long bars (some are added twice but it's not an issue since we're using a map
			for (size_t tmp = 0; tmp < s.second.size(); ++tmp)
			{
				bool found = false;
				for (size_t tmp2 = 0; tmp2 < s.first.size(); ++tmp2)
				{
					if (nearb[tmp] == neart[tmp2])
					{
						found = true;
						presig[nearb[tmp]] = 'l';
						break;
					}
				}
				if (!found)
					presig[s.second[tmp]] = ',';
			}
		} // merge
	} // foreach black stream

	static int dnum = 0;
	// dots
	CRNImageDoubleGray lvv(diff->MakeLvv());
	CRNImageBW lvvmask(new crn::ImageBW(iw, ih));
	CRNImageBW anglemask(new crn::ImageBW(iw, ih, 255));
	CRNImageBW angle2mask(new crn::ImageBW(iw, ih, 255));
	for (size_t x = 0; x < iw; ++x)
	{
		int basey = At(int(x) + bx) - by;
		for (size_t y = crn::Max(size_t(0), basey - lh / 2); y < crn::Min(ih - 1, basey + lh / 2); ++y)
		{
			lvvmask->SetPixel(x, y, (lvv->GetPixel(x, y) > 0) && diff->IsSignificant(x, y) ? crn::PixelBW::BLACK : crn::PixelBW::WHITE);
			if (diff->IsSignificant(x, y))
			{
				int a = (igr->GetTheta(x, y) + crn::Angle<crn::ByteAngle>(32)).value / 64;
				if (a == 0)
					anglemask->SetPixel(x, y, 0);
				else if (a == 2)
					angle2mask->SetPixel(x, y, 0);
			}
		}
	}
	lb->SubstituteBW(lvvmask);

	// XXX display
	//lb->GetBW()->SavePNG("art lvv mask.png");
	//lb->FlushRGB();

	CRNImageIntGray lvvmap = lb->ExtractCC(L"lvv");
	lb->FilterMinAnd(L"lvv", sw * 2, sw * 2); // TODO tweak that… if there is no noise, make it lower
	CRN_FOREACH(CRNBlock llb, lb->GetTree(L"lvv"))
	{
		int id = llb->GetName().ToInt();
		crn::Rect bbox(llb->GetRelativeBBox());
		std::vector<size_t> histo(8, 0);
		size_t tot = 0;
		CRN_FOREACH(const crn::Point2DInt &p, bbox)
		{
			if (!lvvmask->GetPixel(p.X, p.Y) && diff->IsSignificant(p.X, p.Y))
			{
				histo[(igr->GetTheta(p.X, p.Y) + crn::Angle<crn::ByteAngle>(32)).value / 32] += 1;
				tot += 1;
			}
		}
		size_t m = *std::min_element(histo.begin(), histo.end());
		size_t M = *std::max_element(histo.begin(), histo.end());
		if (tot)
		{
			double mse = 0;
			CRN_FOREACH(size_t cnt, histo)
				mse += crn::Sqr(0.125 - double(cnt) / double(tot));
			mse /= double(histo.size());
			if ((sqrt(mse) <= 0.05) && (double(tot) >= bbox.GetArea() / 2)) // a diamond is half the area of its bounding box
			{
				for (int x = bbox.GetLeft(); x <= bbox.GetRight(); ++x)
					presig[x] = '.';
				//lb->GetRGB()->DrawRect(bbox, crn::PixelRGB(crn::Byte(0), 0, 255)); // XXX display
				//crn::ImageRGB out;
				//out.InitFromImage(*b.GetRGB(), crn::Rect(bx + bbox.GetLeft() - int(lh)/2, by + bbox.GetTop() - int(lh)/2, bx + bbox.GetRight() + int(lh)/2, by + bbox.GetBottom() + int(lh)/2));
				//out.SavePNG(crn::Path("dots/") + dnum++ + crn::Path(".png"));
			}
		}
	}
	lb->GetRGB()->SavePNG("art dots.png");

	static int lnum = 0;
	static int rnum = 0;
	// curves
	lb->SubstituteBW(anglemask);

	// XXX display
	//lb->GetBW()->SavePNG("art angle mask.png");
	//lb->FlushRGB();

	CRNImageIntGray anglemap = lb->ExtractCC(L"angle");
	lb->FilterMinAnd(L"angle", sw * 2, sw * 2);
	lb->FilterMinOr(L"angle", sw, sw);
	CRN_FOREACH(CRNBlock llb, lb->GetTree(L"angle"))
	{
		int id = llb->GetName().ToInt();
		crn::Rect bbox(llb->GetRelativeBBox());
		size_t cave = 0, vex = 0;
		CRN_FOREACH(const crn::Point2DInt &p, bbox)
		{
			if (anglemap->GetPixel(p.X, p.Y) == id)
			{
				if (lvv->GetPixel(p.X, p.Y) >= 0.0)
					vex += 1;
				else
					cave += 1;
			}
		}
		size_t tot = vex + cave;
		if (tot)
		{
			//crn::Byte r = crn::Byte(255 * vex / tot);
			crn::Byte r(255);
			if (double(vex) / double(tot) > 0.75)
			{
				//crn::ImageRGB out;
				//out.InitFromImage(*b.GetRGB(), crn::Rect(bx + bbox.GetLeft() - int(lh)/2, by + bbox.GetTop() - int(lh)/2, bx + bbox.GetRight() + int(lh)/2, by + bbox.GetBottom() + int(lh)/2));
				CRN_FOREACH(const crn::Point2DInt &p, bbox)
				{
					if (anglemap->GetPixel(p.X, p.Y) == id)
					{
						presig[p.X] = '(';
						//lb->GetRGB()->SetPixel(p.X, p.Y, 0, 0, r); // XXX display
						//out.SetPixel(p.X - bbox.GetLeft() + lh/2, p.Y - bbox.GetTop() + lh/2, 255, 0, 255);
					}
				}
				//out.SavePNG(crn::Path("left/") + lnum++ + crn::Path(".png"));
			}
		}
	}
	// XXX display
	//lb->GetRGB()->SavePNG("art angle.png");

	lb->SubstituteBW(angle2mask);
	anglemap = lb->ExtractCC(L"angle2");
	lb->FilterMinAnd(L"angle2", sw * 2, sw * 2);
	lb->FilterMinOr(L"angle2", sw, sw);
	CRN_FOREACH(CRNBlock llb, lb->GetTree(L"angle2"))
	{
		int id = llb->GetName().ToInt();
		crn::Rect bbox(llb->GetRelativeBBox());
		size_t cave = 0, vex = 0;
		CRN_FOREACH(const crn::Point2DInt &p, bbox)
		{
			if (anglemap->GetPixel(p.X, p.Y) == id)
			{
				if (lvv->GetPixel(p.X, p.Y) >= 0.0)
					vex += 1;
				else
					cave += 1;
			}
		}
		size_t tot = vex + cave;
		if (tot)
		{
			//crn::Byte r = crn::Byte(255 * vex / tot);
			crn::Byte r(255);
			if (double(vex) / double(tot) > 0.75)
			{
				//crn::ImageRGB out;
				//out.InitFromImage(*b.GetRGB(), crn::Rect(bx + bbox.GetLeft() - int(lh)/2, by + bbox.GetTop() - int(lh)/2, bx + bbox.GetRight() + int(lh)/2, by + bbox.GetBottom() + int(lh)/2));
				CRN_FOREACH(const crn::Point2DInt &p, bbox)
				{
					if (anglemap->GetPixel(p.X, p.Y) == id)
					{
						presig[p.X] = ')';
						//lb->GetRGB()->SetPixel(p.X, p.Y, 0, 0, r); // XXX display
						//out.SetPixel(p.X - bbox.GetLeft() + lh/2, p.Y - bbox.GetTop() + lh/2, 0, 255, 255);
					}
				}
				//out.SavePNG(crn::Path("right/") + rnum++ + crn::Path(".png"));
			}
		}
	}

	// create the actual signature
	std::vector<ImageSignature> sig;
	char prevs = '\0';
	for (int x = 0; x < presig.size(); ++x)
	{
		// compute the bounding box of the signature element
		int orix = x + bx;
		int oriy = At(orix);
		int y = oriy - by;

		if (presig[x] != '\0')
		{
			if (prevs != presig[x])
			{
				prevs = presig[x];
				sig.push_back(ImageSignature(crn::Rect(orix, oriy - int(lh / 2), orix, oriy + int(lh / 2)), presig[x]));
			}
			else
			{
				sig.back().bbox |= crn::Rect(orix, oriy - int(lh / 2));
				sig.back().bbox |= crn::Rect(orix, oriy + int(lh / 2));
			}
		}
		else
			prevs = '\0';
	}

	// spread bounding boxes
	sig.front().bbox.SetLeft(bx);
	for (size_t tmp = 1; tmp < sig.size(); ++tmp)
	{
		int x1 = sig[tmp - 1].bbox.GetRight();
		int x2 = sig[tmp].bbox.GetLeft();
		int y1 = At((x1 + x2) / 2);
		int y2 = y1 + int(lh / 2);
		y1 -= int(lh / 2);
		int lsum = 0;
		int cutx = x1;
		for (int x = x1; x <= x2; ++x)
		{
			int s = 0;
			for (int y = y1; y <= y2; ++y)
				s += b.GetGray()->GetPixel(x, y);
			if (s > lsum)
			{
				lsum = s;
				cutx = x;
			}
		}
		if (sig[tmp - 1].bbox.GetLeft() < (cutx - 1))
			sig[tmp - 1].bbox.SetRight(cutx - 1);
		else
			sig[tmp - 1].bbox.SetRight(cutx);
		sig[tmp].bbox.SetLeft(cutx);
	}
	sig.back().bbox.SetRight(ex);
	
#if 0
	CRN_FOREACH(const ImageSignature &is, sig)
	{ // XXX display
		crn::Byte re = 0, gr = 0, bl = 0;
		switch (is.code)
		{
			case ',':
				gr = 255;
				break;
			case '\'':
				bl = 255;
				break;
			case 'l':
				re = 255;
				break;
			case '.':
				re = gr = bl = 255;
				break;
			case '(':
				re = bl = 255;
				break;
			case ')':
				gr = bl = 255;
				break;
		}
		/*
		CRN_FOREACH(const crn::Point2DInt &p, is.bbox)
			if ((p.X + p.Y) % 2)
				lb->GetRGB()->SetPixel(crn::Cap(p.X - bx, 0, iw - 1), crn::Cap(p.Y - by, 0, ih - 1), re, gr, bl);
				*/
		b.GetRGB()->DrawRect(is.bbox, crn::PixelRGB(re, gr, bl), false);
		/*
		crn::Rect tbox(is.bbox);
		tbox.Translate(-bx, -by);
		lb->GetRGB()->DrawRect(tbox, crn::PixelRGB(re, gr, bl), false);
		*/
	}
#endif

	features = sig;

	//std::cout << std::endl;
	//lb->GetRGB()->SavePNG("xxx line sig.png");

	b.RemoveChild(L"lines", lb);
	return sig;
}

void GraphicalLine::deserialize(crn::xml::Element &el)
{
	crn::xml::Element lel(el.GetFirstChildElement("LinearInterpolation"));
	CRNLinearInterpolation lin(crn::DataFactory::CreateData(lel)); // might throw
	int nlh = el.GetAttribute<int>("lh", false); // might throw
	//int nlumthresh = el.GetAttribute<int>("lum", false); // might throw
	midline = lin;
	lh = nlh;
	//lumthresh = nlumthresh;
	try { left = el.GetAttribute<int>("left", false); }
	catch (...) { left = int(lin->GetData().front().X); }
	try { right = el.GetAttribute<int>("right", false); }
	catch (...) { right = int(lin->GetData().back().X); }
	crn::xml::Element fel(el.GetFirstChildElement("signature"));
	if (fel)
	{
		try
		{
			std::vector<ImageSignature> sig;
			for (crn::xml::Element sel = fel.BeginElement(); sel != fel.EndElement(); ++sel)
			{
				crn::Rect r;
				r.Deserialize(sel);
				sig.push_back(ImageSignature(r, sel.GetAttribute<crn::StringUTF8>("code")[0]));
			}
			features.swap(sig);
		}
		catch (...) { }
	}
}

crn::xml::Element GraphicalLine::serialize(crn::xml::Element &parent) const
{
	crn::xml::Element el(parent.PushBackElement(GetClassName().CStr()));
	midline->Serialize(el);
	el.SetAttribute("lh", int(lh));
	//el.SetAttribute("lum", lumthresh);
	el.SetAttribute("left", left);
	el.SetAttribute("right", right);
	if (!features.empty())
	{
		crn::xml::Element fel(el.PushBackElement("signature"));
		CRN_FOREACH(const ImageSignature &s, features)
		{
			crn::xml::Element sel(s.bbox.Serialize(fel));
			sel.SetAttribute("code", crn::StringUTF8(s.code));
		}
	}
	return el;
}

CRN_BEGIN_CLASS_CONSTRUCTOR(GraphicalLine)
	CRN_DATA_FACTORY_REGISTER(STR("GraphicalLine"), GraphicalLine)
CRN_END_CLASS_CONSTRUCTOR(GraphicalLine)

