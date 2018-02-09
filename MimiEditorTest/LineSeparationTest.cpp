#include "TestCommon.h"
#include "../MimiEditor/FileTypeDetector.h"

using namespace Mimi;
using namespace std;

namespace
{
	class LineSeparationTester
	{
	public:
		LineSeparationTester(lest::env& lest_env)
			: lest_env(lest_env)
		{
		}

	private:
		lest::env& lest_env;

	private:
		static String GetFileName(const char* path)
		{
			Mimi::StringBuilder sb(CodePageManager::UTF8);
			sb.AppendUtf8(ExecutableDirectory);
			sb.AppendUtf8(path);
			return sb.ToString();
		}

	public:
		void TestFile(const char* fn)
		{
			auto file = IFile::CreateFromPath(GetFileName(fn));
			auto r = unique_ptr<IFileReader>(file->Read());

			FileTypeDetectionOptions o;
			o.InvalidCharAction = InvalidCharAction::Cancel;
			o.MinRead = 100;
			o.MaxRead = 1000;
			FileTypeDetector d(std::move(r), o);
			EXPECT(d.GetCodePage() == CodePageManager::UTF8);

			bool lastIsEmpty = false;
			bool lastHasNewline = true;
			while (d.ReadNextLine())
			{
				auto len = d.CurrentLineData.GetLength();
				auto ptr = d.CurrentLineData.GetRawData();

				EXPECT(!lastIsEmpty); //Only last line can be empty (without newline).
				if (len == 0)
				{
					lastIsEmpty = true;
				}

				lastHasNewline = (ptr[len - 1] == '\n');

				String str(ptr, len, d.GetCodePage());
				String strConv = str.ToUtf16String().ToCodePage(d.GetCodePage());
				EXPECT(memcmp(str.Data, strConv.Data, str.Length) == 0); //String is valid.
			}
			EXPECT(!d.IsError()); //Read to EOF.
			EXPECT(!lastHasNewline); //Last line should not have newline.
		}
	};
}

DEFINE_MODULE(TestLineSeparation)
{
	CASE("Single-byte char")
	{
		LineSeparationTester t(lest_env);
		t.TestFile("Line/SingleByte.txt");
	},
	CASE("Multi-byte char")
	{
		LineSeparationTester t(lest_env);
		t.TestFile("Line/MultiByte.txt");
	},
};
