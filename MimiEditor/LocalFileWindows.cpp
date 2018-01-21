#include "IFile.h"

//C++ file stream is not designed for binary nor for string (with encoding). Use Windows API instead.

Mimi::IFile* Mimi::IFile::CreateFromPath(String path)
{
	return nullptr;
}
