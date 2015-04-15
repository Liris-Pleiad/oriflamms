/* Copyright 2013 INSA-Lyon & IRHT
 * 
 * file: OriProject.h
 * \author Yann LEYDIER
 */

#ifndef OriProject_HEADER
#define OriProject_HEADER

#include <oriflamms_config.h>
#include <CRNDocument.h>
#include <CRNUtils/CRNProgress.h>
#include <OriStruct.h>

namespace ori
{
	class Project
	{
		public:
			Project(const crn::Path &fname);
			static crn::SharedPointer<Project> New(const crn::Path &name, const crn::Path &xmlpath, const crn::Path &imagedir, crn::Progress *prog = NULL) { return new Project(name, xmlpath, imagedir, prog); }

			size_t GetNbViews() const { return xdoc.GetViews().size(); }
			struct View
			{
				View(CRNBlock b, ori::View &v):image(b),page(v) { }
				CRNBlock image;
				ori::View &page;
			};
			View GetView(size_t num);

			const CRNDocument GetDoc() const { return doc; }
			const ori::Document& GetStructure() const { return  xdoc; }
			ori::Document& GetStructure() { return  xdoc; }

			void Save(crn::Progress *prog = NULL) const;

			void ReloadTEI();

			void AlignAll(crn::Progress *docprog = NULL, crn::Progress *viewprog = NULL, crn::Progress *colprog = NULL, crn::Progress *linprog = NULL);
			void AlignView(size_t view_num, crn::Progress *viewprog = NULL, crn::Progress *colprog = NULL, crn::Progress *linprog = NULL);
			void AlignColumn(size_t view_num, size_t col_num, crn::Progress *colprog = NULL, crn::Progress *linprog = NULL);
			void AlignLine(size_t view_num, size_t col_num, size_t line_num, crn::Progress *prog = NULL);
			void AlignRange(const WordPath &first, const WordPath &last);

			void ExportStats(const crn::Path &fname) const;

			const crn::StringUTF8 GetTitle() const;

			void ClearSignatures(crn::Progress *prog = NULL);
			void ExportCharacters(const crn::Path &path, bool only_validated, crn::Progress *prog = NULL);

			void PropagateValidation(crn::Progress *prog = NULL);

			static const crn::String LinesKey;
		private:
			Project(const crn::Path &name, const crn::Path &xmlpath, const crn::Path &imagedir, crn::Progress *prog);
			void load_db();
			const std::vector<TextSignature> getSignature(const Line &l) const;
			const std::vector<TextSignature> getSignature(const Word &w) const;

			CRNDocument doc;
			ori::Document xdoc;
			std::map<crn::Char, crn::StringUTF8> signature_db;
	};
}

#endif

