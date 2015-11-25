/* Copyright 2015 Université Paris Descartes
 * 
 * file: OriDocument.h
 * \author Yann LEYDIER
 */

#ifndef OriDocument_HEADER
#define OriDocument_HEADER

#include <CRNStringUTF8.h>
#include <CRNIO/CRNPath.h>
#include <CRNGeometry/CRNRect.h>
#include <CRNGeometry/CRNPoint2DInt.h>
#include <CRNUtils/CRNXml.h>
#include <CRNUtils/CRNProgress.h>
#include <CRNMath/CRNSquareMatrixDouble.h>
#include <CRNBlock.h>
#include <OriAlignConfig.h>
#include <vector>
#include <unordered_map>

namespace ori
{
	using Id = crn::StringUTF8;

	struct ElementPosition
	{
		ElementPosition() = default;
		ElementPosition(const Id& v): view(v) {}
		ElementPosition(const Id& v, const Id& p): view(v), page(p) {}
		ElementPosition(const Id& v, const Id& p, const Id& c): view(v), page(p), column(c) {}
		ElementPosition(const Id& v, const Id& p, const Id& c, const Id& l): view(v), page(p), column(c), line(l) {}
		ElementPosition(const Id& v, const Id& p, const Id& c, const Id& l, const Id& w): view(v), page(p), column(c), line(l), word(w) {}
		ElementPosition(const ElementPosition&) = default;
		ElementPosition(ElementPosition&&) = default;
		ElementPosition& operator=(const ElementPosition&) = default;
		ElementPosition& operator=(ElementPosition&&) = default;
		Id view;
		Id page;
		Id column;
		Id line;
		Id word;
		crn::StringUTF8 ToString() const { return view + ", " + page + ", " + column + ", " + line + ", " + word; }
	};
	
	class CharacterClass
	{
		public:
		private:
			crn::xml::Element el;
	};

	class View;
	class Document;
	class Zone
	{
		public:
			Zone(crn::xml::Element &elem);
			Zone(const Zone&) = delete;
			Zone(Zone&&) = default;
			Zone& operator=(const Zone&) = delete;
			Zone& operator=(Zone&&) = default;
			const crn::Rect& GetPosition() const noexcept;
			const std::vector<crn::Point2DInt>& GetContour() const noexcept { return box; }

			void SetPosition(const crn::Rect &r);
			void SetContour(const std::vector<crn::Point2DInt> &c);
			void Clear();
		private:
			mutable crn::Rect pos;
			std::vector<crn::Point2DInt> box;
			crn::xml::Element el;
		
		friend class View;
		friend class Document;
	};

	class Character
	{
		public:
			Character() = default;
			Character(const Character&) = delete;
			Character(Character&&) = default;
			Character& operator=(const Character&) = delete;
			Character& operator=(Character&&) = default;
			const crn::String& GetText() const noexcept { return text; }
			const Id& GetZone() const noexcept { return zone; }

		private:
			Id zone;
			crn::String text;
		
		friend class View;
		friend class Document;
	};

	class Word
	{
		public:
			Word() = default;
			Word(const Word&) = delete;
			Word(Word&&) = default;
			Word& operator=(const Word&) = delete;
			Word& operator=(Word&&) = default;
			const crn::String& GetText() const noexcept { return text; }
			const std::vector<Id>& GetCharacters() const noexcept { return characters; }
			const Id& GetZone() const noexcept { return zone; }

		private:
			std::vector<Id> characters;
			Id zone;
			crn::String text;
		
		friend class View;
		friend class Document;
	};

	class Line
	{
		public:
			Line() = default;
			Line(const Line&) = delete;
			Line(Line&&) = default;
			Line& operator=(const Line&) = delete;
			Line& operator=(Line&&) = default;
			const std::vector<Id>& GetWords() const;
			const Id& GetZone() const noexcept { return zone; }

		private:
			mutable std::vector<Id> left, center, right;
			Id zone;
		
		friend class View;
		friend class Document;
	};

	class Column
	{
		public:
			Column() = default;
			Column(const Column&) = delete;
			Column(Column&&) = default;
			Column& operator=(const Column&) = delete;
			Column& operator=(Column&&) = default;
			const std::vector<Id>& GetLines() const noexcept { return lines; }
			const Id& GetZone() const noexcept { return zone; }

		private:
			std::vector<Id> lines;
			Id zone;
		
		friend class View;
		friend class Document;
	};

	class Page
	{
		public:
			Page() = default;
			Page(const Page&) = delete;
			Page(Page&&) = default;
			Page& operator=(const Page&) = delete;
			Page& operator=(Page&&) = default;
			const std::vector<Id>& GetColumns() const noexcept { return columns; }
			const Id& GetZone() const noexcept { return zone; }

		private:
			std::vector<Id> columns;
			Id zone;
		
		friend class View;
		friend class Document;
	};

	class GraphicalLine;
	class View
	{
		public:
			View();
			View(const View&) = default;
			View(View&&) = default;
			View& operator=(const View&) = default;
			View& operator=(View&&) = default;
			~View();

			void Save();

			/*! \brief Full path of the image */
			const crn::Path& GetImageName() const noexcept;
			/*! \brief Gets the image */
			crn::Block& GetBlock() const;

			/*! \brief Returns the ordered list of pages' id */
			const std::vector<Id>& GetPages() const noexcept;
			const Page& GetPage(const Id &page_id) const;
			Page& GetPage(const Id &page_id);

			const std::unordered_map<Id, Column>& GetColumns() const;
			const Column& GetColumn(const Id &col_id) const;
			Column& GetColumn(const Id &col_id);
			/*! \brief Returns all median lines of a column */
			const std::vector<GraphicalLine>& GetGraphicalLines(const Id &col_id) const;
			/*! \brief Adds a median line to a column */
			void AddGraphicalLine(const std::vector<crn::Point2DInt> &pts, const Id &col_id);
			/*! \brief Removes a median line from a column */
			void RemoveGraphicalLine(const Id &col_id, size_t index);
			/*! \brief Removes all aligned coordinates in a column */
			void ClearAlignment(const Id &col_id);

			const std::unordered_map<Id, Line>& GetLines() const;
			const Line& GetLine(const Id &line_id) const;
			Line& GetLine(const Id &line_id);
			/*! \brief Returns the median line of a text line */
			const GraphicalLine& GetGraphicalLine(const Id &line_id) const;
			/*! \brief Returns the median line of a text line */
			GraphicalLine& GetGraphicalLine(const Id &line_id);
			/*! \brief Returns the median line's index of a text line */
			size_t GetGraphicalLineIndex(const Id &line_id) const;

			const std::unordered_map<Id, Word>& GetWords() const;
			const Word& GetWord(const Id &word_id) const;
			Word& GetWord(const Id &word_id);
			/*! \brief Checks if a word was validated or rejected by the user */
			const crn::Prop3& IsValid(const Id &word_id) const;
			/*! \brief Sets if a word was validated or rejected by the user */
			void SetValid(const Id &word_id, const crn::Prop3 &val);
			/*! \brief Returns the alignable characters in word */
			crn::String GetAlignableText(const Id &word_id) const;

			const std::unordered_map<Id, Character>& GetCharacters() const;
			const Character& GetCharacter(const Id &char_id) const;
			Character& GetCharacter(const Id &char_id);
			/*! \brief Returns the list of glyph Ids associated to a character */
			const std::vector<Id>& GetClusters(const Id &char_id) const;
			/*! \brief Returns the list of glyph Ids associated to a character */
			std::vector<Id>& GetClusters(const Id &char_id);

			const Zone& GetZone(const Id &zone_id) const;
			Zone& GetZone(const Id &zone_id);
			/*! \brief Sets the bounding box of an element */
			void SetPosition(const Id &id, const crn::Rect &r, bool compute_contour);
			/*! \brief Sets the contour of an element */
			void SetContour(const Id &id, const std::vector<crn::Point2DInt> &c, bool set_position);
			/*! \brief Computes a curve from a vertical line */
			std::vector<crn::Point2DInt> ComputeFrontier(size_t x, size_t y1, size_t y2) const;
			/*! \brief Computes the contour of a zone from its bounding box */
			void ComputeContour(const Id &zone_id);
			/*! \brief Gets the image of a zone */
			crn::SBlock GetZoneImage(const Id &zone_id) const;

			/*! \brief Sets the left frontier of a word */
			void UpdateLeftFrontier(const Id &id, int x);
			/*! \brief Sets the right frontier of a word */
			void UpdateRightFrontier(const Id &id, int x);
			/*! \brief Resets the left and right corrections of a word */
			void ResetCorrections(const Id &id);
			/*! \brief Gets the total left correction of a word */
			int GetLeftCorrection(const Id &id);
			/*! \brief Gets the total right correction of a word */
			int GetRightCorrection(const Id &id);

			/*! \brief Computes alignment on a page */
			void AlignPage(AlignConfig conf, const Id &page_id, crn::Progress *pageprog = nullptr, crn::Progress *colprog = nullptr, crn::Progress *linprog = nullptr);
			/*! \brief Computes alignment on a column */
			void AlignColumn(AlignConfig conf, const Id &col_id, crn::Progress *colprog = nullptr, crn::Progress *linprog = nullptr);
			/*! \brief Computes alignment on a line */
			void AlignLine(AlignConfig conf, const Id &line_id, crn::Progress *prog = nullptr);
			/*! \brief Computes alignment on a range of words */
			void AlignRange(AlignConfig conf, const Id &line_id, size_t first_word, size_t last_word);
			/*! \brief Aligns the characters in a word */
			void AlignWordCharacters(AlignConfig conf, const Id &line_id, const Id &word_id);

		private:
			struct Impl;

			View(const std::shared_ptr<Impl> &ptr):pimpl(ptr) { }
			const crn::ImageGray& getWeight() const;
			Id addZone(Id id_base, crn::xml::Element &elem);
			void detectLines();

			std::shared_ptr<Impl> pimpl;
			
		friend class Document;
	};

	class Glyph
	{
		public:
			Glyph(crn::xml::Element &elem);
			Glyph(const Glyph&) = delete;
			Glyph(Glyph&&) = default;
			Glyph& operator=(const Glyph&) = delete;
			Glyph& operator=(Glyph&&) = default;

			crn::StringUTF8 GetDescription() const;
			Id GetParent() const;
			void SetParent(const Id &parent_id);
			bool IsAuto() const;

			static Id LocalId(const Id &id);
			static Id GlobalId(const Id &id);
			static Id BaseId(const Id &id);
			static bool IsLocal(const Id &id);
			static bool IsGlobal(const Id &id);

		private:
			mutable crn::xml::Element el;
	};

	class TEISelectionNode;
	class Document
	{
		public:
			Document(const crn::Path &dirpath, crn::Progress *prog = nullptr);
			~Document();

			Document(const Document&) = delete;
			Document(Document&&) = default;
			Document& operator=(const Document&) = delete;
			Document& operator=(Document&&) = default;

			const crn::StringUTF8& GetName() const noexcept { return name; }
			const crn::Path& GetBase() const noexcept { return base; }
			const crn::StringUTF8& ErrorReport() const noexcept { return report; }

			void Save() const;

			const ElementPosition& GetPosition(const Id &elem_id) const;

			const std::vector<Id>& GetViews() const noexcept { return views; }
			View GetView(const Id &id);

			/*! \brief Erases image signatures in the document */
			void ClearSignatures(crn::Progress *prog);
			/*! \brief Computes alignment on the whole document */
			void AlignAll(AlignConfig conf, crn::Progress *docprog = nullptr, crn::Progress *pageprog = nullptr, crn::Progress *colprog = nullptr, crn::Progress *linprog = nullptr);
			/*! \brief Propagates the validation of word alignment */
			void PropagateValidation(crn::Progress *prog = nullptr);

			/*! \brief Gets the list of characters sorted by Unicode value */
			std::map<crn::String, std::unordered_map<Id, std::vector<Id>>> CollectCharacters() const;

			const Glyph& GetGlyph(const Id &id) const;
			Glyph& GetGlyph(const Id &id);
			/* \brief Adds a glyph to the local ontology file */
			Glyph& AddGlyph(const Id &id, const crn::StringUTF8 &desc, const Id &parent = Id{}, bool automatic = false);
			const std::unordered_map<Id, Glyph>& GetGlyphs() const noexcept { return glyphs; }

			/*! \brief Exports statistics on alignment validation to an ODS file */
			void ExportStats(const crn::Path &fname);

			/*! \brief Returns the distance matrix for a character */
			const std::pair<std::vector<Id>, crn::SquareMatrixDouble>& GetDistanceMatrix(const crn::String &character) const;
			/*! \brief Sets the distance matrix for a character */
			template<typename VectorOdIds, typename DistanceMatrix> void SetDistanceMatrix(const crn::String &character, VectorOdIds &&ids, DistanceMatrix &&dm) { chars_dm.emplace(character, std::make_pair(std::forward<VectorOdIds>(ids), std::forward<DistanceMatrix>(dm))); }
			/*! \brief Erases the distance matrix for a character */
			void EraseDistanceMatrix(const crn::String &character);

			struct ViewStructure
			{
				std::unordered_map<Id, Page> pages;
				std::vector<Id> pageorder;
				std::unordered_map<Id, Column> columns;
				std::unordered_map<Id, Line> lines;
				std::unordered_map<Id, Word> words;
				std::unordered_map<Id, Character> characters;
			};

		private:
			void readTextWElements(crn::xml::Element &el, ElementPosition &pos, const TEISelectionNode& teisel, std::multimap<int, Id> &milestones);
			void readTextCElements(crn::xml::Element &el, ElementPosition &pos, const TEISelectionNode& teisel);

			using ViewRef = std::weak_ptr<View::Impl>;
			std::unordered_map<Id, ViewRef> view_refs; // weak references to views
			std::vector<Id> views; // views in order of the document
			std::unordered_map<Id, ViewStructure> view_struct;
			std::unordered_map<Id, ElementPosition> positions;
			std::unordered_map<Id, Glyph> glyphs;
			std::unordered_map<crn::String, std::pair<std::vector<Id>, crn::SquareMatrixDouble>> chars_dm;

			crn::Path base;
			crn::StringUTF8 name;
			crn::StringUTF8 report;
			crn::xml::Document global_onto;
			mutable crn::xml::Document local_onto;
			std::unique_ptr<crn::xml::Element> charDecl;
	};

}

#endif

