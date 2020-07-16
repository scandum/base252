Base252
-------

The Base252 encoding draws inspiration from UTF-8, its strengths are minimal overhead and flexible optional escaping.

| Dec |  Hex | Name    | Escape Code |
|----- | ---- | ------  | ------ |
|  0   | 0x00 | NUL     | 0xF5 0x80 |
| 245  | 0xF5 | ESC_0   | 0xF8 0xB5 |
| 246  | 0xF6 | ESC_64  | 0xF8 0xB6 |
| 247  | 0xF7 | ESC_128 | 0xF8 0xB7 |
| 248  | 0xF8 | ESC_192 | 0xF8 0xB8 |

Translating binary data to Base252
----------------------------------

The first step is to convert the data into C strings. This is done by translating
each NUL byte 0x00 to the two byte 0xF5 0x80 sequence.

Subsequently each instance of 0xF5 encountered in the data needs to be translated
as well, using 0xF8 0xB8.

Escaping
--------

Earlier we saw the escaping of 0x00 to 0xC0 0x80. However, in many cases
programming languages have special characters that pose processing or security
issues when they are not escaped. The \\ and " characters come to mind, but in
theory any character can be a special character that needs to be escaped.

In order to escape any ASCII character Base252 reserves 0xF5, 0xF6, 0xF7 and
0xF8. The math to escape is simple and similar to UTF-8.
```c
0xF5 + char / 64
0x80 + char % 64
```
Subsequently we can fully escape any character from 0 through 255 with the
exception of 0xF5 through 0xF8.

Characters 128 through 191 cannot be as easily escaped since 128 through 191 is
typically used for the second byte of each escaped code. However, since the
second byte needs to be modulated by 64, a value between 64 and 127, or between
192 and 255 could be used to encode the second byte.

Special care is needed in that case in order to encode, though the decoding
process remains identical.

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

Translating Base252 data back to its original format is even easier.
```c
        for (cnt = 0 ; cnt < size ; cnt++)
        {
                switch (input[cnt])
                {
                        case 245:
                                cnt++;
                                *output++ = 0 + input[cnt] % 64;
                                break;

                        case 246:
                                cnt++;
                                *output++ = 64 + input[cnt] % 64;
                                break;

                        case 247:
                                cnt++;
                                *output++ = 128 + input[cnt] % 64;
                                break;

                        case 248:
                                cnt++;
                                *output++ = 192 + input[cnt] % 64;
                                break;

                        default:
                                *output++ = input[cnt];
                                break;
                }
        }
        *output++ = 0;
```

Overhead
--------
After this conversion we have a best case overhead of 0% (notably when turning
ASCII or UTF-8 into Base252) and a worst case overhead of 100%. The average
case should be an overhead of 1.7% and this should also be the typical case
for compressed or encrypted data.

JSON
----
Base252 should work with properly written JSON parsers to encode binary data, keep in mind that in this case characters in the 1 to 31 range need to be escaped, as well as the '"' and '\' characters.
