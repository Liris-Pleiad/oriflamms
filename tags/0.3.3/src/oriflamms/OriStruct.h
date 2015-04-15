/* Copyright 2013 INSA-Lyon & IRHT
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
	class Word
	{
		public:
			const crn::StringUTF8& GetId() const { return id; }
			const crn::String& GetText() const { return text; }
			const crn::Rect& GetBBox() const {return bbox; }
			void SetBBox(const crn::Rect &r) { bbox = r; leftcorr = 0; rightcorr = 0; }
			const crn::Prop3& GetValid() const { return valid; }
			void SetValid(const crn::Prop3 &v) { valid = v; }
			const crn::StringUTF8& GetImageSignature() const { return imgsig; }
			void SetImageSignature(const crn::StringUTF8 &sig) { imgsig = sig; }
			int GetLeftCorrection() const { return leftcorr; }
			int GetRightCorrection() const { return rightcorr; } void SetLeft(int l) { leftcorr += l - bbox.GetLeft(); bbox.SetLeft(l); }
			void SetRight(int r) { rightcorr += r - bbox.GetRight(); bbox.SetRight(r); }

		private:
			Word(const crn::StringUTF8 &word_id, const crn::String &content):id(word_id),text(content),valid(crn::Prop3::Unknown),leftcorr(0),rightcorr(0) { }
			void setLeftCorrection(int c) { leftcorr = c; }
			void setRightCorrection(int c) { rightcorr = c; }

			crn::StringUTF8 id; // <w xml::id="…">
			crn::String text; // <choice><me:facs>…
			crn::Rect bbox;
			crn::Prop3 valid;
			crn::StringUTF8 imgsig;
			int leftcorr, rightcorr;

		friend class Document;
	};
	struct WordPath
	{
		WordPath(size_t v, size_t c, size_t l, size_t w):view(v),col(c),line(l),word(w) { }
		WordPath(const crn::String &s);
		const crn::String ToString() const;
		inline bool operator==(const WordPath &p) const { return (view == p.view) && (col == p.col) && (line == p.line) && (word == p.word); }

		size_t view, col, line, word;
	};

	class Line
	{
		public:
			const crn::StringUTF8& GetNumber() const { return num; }
			const std::vector<Word>& GetWords() const { return words; }
			std::vector<Word>& GetWords() { return words; }
			bool IsEmpty() const { return words.empty(); }
			const crn::StringUTF8& GetNum() const { return num; }
			bool IsWrapping() const { return wrapping; }
			void Append(const Line &other);
			//void Align(const std::vector<ImageSignature> &img);
			//void Align(const std::vector<std::pair<crn::Rect, crn::StringUTF8> > &sig);
			bool GetCorrected() const { return corrected; }
			void SetCorrected(bool cor = true) { corrected = cor; }

		private:
			Line(const crn::StringUTF8 &n, bool wrap):num(n),wrapping(wrap),corrected(false) { }
			//const std::vector<TextSignature> featurize() const;

			std::vector<Word> words; // <w>
			crn::StringUTF8 num; // <lb n="…"/>
			bool wrapping; // <lb type="rejet"/>
			bool corrected;

		friend class Document;
	};

	class Column
	{
		public:
			const crn::StringUTF8& GetNumber() const { return num; }
			const crn::StringUTF8& GetId() const { return id; }
			const std::vector<Line>& GetLines() const { return lines; }
			std::vector<Line>& GetLines() { return lines; }
			bool IsEmpty() const { return lines.empty(); }
			void Cleanup();

		private:
			Column(const crn::StringUTF8 &i, const crn::StringUTF8 &n):id(i),num(n) { }

			crn::StringUTF8 num; // <cb n="…">
			crn::StringUTF8 id; // <cb id="…">
			std::vector<Line> lines; // <lb>

		friend class Document;
	};

	class View
	{
		public:
			const crn::StringUTF8& GetImageName() const { return imagename; }
			const crn::StringUTF8& GetId() const { return id; }
			const std::vector<Column>& GetColumns() const { return columns; }
			std::vector<Column>& GetColumns() { return columns; }
			bool IsEmpty() const { return columns.empty(); }
			void Cleanup();

		private:
			View(const crn::StringUTF8 &fname, const crn::StringUTF8 &i):imagename(fname),id(i) { }

			crn::StringUTF8 imagename; // <pb facs="…">
			crn::StringUTF8 id; // <pb id="…">
			std::vector<Column> columns; // <cb n="…">

		friend class Document;
	};

	class Document
	{
		public:
			void AppendView(const crn::StringUTF8 &fname, const crn::StringUTF8 &i = "");
			void AppendColumn(const crn::StringUTF8 &i, const crn::StringUTF8 &n);
			void AppendLine(const crn::StringUTF8 &n, bool wrap = false);
			void AppendWord(const crn::StringUTF8 &word_id, const crn::String &content);

			const std::vector<View>& GetViews() const { return views; }
			std::vector<View>& GetViews() { return views; }
			const View& GetView(const WordPath &p) const { return views[p.view]; }
			View& GetView(const WordPath &p) { return views[p.view]; }
			const Column& GetColumn(const WordPath &p) const { return views[p.view].columns[p.col]; }
			Column& GetColumn(const WordPath &p) { return views[p.view].columns[p.col]; }
			const Line& GetLine(const WordPath &p) const { return views[p.view].columns[p.col].lines[p.line]; }
			Line& GetLine(const WordPath &p) { return views[p.view].columns[p.col].lines[p.line]; }
			const Word& GetWord(const WordPath &p) const { return views[p.view].columns[p.col].lines[p.line].words[p.word]; }
			Word& GetWord(const WordPath &p) { return views[p.view].columns[p.col].lines[p.line].words[p.word]; }

			const crn::Path& GetTEIPath() const { return teipath; }

			void Import(const crn::Path &fname);
			void Load(const crn::Path &fname);
			void Save(const crn::Path &fname, crn::Progress *prog = NULL) const;

			void Cleanup();
		private:

			std::vector<View> views; // <pb>
			crn::Path teipath;
	};
}

namespace std
{
	inline bool operator<(const ori::WordPath &p1, const ori::WordPath &p2)
	{
		if (p1.view < p2.view)
			return true;
		else if (p1.view == p2.view)
		{
			if (p1.col < p2.col)
				return true;
			else if (p1.col == p2.col)
			{
				if (p1.line < p2.line)
					return true;
				else if (p1.line == p2.line)
					return p1.word < p2.word;
			}
		}
		return false;
	}
}
#endif


