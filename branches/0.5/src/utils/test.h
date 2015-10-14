#include <CRNStringUTF8.h>
#include <CRNIO/CRNPath.h>
#include <CRNGeometry/CRNRect.h>
#include <CRNGeometry/CRNPoint2DInt.h>
#include <CRNUtils/CRNXml.h>
#include <CRNUtils/CRNProgress.h>
#include <vector>
#include <unordered_map>

namespace ori
{
	using Id = crn::StringUTF8;

	class Zone
	{
		public:
			const Id& GetId() const { return id; }
		private:
			Id id;
			crn::Rect pos;
			std::vector<crn::Point2DInt> box;
			crn::StringUTF8 type;
			crn::xml::Element el;
	};

	class Character
	{
		public:
			const Id& GetId() const { return id; }
		private:
			Id id;
			Id zone;
			crn::StringUTF8 text;
	};

	class Word
	{
		public:
			const Id& GetId() const { return id; }
		private:
			Id id;
			std::vector<Id> characters;
			Id zone;
			//crn::StringUTF8 text; ???
	};

	class Line
	{
		public:
		private:
			std::vector<Id> left, center, right;
			Id zone;
	};

	class Column
	{
		public:
			const Id& GetId() const { return id; }
		private:
			Id id;
			std::vector<Line> lines;
			Id zone;
	};

	class Document;
	class Page
	{
		public:
			Page();
			Page(const Page&) = default;
			Page(Page&&) = default;
			Page& operator=(const Page&) = default;
			Page& operator=(Page&&) = default;
			~Page();

			const Id& GetId() const { return pimpl->id; }

			const Column& GetColumn(const Id &id) const;
			Column& GetColumn(const Id &id);
			const Word& GetWord(const Id &id) const;
			Word& GetWord(const Id &id);
			const Character& GetCharacter(const Id &id) const;
			Character& GetCharacter(const Id &id);
			const Zone& GetZone(const Id &id) const;
			Zone& GetZone(const Id &id);

		private:
			struct Impl
			{
				Impl(const std::vector<crn::Path> &filenames);

				Id id;
				Id zone;
				
				std::unordered_map<Id, Column> columns;
				std::unordered_map<Id, Word> words;
				std::unordered_map<Id, Character> characters;
				std::unordered_map<Id, Zone> zones;

				std::vector<crn::xml::Document> files;
				std::unordered_map<crn::StringUTF8, crn::xml::Element> link_groups;
			};

			Page(const std::shared_ptr<Impl> &ptr):pimpl(ptr) { }

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

			const crn::StringUTF8& ErrorReport() const { return report; }

			struct Position
			{
				size_t page = 0;
				size_t column = 0;
				size_t line = 0;
				size_t word = 0;
			};
			const Position& GetPosition(const Id &elem_id) const { return positions.find(elem_id)->second; }

			Page GetPage(const Id &id);

		private:
			struct PageRef
			{
				Page GetPage();

				std::weak_ptr<Page::Impl> page;
				std::vector<crn::Path> files;
			};
			std::unordered_map<Id, PageRef> page_refs; // wear references to pages
			std::unordered_map<Id, Position> positions;
			std::vector<Id> pages; // pages in order of the document
			std::vector<Id> columns;
			std::vector<Id> lines;
			std::vector<Id> words;
			std::vector<Id> characters;

			crn::StringUTF8 report;
	};

}

