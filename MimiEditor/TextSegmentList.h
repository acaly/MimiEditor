#pragma once
#include <cstdint>
#include <cstddef>
#include <cassert>

namespace Mimi
{
	const int TextSegmentTreeFactor = 64;

	class TextSegment;
	class TextSegmentList;
	class Document;

	//A B-tree like list to storage TextSegments
	class TextSegmentTree
	{
		friend class TextSegmentList;

	private:
		TextSegmentList* Root;

	public:
		//index
		//enumerate
	};

	class TextSegmentList
	{
	private:
		Document* DocumentPtr;
		TextSegmentTree* Tree;
		TextSegmentList* ParentNode;
		std::uint16_t Index;
		std::uint16_t ChildrenCount;
		std::uint32_t ElementCount;
		std::uint32_t LineCount;
		bool IsLeaf;
		
		void* (Data[TextSegmentTreeFactor]);

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

		std::size_t GetAbsIndex();
		std::size_t GetAbsLineIndex();

		TextSegment* GetElementBefore(std::size_t pos);
		TextSegment* GetElementAfter(std::size_t pos);

	public:
		void InsertElement(std::size_t pos, TextSegment* element);
		void RemoveElement(std::size_t pos);
	};
}
