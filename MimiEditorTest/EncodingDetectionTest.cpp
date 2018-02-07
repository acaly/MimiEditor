#include "TestCommon.h"
#include "../MimiEditor/FileTypeDetector.h"
#include <string>
#include <sstream>

using namespace Mimi;
using namespace std;

extern Mimi::CodePage GetWindowsCodePage936();

namespace
{
	class EncodingDetectionTester
	{
	public:
		EncodingDetectionTester(lest::env& lest_env, CodePage cp)
			: lest_env(lest_env), Encoding(cp)
		{
		}

	private:
		lest::env& lest_env;
		CodePage Encoding;

		static String GetFileName(const char* pathPrefix, int i)
		{
			//TODO use internal format function
			stringstream ss;
			ss << pathPrefix << "-" << (i + 1) << ".txt";
			return String::FromUtf8Ptr(ss.str().c_str());
		}

		void TestFile(const char* pathPrefix, int i)
		{
			auto fn = GetFileName(pathPrefix, i);
			auto file = IFile::CreateFromPath(fn);
			auto r = unique_ptr<IFileReader>(file->Read());

			FileTypeDetectionOptions o;
			o.PrimaryCodePages = CodePageManager::ListCodePages();
			o.InvalidCharAction = InvalidCharAction::Cancel;
			o.MinRead = 100;
			o.MaxRead = 1000;
			FileTypeDetector d(std::move(r), o);

			EXPECT(d.GetCodePage() == Encoding);

			while (d.ReadNextLine())
			{
				String str(
					d.CurrentLineData.GetRawData(),
					d.CurrentLineData.GetLength(),
					d.GetCodePage());
				String strConv = str.ToUtf16String().ToCodePage(d.GetCodePage());
				EXPECT(memcmp(str.Data, strConv.Data, str.Length) == 0);
			}
			EXPECT(!d.IsError());
		}

	public:
		void TestFiles(const char* pathPrefix, int n)
		{
			for (int i = 0; i < n; ++i)
			{
				TestFile(pathPrefix, i);
			}
		}
	};
}

DEFINE_MODULE(TestEncodingDetection)
{
	CASE("ASCII")
	{
		EncodingDetectionTester t(lest_env, CodePageManager::ASCII);
		t.TestFiles("Encoding/ASCII", 1);
	},
	CASE("UTF8")
	{
		EncodingDetectionTester t(lest_env, CodePageManager::UTF8);
		t.TestFiles("Encoding/UTF8", 3);
	},
	CASE("UTF8BOM")
	{
		EncodingDetectionTester t(lest_env, CodePageManager::UTF8);
		t.TestFiles("Encoding/UTF8BOM", 4);
	},
	CASE("UTF16LE")
	{
		EncodingDetectionTester t(lest_env, CodePageManager::UTF16LE);
		t.TestFiles("Encoding/UTF16LE", 3);
	},
	CASE("UTF16LEBOM")
	{
		EncodingDetectionTester t(lest_env, CodePageManager::UTF16LE);
		t.TestFiles("Encoding/UTF16LEBOM", 4);
	},
	CASE("UTF16BE")
	{
		EncodingDetectionTester t(lest_env, CodePageManager::UTF16BE);
		t.TestFiles("Encoding/UTF16BE", 3);
	},
	CASE("UTF16BEBOM")
	{
		EncodingDetectionTester t(lest_env, CodePageManager::UTF16BE);
		t.TestFiles("Encoding/UTF16BEBOM", 4);
	},
	CASE("GBK")
	{
		EncodingDetectionTester t(lest_env, GetWindowsCodePage936());
		t.TestFiles("Encoding/GBK", 1);
	},
};
