#!/bin/bash

# Processing options
while getopts ":au:Nzsh" opt; do
    case ${opt} in
	h)
	    echo "-u: usermod\n-N: sudoer nopasswd\n-a: apt\n-s: openssh server\n-z: zsh"
	;;
        u)
            usermod -aG sudo $OPTARG
        ;;
        N)
            sed -i 's/%sudo	ALL=(ALL:ALL) ALL/%sudo ALL=(ALL:ALL) NOPASSWD:ALL/' /etc/sudoers
        ;;
        a)
            apt update
            apt -y install sudo
            apt -y install zsh
            apt -y install build-essential
	    apt -y install net-tools
            apt -y install vim
            apt -y install gcc
            apt -y install gdb
            apt -y install python
            apt -y install python3
            apt -y install nginx
            
            apt -y install curl
            apt -y install g++
            apt -y install htop
            apt -y install tree
            apt -y install git
            apt -y install brctl
            apt -y install isc-dhcp-client
            apt -y install isc-dhcp-server
            apt -y upgrade
        ;;
        s)
            apt -y install openssh-server
            sed -i 's/.\{0,1\}PasswordAuthentication.\{0,5\}$/PasswordAuthentication no/' /etc/ssh/sshd_config
            systemctl restart sshd
        ;;
        z)
            # Setting up zsh
            wget http://grml.org/zsh/zshrc
            mv zshrc /etc/zsh/
            chsh -s /bin/zsh
        ;;
        \?)
            echo "use -u to specify the user you want to add to sudoers\nuse -N to make the sudoers not requiring password\nuse -a to do apt install" 1>&2
            exit 1
        ;;
        :)
            echo "Invalid Option: -$OPTARG requires an argument" 1>&2
            exit 1
        ;;
    esac
done
