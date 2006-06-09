//
// linebreaks.cc
//
// test striplinebreaks
//

#include <iostream>
#include <string>
#include "src/zmd-backend.h"


using std::endl;
using namespace backend;

int
main(int argc, char *argv[])
{
    if (striplinebreaks("") != "") return 1;
    if (striplinebreaks("\n") != "") return 2;
    if (striplinebreaks("foo\n") != "foo") return 3;
    if (striplinebreaks("\nfoo") != "foo") return 4;
    if (striplinebreaks("\nfoo\n") != "foo") return 5;
    if (striplinebreaks("foo\nbar") != "foobar") return 6;
    if (striplinebreaks("foo\n\nbar") != "foobar") return 7;
    if (striplinebreaks("foo\n\n\nbar") != "foobar") return 8;

    return 0;
}
