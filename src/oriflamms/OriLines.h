/* Copyright 2013-2016 INSA-Lyon, IRHT, ZHAO Xiaojuan, Université Paris Descartes, ENS-Lyon
 *
 * file: OriLines.h
 * \author Yann LEYDIER
 */

#ifndef OriLines_HEADER
#define OriLines_HEADER

#include <oriflamms_config.h>
#include <CRNData/CRNVector.h>
#include <CRNMath/CRNLinearInterpolation.h>
#include <CRNData/CRNDataFactory.h>
#include <OriFeatures.h>
#include <CRNBlock.h>

namespace ori
{
	/*! \brief Reduces the number of points in a curve */
	std::vector<crn::Point2DInt> SimplifyCurve(const std::vector<crn::Point2DInt> &line, double maxdist);
	/*! \brief Reduces the number of points in a curve */
	std::vector<crn::Point2DDouble> SimplifyCurve(const std::vector<crn::Point2DDouble> &line, double maxdist);

	class GraphicalLine: public crn::Object
	{
		public:
			/*! \brief Constructor */
			GraphicalLine(const crn::SLinearInterpolation &lin, size_t lineheight);
			GraphicalLine(const GraphicalLine&) = delete;
			GraphicalLine(GraphicalLine&&) = default;
			GraphicalLine& operator=(const GraphicalLine&) = delete;
			GraphicalLine& operator=(GraphicalLine&&) = default;
			virtual ~GraphicalLine() override {}

			/*! \brief Gets the leftmost point of the line */
			crn::Point2DInt GetFront() const { return crn::Point2DInt(int(midline->GetData().front().X), int(midline->GetData().front().Y)); }
			/*! \brief Gets the rightmost point of the line */
			crn::Point2DInt GetBack() const { return crn::Point2DInt(int(midline->GetData().back().X), int(midline->GetData().back().Y)); }
			/*! \brief Gets the ordinate for a given abscissa */
			int At(int x) const { return (*midline)[x]; }
			/*! \brief Gets the list of key points */
			const std::vector<crn::Point2DDouble>& GetMidline() const { return midline->GetData(); }
			/*! \brief Sets the list of key points */
			void SetMidline(const std::vector<crn::Point2DInt> &line);

			/*! \brief Gets the line height */
			size_t GetLineHeight() const noexcept { return lh; }

			/*! \brief Gets the signature string of the line */
			const std::vector<ImageSignature>& ExtractFeatures(crn::Block &b) const;
			/*! \brief Deletes the cached signature string */
			void ClearFeatures() { features.clear(); }

			void Deserialize(crn::xml::Element &el);
			crn::xml::Element Serialize(crn::xml::Element &parent) const;

		private:

			crn::SLinearInterpolation midline;
			size_t lh;
			mutable std::vector<ImageSignature> features;

			CRN_DECLARE_CLASS_CONSTRUCTOR(GraphicalLine)
			CRN_SERIALIZATION_CONSTRUCTOR(GraphicalLine)
	};

	CRN_ALIAS_SMART_PTR(GraphicalLine)
};
namespace crn
{
	template<> struct IsSerializable<ori::GraphicalLine> : public std::true_type {};
}

#endif


