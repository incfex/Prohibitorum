#!/bin/bash

# Processing options
while getopts ":au:Nzsh" opt; do
    case ${opt} in
	h)
	    echo "-u: usermod"
	    echo "-N: sudoer nopasswd"
	    echo "-a: apt"
	    echo "-s: openssh server"
	    echo "-z: zsh"
	;;
        u)
            apt update
            apt -y install sudo
            usermod -aG sudo $OPTARG
	    sed -i '1 a\Defaults        insults' /etc/sudoers
	    chsh -s /bin/zsh $OPTARG
        ;;
        N)
            sed -i 's/%sudo	ALL=(ALL:ALL) ALL/%sudo ALL=(ALL:ALL) NOPASSWD:ALL/' /etc/sudoers
        ;;
        a)
            apt update
            apt -y install sudo zsh build-essential net-tools vim gcc gdb g++ python python3 curl wget htop tree git bridge-utils mtr-tiny tmux
            #apt -y install nginx
            #apt -y install isc-dhcp-client
            #apt -y install isc-dhcp-server
            apt -y upgrade
        ;;
        s)
            apt -y install openssh-client
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
            echo "use -u to specify the user you want to add to sudoers" 1>&2
            echo "use -N to make the sudoers not requiring password" 1>&2
            echo "use -a to do apt install" 1>&2
            exit 1
        ;;
        :)
            echo "Invalid Option: -$OPTARG requires an argument" 1>&2
            exit 1
        ;;
    esac
done
