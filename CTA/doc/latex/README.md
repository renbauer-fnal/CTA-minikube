# CTA Documentation

To compile the documentation in this directory, type:
```
make
```

The main file for CTA documentation is cta.tex.

Compiling cta.tex requires TeXlive version 2015 or newer. As this is not currently available as an RPM,
you should download the
[TeX Live archive](http://mirror.ctan.org/systems/texlive/tlnet/install-tl-unx.tar.gz) from the 
[TeX User Group](https://tug.org/texlive/acquire-netinstall.html) and follow the
[installation instructions](https://tug.org/texlive/quickinstall.html):
```
$ tar zxvf install-tl-unx.tar.gz
$ cd install-tl-<version>
$ sudo ./install-tl
```
cta.tex also requires the
[TikZ-UML extension](http://perso.ensta-paristech.fr/~kielbasi/tikzuml/)
to manage common UML diagrams, which must be downloaded and installed separately:
```
cd /usr/local/texlive/texmf-local/tex/latex/local
sudo tar xvf /path/to/tikzuml-<version>.tbz
sudo ../../../../2016/bin/x86_64-linux/mktexlsr
```

In order to simplify things, the compiled pdf is also committed (and should be after
updates of the tex files).
