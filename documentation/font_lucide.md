# Lucide Font Adaptations

The [Lucide font](https://github.com/lucide-icons/lucide) was adapted in two ways to better suit Windows 11 default icons "".

* Reduce the stroke width to `1 pixel` when displayed `16 x 16 pixels` instead of `1.333 pixels`.
* Remove the `1 pixel padding` around the entire icon.

Note that these adaptions where made in the `*.ttf` font files, as SVG icons loaded via [`ImageIcon`](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.imageicon?view=windows-app-sdk-1.8) do not have the ability to change their color which is needed to support dark an light themes.



For these changes the fonts need to be re-generated from the SVGs.



# Instructions

Install pnpm:

```
curl -fsSL https://get.pnpm.io/install.sh | sh -
```

Install node-js:

```
nvm install 24
```



Checkout repository:

```
git clone https://github.com/lucide-icons/lucide.git
git checkout 0.562.0

```

Install packages:

```
cd lucide
pnpm install --frozen-lockfile
```



### Stroking + Padding Adjustments

Adjust original fonts:

```
sed -i 's/stroke-width="2"/stroke-width="1.333"/g' icons/*.svg
sed -i 's/viewBox="0 0 24 24"/viewBox="1.333 1.333 21.333 21.333"/g' icons/*.svg
```

Generate outlines

```
pnpm build:outline-icons
```

Fix bug in generated view boxes:

```
sed -i 's/viewBox="0 0 21.333 21.333"/viewBox="0 0 24 24"/g' outlined/*.svg
```

Generate font:

```
pnpm build:font
```

Font is generated in `lucide-font/lucide.ttf` as `lucide_0.562.0_300_nogap.ttf`.



### Only Stroking Adjustments

Adjust original fonts:

```
sed -i 's/stroke-width="2"/stroke-width="1.5"/g' icons/*.svg
```

Generate outlines and font

```
pnpm build:outline-icons
pnpm build:font
```

Font is generated in `lucide-font/lucide.ttf` as `lucide_0.562.0_300.ttf`.





