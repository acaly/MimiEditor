#include "Snapshot.h"
#include "SnapshotPositionConverter.h"

Mimi::SnapshotPositionConverter* Mimi::Snapshot::CreatePositionConverter()
{
	return new SnapshotPositionConverter(this);
}
