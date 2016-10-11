#include "File.h"
#include "StrClass.h"

#include <assert.h>

using namespace NOMADSUtil;

int main (int argc, const char **ppszArgv)
{
    File file ("/some/path/file.ext");

    String parent (file.getParent());
    assert (parent == "/some/path");

    String name (file.getName());
    assert (name == "file.ext");

    String nameNoExt (file.getName (true));
    assert (nameNoExt == "file");

    String extension (file.getExtension());
    assert (extension == "ext");

    return 0;
}


