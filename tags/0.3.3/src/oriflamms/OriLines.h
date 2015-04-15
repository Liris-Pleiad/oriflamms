/* Copyright 2013 INSA-Lyon & IRHT
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

/*! \brief Detect lines and columns on an image
 *
 * \warning	an image of two one-columned pages returns the same results as an image of a one two-columned page
 *
 * \param[in]	b	the block to analyze
 * \return	a vector of columns, columns being vectors of lines, a line being a LinearInterperlation
 */
const CRNVector DetectLines(crn::Block &b);

namespace ori
{
	class GraphicalLine: public crn::Object
	{
		public:
			/*! \brief Constructor */
			GraphicalLine(CRNLinearInterpolation lin, size_t lineheight/*, int thresh*/);

			/*! \brief This is a Serializable object */
			virtual unsigned int GetClassProtocols() const { return crn::protocol::Serializable; }
			/*! \brief Returns the id of the class */
			virtual const crn::String& GetClassName() const { static const crn::String cn(STR("GraphicalLine")); return cn; }

			const crn::Point2DInt GetFront() const { return crn::Point2DInt(left, At(left)); }
			const crn::Point2DInt GetBack() const { return crn::Point2DInt(right, At(right)); }
			int GetLeft() const { return left; }
			int GetRight() const { return right; }
			void SetLeft(int l) { left = l; features.clear(); }
			void SetRight(int r) { right = r; features.clear(); }
			void SetBounds(int l, int r) { left = l; right = r; features.clear(); }
			int At(int x) const { return (*midline)[x]; }

			const std::vector<ImageSignature> ExtractFeatures(crn::Block &b) const;
			void ClearFeatures() { features.clear(); }
		private:

			virtual void deserialize(crn::xml::Element &el);
			virtual crn::xml::Element serialize(crn::xml::Element &parent) const;

			CRNLinearInterpolation midline;
			size_t lh;
			//int lumthresh;
			int left, right;
			mutable std::vector<ImageSignature> features;

		CRN_DECLARE_CLASS_CONSTRUCTOR(, GraphicalLine)
		CRN_SERIALIZATION_CONSTRUCTOR(GraphicalLine)
	};
};

typedef crn::SharedPointer<ori::GraphicalLine> OriGraphicalLine;
#endif


