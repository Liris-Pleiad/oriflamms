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
#include <CRNBlock.h>
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
	};
	
	class CharacterClass
	{
		public:
		private:
			crn::xml::Element el;
	};

	class View;
	class Zone
	{
		public:
			Zone(crn::xml::Element &elem);
			Zone(const Zone&) = delete;
			Zone(Zone&&) = default;
			Zone& operator=(const Zone&) = delete;
			Zone& operator=(Zone&&) = default;
			//const Id& GetId() const noexcept { return id; }
			const crn::Rect& GetPosition() const noexcept;
			const std::vector<crn::Point2DInt>& GetContour() const noexcept { return box; }

			void SetPosition(const crn::Rect &r);
			void SetContour(const std::vector<crn::Point2DInt> &c);
		private:
			//Id id;
			mutable crn::Rect pos;
			std::vector<crn::Point2DInt> box;
			crn::xml::Element el;
		
		friend class View;
	};

	class Character
	{
		public:
			Character() = default;
			Character(const Character&) = delete;
			Character(Character&&) = default;
			Character& operator=(const Character&) = delete;
			Character& operator=(Character&&) = default;
			//const Id& GetId() const noexcept { return id; }
			const crn::StringUTF8& GetText() const noexcept { return text; }
			const Id& GetZone() const noexcept { return zone; }

		private:
			//Id id;
			Id zone;
			crn::StringUTF8 text;
		
		friend class View;
	};

	class Word
	{
		public:
			Word() = default;
			Word(const Word&) = delete;
			Word(Word&&) = default;
			Word& operator=(const Word&) = delete;
			Word& operator=(Word&&) = default;
			//const Id& GetId() const { return id; }
			const crn::StringUTF8& GetText() const noexcept { return text; }
			const std::vector<Id>& GetCharacters() const noexcept { return characters; }
			const Id& GetZone() const noexcept { return zone; }

		private:
			//Id id;
			std::vector<Id> characters;
			Id zone;
			crn::StringUTF8 text;
		
		friend class View;
	};

	class Line
	{
		public:
			Line() = default;
			Line(const Line&) = delete;
			Line(Line&&) = default;
			Line& operator=(const Line&) = delete;
			Line& operator=(Line&&) = default;
			//const Id& GetId() const noexcept { return id; }
			const std::vector<Id>& GetWords() const;
			const Id& GetZone() const noexcept { return zone; }

		private:
			//Id id;
			mutable std::vector<Id> left, center, right;
			Id zone;
		
		friend class View;
	};

	class Column
	{
		public:
			Column() = default;
			Column(const Column&) = delete;
			Column(Column&&) = default;
			Column& operator=(const Column&) = delete;
			Column& operator=(Column&&) = default;
			//const Id& GetId() const noexcept { return id; }
			const std::vector<Id>& GetLines() const noexcept { return lines; }
			const Id& GetZone() const noexcept { return zone; }

		private:
			//Id id;
			std::vector<Id> lines;
			Id zone;
		
		friend class View;
	};

	class Document;
	class Page
	{
		public:
			Page() = default;
			Page(const Page&) = delete;
			Page(Page&&) = default;
			Page& operator=(const Page&) = delete;
			Page& operator=(Page&&) = default;
			//const Id& GetId() const noexcept { return id; }
			const std::vector<Id>& GetColumns() const noexcept { return columns; }
			const Id& GetZone() const noexcept { return zone; }

		private:
			//Id id;
			std::vector<Id> columns;
			Id zone;
		
		friend class View;
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

			/*! \brief Full path of the image */
			const crn::Path& GetImageName() const noexcept;

			/*! \brief Returns the ordered list of pages' id */
			const std::vector<Id>& GetPages() const noexcept;
			const Page& GetPage(const Id &id) const;
			Page& GetPage(const Id &id);

			const Column& GetColumn(const Id &id) const;
			Column& GetColumn(const Id &id);
			/*! \brief Returns all median lines of a column */
			const std::vector<GraphicalLine>& GetGraphicalLines(const Id &id) const;

			const Line& GetLine(const Id &id) const;
			Line& GetLine(const Id &id);
			/*! \brief Returns the median line of a text line */
			const GraphicalLine& GetGraphicalLine(const Id &id) const;
			/*! \brief Returns the median line of a text line */
			GraphicalLine& GetGraphicalLine(const Id &id);

			const Word& GetWord(const Id &id) const;
			Word& GetWord(const Id &id);
			/*! \brief Checks if a word was validated or rejected by the user */
			const crn::Prop3& IsValid(const Id &id) const;
			/*! \brief Sets if a word was validated or rejected by the user */
			void SetValid(const Id &id, const crn::Prop3 &val);

			const Character& GetCharacter(const Id &id) const;
			Character& GetCharacter(const Id &id);

			const Zone& GetZone(const Id &id) const;
			Zone& GetZone(const Id &id);

		private:
			struct Impl;

			View(const std::shared_ptr<Impl> &ptr):pimpl(ptr) { }

			std::shared_ptr<Impl> pimpl;
			
		friend class Document;
	};

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

			const ElementPosition& GetPosition(const Id &elem_id) const { return positions.find(elem_id)->second; }

			const std::vector<Id>& GetViews() const noexcept { return views; }
			View GetView(const Id &id);

		private:
			using ViewRef = std::weak_ptr<View::Impl>;
			std::unordered_map<Id, ViewRef> view_refs; // wear references to pages
			std::unordered_map<Id, ElementPosition> positions;
			std::vector<Id> views; // views in order of the document

			crn::Path base;
			crn::StringUTF8 name;
			crn::StringUTF8 report;
	};

}

#endif

