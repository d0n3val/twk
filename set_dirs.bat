set include=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Include;C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\include

set lib=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib;C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\lib

cl /nologo /WL /Od /GF /Gm- /GL- /Y- /TP /I. code/*.c /MTd /Fetwk
