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

private:
	int InitialSize;
	ModificationTracer Tracer;
	std::vector<int> Items;
	lest::env& lest_env;
};

const lest::test specification[] =
{
	CASE("Single insertion")
	{
		ModificationTester t(lest_env, 4);
		t.Insert(2, 2);
		t.CheckConversion();
	},
	CASE("Single deletion")
	{
		ModificationTester t(lest_env, 6);
		t.Delete(2, 2);
		t.CheckConversion();
	},
	CASE("Deletion at beginning")
	{
		ModificationTester t(lest_env, 6);
		t.Delete(0, 2);
		t.CheckConversion();
	},
	CASE("Insertion with delta")
	{
		ModificationTester t(lest_env, 4);
		t.Insert(0, 1);
		t.Insert(2, 2);
		t.CheckConversion();
	},
	CASE("Deletion with delta")
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
	CASE("Delete after insertion at beginning")
	{
		ModificationTester t(lest_env, 3);
		t.Insert(0, 1);
		t.Delete(1, 1);
		t.CheckConversion();
	},
	//TODO ins & del sharing an edge
};

int main(int argc, char * argv[])
{
	return lest::run(specification, { "-p" }, std::cout);
}
