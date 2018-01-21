#pragma once
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <cstring>

namespace Mimi
{
	const std::size_t TextSegmentTreeFactor = 64;

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
		//TODO convert data pos/char pos?
	};

	class TextSegmentList
	{
		friend class TextSegmentTree;

	private:
		TextSegmentList()
			: Data() //Initialize with nullptrs
		{
		}

	private:
		Document* DocumentPtr;
		TextSegmentTree* Tree;
		TextSegmentList* ParentNode;
		std::uint16_t Index;
		std::uint16_t ChildrenCount;
		std::uint32_t LineCount;
		std::uint32_t DataLength;
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
				if (!n)
				{
					return nullptr;
				}
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
				if (!n)
				{
					return nullptr;
				}
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

	private:
		void InsertPointer(std::size_t pos, void* ptr)
		{
			assert(ChildrenCount + 1 < TextSegmentTreeFactor);
			std::memmove(&Data[pos + 1], &Data[pos], sizeof(void*) * (ChildrenCount - pos));
			Data[pos] = ptr;
			ChildrenCount += 1;
		}

		void RemovePointer(std::size_t pos)
		{
			std::memmove(&Data[pos], &Data[pos + 1], sizeof(void*) * (ChildrenCount - pos - 1));
			ChildrenCount -= 1;
			Data[ChildrenCount] = nullptr;
		}

		void MovePointers(TextSegmentList* dest, std::size_t pos, std::size_t len, std::size_t destPos)
		{
			assert(pos < ChildrenCount && len <= ChildrenCount - pos);
			assert(destPos <= dest->ChildrenCount);
			assert(len < TextSegmentTreeFactor - dest->ChildrenCount);

			std::memmove(&dest->Data[destPos + len], &dest->Data[destPos],
				sizeof(void*) * (dest->ChildrenCount - destPos));
			std::memcpy(&dest->Data[destPos], &Data[pos], sizeof(void*) * len);
			dest->ChildrenCount += static_cast<std::uint16_t>(len);

			std::memmove(&Data[pos], &Data[pos + len], sizeof(void*) * (ChildrenCount - pos - len));
			std::memset(&Data[ChildrenCount - len], 0, sizeof(void*) * len);
			ChildrenCount -= static_cast<std::uint16_t>(len);
		}

		//Caller, after calling this function, must call:
		//  returnvalue.UpdateChildrenIndex()
		//  returnvalue.UpdateLocalCount()
		//  this->UpdateCount() - if with following insertion/deletion
		//  this->UpdateLocalCount() - if no insertion/deletion
		//returnvalue is always in the same parent as this.
		TextSegmentList* Split(std::size_t pos);
		void Merge();
		void CheckSplit(std::size_t index, void* newPtr);
		void CheckMerge();

		void UpdateLocalCount();
		void UpdateCount()
		{
			TextSegmentList* n = this;
			do
			{
				n->UpdateLocalCount();
				n = n->ParentNode;
			} while (n);
		}
		void UpdateChildrenIndex(std::size_t p = 0);

	public:
		void InsertElement(std::size_t pos, TextSegment* element);
		TextSegment* RemoveElement(std::size_t pos);
	};
}
