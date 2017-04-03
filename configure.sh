#!/bin/sh -e

mkdir -p "$HOME/.books"
mkdir -p "$HOME/.books/bookmarks"
mkdir -p "$HOME/.books/config"
printf "width=70\nheight=28" > $HOME/.books/config/config.cfg
