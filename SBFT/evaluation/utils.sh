#!/bin/bash

apt install -y zsh tmux tcpdump
sh -c "$(wget --no-check-certificate https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh -O -)"
echo "set-option -g mouse on" > /root/.tmux.conf