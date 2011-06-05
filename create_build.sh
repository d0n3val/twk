# Compiles and creates a build .zip file

mkdir twk
cp *.dylib twk
cp twk.app twk
mkdir twk/data
cp data/* twk/data
rm -f twk/data/*.bin
#zip -rm twk_%date:~-4%_%date:~3,2%_%date:~0,2%.zip twk
zip -rm twk`date +%Y_%m_%d`-osx.zip twk
