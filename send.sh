# envoie le dossier sur le Desktop et le compile avec le makefile sur le rpi avec ssh 
# adresse ip du rpi
IP=10.206.216.153
# utilisateur du rpi
USER=pi
# nom du dossier a envoyer
DIR=LAB2

# envoie le dossier
scp -r $DIR $USER@$IP:~/Desktop/$DIR
# compile le dossier
ssh $USER@$IP "cd ~/Desktop/$DIR && make clean && make all"
# execute le programme
ssh $USER@$IP "cd ~/Desktop/$DIR && sudo ./labo2"



