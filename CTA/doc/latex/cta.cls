%
% @file     cta.cls
% @brief    Common class for CTA documentation
%

\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{cta}[2018/03/13 LaTeX class for CERN Tape Archive documentation]
\LoadClass[11pt,a4paper]{report}

\RequirePackage[top=2cm,bottom=2cm,left=2cm,right=2cm]{geometry}    % Page size
\RequirePackage{soul}                                               % Fancy boxes
\RequirePackage{parskip}                                            % No indented paragraphs
\RequirePackage[amssymb]{SIunits}                                   % Standardised units
\RequirePackage{tabularx}                                           % Better tables
\RequirePackage{multirow}                                           % Multi-row tables

%%
%% Hyperlinks and PDF options
%%

\RequirePackage{hyperref}
\hypersetup{
  colorlinks=true,
  linkcolor=blue,
  unicode=true
}



%%
%% List environments
%%

\newcommand{\textbulletsquare}{\textcolor{cern@ltcomp}{\raisebox{.25ex}{\rule{0.9ex}{0.9ex}}}}
\renewcommand{\labelitemi}{\textbulletsquare}



%%
%% Graphics options for various environments
%%

\RequirePackage{ifpdf}
\ifpdf
\RequirePackage{graphicx}
\else
\RequirePackage[dvips]{graphicx}
\DeclareGraphicsExtensions{pdf,svg,png}
\fi
\graphicspath{{../images/}}



%%
%% Colours and Fonts
%%

\RequirePackage{xcolor}      % Colours
\RequirePackage{colortbl}    % Colour tables

% CERN blue is Pantone 286 = RGB 56 97 170, defined as cern@blue below
\definecolor{cern@ltblue}{rgb}{0.415686,0.611765,0.964706} % RGB 106 156 246
\definecolor{cern@blue}  {rgb}{0.219608,0.380392,0.666667} % RGB  56  97 170
\definecolor{cern@dkblue}{rgb}{0.082353,0.184314,0.364706} % RGB  21  47  93

% Complimentary colours
\definecolor{cern@ltcomp}{rgb}{0.666667,0.525490,0.219608} % RGB 170 134  56
\definecolor{cern@dkcomp}{rgb}{0.364706,0.266667,0.047059} % RGB  93  68  12

% Set serif font to Paratype
\RequirePackage{paratype}
\RequirePackage[T1]{fontenc}

% Set section headings font/colour
\RequirePackage{sectsty}
%\sectionfont{\color{cern@blue}\sffamily}
%\subsectionfont{\color{cern@blue}\sffamily}



%%
%% Title Page
%%

% Add title to PDF title
\let\std@title\title
\renewcommand{\title}[1]{%
  \std@title{#1}
  \hypersetup{pdftitle={The CERN Tape Archive: #1}}
}

% Redefine title page macro
\renewcommand\maketitle{\begin{titlepage}%
  \let\footnotesize\small
  \let\footnoterule\relax
  \let \footnote \thanks
  \null\vfil
  \vskip 60\p@
  \begin{center}%
    \includegraphics[width=5cm]{CERN_logo}%
    \vskip 5em%
    {\sffamily\Huge\color{cern@blue} The CERN Tape Archive \par}%
    {\color{cern@dkblue}\rule{\textwidth}{1pt} \par}%
    {\sffamily\Huge\color{cern@dkblue} \@title \par}%
    \vskip 5em%
    {\sffamily\LARGE\color{cern@blue}
    \lineskip .75em%
    \begin{tabular}[t]{c}%
    \@author
    \end{tabular}\par}%
    \vskip 1.5em%
    {\sffamily\Large\color{cern@blue} \@date \par}%
  \end{center}\par
  \@thanks
  \vfil\null
  \end{titlepage}%
  \setcounter{footnote}{0}%
  \global\let\thanks\relax
  \global\let\maketitle\relax
  \global\let\@thanks\@empty
  \global\let\@author\@empty
  \global\let\@date\@empty
  \global\let\@title\@empty
  \global\let\title\relax
  \global\let\author\relax
  \global\let\date\relax
  \global\let\and\relax
}



%%
%% Listings
%%

\RequirePackage{upquote}                    % Don't use curly quotes in verbatim text
\RequirePackage{listings}
\RequirePackage{courier}
\lstset{language=bash,frame=single}

\definecolor{background}{rgb}{1,1,0.9}
\definecolor{keyword}{rgb}{0,0.6,0}
\definecolor{string}{rgb}{0.58,0,0.82}

\lstdefinestyle{script}{%
  backgroundcolor=\color{background},   % frame colour
  frame=single,                         % invisible frame with internal margin 1em wide on all sides
  framesep=1em,
  framerule=0pt,
  xleftmargin=\parindent,               % text indented by size of paragraph indent
  xrightmargin=\parindent,
  basicstyle=\footnotesize\ttfamily,    % text in courier font
  breakatwhitespace=true,               % sets if automatic breaks should only happen at whitespace
  breaklines=true,                      % sets automatic line breaking
  captionpos=b,                         % sets the caption-position to bottom
  keepspaces=true,                      % keeps spaces in text, useful for keeping indentation of code (possibly needs columns=flexible)
  language=bash,                        % the language of the code
  numbers=none,                         % where to put the line-numbers; possible values are (none, left, right)
  showspaces=false,                     % show spaces everywhere adding particular underscores; it overrides 'showstringspaces'
  showstringspaces=false,               % underline spaces within strings only
  showtabs=false,                       % show tabs within strings adding particular underscores
  tabsize=2,	                        % sets default tabsize to 2 spaces
  title=\lstname,                       % show the filename of files included with \lstinputlisting (could use caption instead of title)
  stringstyle=\color{orange},           % string literal style
  keywordstyle=\ttfamily,               % keyword style
  commentstyle=\ttfamily                % comment style
}
\lstdefinestyle{bashscript}{%
  keywordstyle=\color{keyword},         % keyword style
  commentstyle=\textit\color{blue},     % comment style
  %morekeywords={*,...},                % if you want to add more keywords to the set
}
\lstset{style=script}



%%
%% Insert links to GitLab issues into text
%%
%% Usage: \gitlab[URL]{issue #}{issue text}
%%

\renewcommand{\tabularxcolumn}[1]{>{\small}m{#1}}
\newcommand{\gitlab}[3][https://gitlab.cern.ch/cta/CTA/issues/]{%
\begin{tabularx}{\textwidth}{>{\centering}m{1cm} >{\centering}m{2cm} >{\arraybackslash}X}
   \includegraphics[width=\linewidth]{gitlab} & {\huge\href{#1#2}{\##2}} & \href{#1#2}{#3}\\
\end{tabularx}}



%%
%% Define an environment for notes
%%

\RequirePackage[utf8]{inputenc}
\RequirePackage{newunicodechar}
\newcommand\Warning{%
\makebox[1.4em][c]{%
\makebox[0pt][c]{\raisebox{.3em}{\small!}}%
\makebox[0pt][c]{\color{red}\Huge$\bigtriangleup$}}}%
\newenvironment{note}{%
\tabularx{\textwidth}{>{\centering}m{1cm} >{\arraybackslash}X}
\Warning &}{\\
\endtabularx\par}



%%
%% Define a box for highlighted text
%%

\RequirePackage[many]{tcolorbox}
\newtcolorbox{alertbox}[1][]{
  breakable,
  freelance,
  title=#1,
  colback=white,
  colbacktitle=white,
  coltitle=black,
  fonttitle=\bfseries,
  bottomrule=0pt,
  boxrule=0pt,
  colframe=white,
  overlay unbroken and first={
  \draw[red!75!black,line width=3pt]
    ([xshift=5pt]frame.north west) -- 
    (frame.north west) -- 
    (frame.south west);
  \draw[red!75!black,line width=3pt]
    ([xshift=-5pt]frame.north east) -- 
    (frame.north east) -- 
    (frame.south east);
  },
  overlay unbroken app={
  \draw[red!75!black,line width=3pt,line cap=rect]
    (frame.south west) -- 
    ([xshift=5pt]frame.south west);
  \draw[red!75!black,line width=3pt,line cap=rect]
    (frame.south east) -- 
    ([xshift=-5pt]frame.south east);
  },
  overlay middle and last={
  \draw[red!75!black,line width=3pt]
    (frame.north west) -- 
    (frame.south west);
  \draw[red!75!black,line width=3pt]
    (frame.north east) -- 
    (frame.south east);
  },
  overlay last app={
  \draw[red!75!black,line width=3pt,line cap=rect]
    (frame.south west) --
    ([xshift=5pt]frame.south west);
  \draw[red!75!black,line width=3pt,line cap=rect]
    (frame.south east) --
    ([xshift=-5pt]frame.south east);
  },
}

