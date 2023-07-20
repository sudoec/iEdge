#include "../common/constants.h"
#include "../common/version.h"
#include <brotli/decode.h>
#include <brotli/encode.h>



bool BrotliDecompress(size_t SizeIn, size_t* SizeOut, const uint8_t* BufferIn, uint8_t* BufferOut) {

    BrotliDecoderState* s = BrotliDecoderCreateInstance(NULL, NULL, NULL);
    BrotliDecoderSetParameter(s, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1u);

    uint8_t* Nextout = BufferOut;

    if (1 != BrotliDecoderDecompressStream(s, &SizeIn, &BufferIn, SizeOut, &Nextout, NULL))
    {
        *SizeOut = 0;
        BrotliDecoderDestroyInstance(s);
        return false;

    }

    *SizeOut = (size_t)(Nextout - BufferOut);
    BrotliDecoderDestroyInstance(s);
    return true;
}


bool BrotliCompress(size_t SizeIn, size_t* SizeOut, const uint8_t* BufferIn, uint8_t* BufferOut){

    BrotliEncoderState* s = BrotliEncoderCreateInstance(NULL, NULL, NULL);
    BrotliEncoderSetParameter(s, BROTLI_PARAM_QUALITY, 11);
    BrotliEncoderSetParameter(s, BROTLI_PARAM_LGWIN, 24);
    BrotliEncoderSetParameter(s, BROTLI_PARAM_SIZE_HINT, SizeIn);

    uint8_t* Nextout = BufferOut;

    if (!BrotliEncoderCompressStream(s, BROTLI_OPERATION_FINISH, &SizeIn, &BufferIn, SizeOut, &Nextout, NULL))
    {
        *SizeOut = 0;
        BrotliEncoderDestroyInstance(s);
        return false;
    }

    *SizeOut = (size_t)(Nextout - BufferOut);
    BrotliEncoderDestroyInstance(s);

    return true;
}
