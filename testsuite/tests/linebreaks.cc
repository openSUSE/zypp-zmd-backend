//
// joinlines.cc
//
// test split/join
//

#include <iostream>
#include <string>
#include <vector>
#include "dbsource/utils.h"

int
main(int argc, char *argv[])
{
    if (joinlines("") != "") return 1;
    if (joinlines("\n") != "") return 2;
    if (joinlines("foo\n") != "foo") return 3;
    if (joinlines("\nfoo") != "foo") return 4;
    if (joinlines("\nfoo\n") != "foo") return 5;
    if (joinlines("foo\nbar") != "foobar") return 6;
    if (joinlines("foo\n\nbar") != "foobar") return 7;
    if (joinlines("foo\n\n\nbar") != "foobar") return 8;

    return 0;
}
