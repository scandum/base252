Base252
-------

The Base252 encoding draws inspiration from UTF-8, its strengths are minimal overhead and flexible optional escaping.

| Dec |  Hex | Name    | Escape Code |
|----- | ---- | ------  | ------ |
|  0   | 0x00 | NUL     | 245 128 |
| 245  | 0xF5 | ESC_0   | 248 245 |
| 246  | 0xF6 | ESC_64  | 248 246 |
| 247  | 0xF7 | ESC_128 | 248 247 |
| 248  | 0xF8 | ESC_192 | 248 248 |

Translating binary data to Base252
----------------------------------

The first step is to convert the data into a C string. A C string exists of a sequence
of bytes with values between 1 and 255. The 0 value is reserved as the string terminator
and is also known as the NUL byte.

In order to turn binary data into Base252 each byte with the 0 value needs to be
converted into a two byte sequence with the values 245 128.

This conversion process is known as escaping, which I'll describe in detail below.

Escaping
--------

Earlier we saw the escaping of 0 to 245 128. However, in many cases
programming languages have special characters that pose processing or security
issues when they are not escaped. The \\ and " characters come to mind, but in
theory any character can be a special character that needs to be escaped.

In order to escape any ASCII character Base252 reserves character value 245, 246,
247 and 248. The math to escape is simple and similar to UTF-8.
```c
245 + char / 64
128 + char % 64
```
Besides the 0 byte each occurance of 245, 246, 247 and 248 in the data needs to
be escaped using the formula above.

Characters 128 through 191 cannot be as easily fully escaped since they are
typically used for the second byte of each escaped code. However,
since the second byte needs to be modulated by 64, a value between 64 and 127,
or between 192 and 255, could be used to encode the second byte. All character
values are valid for the second byte, with the exception of 0.

Subsequently there are three valid ways to fully escape the 0 value: 245 64,
245 128, and 245 192.

The example below escapes the 5 required codes as well as the '\\' character.
```c
        for (cnt = 0 ; cnt < size ; cnt++)
        {
                switch (input[cnt])
                {
                        case  0:
                                *output++ = 245;
                                *output++ = 128 + input[cnt] % 64;
                                break;

                        case '\\':
                                *output++ = 246;
                                *output++ = 128 + input[cnt] % 64;
                                break;

                        case 245:
                        case 246:
                        case 247:
                        case 248:
                                *output++ = 248;
                                *output++ = 128 + input[cnt] % 64;
                                break;

                        default:
                                *output++ = input[cnt];
                                break;
                }
        }
        *output++ = 0;
```

Translating Base252 data back to its original format is even easier and the
code below should translate any proper Base252 string.
```c
        while (input[cnt] != 0)
        {
                switch (input[cnt])
                {
                        case 245:
                                cnt++;
                                *output++ = 0 + input[cnt++] % 64;
                                break;

                        case 246:
                                cnt++;
                                *output++ = 64 + input[cnt++] % 64;
                                break;

                        case 247:
                                cnt++;
                                *output++ = 128 + input[cnt++] % 64;
                                break;

                        case 248:
                                cnt++;
                                *output++ = 192 + input[cnt++] % 64;
                                break;

                        default:
                                *output++ = input[cnt++];
                                break;
                }
        }
        *output++ = 0;
```

Termination
-----------
A Base252 string needs to be terminated with a NUL byte.

Overhead
--------
After Base252 conversion we have a best case overhead of 0% (notably when turning
ASCII or UTF-8 into Base252) and a worst case overhead of 100%. The average
case should be an overhead of 1.7% and this should also be the typical case
for compressed / encrypted data.

JSON
----
Base252 should work with properly written JSON parsers to encode binary data, keep in mind that in this case characters in the 1 to 31 range need to be escaped, as well as the '"' and '\\' characters.
