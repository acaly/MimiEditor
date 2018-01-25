#include "../MimiEditor/IFile.h"
#include <iostream>
#include <string>
#include <memory>

using namespace Mimi;
using namespace std;

void TestReadFile()
{
	string path;
	getline(cin, path);
	String pathStr = String::FromUtf8Ptr(path.c_str());
	unique_ptr<IFile> file = unique_ptr<IFile>(IFile::CreateFromPath(std::move(pathStr)));
	unique_ptr<IFileReader> r = unique_ptr<IFileReader>(file->Read());
	unique_ptr<uint8_t[]> buffer = unique_ptr<uint8_t[]>(new uint8_t[r->GetSize()]);
	r->Read(buffer.get(), r->GetSize(), nullptr);
	String str = String(buffer.get(), r->GetSize(), CodePageManager::UTF8);
}

int TestFile()
{
	TestReadFile();
	return 0;
}
