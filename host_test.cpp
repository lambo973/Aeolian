// Test su host (non Daisy): render offline di eliane.h in un file float32 stereo interleaved.
// Uso: ./host_test out.raw secondi hz [hz_dopo_meta]
#include "eliane.h"
#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char** argv)
{
    if(argc < 4) { std::fprintf(stderr, "uso: %s out.raw secondi hz [hz2]\n", argv[0]); return 1; }
    const float sr = 48000.0f;
    const int   secs = std::atoi(argv[2]);
    const float hz = std::atof(argv[3]);
    const float hz2 = argc > 4 ? std::atof(argv[4]) : hz;
    static float cl[eliane::kCombMax], cr[eliane::kCombMax];
    static eliane::Eliane e;
    if(!e.Init(sr, cl, cr, 12345u)) { std::fprintf(stderr, "init fallita\n"); return 1; }
    e.SetHz(hz); e.SetAmp(0.5f);
    FILE* f = std::fopen(argv[1], "wb");
    const int blocks = int(secs * sr) / eliane::kBlock;
    float l[eliane::kBlock], r[eliane::kBlock], il[eliane::kBlock * 2];
    for(int b = 0; b < blocks; b++)
    {
        if(b == blocks / 2) e.SetHz(hz2);
        e.Process(l, r);
        for(int n = 0; n < eliane::kBlock; n++) { il[2 * n] = l[n]; il[2 * n + 1] = r[n]; }
        std::fwrite(il, sizeof(float), eliane::kBlock * 2, f);
    }
    std::fclose(f);
    return 0;
}
