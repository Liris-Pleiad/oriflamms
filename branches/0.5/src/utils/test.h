#include <CRNStringUTF8.h>
#include <CRNIO/CRNPath.h>
#include <CRNGeometry/CRNRect.h>
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
			crn::xml::Element el;
	};

	class Word
	{
		public:
			const Id& GetId() const { return id; }
		private:
			Id id;
			std::vector<Id> characters;
			Id zone;
			crn::xml::Element el;
	};

	class Line
	{
		public:
		private:
			std::vector<Id> left, center, right;
	};

	class Column
	{
		public:
			const Id& GetId() const { return id; }
		private:
			Id id;
			std::vector<Line> lines;
			Id zone;
			crn::xml::Element el;
	};

	class Page
	{
		public:
			Page();// TODO
			Page(const Page&) = delete;
			Page(Page&&) = delete;
			Page& operator=(const Page&) = delete;
			Page& operator=(Page&&) = delete;
			~Page();

			const Id& GetId() const { return id; }

			const Column& GetColumn(const Id &id) const;
			Column& GetColumn(const Id &id);
			const Word& GetWord(const Id &id) const;
			Word& GetWord(const Id &id);
			const Character& GetCharacter(const Id &id) const;
			Character& GetCharacter(const Id &id);
			const Zone& GetZone(const Id &id) const;
			Zone& GetZone(const Id &id);

		private:
			Id id;
			Id zone;
			std::unordered_map<Id, Column> columns;
			std::unordered_map<Id, Word> words;
			std::unordered_map<Id, Character> characters;
			std::unordered_map<Id, Zone> zones;

			//crn::xml::Document transcription;
			//crn::xml::Document zonedefs;
			//crn::xml::Document links;
			// ou ?
			//std::unordered_map<crn::StringUTF8, crn::xml::Document> files;
			// ou carrément
			std::vector<crn::xml::Document> files;
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

		private:
			/*! \brief Reads the content of a TEI file containing the transcription of a document */
			void readText(const crn::Path &xmlpath);

			struct PageRef
			{
				std::shared_ptr<Page> GetPage();

				std::weak_ptr<Page> page;
				std::vector<crn::Path> files;
			};
			std::unordered_map<Id, PageRef> page_refs; // wear references to pages
			std::vector<Id> pages; // pages in order of the document
	};

}

