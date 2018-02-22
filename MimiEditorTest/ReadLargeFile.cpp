#include "TestCommon.h"
#include "../MimiEditor/FileTypeDetector.h"
#include "../MimiEditor/TextDocument.h"
#include <memory>

using namespace Mimi;
using ReaderHandle = std::unique_ptr<IFileReader>;
using DocumentHandle = std::unique_ptr<TextDocument>;

void TestReadLargeFile(const char* path)
{
	IFile* file = IFile::CreateFromPath(String::FromUtf8Ptr(path));
	Clock clock;
	for (int i = 0; i < 40; ++i)
	{
		ReaderHandle r = ReaderHandle(file->Read());
		FileTypeDetector detector(std::move(r), {});
		DocumentHandle doc = DocumentHandle(TextDocument::CreateFromTextFile(&detector));
	}
	std::cout << "Time:" << clock.GetElapsedMilliSecond<double>() << " ms" << std::endl;
}
