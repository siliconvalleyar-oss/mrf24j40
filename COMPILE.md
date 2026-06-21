
PASSWORD=$1
USER=$2
IP=$3
#RAMA=$4
#sshpass -p "$PASSWORD" ssh $USER@$IP "cd /home/$USER/src/mrf24j40 && make clean && make -j4"


ssh $USER@$IP "cd home/$USER/src/mrf24j40/ && git pull"

ssh $USER@$IP "cd /home/$USER/src/mrf24j40 && make clean && make -j4"

ssh $USER@$IP "cd /home/$USER/src/mrf24j40 && git branch -a"

#if (si no esta en la rama correcta )
#ssh $USER@$IP "cd /home/$USER/src/mrf24j40 && git switch $RAMA"


ssh $USER@$IP "cd /home/$USER/src/mrf24j40 && git pull"
