#include "Snapshot.h"
#include "TextDocument.h"

Mimi::Snapshot::~Snapshot()
{
	if (Document)
	{
		Document->DisposeSnapshot(this);
		Document = nullptr;
	}
	assert(BufferList.size() == 0);
}
