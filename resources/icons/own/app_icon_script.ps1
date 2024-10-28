
# https://learn.microsoft.com/en-us/windows/apps/design/style/iconography/app-icon-construction

mkdir tmp
inkscape --export-type=png --export-filename="tmp/16.png" -w 16 -h 16 .\app_icon_240_15.svg
inkscape --export-type=png --export-filename="tmp/20.png" -w 20 -h 20 .\app_icon_240_15.svg
inkscape --export-type=png --export-filename="tmp/24.png" -w 24 -h 24 .\app_icon_240_15.svg
inkscape --export-type=png --export-filename="tmp/30.png" -w 30 -h 30 .\app_icon_240_15.svg
inkscape --export-type=png --export-filename="tmp/32.png" -w 32 -h 32 .\app_icon_240_15.svg
inkscape --export-type=png --export-filename="tmp/36.png" -w 36 -h 36 .\app_icon_240_15.svg
inkscape --export-type=png --export-filename="tmp/40.png" -w 40 -h 40 .\app_icon_240_15.svg
inkscape --export-type=png --export-filename="tmp/48.png" -w 48 -h 48 .\app_icon_240_15.svg
inkscape --export-type=png --export-filename="tmp/60.png" -w 60 -h 60 .\app_icon_240_15.svg
inkscape --export-type=png --export-filename="tmp/64.png" -w 64 -h 64 .\app_icon_240_15.svg
inkscape --export-type=png --export-filename="tmp/72.png" -w 72 -h 72 .\app_icon_240_15.svg
inkscape --export-type=png --export-filename="tmp/80.png" -w 80 -h 80 .\app_icon_240_15.svg
inkscape --export-type=png --export-filename="tmp/96.png" -w 96 -h 96 .\app_icon_240_15.svg
inkscape --export-type=png --export-filename="tmp/256.png" -w 256 -h 256 .\app_icon_240_15.svg

magick tmp/16.png tmp/20.png  tmp/24.png tmp/30.png tmp/32.png tmp/36.png tmp/40.png tmp/48.png tmp/60.png tmp/64.png tmp/72.png tmp/80.png tmp/96.png tmp/256.png tmp/icon.ico
