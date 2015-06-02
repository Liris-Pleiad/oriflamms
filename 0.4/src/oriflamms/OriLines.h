/* Copyright 2013-2015 INSA-Lyon, IRHT, ZHAO Xiaojuan, Université Paris Descartes
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
	class View;
	/*! \brief Detect lines and columns on an image */
	crn::SVector DetectLines(crn::Block &b, const View &view);
	/*! \brief Reduces the number of points in a curve */
	std::vector<crn::Point2DInt> SimplifyCurve(const std::vector<crn::Point2DInt> &line, double maxdist);
	/*! \brief Reduces the number of points in a curve */
	std::vector<crn::Point2DDouble> SimplifyCurve(const std::vector<crn::Point2DDouble> &line, double maxdist);

	class GraphicalLine: public crn::Object
	{
		public:
			/*! \brief Constructor */
			GraphicalLine(const crn::SLinearInterpolation &lin, size_t lineheight);
			virtual ~GraphicalLine() override {}

			/*! \brief This is a Serializable object */
			virtual crn::Protocol GetClassProtocols() const noexcept override { return crn::Protocol::Serializable; }
			/*! \brief Returns the id of the class */
			virtual const crn::String& GetClassName() const override { static const crn::String cn(U"GraphicalLine"); return cn; }

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
			std::vector<ImageSignature> ExtractFeatures(crn::Block &b) const;
			/*! \brief Deletes the cached signature string */
			void ClearFeatures() { features.clear(); }
		private:

			virtual void deserialize(crn::xml::Element &el) override;
			virtual crn::xml::Element serialize(crn::xml::Element &parent) const override;

			crn::SLinearInterpolation midline;
			size_t lh;
			mutable std::vector<ImageSignature> features;

			CRN_DECLARE_CLASS_CONSTRUCTOR(GraphicalLine)
			CRN_SERIALIZATION_CONSTRUCTOR(GraphicalLine)
	};

	CRN_ALIAS_SMART_PTR(GraphicalLine)
};

#endif


