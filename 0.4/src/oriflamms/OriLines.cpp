/* Copyright 2013-2015 INSA-Lyon, IRHT, ZHAO Xiaojuan
 *
 * file: OriLines.cpp
 * \author Yann LEYDIER
 */

#include <OriLines.h>
#include <CRNMath/CRNLinearInterpolation.h>
#include <CRNImage/CRNColorModel.h>
#include <CRNMath/CRNMatrixComplex.h>
#include <CRNAI/CRN2Means.h>
#include <CRNUtils/CRNXml.h>
#include <CRNMath/CRNPolynomialRegression.h>
#include <CRNImage/CRNDifferential.h>
#include <CRNImage/CRNImageBW.h>
#include <OriStruct.h>
#include <CRNMath/CRNMatrixComplex.h>
#include <numeric>
#include <math.h>
#include <CRNi18n.h>

#include <iostream>

using namespace crn;
using namespace literals;

static std::vector<Rect> detectColumns(const ImageIntGray &ydiff, size_t ncols)
{
	auto ig2 = ydiff;
	Abs(ig2);
	ig2.Negative();
	// projection of line gradients
	auto vp = VerticalProjection(Fisher(ig2));
	ig2 = ImageIntGray{}; // free memory

	// check number of modes for each Y in the histogram
	const size_t max = vp.Max();
	auto modesh = std::vector<size_t>();
	for (size_t y = 0; y < max; ++y)
	{
		int t1 = 0, t2 = 0;
		for (size_t x = 0; x < vp.Size() - 1; ++x)
		{
			bool in1 = vp.GetBin(x) > y;
			bool in2 = vp.GetBin(x + 1) > y;
			if (!in1 && in2)
			{
				t1 += 1; // entering the data
			}
			else if (in1 && !in2)
			{
				t2 += 1; // leaving the data
			}
		}
		int nmodes = Max(t1, t2);
		if (nmodes == ncols)
			modesh.push_back(y);
	}

	if (modesh.empty())
	{ // white page
		auto thumbzones = std::vector<Rect>{};
		int bx = 0;
		const int w = int(ydiff.GetWidth()) / int(ncols);
		const int h = int(ydiff.GetHeight()) - 1;
		for (size_t tmp = 0; tmp < ncols; ++tmp)
		{ // cut the page regularly
			thumbzones.emplace_back(bx, 0, bx + w - 1, h);
			bx += w;
		}
		return thumbzones;
	}

	//std::cout << "************** " << nmodes << std::endl;

	bool in = false;
	auto th = modesh[modesh.size() / 2]; // median Y for the correct number of modes/columns
	//std::cout << th << std::endl;
	int bx = 0;
	auto thumbzones = std::vector<Rect>{};
	for (size_t x = 0; x < vp.Size(); ++x)
	{
		if (in)
		{
			if (vp.GetBin(x) <= th)
			{
				thumbzones.emplace_back(bx, 0, int(x) - 1, int(ydiff.GetHeight()) - 1);
				in = false;
			}
		}
		else
		{
			if (vp.GetBin(x) > th)
			{
				bx = int(x);
				in = true;
			}
		}
	}
	if (in)
	{
		thumbzones.emplace_back(bx, 0, int(ydiff.GetWidth()) - 1, int(ydiff.GetHeight()) - 1);
	}
	return thumbzones;
}

static int lumdiff(const ImageGray &ig, int cx, int cy, int dx, int dy, const ImageGray &mask)
{
	int minv = std::numeric_limits<int>::max(), maxv = 0;
	for (int y = cy - dy; y <= cy + dy; ++y)
	{
		if ((y < 0) || (y >= ig.GetHeight()))
			continue;
		for (int x = cx - dx; x <= cx + dx; ++x)
		{
			if ((x < 0) || (x >= ig.GetWidth()) || mask.At(x, y))
				continue;
			auto val = ig.At(x, y);
			if (val > maxv) maxv = val;
			if (val < minv) minv = val;
		}
	}
	if ((minv == std::numeric_limits<int>::max()) || (maxv == 0))
		return 0;
	else
		return maxv - minv;
}

static int mingrad(const ImageGradient &ig, int cx, int cy, int dx, int dy, const ImageBW &mask)
{
	int minv = std::numeric_limits<int>::max();
	for (int y = cy - dy; y <= cy + dy; ++y)
	{
		if ((y < 0) || (y >= ig.GetHeight()))
			continue;
		for (int x = cx - dx; x <= cx + dx; ++x)
		{
			if ((x < 0) || (x >= ig.GetWidth()) || mask.At(x, y))
				continue;
			auto val = ig.At(x, y).rho;
			if (val < minv) minv = val;
		}
	}
	return minv;
}

static int maxgrad(const ImageGradient &ig, int cx, int cy, int dx, int dy, const ImageBW &mask)
{
	int maxv = 0;
	for (int y = cy - dy; y <= cy + dy; ++y)
	{
		if ((y < 0) || (y >= ig.GetHeight()))
			continue;
		for (int x = cx - dx; x <= cx + dx; ++x)
		{
			if ((x < 0) || (x >= ig.GetWidth()) || mask.At(x, y))
				continue;
			auto val = ig.At(x, y).rho;
			if (val > maxv) maxv = val;
		}
	}
	return maxv;
}

static SLinearInterpolation cut_line(LinearInterpolation &l, Block &b, size_t sw, int th, const ImageBW &enmask, const Rect &clip)
{
	if (l.GetData().size() <= 2)
		return SLinearInterpolation{};

	int bx = int(l.GetData().front().X);
	int ex = int(l.GetData().back().X);

	bool can_enlarge_b = true, can_enlarge_e = true;

	// cut at enlightened marks
	int cx = (bx + ex) / 2;
	for (int x = cx; x > bx; --x)
		if (enmask.At(x, l[x]) != 0)
		{
			bx = x;
			can_enlarge_b = false;
			break;
		}
	for (int x = cx + 1; x < ex; ++x)
		if (enmask.At(x, l[x]) != 0)
		{
			ex = x;
			can_enlarge_e = false;
			break;
		}

	//if (lumdiff(*b.GetGray(), bx + 2*sw, l[bx], 2*sw, 3*sw, enmask) > th)
	if (maxgrad(*b.GetGradient(), bx + 2*int(sw), l[bx], 2*int(sw), 3*int(sw), enmask) > th)
	{
		// grow
		if (can_enlarge_b)
		{
			int x = bx - 1;
			for (; x >= clip.GetLeft() + sw; --x)
				//if (lumdiff(*b.GetGray(), x + 2*sw, l[x], 2*sw, 3*sw, enmask) < th)
				if ((maxgrad(*b.GetGradient(), x + 2*int(sw), l[x], 2*int(sw), 3*int(sw), enmask) < th) || enmask.At(x, l[x]))
					break;
			bx = x + 2*int(sw);
		}
	}
	else
	{
		// shrink
		int x = bx + 1;
		for (; x < clip.GetRight() - sw; ++x)
			//if (lumdiff(*b.GetGray(), x + 2*sw, l[x], 2*sw, 3*sw, enmask) > th)
			if ((maxgrad(*b.GetGradient(), x + 2*int(sw), l[x], 2*int(sw), 3*int(sw), enmask) > th) || enmask.At(x, l[x]))
				break;
		bx = x;
	}

	//if (lumdiff(*b.GetGray(), ex - 2*sw, l[ex], 2*sw, 3*sw, enmask) > th)
	if (maxgrad(*b.GetGradient(), ex - 2*int(sw), l[ex], 2*int(sw), 3*int(sw), enmask) > th)
	{
		// grow
		if  (can_enlarge_e)
		{
			int x = ex + 1;
			for (; x < clip.GetRight() - sw; ++x)
				//if (lumdiff(*b.GetGray(), x - 2*sw, l[x], 2*sw, 3*sw, enmask) < th)
				if ((maxgrad(*b.GetGradient(), x - 2*int(sw), l[x], 2*int(sw), 3*int(sw), enmask) < th) || enmask.At(x, l[x]))
					break;
			ex = x - 2*int(sw);
		}
	}
	else
	{
		// shrink
		int x = ex - 1;
		for (; x >= clip.GetLeft() +	sw; --x)
			//if (lumdiff(*b.GetGray(), x - 2*sw, l[x], 2*sw, 3*sw, enmask) > th)
			if ((maxgrad(*b.GetGradient(), x - 2*int(sw), l[x], 2*int(sw), 3*int(sw), enmask) > th) || enmask.At(x, l[x]))
				break;
		ex = x;
	}

	if (ex <= bx)
		return SLinearInterpolation{};
	// XXX display
	b.GetRGB()->DrawLine(bx, l[bx] - 3*sw, bx, l[bx] + 3*sw, pixel::RGB8(0, 200, 0));
	b.GetRGB()->DrawLine(ex, l[ex] - 3*sw, ex, l[ex] + 3*sw, pixel::RGB8(255, 0, 0));
	// XXX

	// create new line
	auto newline = std::vector<Point2DDouble>{};
	if (bx < l.GetData().front().X) // first point out of the line
		newline.emplace_back(bx, l.GetData().front().Y);
	else // first point in the line
		newline.emplace_back(bx, l[bx]);
	for (const Point2DDouble &p : l.GetData()) // middle points
		if ((p.X > bx) && (p.X < ex))
			newline.push_back(p);
	if (newline.empty())
		return SLinearInterpolation{};
	if (ex > newline.back().X) // end point out of the line
		newline.emplace_back(ex, newline.back().Y);
	else // end point in the line
		newline.emplace_back(ex, l[ex]);
	if (newline.size() < 2)
		return SLinearInterpolation{};
	return std::make_shared<LinearInterpolation>(newline.begin(), newline.end());
}

struct LineSorter: public std::binary_function<const SLinearInterpolation&, const SLinearInterpolation&, bool>
{
	inline bool operator()(const SLinearInterpolation &l1, const SLinearInterpolation &l2) const
	{
		return l1->GetData().front().Y < l2->GetData().front().Y;
	}
};

/*! Detects lines and columns on an image
 *
 * \warning	an image of two one-columned pages returns the same results as an image of a one two-columned page
 *
 * \param[in]	b	the block to analyze
 * \return	a vector of columns, columns being vectors of lines, a line being a LinearInterperlation
 */
SVector ori::DetectLines(Block &b, const View &view)
{
	const size_t sw = StrokesWidth(*b.GetGray());
	const size_t w = b.GetGray()->GetWidth();
	const size_t h = b.GetGray()->GetHeight();
	const size_t lspace1 = EstimateLeading(*b.GetGray());
	auto igr = b.GetGradient(true, double(sw)); // precompute with a huge sigma

	//////////////////////////////////////////////////////////////
	// Borders of the page
	//////////////////////////////////////////////////////////////
	// project horizontal gradients
	auto cropbw = std::make_shared<ImageBW>(w, h, pixel::BWWhite);
	for (auto tmp : Range(*igr))
		if (igr->IsSignificant(tmp) && ((AngularDistance(igr->At(tmp).theta, Angle<ByteAngle>::LEFT) < 32) ||
				(AngularDistance(igr->At(tmp).theta, Angle<ByteAngle>::RIGHT) < 32)))
			cropbw->At(tmp) = pixel::BWBlack;

	auto vproj = VerticalProjection(*cropbw);
	// look for big mods
	vproj.AverageSmoothing(sw / 2);
	auto vmodes = vproj.Modes();
	auto okmodes = std::vector<size_t>{};
	for (auto m : vmodes)
		if (vproj[m] > h / 3)
			okmodes.push_back(m);

	// copy horizontal gradients crossing the big mods to the mask
	auto cropb = Block::New(cropbw);
	auto cropmap = cropb->ExtractCC(U"cc");
	auto ccs = cropb->GetTree(U"cc");
	auto enmask = ImageBW(w, h, pixel::BWBlack);
	if (ccs)
		for (const auto &o : *ccs)
		{
			auto cc = std::static_pointer_cast<Block>(o);
			auto num = cc->GetName().ToInt();
			for (auto x : okmodes)
				if (cc->GetAbsoluteBBox().Contains(int(x), cc->GetAbsoluteBBox().GetCenterY()))
				{
					for (const auto &p : cc->GetAbsoluteBBox())
						if (cropmap->At(p.X, p.Y) == num)
						{
							enmask.At(p.X, p.Y) = pixel::BWWhite;
						}
					break;
				}
		}

	//////////////////////////////////////////////////////////////
	// thumbnail
	//////////////////////////////////////////////////////////////
	const size_t xdiv = sw * 2;
	const size_t ydiv = 1;
	const size_t nw = w / xdiv;
	const size_t nh = h / ydiv;
	const size_t lspace = lspace1 / ydiv;
	ImageIntGray ig(nw, nh);
	std::vector<Angle<ByteAngle>> hues; // used to compute mean hue
	for (size_t y = 0; y < nh; ++y)
		for (size_t x = 0; x < nw; ++x)
		{
			int mval = 255;
			//int mval = 0;
			size_t okx = x * xdiv, oky = y * ydiv;
			for (size_t ty = y * ydiv; ty < Min(h, (y + 1) * ydiv); ++ty)
				for (size_t tx = x * xdiv; tx < Min(w, (x + 1) * xdiv); ++tx)
				{
					int v = b.GetGray()->At(tx, ty);
					//int v = igr->GetRhoPixels()[tx + ty * w];
					if (v < mval)
					//if (v > mval)
					{
						mval = v;
						okx = tx;
						oky = ty;
					}
				}
			// thumbnail
			ig.At(x, y) = mval;
			// hue
			pixel::HSV hsvpix = b.GetRGB()->At(okx, oky);
			hues.emplace_back(hsvpix.h);
		}
	//ig.Negative();
	// x smoothing
	ig.Convolve(MatrixDouble::NewGaussianLine(10.0));

	//////////////////////////////////////////////////////////////
	// compute enlightened marks' mask
	//////////////////////////////////////////////////////////////
	Angle<ByteAngle> meanhue = AngularMean(hues.begin(), hues.end());
	int stdhue = 4 * int(sqrt(AngularVariance(hues.begin(), hues.end(), meanhue)));
	// create a mask of colored parts
	SImageBW colormask(std::make_shared<ImageBW>(w, h, pixel::BWWhite));
	for (auto tmp : Range(*b.GetRGB()))
	{
		pixel::HSV hsvpix(b.GetRGB()->At(tmp));
		if (((hsvpix.s > 127) && (hsvpix.v > 127)) || // colorful
				(AngularDistance(Angle<ByteAngle>(hsvpix.h), meanhue) > stdhue)) // not the main color
		{
			colormask->At(tmp) = pixel::BWBlack;
		}
	}
	// filter the color mask
	SBlock colorb(Block::New(colormask));
	auto colormap = colorb->ExtractCC(U"cc");
	if (colorb->HasTree(U"cc"))
	{
		for (Block::block_iterator it = colorb->BlockBegin(U"cc"); it != colorb->BlockEnd(U"cc"); ++it)
		{
			int num = it.AsBlock()->GetName().ToInt();
			if (/*(it->GetAbsoluteBBox().GetWidth() < 2 * sw) || */(it.AsBlock()->GetAbsoluteBBox().GetHeight() > 2 * lspace1))
			{
				for (Block::pixel_iterator pit = colorb->PixelBegin(it.AsBlock()); pit != colorb->PixelEnd(it.AsBlock()); ++pit)
				{
					if (colormap->At(pit->X, pit->Y) == num)
					{
						enmask.At(pit->X, pit->Y) = pixel::BWWhite;
					}
				}
			}
		}
	}
	colormask.reset();
	colorb.reset();
	colormap.reset();
	// fill the horizontal gaps in the enlightened marks' mask
	for (size_t y = 0; y < h; ++y)
	{
		size_t bx = 0;
		for (size_t x = 0; x < w; ++x)
		{
			if (enmask.At(x, y))
			{
				b.GetRGB()->At(x, y) = {255, 255, 0};
				int diff = int(x) - int(bx);
				if ((diff > 1) && (diff < lspace1/2))
					for (size_t tx = bx + 1; tx < x; ++tx)
					{
						enmask.At(tx, y) = pixel::BWBlack;
						b.GetRGB()->At(tx, y) = {255, 255, 0};
					}
				bx = x;
			}
		}
	}
	//enmask.SavePNG("enmask.png"); // XXX display

	//////////////////////////////////////////////////////////////
	// vertical differential
	//////////////////////////////////////////////////////////////
	MatrixDouble ydiff(MatrixDouble::NewGaussianLineDerivative(double(lspace) / 6.0));
	ydiff.Transpose();
	ydiff.NormalizeForConvolution();
	ig.Convolve(ydiff);

	// Columns
	std::vector<Rect> thumbzones(detectColumns(ig, view.GetColumns().size()));
	for (const Rect &r : thumbzones)
	{
		for (size_t x = r.GetLeft() * xdiv; x < r.GetRight() * xdiv; ++x)
			for (size_t y = 0; y < h; ++y)
				b.GetRGB()->At(x, y).b = 0;
	}

	//////////////////////////////////////////////////////////////
	// find lines
	//////////////////////////////////////////////////////////////
	// create mask of line guides
	auto mask = ImageBW{nw, nh, pixel::BWBlack};
	size_t maxig = 0;
	for (auto tmp : Range(ig))
		if (Abs(ig.At(tmp)) > maxig) maxig = Abs(ig.At(tmp));
	auto linehist = Histogram{maxig + 1}; // histogram of absolute values of the vertical derivative
	for (auto tmp : Range(ig))
		linehist.IncBin(Abs(ig.At(tmp)));
	auto linethresh = linehist.Fisher(); // derivative threshold
	for (const Rect &tz : thumbzones)
	{
		for (size_t y = tz.GetTop(); y < tz.GetBottom() - 1; ++y)
			for (size_t x = tz.GetLeft(); x < tz.GetRight(); ++x)
			{ // inside each column
				if ((ig.At(x, y) >= 0) && (ig.At(x, y + 1) < 0))
				{ // found a point on a median line
					bool ok = false;
					for (size_t ty = Max((size_t)tz.GetTop(), y - lspace/4); ty < Min((size_t)tz.GetBottom(), y + lspace/4); ++ty)
						if (Abs(ig.At(x, ty)) > linethresh)
						{ // found a significant gradient around the point
							ok = true;
							break;
						}
					if (ok)
						mask.At(x, y) = pixel::BWWhite;
				}
			}
	}

	// extract lines
	auto cols = std::make_shared<Vector>(Protocol::Serializable);
	const int line_x_search = 20;
	const int line_y_search = 10;
	for (size_t tmpz = 0; tmpz < thumbzones.size(); ++tmpz)
	{ // for each column
		const Rect &tz(thumbzones[tmpz]);
		auto protolines = std::vector<std::vector<Point2DInt>>{};
		for (size_t x = tz.GetLeft(); x < tz.GetRight(); ++x)
			for (size_t y = tz.GetTop(); y < tz.GetBottom(); ++y)
				if (mask.At(x, y))
				{
					// found a line guide
					mask.At(x, y) = pixel::BWBlack; // erase from mask
					size_t x1 = x;
					size_t y1 = y;
					bool found = true;
					auto points = std::vector<Point2DInt>{};
					points.emplace_back(int(x1 * xdiv), int(y1 * ydiv));
					while (found)
					{
						found = false;
						size_t by = Max(0, int(y1) - line_y_search);
						size_t ey = Min(nh, y1 + line_y_search);
						for (size_t x2 = x1; x2 < Min(size_t(tz.GetRight()), x1 + line_x_search); ++x2)
							for (size_t y2 = by; (y2 < ey) && !found; ++y2)
								if (mask.At(x2, y2))
								{
									// found a line guide
									mask.At(x2, y2) = pixel::BWBlack; // erase from mask
									y1 = y2;
									x1 = x2;
									points.emplace_back(int(x1 * xdiv), int(y1 * ydiv));
									found = true;
									break;
								}
					}
					if (points.size() > 2)
					{
						// merge ?
						auto candidates = std::map<int, size_t>{};
						for (size_t tmp = 0; tmp < protolines.size(); ++tmp)
						{
							if (protolines[tmp].back().X < points.front().X)
							{
								int ly1 = Min(protolines[tmp].front().Y, protolines[tmp].back().Y);
								int ly2 = Max(protolines[tmp].front().Y, protolines[tmp].back().Y);
								int py1 = Min(points.front().Y, points.back().Y);
								int py2 = Max(points.front().Y, points.back().Y);
								int dist = Min(ly2, py2) - Max(ly1, py1);
								if (dist > -int(lspace1)) // TODO maybe /2
									candidates.emplace(dist, tmp);
							}
						}
						if (candidates.empty())
							protolines.push_back(std::move(points));
						else
							protolines[candidates.rbegin()->second].insert(protolines[candidates.rbegin()->second].end(), points.begin(), points.end());
					}
				}

		auto lines = std::vector<SLinearInterpolation>{};
		for (const auto &l : protolines)
			lines.push_back(std::make_shared<LinearInterpolation>(l.begin(), l.end()));

		// compute thresholds
		auto thresholds = std::vector<int>{};
		for (const SLinearInterpolation &l : lines)
		{
			auto diff = std::vector<int>{};
			for (int x = int(l->GetData().front().X); x <= int(l->GetData().back().X); ++x)
			{
				int ty = (*l)[x];
				//diff.push_back(lumdiff(*b.GetGray(), x, ty, 0, int(3*sw), enmask));
				diff.push_back(maxgrad(*igr, x, ty, 0, int(3*sw), enmask));
				diff.push_back(mingrad(*igr, x, ty, 0, int(3*sw), enmask));
			}
			//auto mM = TwoMeans(diff.begin(), diff.end());
			//thresholds.push_back((mM.first + mM.second) / 4); // XXX whhhhhhhhhhhhhhhhhhy /4 ???
			thresholds.push_back(std::accumulate(diff.begin(), diff.end(), 0) / int(diff.size()));
		}
		//auto thr = TwoMeans(thresholds.begin(), thresholds.end()).second;

		// cut lines
		auto filteredlines = std::vector<SLinearInterpolation>{};
		for (size_t tmp = 0; tmp < lines.size(); ++tmp)
		{
			int left = int(tz.GetLeft() * xdiv);
			if (tmpz == 0) left = 0; // grow first zone to beginning
			else left = int(left + thumbzones[tmpz - 1].GetRight() * xdiv) / 2; // grow to half the distance to previous zone
			int right = int(tz.GetRight() * xdiv);
			if (tmpz == thumbzones.size() - 1) right = b.GetAbsoluteBBox().GetRight(); // grow last zone to end
			else right = int(right + thumbzones[tmpz + 1].GetLeft() * xdiv) / 2; // grow to half the distance to next zone

			//auto fl = cut_line(*lines[tmp], b, sw, thr, enmask, Rect{left, int(tz.GetTop() * ydiv), right, int(tz.GetBottom() * ydiv)});
			auto fl = cut_line(*lines[tmp], b, sw, thresholds[tmp], enmask, Rect{left, int(tz.GetTop() * ydiv), right, int(tz.GetBottom() * ydiv)});
			if (fl)
				filteredlines.push_back(fl);

			// XXX DISPLAY
			for (int x = int(lines[tmp]->GetData().front().X); x < int(lines[tmp]->GetData().back().X); ++x)
				b.GetRGB()->At(x, (*lines[tmp])[x]) = {0, 0, 255};
			if (fl)
				for (int x = int(fl->GetData().front().X); x < int(fl->GetData().back().X); ++x)
					b.GetRGB()->At(x, (*fl)[x]) = {0, 200, 0};
		}
		lines.swap(filteredlines);

		/////////////////////////////////////////////////////////////
		// Remove supernumerary lines
		/////////////////////////////////////////////////////////////
		auto nlines = view.GetColumns()[tmpz].GetLines().size();
		if (nlines < lines.size())
		{
			int fw = tz.GetWidth() * int(xdiv);
			while (log2(fw) != floor(log2(fw))) fw += 1;
			
			auto distmat = std::vector<std::vector<double>>(lines.size(), std::vector<double>(lines.size(), 0.0));
			for (size_t l1 = 0; l1 < lines.size(); ++l1)
			{ // for each line
				auto bx = Max(tz.GetLeft(), int(lines[l1]->GetData().front().X));
				auto ex = Min(tz.GetRight(), int(lines[l1]->GetData().back().X));
				auto sig = std::vector<std::complex<double>>(fw, std::complex<double>{0.0, 0.0});
				for (int x = bx; x <= ex; ++x)
					sig[x - bx].real(b.GetGray()->At(x, (*lines[l1])[x]));
				FFT(sig, true);
				for (size_t l2 = l1 + 1; l2 < lines.size(); ++l2)
				{
					bx = Max(tz.GetLeft(), int(lines[l2]->GetData().front().X));
					ex = Min(tz.GetRight(), int(lines[l2]->GetData().back().X));
					auto sig2 = std::vector<std::complex<double>>(fw, std::complex<double>{0.0, 0.0});
					for (int x = bx; x <= ex; ++x)
						sig2[x - bx].real(b.GetGray()->At(x, (*lines[l2])[x]));
					FFT(sig2, true); // TODO optimize: compute only once

					double d = 0.0;
					for (size_t tmp = 0; tmp < fw; ++tmp)
						d += std::norm(sig[tmp] * sig2[tmp]);
					distmat[l1][l2] = distmat[l2][l1] = 1/d;
				}
			}

			// central objects
			auto lsum = std::vector<double>(lines.size());
			std::transform(distmat.begin(), distmat.end(), lsum.begin(), 
					[](const std::vector<double> &l){ return std::accumulate(l.begin(), l.end(), 0.0); });
			
			auto outmap = std::multimap<double, SLinearInterpolation>{};
			for (size_t l1 = 0; l1 < lines.size(); ++l1)
			{
				//outmap.emplace(std::accumulate(distmat[l1].begin(), distmat[l1].end(), 0.0), lines[l1]);
				auto v = 0.0;
				for (size_t l2 = 0; l2 < lines.size(); ++l2)
					v += distmat[l2][l1] / lsum[l2];
				outmap.emplace(v, lines[l1]);
			}

			auto it = outmap.begin();
			std::advance(it, nlines);
			{ // XXX DISPLAY
				for (auto iit = it; iit != outmap.end(); ++iit)
				{
					for (int x = int(iit->second->GetData().front().X); x < int(iit->second->GetData().back().X); ++x)
						b.GetRGB()->At(x, (*iit->second)[x]) = {255, 0, 0};
				}
			}
			outmap.erase(it, outmap.end());
			auto newlines = std::vector<SLinearInterpolation>{};
			for (const auto &p : outmap)
				newlines.push_back(p.second);
			lines.swap(newlines);
		}

		if (!lines.empty())
		{
			auto linestart = std::vector<Point2DDouble>{};
			for (const auto l : lines)
				linestart.emplace_back(l->GetData().front().Y, l->GetData().front().X);
			if (linestart.size() > 3)
			{
				/////////////////////////////////////////////////////////////
				// Cut the beginning of lines that caught an enlightened mark
				/////////////////////////////////////////////////////////////

				// median line start
				std::vector<double> posx;
				for (const Point2DDouble &p : linestart)
				{
					posx.push_back(p.Y);
				}
				std::sort(posx.begin(), posx.end());
				double meanx = posx[posx.size() / 2];
				b.GetRGB()->DrawLine(int(meanx), 0, int(meanx), b.GetAbsoluteBBox().GetBottom(), pixel::RGB8(0, 0, 255)); // XXX DISPLAY
				// keep only nearest lines starts
				std::vector<Point2DDouble> linestart2;
				for (const Point2DDouble &p : linestart)
				{
					double d = Abs(p.Y - meanx);
					if (d < lspace1 / 2) // heuristic :o(
						linestart2.push_back(p);
				}
				if (linestart2.size() > 3)
				{
					PolynomialRegression reg(linestart2.begin(), linestart2.end(), 1);
					b.GetRGB()->DrawLine(reg[0], 0, reg[b.GetAbsoluteBBox().GetBottom()], b.GetAbsoluteBBox().GetBottom(), pixel::RGB8(255, 0, 0)); // XXX DISPLAY
					// cut lines
					for (SLinearInterpolation &l : lines)
					{
						double margin = reg[l->GetData().front().Y];
						double d = margin - l->GetData().front().X; // distance to regression of the margin
						if (d > 2 * sw) // heuristic :o(
						{
							// the line goes too far, we're cutting it at the margin
							std::vector<Point2DDouble> newline;
							newline.emplace_back(Point2DDouble(margin, (*l)[margin]));
							for (const Point2DDouble &p : l->GetData())
							{
								if (p.X > margin)
									newline.push_back(p);
							}
							//int thresh = filteredthresh[l];
							if (newline.size() > 2)
								newline = SimplifyCurve(newline, 0.005, 0.01);
							else
								newline = SimplifyCurve(l->GetData(), 0.005, 0.01);
							l = std::make_shared<LinearInterpolation>(newline.begin(), newline.end());
							// TODO XXX ELSE WHAT ???
							//filteredthresh[l] = thresh;

							// XXX DISPLAY
							Rect r(int(l->GetData().front().X) - 20, int(l->GetData().front().Y) - 20, int(l->GetData().front().X) + 20, int(l->GetData().front().Y) + 20);
							for (const Point2DInt &p : r)
							{
								b.GetRGB()->At(p.X, p.Y).g = 0;
								b.GetRGB()->At(p.X, p.Y).b = 0;
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
			SVector col(std::make_shared<Vector>(Protocol::Serializable));
			for (const SLinearInterpolation &l : lines)
				col->PushBack(std::make_shared<ori::GraphicalLine>(l, lspace1/*, filteredthresh[l]*/));
			cols->PushBack(col);
		}
	}

	return cols;
}

template<typename T> std::vector<T> doSimplify(const std::vector<T> &line, double dist, double d)
{
	std::vector<T> simplified_line;
	if (line.size() > 6)
	{
		simplified_line.push_back(line[0]);
		simplified_line.push_back(line[1]);
		size_t id = 0;
		while (id < line.size() - 5)
		{
			size_t jump = 6;
			size_t id_end = simplified_line.size() - 1;
			Angle<Radian> angle1 = Angle<Radian>::Atan(simplified_line[id_end].Y-simplified_line[id_end - 1].Y, simplified_line[id_end].X-simplified_line[id_end - 1].X);
			for (size_t n = 0; n < 4; ++n)
			{
				Angle<Radian> angle2 = Angle<Radian>::Atan(line[id + n + 1].Y-simplified_line[id_end].Y, line[id + n + 1].X-simplified_line[id_end].X);
				Angle<Radian> disangle = AngularDistance(angle1, angle2);
				double di = sqrt(Sqr(line[id + n + 1].Y-simplified_line[id_end].Y) + Sqr(line[id + n + 1].X-simplified_line[id_end].X))*sin(disangle.value);

				size_t last = simplified_line.size() - 1;
				Angle<Radian> angleA = Angle<Radian>::Atan(simplified_line[last].Y-simplified_line[last - 1].Y, simplified_line[last].X-simplified_line[last - 1].X);
				Angle<Radian> angleB = Angle<Radian>::Atan(line[id + n + 1].Y-simplified_line[last].Y, line[id + n + 1].X-simplified_line[last].X);
				Angle<Radian> disan = AngularDistance(angleA, angleB);
				if (disan.value < d)
				{
					simplified_line.back() = line[id+ n +1];
					//id_end = id+ n +1;
					jump = n + 1;
				}
				else
				{
					if(di > dist)
					{
						simplified_line.push_back(line[id+ n +1]);
						//id_end = id+ n +1;
						jump = n + 1;
						id += jump; ;
						break;
					}
				}
			}
			id += jump; ;
		}
		simplified_line.push_back(line.back());
	}
	else
		simplified_line = line;

	return simplified_line;
}

std::vector<Point2DInt> ori::SimplifyCurve(const std::vector<Point2DInt> &line, double dist, double d)
{
	return doSimplify(line, dist, d);
}

std::vector<Point2DDouble> ori::SimplifyCurve(const std::vector<Point2DDouble> &line, double dist, double d)
{
	return doSimplify(line, dist, d);
}

using namespace ori;
GraphicalLine::GraphicalLine(const SLinearInterpolation &lin, size_t lineheight):
	midline(lin),
	lh(lineheight)
{
	if (!lin)
		throw ExceptionInvalidArgument {};
	if (lin->GetData().empty())
		throw ExceptionInvalidArgument {};
}

/*! Gets the signature string of the line
 * \param[in]	b	the image of the whole view
 * \return	a list of signature elements
 */
std::vector<ImageSignature> GraphicalLine::ExtractFeatures(Block &b) const
{
	if (!features.empty())
		return features; // do not recompute

	// create subblock
	int bx = GetFront().X;
	int ex = GetBack().X;
	int by = GetFront().Y - int(lh/2);
	int ey = GetBack().Y + int(lh/2);
	SBlock lb = b.AddChildAbsolute(U"lines", Rect(bx, by, ex, ey));

	Differential diff(Differential::NewGaussian(*lb->GetRGB(), Differential::RGBProjection::ABSMAX, 0));
	size_t sw = StrokesWidth(*b.GetGray());
	diff.Diffuse(sw*1);
	ImageGradient igr(diff.MakeImageGradient());

	// compute horizontal strokes (points between left and right gradients)
	ImageBW strokes(igr.GetWidth(), igr.GetHeight(), pixel::BWBlack);
	const size_t iw = strokes.GetWidth();
	const size_t ih = strokes.GetHeight();
	for (int y = 0; y < ih; ++y)
	{
		int sx = 0;
		bool in = false;
		for (int x = 0; x < iw; ++x)
		{
			if (igr.IsSignificant(x, y))
			{
				int a = (igr.At(x, y).theta + Angle<ByteAngle>(32)).value / 64; // 4 angles (add 32 to rotate the frame 45°)
				if (a == 0) // left
				{
					sx = x;
					in = true;
				}
				else if (a == 2) // right
				{
					if (in)
					{
						// fill
						for (size_t tx = sx; tx <= x; ++tx)
						{
							strokes.At(tx, y) = pixel::BWWhite;
							//lb->GetRGB()->SetPixel(tx, y, 255, 255, 255); // XXX display
						}
					}
					in = false;
				}
				else
				{
					in = false;
				}

			} // is significant
		} // x
	} // y
	//strokes.SavePNG("art strokes.png");

	// project pixels between opposing horizontal gradients, above an below the midline
	Histogram projt(iw);
	Histogram projb(iw);
	//int precx = 0, precy1 = 0, precy2 = 0; // XXX display
	for (int x = 0; x < iw; ++x)
	{
		int y = At(x + bx) - by;
		for (int dy = -int(lh/2); dy < int(lh/2); ++dy)
		{
			if (strokes.At(x, Cap(y + dy, 0, int(ih) - 1)))
			{
				if (dy < 0)
					projt.IncBin(x);
				else
					projb.IncBin(x);
			}
		}

		/*
		// XXX display
		lb->GetRGB()->SetPixel(x, Max(0, y - int(projt[x])), 255, 0, 255);
		lb->GetRGB()->SetPixel(x, Min(int(ih) - 1, y + int(projb[x])), 255, 0, 255);
		*/

		/*
		// XXX display
		if (precy1 == 0) precy1 = y;
		if (precy2 == 0) precy2 = y;
		lb->GetRGB()->DrawLine(precx, precy1, x, Max(0, y - int(projt[x])), PixelRGB(uint8_t(255), 255, 255));
		lb->GetRGB()->DrawLine(precx, precy2, x, Min(int(ih) - 1, y + int(projb[x])), PixelRGB(uint8_t(255), 255, 255));
		precx = x; precy1 = Max(0, y - int(projt[x])); precy2 = Min(int(ih) - 1, y + int(projb[x]));
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
		 lb->GetRGB()->DrawLine(x, 0, x, At(bx + int(x)) - by, PixelRGB(uint8_t(0), 0, 255));
		 CRN_FOREACH(size_t x, modesb) // XXX display
		 lb->GetRGB()->DrawLine(x, At(bx + int(x)) - by, x, ih - 1, PixelRGB(uint8_t(0), 0, 255));
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
	b.GetRGB()->DrawLine(bx + x - 1, At(bx + int(x)) - (projt[x - 1] - mint) / 255, bx + x, At(bx + int(x)) - (projt[x] - mint) / 255, PixelRGB(uint8_t(255), 0, 255));
	b.GetRGB()->DrawLine(bx + x - 1, At(bx + int(x)) + (projb[x - 1] - minb) / 255, bx + x, At(bx + int(x)) + (projb[x] - minb) / 255, PixelRGB(uint8_t(255), 0, 255));
	}
	// XXX display
	*/

	// compute center guide
	std::vector<bool> centerguide(iw, false);
	for (size_t x = 0; x < iw; ++x)
	{
		int y = At(int(x) + bx) - by;
		if (strokes.At(x, y))
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

	// vertical small and long strokes are computed first so that dots and curves can overwrite them
	bool in = false;
	std::vector<char> presig(iw, '\0'); // signature
	std::vector<std::pair<std::vector<size_t>, std::vector<size_t>>> segments; // top and bottom modes that are on a same black stream
	size_t topm = 0, botm = 0; // iterate on the modes
	for (int x = 0; x < iw; ++x)
	{
		int y = At(x + bx) - by;
		if (!in && centerguide[x])
		{
			// enter black stream
			in = true;
		}
		if (!centerguide[x])
		{
			if (in)
			{
				// enter white stream
				in = false;
				// gather modes that were in the black stream
				std::vector<size_t> topseg;
				for (; (topm < modest.size()) && (modest[topm] <= x); ++ topm)
					topseg.push_back(modest[topm]);
				std::vector<size_t> botseg;
				for (; (botm < modesb.size()) && (modesb[botm] <= x); ++ botm)
					botseg.push_back(modesb[botm]);
				segments.emplace_back(std::make_pair(topseg, botseg));
			}
			else
			{
				// already inside a white stream
				// add modes to signature if there are modes here
				if ((topm < modest.size()) && (modest[topm] == x))
					presig[modest[topm++]] = '\'';
				if ((botm < modesb.size()) && (modesb[botm] == x))
				{
					if (presig[modesb[botm]] == '\'')
					{
						// there is already a symbol here, make it a long stroke (not sure I should…)
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
	for (const std::pair<std::vector<size_t>, std::vector<size_t> > &s : segments)
	{
		if (s.first.empty() && s.second.empty())
		{
			// nothing
		}
		else if (s.first.empty())
		{
			// just copy bottom parts
			for (size_t x : s.second)
				presig[x] = ',';
		}
		else if (s.second.empty())
		{
			// just copy top parts
			for (size_t x : s.first)
				presig[x] = '\'';
		}
		else
		{
			// for each top part find nearest bottom part
			std::vector<size_t> neart;
			for (size_t x : s.first)
			{
				int dmin = std::numeric_limits<int>::max();
				size_t p = 0;
				for (size_t xx : s.second)
				{
					int d = Abs(int(x) - int(xx));
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
			for (size_t x : s.second)
			{
				int dmin = std::numeric_limits<int>::max();
				size_t p = 0;
				for (size_t xx : s.first)
				{
					int d = Abs(int(x) - int(xx));
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
	ImageDoubleGray lvv(diff.MakeLvv());
	SImageBW lvvmask(std::make_shared<ImageBW>(iw, ih));
	SImageBW anglemask(std::make_shared<ImageBW>(iw, ih, 255));
	SImageBW angle2mask(std::make_shared<ImageBW>(iw, ih, 255));
	for (size_t x = 0; x < iw; ++x)
	{
		int basey = At(int(x) + bx) - by;
		for (size_t y = Max(size_t(0), basey - lh / 2); y < Min(ih - 1, basey + lh / 2); ++y)
		{
			lvvmask->At(x, y) = (lvv.At(x, y) > 0) && diff.IsSignificant(x, y) ? pixel::BWBlack : pixel::BWWhite;
			if (diff.IsSignificant(x, y))
			{
				int a = (igr.At(x, y).theta + Angle<ByteAngle>(32)).value / 64;
				if (a == 0)
					anglemask->At(x, y) = pixel::BWBlack;
				else if (a == 2)
					angle2mask->At(x, y) = pixel::BWBlack;
			}
		}
	}
	lb->SubstituteBW(lvvmask);

	// XXX display
	//lb->GetBW()->SavePNG("art lvv mask.png");
	//lb->FlushRGB();

	UImageIntGray lvvmap = lb->ExtractCC(U"lvv");
	if (lb->HasTree(U"lvv"))
	{
		lb->FilterMinAnd(U"lvv", sw * 2, sw * 2); // TODO tweak that… if there is no noise, make it lower
		for (const SObject &o : lb->GetTree(U"lvv"))
		{
			SBlock llb(std::static_pointer_cast<Block>(o));
			int id = llb->GetName().ToInt();
			Rect bbox(llb->GetRelativeBBox());
			std::vector<size_t> histo(8, 0);
			size_t tot = 0;
			for (const Point2DInt &p : bbox)
			{
				if (!lvvmask->At(p.X, p.Y) && diff.IsSignificant(p.X, p.Y))
				{
					histo[(igr.At(p.X, p.Y).theta + Angle<ByteAngle>(32)).value / 32] += 1;
					tot += 1;
				}
			}
			size_t m = *std::min_element(histo.begin(), histo.end());
			size_t M = *std::max_element(histo.begin(), histo.end());
			if (tot)
			{
				double mse = 0;
				for (size_t cnt : histo)
					mse += Sqr(0.125 - double(cnt) / double(tot));
				mse /= double(histo.size());
				if ((sqrt(mse) <= 0.05) && (double(tot) >= bbox.GetArea() / 2)) // a diamond is half the area of its bounding box
				{
					for (int x = bbox.GetLeft(); x <= bbox.GetRight(); ++x)
						presig[x] = '.';
					//lb->GetRGB()->DrawRect(bbox, PixelRGB(uint8_t(0), 0, 255)); // XXX display
					//ImageRGB out;
					//out.InitFromImage(*b.GetRGB(), Rect(bx + bbox.GetLeft() - int(lh)/2, by + bbox.GetTop() - int(lh)/2, bx + bbox.GetRight() + int(lh)/2, by + bbox.GetBottom() + int(lh)/2));
					//out.SavePNG(Path("dots/") + dnum++ + Path(".png"));
				}
			}
		}
	}
	//lb->GetRGB()->SavePNG("art dots.png");

	static int lnum = 0;
	static int rnum = 0;
	// curves
	lb->SubstituteBW(anglemask);

	// XXX display
	//lb->GetBW()->SavePNG("art angle mask.png");
	//lb->FlushRGB();

	UImageIntGray anglemap = lb->ExtractCC(U"angle");
	if (lb->HasTree(U"angle"))
	{
		lb->FilterMinAnd(U"angle", sw * 2, sw * 2);
		lb->FilterMinOr(U"angle", sw, sw);
		for (const SObject &o : lb->GetTree(U"angle"))
		{
			SBlock llb(std::static_pointer_cast<Block>(o));
			int id = llb->GetName().ToInt();
			Rect bbox(llb->GetRelativeBBox());
			size_t cave = 0, vex = 0;
			for (const Point2DInt &p : bbox)
			{
				if (anglemap->At(p.X, p.Y) == id)
				{
					if (lvv.At(p.X, p.Y) >= 0.0)
						vex += 1;
					else
						cave += 1;
				}
			}
			size_t tot = vex + cave;
			if (tot)
			{
				//uint8_t r = uint8_t(255 * vex / tot);
				uint8_t r(255);
				if (double(vex) / double(tot) > 0.75)
				{
					//ImageRGB out;
					//out.InitFromImage(*b.GetRGB(), Rect(bx + bbox.GetLeft() - int(lh)/2, by + bbox.GetTop() - int(lh)/2, bx + bbox.GetRight() + int(lh)/2, by + bbox.GetBottom() + int(lh)/2));
					for (const Point2DInt &p : bbox)
					{
						if (anglemap->At(p.X, p.Y) == id)
						{
							presig[p.X] = '(';
							//lb->GetRGB()->SetPixel(p.X, p.Y, 0, 0, r); // XXX display
							//out.SetPixel(p.X - bbox.GetLeft() + lh/2, p.Y - bbox.GetTop() + lh/2, 255, 0, 255);
						}
					}
					//out.SavePNG(Path("left/") + lnum++ + Path(".png"));
				}
			}
		}
	}
	// XXX display
	//lb->GetRGB()->SavePNG("art angle.png");

	lb->SubstituteBW(angle2mask);
	anglemap = lb->ExtractCC(U"angle2");
	if (lb->HasTree(U"angle2"))
	{
		lb->FilterMinAnd(U"angle2", sw * 2, sw * 2);
		lb->FilterMinOr(U"angle2", sw, sw);
		for (const SObject &o : lb->GetTree(U"angle2"))
		{
			SBlock llb(std::static_pointer_cast<Block>(o));
			int id = llb->GetName().ToInt();
			Rect bbox(llb->GetRelativeBBox());
			size_t cave = 0, vex = 0;
			for (const Point2DInt &p : bbox)
			{
				if (anglemap->At(p.X, p.Y) == id)
				{
					if (lvv.At(p.X, p.Y) >= 0.0)
						vex += 1;
					else
						cave += 1;
				}
			}
			size_t tot = vex + cave;
			if (tot)
			{
				//uint8_t r = uint8_t(255 * vex / tot);
				uint8_t r(255);
				if (double(vex) / double(tot) > 0.75)
				{
					//ImageRGB out;
					//out.InitFromImage(*b.GetRGB(), Rect(bx + bbox.GetLeft() - int(lh)/2, by + bbox.GetTop() - int(lh)/2, bx + bbox.GetRight() + int(lh)/2, by + bbox.GetBottom() + int(lh)/2));
					for (const Point2DInt &p : bbox)
					{
						if (anglemap->At(p.X, p.Y) == id)
						{
							presig[p.X] = ')';
							//lb->GetRGB()->SetPixel(p.X, p.Y, 0, 0, r); // XXX display
							//out.SetPixel(p.X - bbox.GetLeft() + lh/2, p.Y - bbox.GetTop() + lh/2, 0, 255, 255);
						}
					}
					//out.SavePNG(Path("right/") + rnum++ + Path(".png"));
				}
			}
		}
	}

	// enlarge the remaining small and long vertical strokes
	auto tmpsig = presig;
	for (const auto x : Range(presig))
	{
		if ((presig[x] == '\'') || (presig[x] == 'l') || (presig[x] == ','))
			for (size_t dx = 1; dx <= sw/2; ++dx)
			{
				if (x >= dx)
				{
					if (presig[x - dx] == '\0')
						tmpsig[x - dx] = presig[x];
				}
				if (x + dx < presig.size())
				{
					if (presig[x + dx] == '\0')
						tmpsig[x + dx] = presig[x];
				}
			}
	}
	tmpsig.swap(presig);

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
				sig.emplace_back(ImageSignature(Rect(orix, oriy - int(lh / 2), orix, oriy + int(lh / 2)), presig[x]));
			}
			else
			{
				sig.back().bbox |= Rect(orix, oriy - int(lh / 2));
				sig.back().bbox |= Rect(orix, oriy + int(lh / 2));
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
				s += b.GetGray()->At(x, y);
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
	{
		// XXX display
		uint8_t re = 0, gr = 0, bl = 0;
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
			 CRN_FOREACH(const Point2DInt &p, is.bbox)
			 if ((p.X + p.Y) % 2)
			 lb->GetRGB()->SetPixel(Cap(p.X - bx, 0, iw - 1), Cap(p.Y - by, 0, ih - 1), re, gr, bl);
			 */
		b.GetRGB()->DrawRect(is.bbox, PixelRGB(re, gr, bl), false);
		/*
			 Rect tbox(is.bbox);
			 tbox.Translate(-bx, -by);
			 lb->GetRGB()->DrawRect(tbox, PixelRGB(re, gr, bl), false);
			 */
	}
#endif

	// compute cutproba
	for (auto &s : sig)
	{
		auto cumul = 0;
		for (auto y = s.bbox.GetTop(); y <= s.bbox.GetBottom(); ++y)
			cumul += b.GetGray()->At(s.bbox.GetLeft(), y); // TODO do better
		s.cutproba = uint8_t(cumul / s.bbox.GetHeight());
	}

	features = sig;

	//std::cout << std::endl;
	//lb->GetRGB()->SavePNG("xxx line sig.png");

	if (b.HasTree(U"lines"))
		b.RemoveChild(U"lines", lb);
	return sig;
}

void GraphicalLine::deserialize(xml::Element &el)
{
	xml::Element lel(el.GetFirstChildElement("LinearInterpolation"));
	SLinearInterpolation lin(std::static_pointer_cast<LinearInterpolation>(SObject(DataFactory::CreateData(lel)))); // might throw
	int nlh = el.GetAttribute<int>("lh", false); // might throw
	midline = lin;
	lh = nlh;
	xml::Element fel(el.GetFirstChildElement("signature"));
	if (fel)
	{
		try
		{
			std::vector<ImageSignature> sig;
			for (xml::Element sel = fel.BeginElement(); sel != fel.EndElement(); ++sel)
			{
				Rect r;
				r.Deserialize(sel);
				sig.emplace_back(ImageSignature(r, sel.GetAttribute<StringUTF8>("code")[0], uint8_t(sel.GetAttribute<int>("cutproba", true))));
			}
			features.swap(sig);
		}
		catch (...) { }
	}
}

xml::Element GraphicalLine::serialize(xml::Element &parent) const
{
	xml::Element el(parent.PushBackElement(GetClassName().CStr()));
	midline->Serialize(el);
	el.SetAttribute("lh", int(lh));
	if (!features.empty())
	{
		xml::Element fel(el.PushBackElement("signature"));
		for (const ImageSignature &s : features)
		{
			xml::Element sel(s.bbox.Serialize(fel));
			sel.SetAttribute("code", StringUTF8(s.code));
			sel.SetAttribute("cutproba", StringUTF8(s.cutproba));
		}
	}
	return el;
}
void GraphicalLine::SetMidline(const std::vector<Point2DInt> & line)
{
	midline = std::make_shared<LinearInterpolation> (line.begin(),line.end());
	features.clear();
}

CRN_BEGIN_CLASS_CONSTRUCTOR(GraphicalLine)
	CRN_DATA_FACTORY_REGISTER(U"GraphicalLine", GraphicalLine)
CRN_END_CLASS_CONSTRUCTOR(GraphicalLine)
