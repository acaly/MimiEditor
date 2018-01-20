#pragma once
#include <cstdint>
#include <cstddef>
#include <cassert>

namespace Mimi
{
	const int TextSegmentTreeFactor = 64;

	class TextSegment;
	class TextSegmentList;
	class TextSegmentTree;
	class Document;

	struct DocumentPositionS
	{
		TextSegment* Segment;
		std::size_t Position;
	};

	struct DocumentPositionI
	{
		std::size_t Line;
		std::size_t Position;
	};

	//A B-tree like list to storage TextSegments
	class TextSegmentTree
	{
		friend class TextSegmentList;

	private:
		TextSegmentList* Root;

	public:
		//index
		//enumerate
		TextSegment* GetFirstSegment();
		TextSegment* GetSegmentWithLineIndex(std::size_t index);
		DocumentPositionI ConvertDocumentI(DocumentPositionS s);
		DocumentPositionS ConvertDocumentS(DocumentPositionI i);
	};

	class TextSegmentList
	{
		friend class TextSegmentTree;

	private:
		Document* DocumentPtr;
		TextSegmentTree* Tree;
		TextSegmentList* ParentNode;
		std::uint16_t Index;
		std::uint16_t ChildrenCount;
		std::uint32_t ElementCount;
		std::uint32_t LineCount;
		bool IsLeaf;
		
		void* (Data[TextSegmentTreeFactor + 1]); //TODO ensure last is nullptr

	public:
		TextSegmentList** DataAsNode()
		{
			assert(!IsLeaf);
			return reinterpret_cast<TextSegmentList**>(static_cast<void**>(Data));
		}

		TextSegment** DataAsElement()
		{
			assert(IsLeaf);
			return reinterpret_cast<TextSegment**>(static_cast<void**>(Data));
		}

	public:
		Document* GetDocument()
		{
			return DocumentPtr;
		}

		std::size_t GetAbsLineIndex()
		{
			if (!ParentNode) return 0;
			std::size_t l = ParentNode->GetAbsLineIndex();
			TextSegmentList** ptr = ParentNode->DataAsNode();
			for (std::size_t i = 0; i < Index; ++i)
			{
				l += ptr[i]->LineCount;
			}
			return l;
		}
		
		TextSegment* GetFirstElement()
		{
			TextSegmentList* n = this;
			while (!n->IsLeaf)
			{
				n = n->DataAsNode()[0];
			}
			return n->DataAsElement()[0];
		}

		TextSegment* GetLastElement()
		{
			TextSegmentList* n = this;
			while (!n->IsLeaf)
			{
				n = n->DataAsNode()[n->ChildrenCount - 1];
			}
			return n->DataAsElement()[n->ChildrenCount - 1];
		}

		TextSegment* GetElementBefore(std::size_t pos)
		{
			assert(IsLeaf);
			std::size_t p = pos;
			TextSegmentList* n = this;
			while (p == 0)
			{
				p = n->Index;
				n = n->ParentNode;
			}
			if (n->IsLeaf)
			{
				return n->DataAsElement()[p - 1];
			}
			else
			{
				return n->DataAsNode()[p - 1]->GetLastElement();
			}
		}

		TextSegment* GetElementAfter(std::size_t pos)
		{
			assert(IsLeaf);
			std::size_t p = pos;
			TextSegmentList* n = this;
			while (p == n->ChildrenCount - 1)
			{
				p = n->Index;
				n = n->ParentNode;
			}
			assert(p < n->ChildrenCount - 1u);
			if (n->IsLeaf)
			{
				return n->DataAsElement()[p + 1];
			}
			else
			{
				return n->DataAsNode()[p + 1]->GetFirstElement();
			}
		}

	private:
		static int CompareIndex(std::size_t i1, std::size_t i2)
		{
			if (i1 == i2) return 0;
			if (i1 > i2) return 1;
			return -1;
		}

	public:
		//0: same or error, 1: l1 is after l2, -1: l1 is before l2.
		static int ComparePosition(TextSegmentList* l1, TextSegmentList* l2)
		{
			if (l1 == l2) return 0;
			TextSegmentList* p1 = l1->ParentNode;
			TextSegmentList* p2 = l2->ParentNode;
			while (p1 != p2)
			{
				assert(p1 && p2);
				l1 = p1;
				l2 = p2;
				p1 = p1->ParentNode;
				p2 = p2->ParentNode;
			}
			return CompareIndex(l1->Index, l2->Index);
		}

	public:
		void InsertElement(std::size_t pos, TextSegment* element);
		void RemoveElement(std::size_t pos);
	};
}
