call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cl.exe -std:c++latest gear.cpp
del gear.obj

cl.exe -std:c++latest obj.cpp
del obj.obj

cl.exe -std:c++latest png.cpp
del png.obj
