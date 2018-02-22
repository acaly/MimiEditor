#include "TextSegmentList.h"
#include "TextSegment.h"

Mimi::TextSegmentTree::TextSegmentTree(TextDocument* document, TextSegment* element)
{
	TextSegmentList* root = new TextSegmentList();
	Root = root;

	root->DocumentPtr = document;
	root->Index = 0;
	root->IsLeaf = true;
	root->ParentNode = nullptr;
	root->Tree = this;
	root->InsertElement(0, element);
}

Mimi::TextSegmentTree::~TextSegmentTree()
{
	assert(Root);
	delete Root;
	Root = nullptr;
}

Mimi::TextSegment* Mimi::TextSegmentTree::GetSegmentWithLineIndex(std::size_t index)
{
	TextSegmentList* node = Root;
	std::size_t count = 0;
	while (!node->IsLeaf)
	{
		TextSegmentList** ptr = node->DataAsNode();
		while (count + (*ptr)->LineCount <= index)
		{
			count = count + (*ptr)->LineCount;
			ptr += 1;
			assert(*ptr);
		}
		node = *ptr;
	}
	{
		TextSegment** ptr = node->DataAsElement();
		while (count < index)
		{
			if (!(*ptr)->IsContinuous())
			{
				count += 1;
			}
			ptr += 1;
			assert(*ptr);
		}
		return *ptr;
	}
}

Mimi::DocumentPositionL Mimi::TextSegmentTree::ConvertPositionToL(DocumentPositionS s)
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

Mimi::DocumentPositionS Mimi::TextSegmentTree::ConvertPositionFromL(DocumentPositionL l)
{
	TextSegment* seg = GetSegmentWithLineIndex(l.Line);
	std::size_t count = l.Position;
	while (count > seg->GetCurrentLength())
	{
		assert(seg->IsContinuous());
		count -= seg->GetCurrentLength();
		seg = seg->GetNextSegment();
	}
	return { seg, count };
}

Mimi::DocumentPositionD Mimi::TextSegmentTree::ConvertPositionToD(DocumentPositionS s)
{
	std::size_t count = s.Position;

	TextSegmentList* node = s.Segment->GetParent();

	{
		TextSegment** ptr = node->DataAsElement();
		std::size_t index = s.Segment->GetIndexInList();
		for (std::size_t i = 0; i < index; ++i)
		{
			count += ptr[i]->GetCurrentLength();
		}
	}

	while (node->ParentNode)
	{
		TextSegmentList** ptr = node->ParentNode->DataAsNode();
		std::size_t index = node->Index;
		for (std::size_t i = 0; i < index; ++i)
		{
			count += ptr[i]->DataLength;
		}
		node = node->ParentNode;
	}

	return { count };
}

Mimi::DocumentPositionS Mimi::TextSegmentTree::ConvertPositionFromD(DocumentPositionD d)
{
	TextSegmentList* node = Root;
	std::size_t count = 0;
	while (!node->IsLeaf)
	{
		TextSegmentList** ptr = node->DataAsNode();
		while (count + (*ptr)->DataLength <= d.Position)
		{
			count = count + (*ptr)->DataLength;
			ptr += 1;
			assert(*ptr);
		}
		node = *ptr;
	}
	{
		TextSegment** ptr = node->DataAsElement();
		while (count + (*ptr)->GetCurrentLength() <= d.Position)
		{
			count += (*ptr)->GetCurrentLength();
			ptr += 1;
			assert(*ptr);
		}
		return { *ptr, d.Position - count };
	}
}

void Mimi::TextSegmentTree::RemoveElement(TextSegment* e)
{
	assert(e->GetParent()->Tree == this);
	e->GetParent()->RemoveElement(e->GetIndexInList());
}

void Mimi::TextSegmentTree::InsertBefore(TextSegment* pos, TextSegment* newSegment)
{
	assert(pos->GetParent()->Tree == this);
	assert(newSegment->GetParent() == nullptr);
	pos->GetParent()->InsertElement(pos->GetIndexInList(), newSegment);
}

void Mimi::TextSegmentTree::InsertAfter(TextSegment* pos, TextSegment* newSegment)
{
	assert(pos->GetParent()->Tree == this);
	assert(newSegment->GetParent() == nullptr);
	pos->GetParent()->InsertElement(pos->GetIndexInList() + 1, newSegment);
}

void Mimi::TextSegmentTree::CheckChildrenIndexAndCount()
{
	Root->CheckChildrenIndexAndCount();
}

Mimi::TextSegmentList::~TextSegmentList()
{
	for (std::size_t i = 0; i < ChildrenCount; ++i)
	{
		if (IsLeaf)
		{
			delete DataAsElement()[i];
			DataAsElement()[i] = nullptr;
		}
		else
		{
			delete DataAsNode()[i];
			DataAsNode()[i] = nullptr;
		}
	}
	ChildrenCount = 0;
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
		newRoot->ElementCount = this->ElementCount;
		this->ParentNode = newRoot;
		Tree->Root = newRoot;
	}

	//Split with parent
	TextSegmentList* newNode = new TextSegmentList();
	newNode->DocumentPtr = this->DocumentPtr;
	newNode->Tree = this->Tree;
	newNode->IsLeaf = this->IsLeaf;
	newNode->ChildrenCount = 0;
	newNode->ElementCount = newNode->DataLength = newNode->LineCount = 0;
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

bool Mimi::TextSegmentList::CheckMerge()
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
				last->Merge(); //Delete this.
				last->UpdateLocalCount();
				return false;
			}
		}
		else
		{
			//The only child.
			//We need (and only need) to ensure there's no empty nodes.
			if (ChildrenCount == 0)
			{
				//Save it to stack as this is going to be deleted.
				TextSegmentList* p = ParentNode;
				p->RemovePointer(Index); //Delete this.
				p->CheckMerge();
				return false;
			}
		}
	}
	return true;
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
		ElementCount = ChildrenCount;
	}
	else
	{
		std::size_t l = 0, d = 0, e = 0;
		for (std::size_t i = 0; i < ChildrenCount; ++i)
		{
			TextSegmentList* node = DataAsNode()[i];
			l += node->LineCount;
			d += node->DataLength;
			e += node->ElementCount;
		}
		LineCount = static_cast<std::uint32_t>(l);
		DataLength = static_cast<std::uint32_t>(d);
		ElementCount = static_cast<std::uint32_t>(e);
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
	UpdateCount();
}

Mimi::TextSegment* Mimi::TextSegmentList::RemoveElement(std::size_t pos)
{
	assert(IsLeaf);
	//Don't remove the last element!
	assert(!(ParentNode == nullptr && ChildrenCount == 1));

	TextSegment* ret = DataAsElement()[pos];
	RemovePointer(pos);
	if (CheckMerge())
	{
		UpdateChildrenIndex(pos);
		UpdateCount();
	}
	return ret;
}

void Mimi::TextSegmentList::CheckChildrenIndexAndCount()
{
	if (IsLeaf)
	{
		std::size_t elements = 0, lines = 0, data = 0;
		for (std::size_t i = 0; i < TextSegmentTreeFactor; ++i)
		{
			if (i < ChildrenCount)
			{
				TextSegment* s = DataAsElement()[i];
				elements += 1;
				lines += s->IsContinuous() ? 0 : 1;
				data += s->GetCurrentLength();
			}
			else
			{
				assert(DataAsElement()[i] == nullptr);
			}
		}
		assert(elements == ElementCount);
		assert(lines == LineCount);
		assert(data == DataLength);
	}
	else
	{
		std::size_t elements = 0, lines = 0, data = 0;
		for (std::size_t i = 0; i < TextSegmentTreeFactor; ++i)
		{
			if (i < ChildrenCount)
			{
				TextSegmentList* n = DataAsNode()[i];
				n->CheckChildrenIndexAndCount();
				elements += n->ElementCount;
				lines += n->LineCount;
				data += n->DataLength;
			}
			else
			{
				assert(DataAsNode()[i] == nullptr);
			}
		}
		assert(elements == ElementCount);
		assert(lines == LineCount);
		assert(data == DataLength);
	}
}
