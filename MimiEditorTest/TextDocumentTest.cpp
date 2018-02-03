#include "../MimiEditor/TextDocument.h"
#include "../MimiEditor/FileTypeDetector.h"
#include <string>
#include <iostream>

using namespace Mimi;
using namespace std;

int TestTextDocument()
{
	string path;
	getline(cin, path);
	String pathStr = String::FromUtf8Ptr(path.c_str());
	unique_ptr<IFile> file = unique_ptr<IFile>(IFile::CreateFromPath(std::move(pathStr)));
	unique_ptr<IFileReader> r = unique_ptr<IFileReader>(file->Read());

	FileTypeDetectionOptions option;
	option.InvalidCharAction = InvalidCharAction::ReplaceQuestionMark;
	option.MaxRead = 1000;
	option.MinRead = 100;

	FileTypeDetector detector(std::move(r), option);

	TextDocument* doc = TextDocument::CreateFromTextFile(&detector);

	return 0;
}
