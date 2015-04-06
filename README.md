# Hippo Notes 
An open source client for Evernote. Evernote is a suite of software and services, designed for notetaking and archiving. A "note" can be a piece of formatted text, a full webpage or webpage excerpt, a photograph, a voice memo, or a handwritten "ink" note. Notes can also have file attachments. Notes can be sorted into folders, then tagged, annotated, edited, given comments, searched, and exported as part of a notebook.

The application currently support:
* Synchronization with Evernote servers
* Local data caching
* Embbeded PDF documents prieview with poppler
* Note printing and export to PDF, html or text file. 

#Requirements 
For Qt4 builds: 
* libqt4 >= 4.8 
* poppler-qt4
* libhunspell

For Qt5 builds: 
* libqt5 >= 5.3
* poppler-qt5
* libhunspell

#Build
<pre>
$ git clone --recursive https://github.com/Vytax/hippo.git 
$ cd hippo 
$ qmake
$ make</pre>

##Note
Some disturbutions might refer to qmake as qmake-qt4


