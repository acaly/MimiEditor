#include "TextSegmentList.h"
#include "TextSegment.h"

Mimi::TextSegment* Mimi::TextSegmentTree::GetFirstSegment()
{
	TextSegmentList* node = Root;
	while (!node->IsLeaf)
	{
		node = node->DataAsNode()[0];
	}
	return node->DataAsElement()[0];
}

Mimi::TextSegment* Mimi::TextSegmentTree::GetSegmentWithLineIndex(std::size_t index)
{
	TextSegmentList* node = Root;
	std::size_t count = 0;
	while (!node->IsLeaf)
	{
		TextSegmentList** ptr = node->DataAsNode();
		while (count + (*ptr)->LineCount < index)
		{
			count = count + (*ptr)->LineCount;
			ptr += 1;
			assert(*ptr);
		}
		node = *ptr;
	}
	TextSegment** ptrs = node->DataAsElement();
	while (count < index)
	{
		if (!(*ptrs)->IsContinuous())
		{
			count += 1;
		}
		ptrs += 1;
		assert(*ptrs);
	}
	return *ptrs;
}

Mimi::DocumentPositionI Mimi::TextSegmentTree::ConvertDocumentI(DocumentPositionS s)
{
	TextSegment* seg = s.Segment;
	std::size_t count = s.Position;
	std::size_t line = seg->GetLineIndex();
	while (seg->IsContinuous())
	{
		seg = seg->GetPreviousSegment();
		count += seg->GetCurrentLength();
	}
	return { line, count };
}

Mimi::DocumentPositionS Mimi::TextSegmentTree::ConvertDocumentS(DocumentPositionI i)
{
	TextSegment* seg = GetSegmentWithLineIndex(i.Line);
	std::size_t count = i.Position;
	while (count < seg->GetCurrentLength())
	{
		assert(seg->IsContinuous());
		count -= seg->GetCurrentLength();
		seg = seg->GetNextSegment();
	}
	return { seg, count };
}

Mimi::TextSegmentList* Mimi::TextSegmentList::Split(std::size_t pos)
{
	assert(pos < ChildrenCount);
	if (ParentNode == nullptr)
	{
		//Split root
		TextSegmentList* newRoot = new TextSegmentList();
		newRoot->ParentNode = nullptr;
		newRoot->DocumentPtr = this->DocumentPtr;
		newRoot->Tree = this->Tree;
		newRoot->Index = 0;
		newRoot->IsLeaf = false;
		newRoot->ChildrenCount = 1;
		newRoot->DataAsNode()[0] = this;
		newRoot->DataLength = this->DataLength;
		newRoot->LineCount = this->LineCount;
		this->ParentNode = newRoot;
		Tree->Root = newRoot;
	}

	//Split with parent
	TextSegmentList* newNode = new TextSegmentList();
	newNode->DocumentPtr = this->DocumentPtr;
	newNode->Tree = this->Tree;
	newNode->IsLeaf = this->IsLeaf;
	newNode->ChildrenCount = 0;
	newNode->DataLength = newNode->LineCount = 0;
	MovePointers(newNode, pos, ChildrenCount - pos, 0);
	newNode->UpdateLocalCount();
	//newNode->UpdateChildrenIndex is delayed to caller.
	//this->UpdateLocalCount is delayed to caller.
	//this->UpdateChildrenIndex is not necessary as these children are not moved.

	ParentNode->CheckSplit(Index + 1, newNode);
	return newNode;
}

void Mimi::TextSegmentList::Merge()
{
	assert(ParentNode);
	assert(Index < ParentNode->ChildrenCount - 1); //not last
	TextSegmentList* next = ParentNode->DataAsNode()[Index + 1];
	assert(next);
	assert(next->ChildrenCount < TextSegmentTreeFactor - ChildrenCount);
	std::size_t oldChildrenCount = ChildrenCount;
	next->MovePointers(this, 0, next->ChildrenCount, ChildrenCount);
	ParentNode->RemovePointer(next->Index);
	ParentNode->UpdateChildrenIndex(Index + 1);
	UpdateChildrenIndex(oldChildrenCount);
	UpdateLocalCount();
	delete next;
	ParentNode->CheckMerge();
}

void Mimi::TextSegmentList::CheckSplit(std::size_t index, void* newPtr)
{
	if (ChildrenCount + 1 < TextSegmentTreeFactor)
	{
		InsertPointer(index, newPtr);
		UpdateChildrenIndex(index);
		//UpdateLocalCount is not necessary.
	}
	else
	{
		std::size_t splitPos = ChildrenCount / 2;
		TextSegmentList* next = Split(splitPos);
		//We must call following after this split, but let's wait for the insertion.
		//  next->Index
		//  next->Count
		//  this->Count

		//Check with index - 1 to ensure the newPtr is always with previous ptr.
		//This makes Split easier, as the two Node shares one ParentNode. Count and index
		//updates are only performed once. This also avoids insertion to the beginning of
		//an array moving all existing values.
		if (index - 1 < splitPos)
		{
			InsertPointer(index, newPtr);
			UpdateChildrenIndex(index);
		}
		else
		{
			next->InsertPointer(index - splitPos, newPtr);
		}
		next->UpdateChildrenIndex();
		next->UpdateLocalCount();
		UpdateLocalCount();
	}
}

void Mimi::TextSegmentList::CheckMerge()
{
	//Check threshold = 2/3 capacity
	if (ChildrenCount < TextSegmentTreeFactor / 3 * 2 && ParentNode)
	{
		if (Index < ParentNode->ChildrenCount - 1)
		{
			TextSegmentList* next = ParentNode->DataAsNode()[Index + 1];
			assert(next);
			if (next->ChildrenCount < TextSegmentTreeFactor - ChildrenCount)
			{
				Merge();
			}
		}
		else if (Index > 0)
		{
			TextSegmentList* last = ParentNode->DataAsNode()[Index - 1];
			assert(last);
			if (last->ChildrenCount < TextSegmentTreeFactor - ChildrenCount)
			{
				last->Merge();
			}
		}
	}
}

void Mimi::TextSegmentList::UpdateLocalCount()
{
	if (IsLeaf)
	{
		std::size_t l = 0, d = 0;
		for (std::size_t i = 0; i < ChildrenCount; ++i)
		{
			l += DataAsElement()[i]->IsContinuous() ? 0 : 1;
			d += DataAsElement()[i]->GetCurrentLength();
		}
		LineCount = static_cast<std::uint32_t>(l);
		DataLength = static_cast<std::uint32_t>(d);
	}
	else
	{
		std::size_t l = 0, d = 0;
		for (std::size_t i = 0; i < ChildrenCount; ++i)
		{
			l += DataAsNode()[i]->LineCount;
			d += DataAsNode()[i]->DataLength;
		}
		LineCount = static_cast<std::uint32_t>(l);
		DataLength = static_cast<std::uint32_t>(d);
	}
}

void Mimi::TextSegmentList::UpdateChildrenIndex(std::size_t p)
{
	if (IsLeaf)
	{
		for (std::size_t i = p; i < ChildrenCount; ++i)
		{
			DataAsElement()[i]->Index = static_cast<std::uint16_t>(i);
			DataAsElement()[i]->Parent = this;
		}
	}
	else
	{
		for (std::size_t i = p; i < ChildrenCount; ++i)
		{
			DataAsNode()[i]->Index = static_cast<std::uint16_t>(i);
			DataAsNode()[i]->ParentNode = this;
		}
	}
}

void Mimi::TextSegmentList::InsertElement(std::size_t pos, TextSegment* element)
{
	assert(IsLeaf);
	CheckSplit(pos, element);
	//CheckSplit has updated local count. Continue from parent.
	if (ParentNode)
	{
		ParentNode->UpdateCount();
	}
}

Mimi::TextSegment* Mimi::TextSegmentList::RemoveElement(std::size_t pos)
{
	assert(IsLeaf);
	TextSegment* ret = DataAsElement()[pos];
	RemovePointer(pos);
	CheckMerge();
	return ret;
}
