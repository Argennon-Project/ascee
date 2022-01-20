// Copyright (c) 2022 aybehrouz <behrouz_ayati@yahoo.com>. All rights
// reserved. This file is part of the C++ implementation of the Argennon smart
// contract Execution Environment (AscEE).
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option) any
// later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License
// for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
/**
 * @brief Base64url encoding scheme
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2021 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneCRYPTO Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.1.2
 **/

#define NO_ERROR 0
#define ERROR_INVALID_PARAMETER 1
#define ERROR_INVALID_LENGTH 2
#define ERROR_INVALID_CHARACTER 3

#include <cstdint>
#include <stdexcept>
#include "encoding.h"

typedef int error_t;

using namespace argennon;

//Base64url encoding table
static const char base64urlEncTable[64] =
        {
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_'
        };

//Base64url decoding table
static const uint8_t base64urlDecTable[128] =
        {
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF,
                0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
                0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F,
                0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
                0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        };


size_t util::base64DecodeLen(size_t base64Len) {
    auto r = base64Len % 4;
    return r == 1 ? 0 : 3 * (base64Len / 4) + 3 * r / 4;
}

/**
 * @brief Base64url encoding algorithm
 * @param[in] input Input data to encode
 * @param[in] inputLen Length of the data to encode
 * @return std::string encoded with Base64url algorithm
 */
std::string util::base64urlEncode(const void* input, size_t inputLen) {
    // The buf should have one extra space for the terminating-NULL
    char buf[4 * (inputLen / 3) + 4];
    size_t len;
    base64urlEncode(input, inputLen, buf, &len);
    printf("%s\n", buf);
    return {buf, len};
}

/**
 * @brief Base64url encoding algorithm
 * @param[in] input Input data to encode
 * @param[in] inputLen Length of the data to encode
 * @param[out] output NULL-terminated string encoded with Base64url algorithm
 * @param[out] outputLen Length of the encoded string (optional parameter)
 **/
void util::base64urlEncode(const void* input, size_t inputLen, char* output,
                           size_t* outputLen) {
    size_t n;
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    const uint8_t* p;

    //Point to the first byte of the input data
    p = (const uint8_t*) input;

    //Divide the input stream into blocks of 3 bytes
    n = inputLen / 3;

    //A full encoding quantum is always completed at the end of a quantity
    if (inputLen == (n * 3 + 1)) {
        //The final quantum of encoding input is exactly 8 bits
        if (input != nullptr && output != nullptr) {
            //Read input data
            a = (p[n * 3] & 0xFC) >> 2;
            b = (p[n * 3] & 0x03) << 4;

            //The final unit of encoded output will be two characters
            output[n * 4] = base64urlEncTable[a];
            output[n * 4 + 1] = base64urlEncTable[b];
            output[n * 4 + 2] = '\0';
        }

        //Length of the encoded string (excluding the terminating nullptr)
        if (outputLen != nullptr) {
            *outputLen = n * 4 + 2;
        }
    } else if (inputLen == (n * 3 + 2)) {
        //The final quantum of encoding input is exactly 16 bits
        if (input != nullptr && output != nullptr) {
            //Read input data
            a = (p[n * 3] & 0xFC) >> 2;
            b = ((p[n * 3] & 0x03) << 4) | ((p[n * 3 + 1] & 0xF0) >> 4);
            c = (p[n * 3 + 1] & 0x0F) << 2;

            //The final unit of encoded output will be three characters followed
            //by one "=" padding character
            output[n * 4] = base64urlEncTable[a];
            output[n * 4 + 1] = base64urlEncTable[b];
            output[n * 4 + 2] = base64urlEncTable[c];
            output[n * 4 + 3] = '\0';
        }

        //Length of the encoded string (excluding the terminating nullptr)
        if (outputLen != nullptr) {
            *outputLen = n * 4 + 3;
        }
    } else {
        //The final quantum of encoding input is an integral multiple of 24 bits
        if (output != nullptr) {
            //The final unit of encoded output will be an integral multiple of 4
            //characters
            output[n * 4] = '\0';
        }

        //Length of the encoded string (excluding the terminating nullptr)
        if (outputLen != nullptr) {
            *outputLen = n * 4;
        }
    }

    //If the output parameter is nullptr, then the function calculates the
    //length of the resulting Base64url string without copying any data
    if (input != nullptr && output != nullptr) {
        //The input data is processed block by block
        while (n-- > 0) {
            //Read input data
            a = (p[n * 3] & 0xFC) >> 2;
            b = ((p[n * 3] & 0x03) << 4) | ((p[n * 3 + 1] & 0xF0) >> 4);
            c = ((p[n * 3 + 1] & 0x0F) << 2) | ((p[n * 3 + 2] & 0xC0) >> 6);
            d = p[n * 3 + 2] & 0x3F;

            //Map each 3-byte block to 4 printable characters using the Base64url
            //character set
            output[n * 4] = base64urlEncTable[a];
            output[n * 4 + 1] = base64urlEncTable[b];
            output[n * 4 + 2] = base64urlEncTable[c];
            output[n * 4 + 3] = base64urlEncTable[d];
        }
    }
}


/**
 * @brief Base64url decoding algorithm
 * @param[in] input Base64url-encoded string
 * @param[in] inputLen Length of the encoded string
 * @param[out] output Resulting decoded data
 * @param[out] outputLen Length of the decoded data
 * @return Error code
 **/
static
error_t base64urlDecodeImpl(const char* input, size_t inputLen, void* output,
                            size_t* outputLen) {
    error_t error;
    uint32_t value;
    uint8_t c;
    size_t i;
    size_t n;
    uint8_t* p;

    //Check parameters
    if (input == nullptr && inputLen != 0)
        return ERROR_INVALID_PARAMETER;
    if (outputLen == nullptr)
        return ERROR_INVALID_PARAMETER;

    //Check the length of the input string
    if ((inputLen % 4) == 1)
        return ERROR_INVALID_LENGTH;

    //Initialize status code
    error = NO_ERROR;

    //Point to the buffer where to write the decoded data
    p = (uint8_t*) output;

    //Initialize variables
    n = 0;
    value = 0;

    //Process the Base64url-encoded string
    for (i = 0; i < inputLen && !error; i++) {
        //Get current character
        c = (uint8_t) input[i];

        //Check the value of the current character
        if (c < 128 && base64urlDecTable[c] < 64) {
            //Decode the current character
            value = (value << 6) | base64urlDecTable[c];

            //Divide the input stream into blocks of 4 characters
            if ((i % 4) == 3) {
                //Map each 4-character block to 3 bytes
                if (p != nullptr) {
                    p[n] = (value >> 16) & 0xFF;
                    p[n + 1] = (value >> 8) & 0xFF;
                    p[n + 2] = value & 0xFF;
                }

                //Adjust the length of the decoded data
                n += 3;
                //Decode next block
                value = 0;
            }
        } else {
            //Implementations must reject the encoded data if it contains
            //characters outside the base alphabet
            error = ERROR_INVALID_CHARACTER;
        }
    }

    //Check status code
    if (!error) {
        //All trailing pad characters are omitted in Base64url
        if ((inputLen % 4) == 2) {
            //The last block contains only 1 byte
            if (p != nullptr) {
                //Decode the last byte
                p[n] = (value >> 4) & 0xFF;
            }

            //Adjust the length of the decoded data
            n++;
        } else if ((inputLen % 4) == 3) {
            //The last block contains only 2 bytes
            if (p != nullptr) {
                //Decode the last two bytes
                p[n] = (value >> 10) & 0xFF;
                p[n + 1] = (value >> 2) & 0xFF;
            }

            //Adjust the length of the decoded data
            n += 2;
        } else {
            //No pad characters in this case
        }
    }

    //Total number of bytes that have been written
    *outputLen = n;

    //Return status code
    return error;
}

/**
 * @brief Base64url decoding algorithm
 * @param[in] input Base64url-encoded string
 * @param[in] inputLen Length of the encoded string
 * @param[out] output Resulting decoded data
 * @return Length of the decoded data
 */
size_t util::base64urlDecode(const char* input, size_t inputLen, void* output) {
    size_t outputLen = 0;
    auto err = base64urlDecodeImpl(input, inputLen, output, &outputLen);
    if (err == NO_ERROR) return outputLen;

    if (err == ERROR_INVALID_CHARACTER) throw std::invalid_argument("base64: input contains illegal characters");

    if (err == ERROR_INVALID_LENGTH) throw std::invalid_argument("base64: input length is not valid");

    // err == ERROR_INVALID_PARAMETER
    throw std::invalid_argument("base64: invalid input parameters");
}
