/* Copyright 2013-2015 INSA-Lyon, IRHT, ZHAO Xiaojuan, Université Paris Descartes
 *
 * file: OriStruct.h
 * \author Yann LEYDIER
 */

#ifndef OriStruct_HEADER
#define OriStruct_HEADER

#include <oriflamms_config.h>
#include <CRNGeometry/CRNRect.h>
#include <CRNMath/CRNProp3.h>
#include <CRNUtils/CRNProgress.h>
#include <OriFeatures.h>

namespace ori
{
	class Document;
	class TEISelectionNode;
	/*! \brief A word and its data */
	class Word
	{
		public:
			// can move, cannot copy
			Word(const Word&) = delete;
			Word(Word&&) = default;
			Word& operator=(const Word&) = delete;
			Word& operator=(Word&&) = default;

			/*! \brief Gets the xml:id of the word */
			const crn::StringUTF8& GetId() const { return id; }
			/*! \brief Gets the Unicode transcription */
			const crn::String& GetText() const { return text; }

			/*! \brief Gets the bounding box on the image */
			const crn::Rect& GetBBox() const { return bbox; }
			/*! \brief Sets the bounding box on the image */
			void SetBBox(const crn::Rect &r) { bbox = r; leftcorr = 0; rightcorr = 0; ClearFrontiers(); }

			/*! \brief Gets the user validation status */
			const crn::Prop3& GetValid() const { return valid; }
			/*! \brief Sets the user validation status */
			void SetValid(const crn::Prop3 &v) { valid = v; }

			/*! \brief Gets the associated signature string on the image */
			const crn::StringUTF8& GetImageSignature() const { return imgsig; }
			/*! \brief Sets the associated signature string on the image */
			void SetImageSignature(const crn::StringUTF8 &sig) { imgsig = sig; ClearFrontiers(); }

			/*! \brief Gets the total user correction on the left bound */
			int GetLeftCorrection() const { return leftcorr; }
			/*! \brief Gets the total user correction on the right bound */
			int GetRightCorrection() const { return rightcorr; }
			/*! \brief Sets the left bound */
			void SetLeft(int l) { leftcorr += l - bbox.GetLeft(); bbox.SetLeft(l); ClearFrontiers(); }
			/*! \brief Sets the right bound */
			void SetRight(int r) { rightcorr += r - bbox.GetRight(); bbox.SetRight(r); ClearFrontiers(); }

			/*! \brief Gets the left frontier */
			const std::vector<crn::Point2DInt>& GetFrontFrontier() const { return frontfrontier; }
			/*! \brief Gets the right frontier */
			const std::vector<crn::Point2DInt>& GetBackFrontier() const { return backfrontier; }
			/*! \brief Sets the left frontier */
			void SetFrontFrontier(const std::vector<crn::Point2DInt> &frontier) { frontfrontier = frontier; }
			/*! \brief Sets the left frontier */
			void SetFrontFrontier(std::vector<crn::Point2DInt> &&frontier) { frontfrontier = std::move(frontier); }
			/*! \brief Sets the right frontier */
			void SetBackFrontier(const std::vector<crn::Point2DInt> &frontier) { backfrontier = frontier; }
			/*! \brief Sets the right frontier */
			void SetBackFrontier(std::vector<crn::Point2DInt> &&frontier) { backfrontier = std::move(frontier); }

			/*! \brief Gets the inner character frontiers */
			const std::vector<std::vector<crn::Point2DInt>>& GetCharacterFrontiers() const { return frontiers; }
			/*! \brief Adds an inner character frontier */
			void AddCharacterFrontier(const std::vector<crn::Point2DInt> &frontier) { frontiers.push_back(frontier); }
			/*! \brief Adds an inner character frontier */
			void AddCharacterFrontier(std::vector<crn::Point2DInt> &&frontier) { frontiers.emplace_back(std::move(frontier)); }
			/*! \brief Gets the left frontier of a character */
			const std::vector<crn::Point2DInt>& GetCharacterFront(size_t i) const;
			/*! \brief Gets the right frontier of a character */
			const std::vector<crn::Point2DInt>& GetCharacterBack(size_t i) const;
			/*! \brief Representation of the characters (not used for storage!) */
			struct Character
			{
				Character(const crn::String &txt, const std::vector<crn::Point2DInt> &f, const std::vector<crn::Point2DInt> &b):text(txt),front(f),back(b) { }

				crn::String text;
				const std::vector<crn::Point2DInt> &front;
				const std::vector<crn::Point2DInt> &back;
			};
			/*! \brief Gets a representation of the characters */
			std::vector<Character> GetCharacters() const;

			/*! \brief Removes all word and character alignment info */
			void ClearAlignment();
			/*! \brief Removes character frontiers inside the word */
			void ClearCharacterFrontiers() { frontiers.clear(); }
			/*! \brief Removes all word and character frontiers */
			void ClearFrontiers() { frontiers.clear(); frontfrontier.clear(); backfrontier.clear(); }

			/*! \brief Gets the list of characters that have no frontiers (subscripted or diacritical) */
			const std::set<size_t> & GetIgnoreList() const { return ignore_list; }
			/*! \brief Sets the list of characters that have no frontiers (subscripted or diacritical) */
			void SetIgnoreList(const std::set<size_t> &il) { ignore_list = il; }
			/*! \brief Sets the list of characters that have no frontiers (subscripted or diacritical) */
			void SetIgnoreList(std::set<size_t> &&il) { ignore_list = std::move(il); }

			/*! \brief Is the word aligned? */
			bool IsAligned() const;
			/*! \brief Are the characters aligned? */
			bool CharactersAligned() const;

		private:
			Word(const crn::StringUTF8 &word_id, const crn::String &content):id(word_id),text(content),valid(crn::Prop3::Unknown),leftcorr(0),rightcorr(0) { }
			void setLeftCorrection(int c) { leftcorr = c; }
			void setRightCorrection(int c) { rightcorr = c; }

			crn::StringUTF8 id; // <w xml::id="…">
			crn::String text;
			crn::Rect bbox;
			crn::Prop3 valid;
			crn::StringUTF8 imgsig;
			int leftcorr, rightcorr;
			std::vector<std::vector<crn::Point2DInt>> frontiers;
			std::vector<crn::Point2DInt> frontfrontier;
			std::vector<crn::Point2DInt> backfrontier;
			std::set<size_t> ignore_list;

			friend class Document;
	};
	/*! \brief Path of a word in the TEI XML file */
	struct WordPath
	{
		WordPath(size_t v, size_t c, size_t l, size_t w):view(v),col(c),line(l),word(w) { }
		WordPath(const crn::String &s);
		crn::String ToString() const;
		inline bool operator==(const WordPath &p) const { return (view == p.view) && (col == p.col) && (line == p.line) && (word == p.word); }
		inline bool operator!=(const WordPath &p) const { return ! (*this == p); }
		inline bool operator<(const WordPath &p) const
		{
			if (view < p.view) return true;
			else if (view == p.view)
			{
				if (col < p.col) return true;
				else if (col == p.col)
				{
					if (line < p.line) return true;
					else if (line == p.line) return word < p.word;
				}
			}
			return false;
		}

		size_t view, col, line, word;
	};

	/*! \brief A text line and its data */
	class Line
	{
		public:
			/*! \brief Gets the line number */
			const crn::StringUTF8& GetNumber() const { return num; }
			/*! \brief Gets the list of words */
			const std::vector<Word>& GetWords() const { return words; }
			/*! \brief Gets the list of words */
			std::vector<Word>& GetWords() { return words; }
			/*! \brief Does the line contain words? */
			bool IsEmpty() const { return words.empty(); }
			/*! \brief Does the line a wrapped part of another line? */
			bool IsWrapping() const { return wrapping; }
			/*! \brief Appends the content of a wrapped part */
			void Append(Line &other);
			/*! \brief Is the line manually corrected */
			bool GetCorrected() const { return corrected; }
			/*! \brief Sets if the line is manually corrected */
			void SetCorrected(bool cor = true) { corrected = cor; }

		private:
			Line(const crn::StringUTF8 &n, bool wrap):num(n),wrapping(wrap),corrected(false) { }

			std::vector<Word> words; // <w>
			crn::StringUTF8 num; // <lb n="…"/>
			bool wrapping; // <lb type="rejet"/>
			bool corrected;

			friend class Document;
	};

	/*! \brief A text column and its data */
	class Column
	{
		public:
			/*! \brief Gets the column number */
			const crn::StringUTF8& GetNumber() const { return num; }
			/*! \brief Gets the column xml:id */
			const crn::StringUTF8& GetId() const { return id; }
			/*! \brief Gets the list of text lines */
			std::vector<Line>& GetLines() { return lines; }
			/*! \brief Gets the list of text lines */
			const std::vector<Line>& GetLines() const { return lines; }
			/*! \brief Does the column contain text lines? */
			bool IsEmpty() const { return lines.empty(); }
			/*! \brief Merges wrapping lines and removes empty text lines */
			void Cleanup();

		private:
			Column(const crn::StringUTF8 &i, const crn::StringUTF8 &n):id(i),num(n) { }

			crn::StringUTF8 num; // <cb n="…">
			crn::StringUTF8 id; // <cb id="…">
			std::vector<Line> lines; // <lb>

			friend class Document;
	};

	/*! \brief An image and its content */
	class View
	{
		public:
			/*! \brief Gets the file name of the corresponding image */
			const crn::StringUTF8& GetImageName() const { return imagename; }
			/*! \brief Gets the page xml:id */
			const crn::StringUTF8& GetId() const { return id; }
			/*! \brief Gets the list of columns */
			std::vector<Column>& GetColumns() { return columns; }
			/*! \brief Gets the list of columns */
			const std::vector<Column>& GetColumns() const { return columns; }
			/*! \brief Does the view contain text columns? */
			bool IsEmpty() const { return columns.empty(); }
			/*! \brief Merges wrapping lines and removes empty text lines and columns */
			void Cleanup();

		private:
			View(const crn::StringUTF8 &fname, const crn::StringUTF8 &i):imagename(fname),id(i) { }

			crn::StringUTF8 imagename; // <pb facs="…">
			crn::StringUTF8 id; // <pb id="…">
			std::vector<Column> columns; // <cb n="…">

			friend class Document;
	};

	/*! \brief A TEI document */
	class Document
	{
		public:
			/*! \brief Adds a view at the end of the document */
			void AppendView(const crn::StringUTF8 &fname, const crn::StringUTF8 &i = "");
			/*! \brief Adds a column at the end of the last view */
			void AppendColumn(const crn::StringUTF8 &i, const crn::StringUTF8 &n);
			/*! \brief Adds a text line at the end of the last column */
			void AppendLine(const crn::StringUTF8 &n, bool wrap = false);
			/*! \brief Adds a word at the end of the last text line */
			void AppendWord(const crn::StringUTF8 &word_id, const crn::String &content);

			/*! \brief Gets the list of views */
			const std::vector<View>& GetViews() const { return views; }
			/*! \brief Gets the list of views */
			std::vector<View>& GetViews() { return views; }
			/*! \brief Gets a view */
			View& GetView(const WordPath &p) { return views[p.view]; }
			/*! \brief Gets a view */
			const View& GetView(const WordPath &p) const { return views[p.view]; }
			/*! \brief Gets a column */
			Column& GetColumn(const WordPath &p) { return views[p.view].columns[p.col]; }
			/*! \brief Gets a column */
			const Column& GetColumn(const WordPath &p) const { return views[p.view].columns[p.col]; }
			/*! \brief Gets a text line */
			Line& GetLine(const WordPath &p) { return views[p.view].columns[p.col].lines[p.line]; }
			/*! \brief Gets a text line */
			const Line& GetLine(const WordPath &p) const { return views[p.view].columns[p.col].lines[p.line]; }
			/*! \brief Gets a word */
			Word& GetWord(const WordPath &p) { return views[p.view].columns[p.col].lines[p.line].words[p.word]; }
			/*! \brief Gets a word */
			const Word& GetWord(const WordPath &p) const { return views[p.view].columns[p.col].lines[p.line].words[p.word]; }

			/*! \brief Gets the file name of the TEI XML file */
			const crn::Path& GetTEIPath() const { return teipath; }

			/*! \brief Imports the data of a TEI XML file */
			void Import(const crn::Path &fname, const TEISelectionNode &sel);
			/*! \brief Loads the saved structure */
			void Load(const crn::Path &fname);
			/*! \brief Saves the structure */
			void Save(const crn::Path &fname, crn::Progress *prog = nullptr) const;

			/*! \brief Merges wrapping lines and removes empty text lines and columns */
			void Cleanup();

		private:
			void read_tei(crn::xml::Element &el, const TEISelectionNode &sel, crn::StringUTF8 &lastwordid, crn::StringUTF8 &txt);

			std::vector<View> views; // <pb>
			crn::Path teipath;
	};

}

#endif


