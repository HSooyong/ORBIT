// Prints per-preset output-vs-dry RMS / BS.1770 LUFS table. --check: exit 1 if any case > 0.5 LU.
#include "level_match.h"
int main (int argc, char** argv)
{
    double worst, pr; const int f = levelmatch::run (true, worst, pr);
    return (argc > 1 && std::string (argv[1]) == "--check" && f) ? 1 : 0;
}
