# books
Ebook reader for .txt files, so you can read books from archive.org in CLI

## Installation

run ./configure to create the bookmark folder in ~/.books/bookmarks

## Instructions

run ./books [Filename] to open a book to page one

books accepts the following optional parameters:

* -p [###]
	Go to a specific page
* -w [###]
	Set the reader to a specific width (in characters)
* -h [###]
	Set the reader to a specific height (in characters)
* -b
	Open the reader to a bookmark (if one exists)

In the reader, use the following commands and keys:

* n 
	Next page
* b	
	Previous page
* :g [###]
	Go to a specific page
* :s
	Save a bookmark at the current location
* :q
	Quit
* /[...]
	Search for a specific string in the book, starting from the current location
