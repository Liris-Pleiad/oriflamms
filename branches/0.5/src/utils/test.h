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
	
	class View;
	class CharacterClass
	{
		public:
		private:
			crn::xml::Element el;
		
		friend class View;
	};

	class Zone
	{
		public:
			const Id& GetId() const noexcept { return id; }
		private:
			Id id;
			crn::Rect pos;
			std::vector<crn::Point2DInt> box;
			crn::StringUTF8 type;
			crn::xml::Element el;
		
		friend class View;
	};

	class Character
	{
		public:
			const Id& GetId() const noexcept { return id; }
			const crn::StringUTF8& GetText() const noexcept { return text; }
		private:
			Id id;
			Id zone;
			crn::StringUTF8 text;
		
		friend class View;
	};

	class Word
	{
		public:
			const Id& GetId() const { return id; }
			const crn::StringUTF8& GetText() const noexcept { return text; }
		private:
			Id id;
			std::vector<Id> characters;
			Id zone;
			crn::StringUTF8 text;
		
		friend class View;
	};

	class Line
	{
		public:
			const Id& GetId() const noexcept { return id; }
			Id id;
		private:
			std::vector<Id> left, center, right;
			Id zone;
		
		friend class View;
	};

	class Column
	{
		public:
			const Id& GetId() const noexcept { return id; }
		private:
			Id id;
			std::vector<Id> lines;
			Id zone;
		
		friend class View;
	};

	class Document;
	class Page
	{
		public:
			const Id& GetId() const noexcept { return id; }

		private:
			Id id;
			std::vector<Id> columns;
			Id zone;
		
		friend class View;
	};

	class View
	{
		public:
			View();
			View(const View&) = default;
			View(View&&) = default;
			View& operator=(const View&) = default;
			View& operator=(View&&) = default;
			~View();

			const Page& GetPage(const Id &id) const;
			Page& GetPage(const Id &id);
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
				Impl(const Id &surfid, const crn::Path &base, const crn::StringUTF8 &projname);
				void readTextWElement(crn::xml::Element &el, ElementPosition &pos);

				Id id;
				crn::SBlock img;
				std::vector<Id> pageorder;
				crn::Path imagename;
				
				std::unordered_map<Id, Page> pages;
				std::unordered_map<Id, Column> columns;
				std::unordered_map<Id, Line> lines;
				std::unordered_map<Id, Word> words;
				std::unordered_map<Id, Character> characters;
				std::unordered_map<Id, Zone> zones;

				crn::xml::Document links;
				std::unordered_map<crn::StringUTF8, crn::xml::Element> link_groups;
			};

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

			const crn::StringUTF8& ErrorReport() const noexcept { return report; }

			const ElementPosition& GetPosition(const Id &elem_id) const { return positions.find(elem_id)->second; }

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

