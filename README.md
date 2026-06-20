#version 1.4
	-	niveles de seguridad 
	-	ruteo , mapa o recorrido del mensaje
	-	capas 
	- 	modo sleep

#version 1.3
	- spi por BCM2835 , reever codigo implementado junto con gpio a bcm2835
	
#version 1.2
	-	implementar ACK
	-	implementar epapper
	-	implementar envio y recepcion checkeo del envio de packetes 
	-	utilizacion de seguridad encryptar de desencryptar
	-	modo router , coordinador , end device
	-	reduccion de tiempos para enviar y recibir paquetes
	- 	REVISION de GPIOs configuracion y seteo del funcionamiento de las interrupciones
	-	
	
#version 1.1
	-	Envia datos correctamente 
	-	envia header , size , buffer , checksum
	-	Version exitosa 


#vesion 1.0.1

Codigo c++ para raspberry pi y mrf24j40ma
SPI config 

gpio MOSI //master output , slave input
gpio MISO //master input , slave output 
gpio SCK // Clock 
gpio CS //Chip Select
gpio WAKE //Wake
gpio INT //Interrupt
gpio RESET //

# helps:

	http://www.microchip.com/wwwproducts/Devices.aspx?dDocName=en535967

commands :

#!/bin/bash

# Obtener la fecha y hora actual en el formato YYYYMMDDHHMM
timestamp=$(date +'%Y%m%d%H%M')

# Construir el mensaje de commit
commit_message="update $timestamp"

# Ejecutar los comandos git
git add . && git commit -m "$commit_message" && git push -u origin master

# Dependencias & Librerias

sudo apt-get install qrencode libqrencode-dev -y
# pip install qrcode

# Library PNG
sudo apt-get install libpng-dev -y

sudo apt-get install zlib1g-dev -y



comunication

		ssh-keygen -R raspberry.local


create comunication secure

		ssh-keygen -t rsa
		ssh-copy-id root@127.0.0.1
		cat ~/.ssh/id_rsa.pub | ssh root@127.0.0.1 'cat >> ~/.ssh/authorized_keys'


