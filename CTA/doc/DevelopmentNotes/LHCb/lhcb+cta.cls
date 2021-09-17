\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{lhcb+cta}[2018/07/04 LaTeX class for minutes of LHCb+CTA discussions]
\LoadClass[11pt,a4paper]{article}

%%
%% Page size
%%

\RequirePackage[top=3cm, bottom=3cm, left=3cm, right=3cm]{geometry}
\RequirePackage{parskip}

%%
%% Colours and Fonts
%%

\RequirePackage{color}

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
%\setsansfont{AvenirLTStd-Book}
\sectionfont{\color{cern@blue}\sffamily}
\subsectionfont{\color{cern@blue}\sffamily}

% Set itemize bullet style
\newcommand{\textbulletsquare}{\textcolor{cern@ltcomp}{\raisebox{.25ex}{\rule{0.9ex}{0.9ex}}}}
\renewcommand{\labelitemi}{\textbulletsquare}



%%
%% Page Headers/Footers
%%

\RequirePackage{fancyhdr}
\fancypagestyle{plain}{%
   \fancyhf{} % clear all header and footer fields
   \fancyfoot[C]{\color{cern@dkblue}\sffamily\fontsize{9pt}{9pt}\selectfont\thepage} % except the center
   \renewcommand{\headrulewidth}{0pt}
   \renewcommand{\footrulewidth}{0pt}}
\pagestyle{plain}



%%
%% Tables
%%
\RequirePackage{tabularx}
\newcommand{\colhead}[1]{\multicolumn{1}{c}{\color{cern@blue}\textsf{#1}}}
\renewcommand{\arraystretch}{1.5}



%%
%% Other packages and macros
%%

\RequirePackage{hyperref}
\hypersetup{
   colorlinks=true,         % false: boxed links / true: coloured links
   linkcolor=cern@ltblue,   % colour of internal links (change box color with linkbordercolor)
   citecolor=cern@ltblue,   % colour of bibliography links
   filecolor=cern@ltblue,   % colour of file links
   urlcolor=cern@ltblue     % colour of external links
}



%%
%% Triangle with an exclamation point
%%

\RequirePackage[utf8]{inputenc}
\RequirePackage{newunicodechar}

\newcommand\Alert{%
 \makebox[1.4em][c]{%
 \makebox[0pt][c]{\raisebox{.1em}{\small!}}%
 \makebox[0pt][c]{\color{red}\Large$\bigtriangleup$}}}%


% Shortcuts to handle 1st, 2nd, 3rd, 4th
\RequirePackage{relsize}
\newcommand{\squared}{\ensuremath{^{\textsf{2}}} }
\newcommand{\superscript}[1]{\ensuremath{^{\textsf{\smaller #1}}}}
\newcommand{\subscript}[1]{\ensuremath{_{\textsf{\smaller #1}}}}
\newcommand{\superrm}[1]{\ensuremath{^{\textrm{\smaller #1}}}}

\newcommand{\st}[0]{\superscript{st} }
\newcommand{\nd}[0]{\superscript{nd} }
\newcommand{\rd}[0]{\superscript{rd} }
\renewcommand{\th}[0]{\superscript{th} }

\newcommand{\strm}[0]{\superrm{st} }
\newcommand{\ndrm}[0]{\superrm{nd} }
\newcommand{\rdrm}[0]{\superrm{rd} }
\newcommand{\thrm}[0]{\superrm{th} }

% Header on first page

\newcommand{\mainheading}{CTA+LHCb Discussion}
\newcommand{\subheading}{Minutes of the meeting of \thisminutesdate}

\AtBeginDocument{
{\sffamily\Huge\color{cern@dkblue} \mainheading}\\
{\color{cern@dkblue}\rule{\textwidth}{1pt}
\color{cern@ltblue}\sffamily\Large\subheading}\\
}

% Definitions and environments for standard sections

\newcommand{\present}[2]{%
\section*{Present}
\begin{itemize}
   \item \textbf{IT:} #1
   \item \textbf{LHCb:} #2
\end{itemize}}

\newcommand{\agenda}[1][%
\item Review of the previous minutes
\item Review of the previous action list
\item News from IT
\item News from LHCb
\item AOB
\item Date of the next meeting]{%
\section*{Agenda}
\begin{enumerate}
#1
\end{enumerate}}

\newcommand{\previousminutes}[1][Approved.]{%
\section*{Review of the Previous Minutes}
#1}

\newcommand{\aob}[1][There was no other business.]{%
\section*{AOB}
#1
}

\newcommand{\nextmeeting}[1][09h30 on Wednesday \nextminutesdate, 2--R--030]{
\section*{Next Meeting}
#1
}

\newenvironment{actionlist}
{\begingroup\tabularx{\textwidth}{clXll}
\colhead{\#} & \colhead{Who} & \colhead{What} & \colhead{Added} & \colhead{Status}\\
\hline}{\endtabularx\endgroup}

% Default fixed font does not support bold face
\DeclareFixedFont{\ttb}{T1}{txtt}{bx}{n}{12} % for bold
\DeclareFixedFont{\ttm}{T1}{txtt}{m}{n}{12}  % for normal

% Custom colors
\definecolor{deepblue}{rgb}{0,0,0.5}
\definecolor{deepred}{rgb}{0.6,0,0}
\definecolor{deepgreen}{rgb}{0,0.5,0}

\RequirePackage{listings}

% Python style for highlighting
\newcommand\pythonstyle{\lstset{
language=Python,
basicstyle=\ttm,
otherkeywords={self},             % Add keywords here
keywordstyle=\ttb\color{deepblue},
emph={MyClass,__init__},          % Custom highlighting
emphstyle=\ttb\color{deepred},    % Custom highlighting style
stringstyle=\color{deepgreen},
frame=tb,                         % Any extra options here
showstringspaces=false            % 
}}

% Python environment
\lstnewenvironment{python}[1][]
{
\pythonstyle
\lstset{#1}
}
{}

% Python for external files
\newcommand\pythonexternal[2][]{{
\pythonstyle
\lstinputlisting[#1]{#2}}}

% Python for inline
\newcommand\pythoninline[1]{{\pythonstyle\lstinline!#1!}}