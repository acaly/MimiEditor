#include "TestCommon.h"
#include "../MimiEditor/TextSegment.h"
#include "../MimiEditor/TextDocument.h"
#include "../MimiEditor/Snapshot.h"
#include "../MimiEditor/SnapshotReader.h"

using namespace Mimi;

namespace
{
	class LineModificationTester
	{
	public:
		LineModificationTester(lest::env& lest_env)
			: lest_env(lest_env),
				Doc(TextDocument::CreateEmpty(CodePageManager::UTF16LE)),
				LineBuffer(10)
		{
		}

		~LineModificationTester()
		{
			delete Doc;
		}

	private:
		lest::env& lest_env;
		TextDocument* const Doc;
		std::vector<int> Lines;
		int NextLineId = 0;
		DynamicBuffer LineBuffer;

	private:
		void AppendChar(char16_t c)
		{
			LineBuffer.Append(reinterpret_cast<const uint8_t*>(&c), 2);
		}

		DynamicBuffer& MakeBuffer(int id)
		{
			LineBuffer.Clear();
			assert(id + 128 < 0xD800);
			AppendChar(static_cast<char16_t>(id + 128));
			AppendChar('\r');
			AppendChar('\n');
			return LineBuffer;
		}

	public:
		void Append()
		{
			TextSegment* s = Doc->SegmentTree.GetLastSegment();
			assert(s->GetCurrentLength() == 0);
			int id = NextLineId++;
			Lines.push_back(id);
			Doc->Insert(Doc->GetTime(), { s, 0 }, MakeBuffer(id), true, 0, 0);
			Doc->SegmentTree.CheckChildrenIndexAndCount();
		}

		void Insert(std::size_t pos)
		{
			DocumentPositionL l = { pos, 0 };
			DocumentPositionS s = Doc->SegmentTree.ConvertPositionFromL(l);
			int id = NextLineId++;
			Lines.insert(Lines.begin() + pos, id);
			Doc->Insert(Doc->GetTime(), { s.Segment, 0 }, MakeBuffer(id), true, 0, 0);
			Doc->SegmentTree.CheckChildrenIndexAndCount();
		}

		void Delete(std::size_t pos)
		{
			Lines.erase(Lines.begin() + pos);
			DocumentPositionL l = { pos, 0 };
			DocumentPositionS s = Doc->SegmentTree.ConvertPositionFromL(l);
			std::size_t len = s.Segment->GetCurrentLength();
			assert(len > 0);
			Doc->DeleteRange(Doc->GetTime(), { s.Segment, 0 }, { s.Segment, len });
			Doc->SegmentTree.CheckChildrenIndexAndCount();
		}

	private:
		void CheckConnectivity()
		{
			TextSegment* s = Doc->SegmentTree.GetFirstSegment();
			TextSegment* last = Doc->SegmentTree.GetLastSegment();
			EXPECT(s);
			EXPECT(last);
			std::size_t n = Doc->SegmentTree.GetLineCount();
			std::size_t n2 = Doc->SegmentTree.GetElementCount();
			EXPECT(n > 0);
			EXPECT(n == n2);
			EXPECT(s->GetPreviousSegment() == nullptr);
			while (s != last)
			{
				n -= 1;
				s = s->GetNextSegment();
				EXPECT(s);
			}
			EXPECT(n == 1);
		}

		void CheckData()
		{
			std::unique_ptr<Snapshot> snapshot = std::unique_ptr<Snapshot>(Doc->CreateSnapshot());
			SnapshotReader r(snapshot.get());
			char16_t buffer[3];
			std::size_t checkRead;
			int vectorIndex = 0;
			while (r.GetPosition() < r.GetSize())
			{
				int id = Lines[vectorIndex++];
				bool suc = r.Read(reinterpret_cast<mchar8_t*>(buffer), 6, &checkRead);
				EXPECT((vectorIndex <= Lines.size()) && suc && checkRead == 6 && buffer[0] == id + 128);
			}
			EXPECT(vectorIndex == Lines.size());
		}

	public:
		void CheckList()
		{
			CheckConnectivity();
			CheckData();
		}
	};
}

DEFINE_MODULE(TestSegmentListModification)
{
	CASE("Append")
	{
		LineModificationTester t(lest_env);
		for (int i = 0; i < 10000; ++i)
		{
			t.Append();
		}
		t.CheckList();
	},
	CASE("Insert forward")
	{
		LineModificationTester t(lest_env);
		for (int i = 0; i < 200; ++i)
		{
			t.Append();
		}
		for (int i = 0; i < 10000; ++i)
		{
			t.Insert(100 + i);
		}
		t.CheckList();
	},
	CASE("Insert backward")
	{
		LineModificationTester t(lest_env);
		for (int i = 0; i < 200; ++i)
		{
			t.Append();
		}
		for (int i = 0; i < 10000; ++i)
		{
			t.Insert(100);
		}
		t.CheckList();
	},
	CASE("Insert to beginning")
	{
		LineModificationTester t(lest_env);
		for (int i = 0; i < 10000; ++i)
		{
			t.Insert(0);
		}
		t.CheckList();
	},
	CASE("Insert randomly")
	{
		LineModificationTester t(lest_env);
		for (int i = 0; i < 200; ++i)
		{
			t.Append();
		}
		int pos = 0;
		for (int i = 0; i < 10000; ++i)
		{
			pos = (pos + 23456789) % (200 + i);
			t.Insert(pos);
		}
		t.CheckList();
	},
	CASE("Delete forward")
	{
		LineModificationTester t(lest_env);
		for (int i = 0; i < 10200; ++i)
		{
			t.Append();
		}
		for (int i = 0; i < 10000; ++i)
		{
			t.Delete(100);
		}
		t.CheckList();
	},
	CASE("Delete backward")
	{
		LineModificationTester t(lest_env);
		for (int i = 0; i < 10200; ++i)
		{
			t.Append();
		}
		for (int i = 0; i < 10000; ++i)
		{
			t.Delete(10000 - i + 100);
		}
		t.CheckList();
	},
	CASE("Delete from beginning")
	{
		LineModificationTester t(lest_env);
		for (int i = 0; i < 10200; ++i)
		{
			t.Append();
		}
		for (int i = 0; i < 10000; ++i)
		{
			t.Delete(0);
		}
		t.CheckList();
	},
	CASE("Delete from end")
	{
		LineModificationTester t(lest_env);
		for (int i = 0; i < 10200; ++i)
		{
			t.Append();
		}
		for (int i = 0; i < 10000; ++i)
		{
			t.Delete(10200 - i - 1);
		}
		t.CheckList();
	},
	CASE("Delete randomly")
	{
		LineModificationTester t(lest_env);
		for (int i = 0; i < 10200; ++i)
		{
			t.Append();
		}
		int pos = 0;
		for (int i = 0; i < 10000; ++i)
		{
			pos = (pos + 23456789) % (10200 - i);
			t.Delete(pos);
		}
		t.CheckList();
	},
};
