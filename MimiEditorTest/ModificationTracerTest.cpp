#include "lest.hpp"
#include "../MimiEditor/ModificationTracer.h"
#include <vector>

using namespace Mimi;

class ModificationTester
{
public:
	ModificationTester(lest::env& lest_env, int size)
		: lest_env(lest_env)
	{
		Tracer.Resize(1);
		Tracer.NewSnapshot(1, size);

		InitialSize = size;
		for (int i = 0; i < size; ++i)
		{
			Items.push_back(i);
		}
	}

	void Insert(int pos, int len)
	{
		Tracer.Insert(pos, len);
		Items.insert(Items.begin() + pos, len, -1);
		CheckSequence();
	}

	void Delete(int pos, int len)
	{
		Tracer.Delete(pos, len);
		Items.erase(Items.begin() + pos, Items.begin() + pos + len);
		CheckSequence();
	}

	void CheckSequence()
	{
		EXPECT(Tracer.CheckSequence(1, InitialSize));
	}

	void CheckConversion()
	{
		int last = -1;
		unsigned int vectorPosition = 0;
		unsigned int lastVectorPosition = ModificationTracer::PositionDeleted;
		while (vectorPosition < Items.size())
		{
			if (Items[vectorPosition] != -1)
			{
				int current = Items[vectorPosition];

				for (int i = last + 1; i < current; ++i)
				{
					EXPECT(Tracer.ConvertFromSnapshot(0, i, 0) == ModificationTracer::PositionDeleted);
					EXPECT(Tracer.ConvertFromSnapshot(0, i, -1) == vectorPosition);
					EXPECT(Tracer.ConvertFromSnapshot(0, i, 1) == lastVectorPosition);
				}

				EXPECT(Tracer.ConvertFromSnapshot(0, current, 0) == vectorPosition);
				EXPECT(Tracer.ConvertFromSnapshot(0, current, -1) == vectorPosition);
				EXPECT(Tracer.ConvertFromSnapshot(0, current, 1) == vectorPosition);

				lastVectorPosition = vectorPosition;
				last = current;
			}
			vectorPosition += 1;
		}
	}

	void Split(int pos, ModificationTester& other)
	{
		Tracer.SplitInto(other.Tracer, 1, pos);
		other.Items.clear();
		int offset = -1;
		for (int i = pos; i < Items.size(); ++i)
		{
			if (offset == -1 && Items[i] != -1)
			{
				offset = Items[i];
			}
			other.Items.push_back(Items[i] - (offset == -1 || Items[i] == -1 ? 0 : offset));
		}
		Items.erase(Items.begin() + pos, Items.end());
		if (offset == -1)
		{
			other.InitialSize = 0;
		}
		else
		{
			other.InitialSize = InitialSize - offset;
			InitialSize = offset;
		}
	}

private:
	int InitialSize;
	ModificationTracer Tracer;
	std::vector<int> Items;
	lest::env& lest_env;
};

const lest::test specification[] =
{
	CASE("Insert simple")
	{
		ModificationTester t(lest_env, 4);
		t.Insert(2, 2);
		t.CheckConversion();
	},
	CASE("Delete simple")
	{
		ModificationTester t(lest_env, 6);
		t.Delete(2, 2);
		t.CheckConversion();
	},
	CASE("Delete at beginning")
	{
		ModificationTester t(lest_env, 6);
		t.Delete(0, 2);
		t.CheckConversion();
	},
	CASE("Insert with delta")
	{
		ModificationTester t(lest_env, 4);
		t.Insert(0, 1);
		t.Insert(2, 2);
		t.CheckConversion();
	},
	CASE("Delete with delta")
	{
		ModificationTester t(lest_env, 6);
		t.Insert(0, 1);
		t.Delete(2, 2);
		t.CheckConversion();
	},
	CASE("Insertion after deletion")
	{
		ModificationTester t(lest_env, 4);
		t.Insert(0, 1);
		t.Delete(2, 1);
		t.Insert(3, 1);
		t.Insert(2, 1);
		t.Insert(1, 1);
		t.CheckConversion();
	},
	CASE("Delete after insertion (normal)")
	{
		ModificationTester t(lest_env, 12);
		t.Insert(9, 3);
		t.Insert(6, 3);
		t.Insert(3, 3);
		t.Delete(17, 2); //right
		t.Delete(13, 1); //outside
		t.Delete(8, 2); //left
		t.Delete(4, 1); //inside
		t.CheckConversion();
	},
	CASE("Delete after insertion (normal 2)")
	{
		ModificationTester t(lest_env, 18);
		t.Insert(15, 3);
		t.Insert(12, 3);
		t.Insert(9, 3);
		t.Insert(6, 3);
		t.Insert(3, 3);
		t.Delete(24, 1);
		t.Delete(18, 1);
		t.Delete(13, 3);
		t.Delete(7, 3);
		t.Delete(5, 1);
		t.Delete(1, 2);
		t.CheckConversion();
	},
	CASE("Delete after insertion (beginning)")
	{
		ModificationTester t(lest_env, 3);
		t.Insert(0, 1);
		t.Delete(1, 1);
		t.CheckConversion();
	},
	CASE("Delete after insertion (adjacent)")
	{
		ModificationTester t(lest_env, 18);
		t.Insert(15, 2);
		t.Insert(12, 2);
		t.Insert(9, 2);
		t.Insert(6, 2);
		t.Insert(3, 2);
		t.Delete(24, 1);
		t.Delete(18, 1);
		t.Delete(13, 3);
		t.Delete(7, 3);
		t.Delete(5, 1);
		t.Delete(1, 2);
		t.CheckConversion();
	},
	CASE("Delete after insertion (same)")
	{
		ModificationTester t(lest_env, 2);
		t.Insert(1, 1);
		t.Insert(2, 1);
		t.Delete(2, 1);
		t.Delete(1, 1);
		t.CheckConversion();
	},
	CASE("Delete at pair")
	{
		ModificationTester t(lest_env, 11);
		t.Delete(8, 1);
		t.Insert(8, 2);
		t.Delete(5, 1);
		t.Insert(5, 2);
		t.Delete(2, 1);
		t.Insert(2, 2);

		t.Delete(12, 1);
		t.Delete(7, 1);
		t.Delete(1, 3);
		t.CheckConversion();
	},
	CASE("Delete merging pairs")
	{
		ModificationTester t(lest_env, 8);
		t.Insert(6, 2);
		t.Delete(2, 2);
		t.Insert(2, 2);

		t.Delete(4, 3);
		t.CheckConversion();
	},
	CASE("Split empty")
	{
		ModificationTester t1(lest_env, 4);
		ModificationTester t2(lest_env, 0);
		t1.Split(2, t2);
		t1.CheckConversion();
		t2.CheckConversion();
	},
	CASE("Split insertion")
	{
		ModificationTester t1(lest_env, 4);
		ModificationTester t2(lest_env, 0);
		t1.Insert(1, 2);
		t1.Insert(5, 2);
		t1.Split(4, t2);
		t1.CheckConversion();
		t2.CheckConversion();
	},
	CASE("Split deletion")
	{
		ModificationTester t1(lest_env, 8);
		ModificationTester t2(lest_env, 0);
		t1.Delete(5, 2);
		t1.Delete(1, 2);
		t1.Split(2, t2);
		t1.CheckConversion();
		t2.CheckConversion();
	},
	CASE("Split inside insertion")
	{
		ModificationTester t1(lest_env, 4);
		ModificationTester t2(lest_env, 0);
		t1.Insert(2, 2);
		t1.Split(3, t2);
		t1.CheckConversion();
		t2.CheckConversion();
	},
	CASE("Split at deletion")
	{
		ModificationTester t1(lest_env, 6);
		ModificationTester t2(lest_env, 0);
		t1.Delete(2, 2);
		t1.Split(2, t2);
		t1.CheckConversion();
		t2.CheckConversion();
	},
	CASE("Split at pair")
	{
		ModificationTester t1(lest_env, 6);
		ModificationTester t2(lest_env, 0);
		t1.Delete(2, 2);
		t1.Insert(2, 2);
		t1.Split(4, t2);
		t1.CheckConversion();
		t2.CheckConversion();
	},
};

int TestModificationTracer()
{
	return lest::run(specification, { "-p" }, std::cout);
}
