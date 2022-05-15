#include <iostream>
#include "skiplist.h"
#define FILE_PATH "./store/file"
#define ALLOW_MAX_SIZE 30

int main()
{
    SkipList<int, std::string> skipList(ALLOW_MAX_SIZE);
    std::ifstream f(FILE_PATH);
    skipList.load_file(f);
    skipList.display_skiplist();
    return 0;
}