#include "../MimiEditor/ModificationTracer.h"

using namespace Mimi;

std::size_t TestModificationTracerSpeed()
{
	ModificationTracer t;
	t.Resize(1);
	const int WriteLen = 100;
	const int Repeat = 5000000;

	std::size_t ret = 0;
	for (int i = 0; i < Repeat; ++i)
	{
		t.NewSnapshot(1, 10);

		for (int j = 0; j < WriteLen; ++j)
		{
			t.Insert(5 + j, 1);
		}
		for (int j = WriteLen - 1; j >= 0; --j)
		{
			t.Delete(5 + j, 1);
		}

		t.DisposeSnapshot(1, 1);
		ret += t.ConvertFromSnapshot(0, 8, 0);
	}

	return ret;
}
