gcc -std=gnu99 -Wall -Wextra -Wno-unused-function -Wno-sign-compare -Wno-unused-parameter -Wno-missing-field-initializers -Werror -O2 -I. -o twk.exe -lGL -lGLU -lglut code/*.c -Imxml -Lmxml -lmxml
