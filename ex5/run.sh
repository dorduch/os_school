#!/bin/bash
sudo make
sudo insmod message_slot.ko
sudo mknod /dev/simple_message_slot c 245 0
sudo chmod 777 /dev/simple_message_slot
./message_sender 1 hello
sudo rm /dev/simple_message_slot
sudo rmmod message_slot.ko


