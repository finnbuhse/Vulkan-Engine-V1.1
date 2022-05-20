#include "float16.h"
#include <cstring>
/* From https://stackoverflow.com/questions/1659440/32-bit-to-16-bit-floating-point-conversion
in an answer from ProjectPhysX */

float16 floatToFloat16(const float& f)
{
    int fInt;
    memcpy(&fInt, &f, sizeof(float));
    float16 f16 = ((fInt & 0x7fffffff) >> 13) - (0x38000000 >> 13);
    f16 |= ((fInt & 0x80000000) >> 16);
    return f16;
}

float float16ToFloat(const float16& f16)
{
    int fInt = ((f16 & 0x8000) << 16);
    fInt |= ((f16 & 0x7fff) << 13) + 0x38000000;

    float f;
    memcpy(&f, &fInt, sizeof(float));
    return f;
}