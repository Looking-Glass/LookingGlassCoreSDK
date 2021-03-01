/**
 * main.cpp
 * Contributors:
 *      * Arthur Sonzogni (author), Looking Glass Factory Inc.
 * Licence:
 *      * MIT
 */

#ifdef WIN32
#pragma warning(disable : 4464 4820 4514 5045 4201 5039 4061 4710 4458 4626 5027 4365 4312)
#endif

#include "SampleScene.hpp"

using namespace std;

#define UNUSED(x) [&x]{}()

int main(int argc, const char *argv[])
{
  UNUSED(argc);
  UNUSED(argv);

  SampleScene sampleScene;

  sampleScene.run();

  return 0;
}