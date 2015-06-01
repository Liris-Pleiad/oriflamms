/* Copyright 2013-2015 INSA-Lyon, IRHT, ZHAO Xiaojuan, Université Paris Descartes
 *
 * file: OriProject.h
 * \author Yann LEYDIER
 */

#ifndef OriProject_HEADER
#define OriProject_HEADER

#include <oriflamms_config.h>
#include <CRNDocument.h>
#include <CRNUtils/CRNProgress.h>
#include <OriTEIImporter.h>
#include <OriStruct.h>

namespace ori
{
	class Project
	{
		public:
			/*! \brief Project constructor */
			Project(const crn::Path &fname);
			/*! \brief Creates a new project
			 * \param[in]	name	name of the project
			 * \param[in]	xmlpath	path to the TEI file
			 * \param[in]	sel	selection of elements in the TEI file
			 * \param[in]	imagedir	path to the images
			 * \param[in]	prog	a progress bar
			 */
			static std::unique_ptr<Project> New(const crn::Path &name, const crn::Path &xmlpath, const STEISelectionNode &sel, const crn::Path &imagedir, crn::Progress *prog = nullptr) { return std::unique_ptr<Project>(new Project(name, xmlpath, sel, imagedir, prog)); }

			/*! \brief Returns the number of views */
			size_t GetNbViews() const { return xdoc.GetViews().size(); }
			/*! \brief Wrapper around a view (crn and Oriflamms structures) */
			struct View
			{
				View(const crn::SBlock &b, ori::View &v):image(b),page(v) { }
				crn::SBlock image;
				ori::View &page;
			};
			/*! \brief Gets a wrapper around a view */
			View GetView(size_t num);

			/*! \brief Returns the crn document structure */
			const crn::SDocument& GetDoc() { return doc; }
			/*! \brief Returns the crn document structure */
			crn::SCDocument GetDoc() const { return doc; }
			/*! \brief Returns the Oriflamms document structure */
			ori::Document& GetStructure() { return  xdoc; }
			/*! \brief Returns the Oriflamms document structure */
			const ori::Document& GetStructure() const { return  xdoc; }

			/*! \brief Saves the project */
			void Save(crn::Progress *prog = nullptr) const;

			/*! \brief Reloads the TEI file */
			void ReloadTEI();

			/*! \brief Computes alignment on the whole document */
			void AlignAll(crn::Progress *docprog = nullptr, crn::Progress *viewprog = nullptr, crn::Progress *colprog = nullptr, crn::Progress *linprog = nullptr);
			/*! \brief Computes alignment on a view */
			void AlignView(size_t view_num, crn::Progress *viewprog = nullptr, crn::Progress *colprog = nullptr, crn::Progress *linprog = nullptr);
			/*! \brief Computes alignment on a column */
			void AlignColumn(size_t view_num, size_t col_num, crn::Progress *colprog = nullptr, crn::Progress *linprog = nullptr);
			/*! \brief Computes alignment on a line */
			void AlignLine(size_t view_num, size_t col_num, size_t line_num, crn::Progress *prog = nullptr);
			/*! \brief Computes alignment on a range of words */
			void AlignRange(const WordPath &first, const WordPath &last);
			/*! \brief Computes a word's frontiers */
			void ComputeWordFrontiers(const ori::WordPath &wp);
			/*! \brief Aligns the characters in a word */
			void AlignWordCharacters(const ori::WordPath &wp);

			/*! \brief Exports statistics on alignment validation to an ODS file */
			void ExportStats(const crn::Path &fname) const;

			/*! \brief Gets the name of the project */
			crn::StringUTF8 GetTitle() const;

			/*! \brief Erases a column's alignment */
			void ClearAlignment(size_t view_num, size_t col_num);
			/*! \brief Erases all signatures in the document */
			void ClearSignatures(crn::Progress *prog = nullptr);

			/*! \brief Propagates the validation of word alignment */
			void PropagateValidation(crn::Progress *prog = nullptr);

			static const crn::String LinesKey; /*!< UserData name of the line informations in the crn structure's views */
		private:
			/*! \brief Project constructor */
			Project(const crn::Path &name, const crn::Path &xmlpath, const STEISelectionNode &sel, const crn::Path &imagedir, crn::Progress *prog);
			/*! \brief Loads the signature database */
			void load_db();
			/*! \brief Computes the signature of a text line's transcription */
			std::vector<TextSignature> getSignature(const Line &l) const;
			/*! \brief Computes the signature of a word's transcription */
			std::vector<TextSignature> getSignature(const Word &w) const;

			crn::SDocument doc; /*!< crn document structure */
			ori::Document xdoc; /*!< Oriflamms document structure */
			std::map<char32_t, crn::StringUTF8> signature_db; /*!< signature database */
	};
}

#endif

